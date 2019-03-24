#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mm.h> 
#include <asm/page.h> 
#include <mach/mem_define.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <mach/hardware.h>
#include "blit_ioctl.h"

// extern unsigned int ioread32(void __iomem *);
// extern void iowrite32(u32, void __iomem *);

#define BLIT_MAGIC          'B'
#define BLIT_FILL 	         _IOW(BLIT_MAGIC, 1,  int)
#define BLIT_COPY 	         _IOW(BLIT_MAGIC, 2,  int)
#define BLIT_SCALOR 	     _IOW(BLIT_MAGIC, 3,  int)
#define BLIT_COMP 	         _IOW(BLIT_MAGIC, 4,  int)
#define BLIT_GET_FB0_ADDR 	 _IOW(BLIT_MAGIC, 5,  int)
#define BLIT_GET_FB1_ADDR 	 _IOW(BLIT_MAGIC, 6,  int)
#define BLIT_MALLOC_DMA 	 _IOW(BLIT_MAGIC, 7,  int)
#define BLIT_FREE_DMA 	     _IOW(BLIT_MAGIC, 8,  int)
#define BLIT_GET_FB0_SIZE 	 _IOW(BLIT_MAGIC, 9,  int)
#define BLIT_GET_FB1_SIZE 	 _IOW(BLIT_MAGIC, 10, int)
#define CSBLIT_GET_FB2_ADDR  _IOW(BLIT_MAGIC, 11, int)
#define CSBLIT_GET_FB2_SIZE  _IOW(BLIT_MAGIC, 12, int)
#define CSBLIT_GET_FB3_ADDR  _IOW(BLIT_MAGIC, 13, int)
#define CSBLIT_GET_FB3_SIZE  _IOW(BLIT_MAGIC, 14, int)
#define BLIT_MAXNR 15

typedef struct BLIT_DMA_BUFFER_STUFF_s
{
	void *phy_addr;/*physics memory address*/
	void *kernel_virt_addr;/*virtual memory address in kernel level*/
	unsigned int kernel_needed_size;
	void *user_virt_addr;/*user can use mapped memory start address in user level*/
	unsigned int user_needed_size;/*how many buffer do user want to malloc*/
}BLIT_DMA_BUFFER_STUFF_t;

typedef struct CSBlit_Device 
{
	CSBlit_HW_Device_t	hwdev;
	struct list_head		buffer_head;
} blit_dev;

typedef struct BLIT_DMA_BUFFER_LIST_s
{
	struct list_head buffer_list;
	BLIT_DMA_BUFFER_STUFF_t pointer;
}blit_buffer_list;

blit_dev*  pblit_dev = NULL;	/* allocated in blit_init_module */
volatile unsigned int *cs_blit_base = NULL;
static spinlock_t 	blit_lock;
static struct proc_dir_entry *blit_proc_entry = NULL;
/* function declaration ------------------------------------- */
static int __init 	blit_init(void);
static void __exit 	blit_exit(void);
static int csblit_open(struct inode *inode, struct file *file);
static int csblit_release(struct inode *inode, struct file *file);
static int blit_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
//static int cs_blit_mmap(struct file *file, struct vm_area_struct *vma);
/* ----------------------------------------------------------- */
/* Allocate physic continue memory for buffers */
void *testpage = NULL;
static void *malloc_config(unsigned int buf_size)
{
	void *mem = 0;
	unsigned int size = PAGE_SIZE << (get_order(buf_size));
#if 1
	struct page * page;

	page = alloc_pages(GFP_KERNEL, get_order(size));
	if (!page){
		printk( "dma memory can't be alloced error 1!!\n");
		return 0;
	}
	else{
		mem = page_address(page);
		if (mem) {
			unsigned long adr = (unsigned long)mem;
			while (size > 0) {
                SetPageReserved(page);

                //ClearPagePrivate(virt_to_page(adr));
                //ClearPageDirty(virt_to_page(adr));
			
                adr += PAGE_SIZE;
                size -= PAGE_SIZE;
			}
		} 
		else{
			__free_pages(page, get_order(size));
			printk( "dma memory can't be alloced error 2!!\n");
			return 0;
		}
		//phy_addr = page_to_phys(page);
		printk("mem->flag = 0x%x\n",(unsigned int)page->flags);
	}
#else
    mem = (void *)__get_dma_pages(GFP_KERNEL, get_order(buf_size));
    if (mem) {
        unsigned long adr = (unsigned long)mem;
        while (size > 0) {
            SetPageReserved(virt_to_page(adr));
            adr += PAGE_SIZE;
            size -= PAGE_SIZE;
        }
    } else
        printk( "dma memory can't be alloced!!\n");
#endif
    return mem;
}

/* Free continue memory for buffers */
static void free_contig(void *addr, unsigned int buf_size)
{
	unsigned int size, adr;

	if (!addr)
		return;
	adr = (unsigned int)addr;
	size = PAGE_SIZE << (get_order(buf_size));
	while (size > 0) {
		ClearPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages((unsigned int)addr, get_order(buf_size));
}

/* blit device struct */
struct file_operations cs_blit_fops = 
    {
        .owner 	=  THIS_MODULE,
        .open = csblit_open,
        .release = csblit_release,
        .ioctl  =  blit_ioctl,
        //        .mmap = cs_blit_mmap
    };

static struct miscdevice cs_blit_miscdev = {
	MISC_DYNAMIC_MINOR,
	"cs_blit",
	&cs_blit_fops
};

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
static inline int uncached_access(struct file *file, unsigned long addr)
{

	/*
	 * Accessing memory above the top the kernel knows about or through a file pointer
	 * that was marked O_SYNC will be done non-cached.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	return addr >= __pa(high_memory);

}
#if 0
struct page *cs_vma_nopage(struct vm_area_struct *vma,unsigned long address, int *type)
{
	struct page *pageptr;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long phyaddr = address -vma->vm_start + offset;
	unsigned long pageframe = phyaddr >> PAGE_SHIFT;

	DEBUG_PRINTF("address = 0x%lx,phyaddr = 0x%lx, offset = 0x%lx,pageframe = 0x%lx\n",
                 address,phyaddr,offset,pageframe);
	DEBUG_PRINTF("call cs_vma_nopage\n");
	if(!pfn_valid(pageframe)){
		printk("pfn invalid!!!\n");
		return NOPAGE_SIGBUS;
	}
	//pageptr = virt_to_page(testpage+(pagecount<<PAGE_SHIFT));
	pageptr = pfn_to_page(pageframe);

	get_page(pageptr);
	SetPageReserved(pageptr);
	ClearPagePrivate(pageptr);
	ClearPageDirty(pageptr);

	if(type){
		*type = VM_FAULT_MINOR;
	}
	return pageptr;
}
#endif
// static struct vm_operations_struct cs_vm_ops =
//     {
//         .nopage = cs_vma_nopage,
//     };


// #ifndef __HAVE_PHYS_MEM_ACCESS_PROT
// static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
// 				     unsigned long size, pgprot_t vma_prot)
// {
// #ifdef pgprot_noncached
// 	unsigned long offset = pfn << PAGE_SHIFT;

// 	if (uncached_access(file, offset))
// 		return pgprot_noncached(vma_prot);
// #endif
// 	return vma_prot;
// }
// #endif

// static int cs_blit_mmap(struct file *file, struct vm_area_struct *vma)
// {
// 	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
// 	size_t size = vma->vm_end - vma->vm_start;
    
// 	if((FB0_REGION == offset)||(FB1_REGION == offset)||(offset == offset)){
//         if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
//             return -EINVAL;
        
//         if (!(vma->vm_flags & VM_MAYSHARE))
//             return -ENOSYS;
        
//         vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
//                                                  size,
//                                                  vma->vm_page_prot);
    
// 		/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
// 		if (remap_pfn_range(vma, 
//                             vma->vm_start, 
//                             vma->vm_pgoff, 
//                             vma->vm_end - vma->vm_start, 
//                             vma->vm_page_prot))
// 			return -EAGAIN;
// 	}
// 	// else{
// 	// 	if((offset >= __pa(high_memory))||(file->f_flags & O_SYNC)){
// 	// 		vma->vm_flags |= VM_IO;
// 	// 	}
// 	// 	vma->vm_flags |= VM_RESERVED;
// 	// 	vma->vm_ops = &cs_vm_ops;
// 	// }
// 	return 0;
// }

static int csblit_open(struct inode *inode, struct file *file)
{
	DEBUG_PRINTF("%s\n",__FUNCTION__);
	
	return 0;
}

static int csblit_release(struct inode *inode, struct file *file)
{
	blit_buffer_list * dma_buffer_list;
	struct list_head *pos, *temp;
    
	DEBUG_PRINTF("%s\n",__FUNCTION__);
    
	list_for_each_safe(pos, temp, &pblit_dev->buffer_head){
		dma_buffer_list = list_entry(pos, blit_buffer_list, buffer_list);
        DEBUG_PRINTF("pointer = 0x%x\n",(unsigned int)dma_buffer_list->pointer.phy_addr);
        if(dma_buffer_list->pointer.phy_addr !=0){
            free_contig(dma_buffer_list->pointer.kernel_virt_addr, dma_buffer_list->pointer.kernel_needed_size);
        }
        list_del(pos);
        kfree(dma_buffer_list);
	}
    
	return 0;
}

static int blit_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int err = -1;
    union _cmd_param_
    {
        CSBLIT_FillRectParams_t fillparam;
        CSBLIT_CopyRectParams_t copyparam;
        CSBLIT_ScalorParams_t 	scalorparam;
        CSBLIT_CompParams_t 	compparam;
    } cmd_param;
    
    CSBLIT_Rectangle_t tmprect;		/* only for dest rectangle copy */
    
    spin_lock(&blit_lock);
    
    switch(cmd) 
        {
        case BLIT_FILL:
            DEBUG_PRINTF("CS_BLIT_IOCTL_CMD_FILL\n");
            if(copy_from_user(&cmd_param.fillparam, (void *)arg, sizeof(CSBLIT_FillRectParams_t)))
                goto ERR_INVAL;
            err = Fill(&pblit_dev->hwdev, 1,        \
                       &cmd_param.fillparam.Bitmap,     \
                       &cmd_param.fillparam.Rectangle,	\
                       &cmd_param.fillparam.Color,      \
                       cmd_param.fillparam.AluMode);
            DEBUG_PRINTF("CS_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
            break;
            
        case BLIT_COPY:
            DEBUG_PRINTF("CS_BLIT_IOCTL_CMD_COPY\n");
            if(copy_from_user(&cmd_param.copyparam, (void *)arg, sizeof(CSBLIT_CopyRectParams_t)))
                goto ERR_INVAL;
            
            tmprect.PositionX = cmd_param.copyparam.DstPositionX;
            tmprect.PositionY = cmd_param.copyparam.DstPositionY;
            tmprect.Width = cmd_param.copyparam.SrcRectangle.Width;
            tmprect.Height = cmd_param.copyparam.SrcRectangle.Height;
            err = Copy(&pblit_dev->hwdev, 1,        \
                       &cmd_param.copyparam.SrcBitmap,	\
                       &cmd_param.copyparam.SrcRectangle,	\
                       0,                                   \
                       &cmd_param.copyparam.DstBitmap,      \
                       &tmprect,                            \
                       cmd_param.copyparam.AluMode,  0);
            break;
            
        case BLIT_SCALOR:
            DEBUG_PRINTF("CS_BLIT_IOCTL_CMD_SCALOR\n");
            if(copy_from_user(&cmd_param.scalorparam, (void *)arg, sizeof(CSBLIT_ScalorParams_t)))
                goto ERR_INVAL;
        
            if(cmd_param.scalorparam.VInitialPhase > 7)
                cmd_param.scalorparam.VInitialPhase = 4;
            if(cmd_param.scalorparam.HInitialPhase > 7)
                cmd_param.scalorparam.HInitialPhase = 0;
        
            err = Scalor(&pblit_dev->hwdev,     \
                         &cmd_param.scalorparam.Src,    \
                         &cmd_param.scalorparam.Dst,    \
                         cmd_param.scalorparam.VInitialPhase,   \
                         cmd_param.scalorparam.HInitialPhase,   \
                         cmd_param.scalorparam.AluMode);
            break;

        case BLIT_COMP:
            DEBUG_PRINTF("CS_BLIT_IOCTL_CMD_COMP\n");
            if(copy_from_user(&cmd_param.compparam, (void *)arg, sizeof(CSBLIT_CompParams_t)))
                goto ERR_INVAL;
            
            if(cmd_param.compparam.Src0.Type == CSBLIT_SOURCE_TYPE_BITMAP && 
               cmd_param.compparam.Src1.Type == CSBLIT_SOURCE_TYPE_BITMAP)
                {
                    err = Composite(&pblit_dev->hwdev,              \
                                    &cmd_param.compparam.Src0, 		\
                                    &cmd_param.compparam.Src1, 		\
                                    &cmd_param.compparam.Dst,           \
                                    cmd_param.compparam.AluMode, 		\
                                    cmd_param.compparam.BlendEnable, 	\
                                    cmd_param.compparam.IsS0OnTopS1, 	\
                                    cmd_param.compparam.ROPAlphaCtrl);
                }
            else if(cmd_param.compparam.Src0.Type == CSBLIT_SOURCE_TYPE_COLOR &&
                    cmd_param.compparam.Src1.Type == CSBLIT_SOURCE_TYPE_BITMAP)
                {
                    err = CompositeSrc1(&pblit_dev->hwdev,		\
                                        &cmd_param.compparam.Src0, 		\
                                        &cmd_param.compparam.Src1, 		\
                                        &cmd_param.compparam.Dst, 		\
                                        cmd_param.compparam.AluMode, 		\
                                        cmd_param.compparam.BlendEnable, 	\
                                        cmd_param.compparam.IsS0OnTopS1, 	\
                                        cmd_param.compparam.ROPAlphaCtrl);
                }
            else if(cmd_param.compparam.Src1.Type == CSBLIT_SOURCE_TYPE_COLOR &&
                    cmd_param.compparam.Src0.Type == CSBLIT_SOURCE_TYPE_BITMAP)
                {
                    err = CompositeSrc1(&pblit_dev->hwdev,		\
                                        &cmd_param.compparam.Src0, 		\
                                        &cmd_param.compparam.Src1, 		\
                                        &cmd_param.compparam.Dst, 		\
                                        cmd_param.compparam.AluMode, 		\
                                        cmd_param.compparam.BlendEnable, 	\
                                        !cmd_param.compparam.IsS0OnTopS1,	/*src0/1 reversed*/ 	\
                                        cmd_param.compparam.ROPAlphaCtrl);
                }
            else 
                {
                    printk(KERN_WARNING "Slave Now can't support 2 A0 Source Composite!\n");
                    goto ERR_INVAL;
                }
            break;

        case BLIT_GET_FB0_ADDR:
            DEBUG_PRINTF("BLIT_GET_FB0_ADDR\n");
            err = __put_user(FB0_REGION, (unsigned int __user *) arg);
            break;
        case BLIT_GET_FB1_ADDR:
            DEBUG_PRINTF("BLIT_GET_FB1_ADDR\n");
            err = __put_user(FB1_REGION, (unsigned int __user *) arg);
            break;
        case BLIT_GET_FB0_SIZE:
            DEBUG_PRINTF("BLIT_GET_FB0_SIZE\n");
            err = __put_user(FB0_SIZE, (unsigned int __user *) arg);
            break;
        case BLIT_GET_FB1_SIZE:
            DEBUG_PRINTF("BLIT_GET_FB1_SIZE\n");
            err = __put_user(FB1_SIZE, (unsigned int __user *) arg);
            break;

        case BLIT_MALLOC_DMA:
            {
                BLIT_DMA_BUFFER_STUFF_t dma_buf;
                blit_buffer_list * dma_buffer_list;
                DEBUG_PRINTF("BLIT_MALLOC_DMA\n");
                copy_from_user(&dma_buf, (unsigned int __user *) arg, sizeof(BLIT_DMA_BUFFER_STUFF_t));
                dma_buf.kernel_virt_addr = malloc_config(dma_buf.user_needed_size);
                if(dma_buf.kernel_virt_addr == 0){
                    dma_buf.kernel_needed_size = 0;
                    dma_buf.phy_addr = 0;
                    dma_buf.user_needed_size = 0;
                    dma_buf.user_virt_addr = 0;
                }
                else{
                    dma_buf.kernel_needed_size = PAGE_SIZE << (get_order(dma_buf.user_needed_size));
                    dma_buf.phy_addr = (void*)page_to_phys(virt_to_page(dma_buf.kernel_virt_addr));
                }
                DEBUG_PRINTF("BLIT_MALLOC_DMA:0x%x,0x%x,0x%x,0x%x,0x%x\n",dma_buf.user_needed_size,
                             (unsigned int)dma_buf.user_virt_addr,(unsigned int)dma_buf.phy_addr,
                             dma_buf.kernel_needed_size,(unsigned int)dma_buf.kernel_virt_addr);

                copy_to_user((unsigned int __user *) arg, &dma_buf, sizeof(BLIT_DMA_BUFFER_STUFF_t));
                dma_buffer_list = (blit_buffer_list *)kmalloc(sizeof(blit_buffer_list),GFP_KERNEL);
                memcpy(&dma_buffer_list->pointer,&dma_buf,sizeof(BLIT_DMA_BUFFER_STUFF_t));
                INIT_LIST_HEAD(&dma_buffer_list->buffer_list);
                list_add_tail(&dma_buffer_list->buffer_list, &pblit_dev->buffer_head);
                err = 0;
            }
            break;
        case BLIT_FREE_DMA:
            {
                BLIT_DMA_BUFFER_STUFF_t dma_buf;
                blit_buffer_list * dma_buffer_list;
                struct list_head *pos, *temp;

                DEBUG_PRINTF("BLIT_FREE_DMA\n");
                copy_from_user(&dma_buf, (unsigned int __user *) arg, sizeof(BLIT_DMA_BUFFER_STUFF_t));
                DEBUG_PRINTF("BLIT_FREE_DMA:0x%x,0x%x,0x%x,0x%x,0x%x\n",dma_buf.user_needed_size,
                             (unsigned int)dma_buf.user_virt_addr,(unsigned int)dma_buf.phy_addr,
                             dma_buf.kernel_needed_size,(unsigned int)dma_buf.kernel_virt_addr);

                list_for_each_safe(pos, temp, &pblit_dev->buffer_head){
                    dma_buffer_list = list_entry(pos, blit_buffer_list, buffer_list);
                    if(dma_buffer_list->pointer.phy_addr == dma_buf.phy_addr){
                        list_del(pos);
                        kfree(dma_buffer_list);
                    }
                }

                free_contig(dma_buf.kernel_virt_addr, dma_buf.kernel_needed_size);
                dma_buf.kernel_virt_addr = 0;
                dma_buf.phy_addr = 0;
                dma_buf.kernel_needed_size = 0;
                copy_to_user((unsigned int __user *) arg, &dma_buf, sizeof(BLIT_DMA_BUFFER_STUFF_t));
                err = 0;
            }
            break;

        default:
            goto ERR_INVAL;
        }

    spin_unlock(&blit_lock);
    return err;

 ERR_INVAL:
    spin_unlock(&blit_lock);
    return -EINVAL;
}

static unsigned char*v_addr = NULL;
static unsigned int * phy_addr = NULL;
static unsigned int size = 0;
static int blit_proc_write(struct file *file, const char *buffer,unsigned long count, void *data)
{
	unsigned int addr;
	unsigned int val;

	const char *cmd_line = buffer;

	if (strncmp("rl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = ioread32((void __iomem *)cs_blit_base+addr);
		printk(" readl [0x%04x] = 0x%08x \n", addr, val);
	}
	else if (strncmp("wl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = simple_strtol(&cmd_line[7], NULL, 16);
		iowrite32(val, (void __iomem *)cs_blit_base+(addr>>2));
	}
	else if(strncmp("dumpreg", cmd_line, 7) == 0){
		int i = 0;
		for(i = 0;i<100;i +=4){
			printk(" cs_blit register : addr[0x%08x] = 0x%08x \n", 0x40000000+i, ioread32((void __iomem *)(cs_blit_base+(i>>2))));
		}
	}
	else if(strncmp("page_malloc", cmd_line, 11) == 0){
		int i = 0;
		unsigned int aaa = 0;
		struct page * page;
		size = simple_strtol(&cmd_line[12], NULL, 16);
		printk("size = 0x%x\n",size);
		//v_addr = __get_free_pages(GFP_DMA|GFP_KERNEL, size);
		//phy_addr = virt_to_phys(v_addr);
		page = alloc_pages(GFP_DMA|GFP_KERNEL, get_order(size));
		aaa =size;
		if (!page){
			printk("111111111111111111111111\n");
		}
		else{
			v_addr = (unsigned char *) page_address(page);
			if (v_addr) {
				unsigned long adr = (unsigned long)v_addr;
				while (size > 0) {
					//SetPageReserved(virt_to_page(adr));
					adr += PAGE_SIZE;
					size -= PAGE_SIZE;
				}
			} else
				printk( "dma memory can't be alloced!!\n");

            phy_addr = (unsigned int *)page_to_phys(page);
			for(i = 0; i< 0x1000;i++){
				*(v_addr+i) = 0xee;
				printk("0x%x = %d\n",(unsigned int)v_addr+i,*(v_addr+i));
			}
		}
		printk("dma_malloc:wanted size = 0x%x, malloced phy_addr = 0x%x, v_addr = 0x%x\n",size,(unsigned int)phy_addr,(unsigned int)v_addr);
	}
	else if(strncmp("page_free", cmd_line, 9) == 0){
		free_pages((unsigned long)v_addr, size);
		printk("dma_free:wanted size = 0x%x, phy_addr = 0x%x, v_addr = 0x%x\n",size,(unsigned int)phy_addr,(unsigned int)v_addr);
	}
	return count;
}

static int __init blit_init(void)
{
	if (misc_register(&cs_blit_miscdev))
		return -EIO;

    // if (!request_mem_region(CS_BLIT_BASE, CS_BLIT_SIZE, "ORION BLIT"))
    // {
    // misc_deregister(&cs_blit_miscdev);
    // return -EIO;
    // }

	// cs_blit_base = ioremap(CS_BLIT_BASE, CS_BLIT_SIZE);
 
    cs_blit_base = (unsigned int *)VA_IO_GRAPHICS_BASE;
	if(!cs_blit_base) {
		// release_mem_region(CS_BLIT_BASE, CS_BLIT_SIZE);
		misc_deregister(&cs_blit_miscdev);
		return -EIO;
	}

	pblit_dev = kzalloc ( 1 * sizeof(blit_dev), GFP_KERNEL);
	if (!pblit_dev) {
		// iounmap((void *)cs_blit_base);
		// release_mem_region(CS_BLIT_BASE, CS_BLIT_SIZE);
        misc_deregister(&cs_blit_miscdev);
		return -ENOMEM;
	}

	// memset ( pblit_dev, 0, 1 * sizeof(blit_dev));

	INIT_LIST_HEAD(&pblit_dev->buffer_head);
	
	spin_lock_init(&blit_lock);

	/* Initialize hardware device. */
	spin_lock(&blit_lock);
	pblit_dev->hwdev.RegBaseAddr = (unsigned int)cs_blit_base;
	pblit_dev->hwdev.BlitType = BLIT_TYPE_1200;
	blit_initialize(&pblit_dev->hwdev);
	spin_unlock(&blit_lock);

	blit_proc_entry = create_proc_entry("blit_io", 0, NULL);
	if (NULL != (void *)blit_proc_entry) {
		blit_proc_entry->write_proc = &blit_proc_write;
	}

	printk(KERN_INFO "CS BLIT Initialed\n");

	return 0;
}

static void __exit blit_exit(void)
{
	if (pblit_dev) {
        // iounmap((void *)pblit_dev->hwdev.RegBaseAddr);
        kfree ( pblit_dev );
        pblit_dev = NULL;
    }
    
    // release_mem_region(CS_BLIT_BASE, CS_BLIT_SIZE);
	misc_deregister(&cs_blit_miscdev);
}

module_init(blit_init);
module_exit(blit_exit);

MODULE_AUTHOR("jia.ma,Celestial Semiconductor");
MODULE_DESCRIPTION("Celestial Semiconductor BLIT driver");
MODULE_LICENSE("GPL");
