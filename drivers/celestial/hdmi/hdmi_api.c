#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "hdmi_api.h"
#include "TypeDefs.h"
#include "SiIIIC.h"
#include "SiITX.h"
#include "SiITXInfoPkts.h"
#include "SiITXDefs.h"
#include "EDID.h"
#include "si_apiCEC.h"
#include "hdmi_sw_hw_if.h"
#include "SiITXHDCP.h"
#include "si_cec_enums.h"

#define HDMI_MAJOR 222
#define HDMI_MINOR 0

static int hdmi_main(void *);
static unsigned int HDMISecureChip(void);

void volatile __iomem *hdmi_reg_addr;

static unsigned int timingvideomode             = 0;
unsigned int PictureAspectRatio                 = 1;
static unsigned int siiTXInputAudioFs           = HDMI_AUDIO_48KHz; // default = 48kHz;//0:44.1kHZ//3:32kH
static unsigned int siiTXInputAudioType         = 1;
static unsigned int siiTXInputAudioSampleLength = 0xb;
static unsigned int siiTXInputAudioChannel      = 0x1;
static unsigned int timingvideomode_current     = 0;
static unsigned int siiTXHDCPEnable             = 0;
static unsigned int hdmi_events = 0;
unsigned int rgb_limited_range = 1;
static int hdmi_wakeup_flag = 0;
BYTE EDID_ERROR;

extern int outputmode;

extern BYTE *EDID_Raw_Data;
extern int EDID_Raw_Data_Length;
extern CSTVOUT_HDMI_CEC_RECEIVE_MESSAGE cec_recv_msg;

extern void siiTXSetVendorSpecificInfoFramePacket( int Format_3D);

static struct class *hdmi_class;
HDMI_DEV *hdmi_dev;
DECLARE_WAIT_QUEUE_HEAD(hdmi_wait_queue);

struct HDMI_VIC_AR
{
	int timing;
	int ar;
};

static int hdmi_open(struct inode *inode, struct file *file)
{
    //printk("@@@hdmi_open\n");
    file->private_data = (void *)hdmi_dev;

	return 0;
}

static int hdmi_release(struct inode *inode, struct file *file)
{
    //printk("@@@hdmi_release\n");

	return 0;
}

static unsigned int hdmi_poll(struct file *filp, poll_table* wait)
{
    unsigned int mask = 0;

    //printk("hdmi_poll called.\n");

    if (hdmi_events)
    {
        mask = POLLIN;
        return mask;
    }

    poll_wait(filp, &hdmi_wait_queue, wait);

	return mask;
}

static int hdmi_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct HDMI_VIC_AR hdmi_mode;
    int cec_is_supported;
    int format_3d;
    int cec_task_status;
    //unsigned int reg_val = 0;
    unsigned char src_la;
    unsigned char capture_id[2];
    int ack_status;
    int enable_cec;
    CSTVOUT_HDMI_CEC_SEND_MESSAGE msg;
    struct HDMI_Audio_Param audio_param;
    int enable_hdmi;

	switch(cmd)
	{
        case HDMI_ENA:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(&enable_hdmi, (void __user *)arg, sizeof(enable_hdmi));
            if (enable_hdmi)
            {
                if (hdmi_dev->task_running == 0)
                {
                    hdmi_dev->task_running = 1;
                    hdmi_wakeup_flag = 1;
                }
            }
            else
            {
                if (hdmi_dev->task_running == 1)
                    hdmi_dev->task_running = 0;
            }
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_SET_AUDIO:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(&audio_param, (void __user *)arg, sizeof(audio_param));
            HDMISetAudio(audio_param.AudioFsId, audio_param.AudioTypeId, audio_param.AudioLenId, audio_param.AudioChannelId);
            up(&hdmi_dev->hdmi_sem);
            break;
		case HDMI_SETMODE:
			down(&hdmi_dev->hdmi_sem);
			copy_from_user(&hdmi_mode, (void __user *)arg, sizeof(hdmi_mode));
			timingvideomode = hdmi_mode.timing;
			PictureAspectRatio = hdmi_mode.ar;
			//printk("@@@timing is %d, ar is %d\n", hdmi_mode.timing, hdmi_mode.ar);
			up(&hdmi_dev->hdmi_sem);
			break;
        case HDMI_CHECKEVENT:
            down(&hdmi_dev->hdmi_sem);
            copy_to_user((void __user *)arg, &hdmi_events, sizeof(hdmi_events));
            hdmi_events = 0;
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_GETEDIDDATALEN:
            down(&hdmi_dev->hdmi_sem);
            copy_to_user((void __user *)arg, &EDID_Raw_Data_Length, sizeof(EDID_Raw_Data_Length));
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_GETEDIDDATA:
            down(&hdmi_dev->hdmi_sem);
            copy_to_user((void __user *)arg, (void *)EDID_Raw_Data, EDID_Raw_Data_Length * sizeof(BYTE));
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_ENABLE:
            down(&hdmi_dev->hdmi_sem);
            __get_user(enable_cec, (int __user *) arg);
            if (enable_cec)
            {
                //printk("@@@enable_cec.\n");
                SI_CecInit();
                EnableCECSupport();
            }
            else
            {
                //printk("@@@disable_cec.\n");
                clear_capture_id();
                DisableCECSupport();
            }
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_ENA_VENDORSPECIFICIF:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(&format_3d, (void __user *)arg, sizeof(format_3d));
            HDMI_Enable_VendorSpecificIF(format_3d);
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_DIS_VENDORSPECIFICIF:
            down(&hdmi_dev->hdmi_sem);
            HDMI_Disable_VendorSpecificIF();
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CHECKCECSUPPORT:
            down(&hdmi_dev->hdmi_sem);
            cec_is_supported = CECIsSupported();
            copy_to_user((void __user *)arg, &cec_is_supported, sizeof(cec_is_supported));
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CHECKCECSTATUS:
            down(&hdmi_dev->hdmi_sem);
            cec_task_status = si_CecCheckTaskStatus();
            copy_to_user((void __user *)arg, &cec_task_status, sizeof(cec_task_status));
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_ONETOUCHPLAY:
            down(&hdmi_dev->hdmi_sem);
            if (!SI_CecOneTouchPlay())
            {
                printk("SI_CecOneTouchPlay failed.\n");
            }
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_SETLA:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(&src_la, (void __user *)arg, sizeof(src_la));
            SI_CecSetSrcLA(src_la);
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_SETCAPTUREID:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(capture_id, (void __user *)arg, sizeof(unsigned char) * 2);
            //printk("@@@capture_id[0] is 0x%x, capture_id[1] is 0x%x\n", capture_id[0], capture_id[1]);
            SI_CecSetCaptureID(capture_id);
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_GETACKSTATUS:
            down(&hdmi_dev->hdmi_sem);
            ack_status = si_CecGetACKStatus();
            copy_to_user((void __user *)arg, &ack_status, sizeof(ack_status));
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_CLEARACKSTATUS:
            down(&hdmi_dev->hdmi_sem);
            si_CecClearTxStatus();
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_SENDMESSAGE:
            down(&hdmi_dev->hdmi_sem);
            copy_from_user(&msg, (void __user *)arg, sizeof(msg));
            si_CecSendMessage_api(msg.opcode, msg.dst_la, msg.arg_count, msg.args);
            up(&hdmi_dev->hdmi_sem);
            break;
        case HDMI_CEC_READMESSAGE:
            down(&hdmi_dev->hdmi_sem);
            copy_to_user((void __user *)arg, &cec_recv_msg, sizeof(cec_recv_msg));
            up(&hdmi_dev->hdmi_sem);
            break;
		default:
			break;
	}

	return 0;
}

static struct file_operations hdmi_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_open,
	.release = hdmi_release,
	.ioctl = hdmi_ioctl,
	.poll = hdmi_poll,
};

static int __init cnc_hdmi_init(void)
{
    hdmi_events = 0;

	hdmi_dev = kmalloc(sizeof(HDMI_DEV), GFP_KERNEL);
	if (hdmi_dev == NULL) {
		goto err1;
	}

	init_MUTEX(&(hdmi_dev->hdmi_sem));

	hdmi_reg_addr = (__iomem char *)VA_HDMI_BASE;
	if (NULL == hdmi_reg_addr) {
		goto err2;
	}

	if(outputmode != -1){
		timingvideomode = timingvideomode_current = outputmode;
		siiTX.is_first_time = 1;
		//siiTXSendCP_Packet( ON );
		//siiTXSetEncryption( OFF );
	}
	else{
		siiTX.is_first_time = 0;
	}

#if 0
	if (CECIsSupported()) {
		SI_CecInit();
	} else {
		clear_capture_id();
	}
#endif

	if (register_chrdev(HDMI_MAJOR, "cnc1800l_hdmi", &hdmi_fops)){
		goto err3;
	}

	hdmi_class = class_create(THIS_MODULE, "cnc1800l_hdmi");
	if (IS_ERR(hdmi_class)){
		printk(KERN_ERR "HDMI: class create failed.\n");
		goto err4;
	}

	device_create(hdmi_class,NULL,MKDEV(HDMI_MAJOR, HDMI_MINOR),NULL,"cnc_hdmi");

	printk("Checking for secure chip and enabling HDCP\n");
	if (HDMISecureChip()) {
		//printk("@@@HDCP enabled.\n");
		HDMIEnableHDCP(1);
	}

	printk("Creating the main HDMI thread\n");
	hdmi_dev->task = kthread_run(hdmi_main, NULL, "cnc1800l_hdmi");
	if (IS_ERR(hdmi_dev->task))
        goto err5;
        //return PTR_ERR(hdmi_dev->task);

    hdmi_dev->task_running = 1;

	return 0;

err5:
    device_destroy(hdmi_class,MKDEV(HDMI_MAJOR, HDMI_MINOR));
    class_destroy(hdmi_class);
err4:
	unregister_chrdev(HDMI_MAJOR, "cnc1800l_hdmi");
err3:
	hdmi_reg_addr = NULL;
err2:
	kfree(hdmi_dev);
err1:

	return -ENODEV;
}

static void __exit cnc_hdmi_exit(void)
{
    kthread_stop(hdmi_dev->task);
    hdmi_dev->task_running = 0;
    device_destroy(hdmi_class,MKDEV(HDMI_MAJOR, HDMI_MINOR));
	class_destroy(hdmi_class);
	unregister_chrdev(HDMI_MAJOR, "cnc1800l_hdmi");
	hdmi_reg_addr = NULL;
	kfree(hdmi_dev);

	if(EDID_Raw_Data != NULL){
		kfree(EDID_Raw_Data);
		EDID_Raw_Data=NULL;
	}
	EDID_Raw_Data_Length = 0;

	return;
}

int HDMISetVideo(unsigned int VideoTimingId,unsigned int PictureAspectId)
{
	timingvideomode          = VideoTimingId;
	PictureAspectRatio       = PictureAspectId;
	siiTX.is_first_time = 0;
	printk("HDMISetVideo: timingvideomode = %d, PictureAspectRatio = %d\n",timingvideomode,PictureAspectRatio);
	return 1;
}

int HDMISetAudio(unsigned int AudioFsId,unsigned int AudioTypeId,unsigned int AudioLenId,unsigned int AudioChannelId)
{

	// Parameters changed if different than 0xFFFFFFFF. Thus, you do not need to know the correct value of all parameters
	// to change a specific parameter
	if(AudioFsId != 0xFFFFFFFF)         siiTXInputAudioFs           =AudioFsId;
	if(AudioTypeId != 0xFFFFFFFF)       siiTXInputAudioType         =AudioTypeId;
	if(AudioLenId != 0xFFFFFFFF)        siiTXInputAudioSampleLength =AudioLenId;
	if(AudioChannelId != 0xFFFFFFFF)    siiTXInputAudioChannel      =AudioChannelId;
#if 0
	printk("HDMISetAudio : siiTXOutputMode = %d\n",siiTX.siiTXOutputMode);
	if((siiTXInputAudioFs != siiTX.siiTXInputAudioFs)
			|| (siiTXInputAudioType != siiTX.siiTXInputAudioType)
			|| (siiTXInputAudioSampleLength != siiTX.siiTXInputAudioSampleLength)
			|| (siiTXInputAudioChannel != siiTX.siiTXInputAudioChannel))
	{
		printk("HDMISetAudio   : %d,%d,%d,%d\n",siiTXInputAudioFs,siiTX.siiTXInputAudioFs,siiTXInputAudioType,siiTX.siiTXInputAudioType);
		printk(".................................................\n");
		printk("HDMISetAudio    :%d,%d,%d,%d\n",siiTXInputAudioSampleLength,siiTX.siiTXInputAudioSampleLength,siiTXInputAudioChannel,siiTX.siiTXInputAudioChannel);
		printk(".....change Audio Format.......................\n");
		printk(".................................................\n");
		siiTX.siiTXInputAudioFs           = siiTXInputAudioFs;
		siiTX.siiTXInputAudioType         = siiTXInputAudioType;
		siiTX.siiTXInputAudioSampleLength = siiTXInputAudioSampleLength;
		siiTX.siiTXInputAudioChannel      = siiTXInputAudioChannel;

		siiTXInitAudioPart();
		siiTXSetAudioPath();
	}
#endif
	DEBUG_PRINTK("6666666666666666666666666666666666666666666666666666666666    :siiTXInputAudioFs = %d\n",siiTXInputAudioFs);
	return 1;
}

//extern hpd_callback_function hpd_callback;
//CSTVOUT_HDMI_HPD_NOTIFYEVENT actual_event;
//char plugging;

int HDMI_Enable_VendorSpecificIF(int format)
{
	siiTXSetVendorSpecificInfoFramePacket(format);

	return siiTXEnableInfoFrame(VENDORSPEC_TYPE);
}

int HDMI_Disable_VendorSpecificIF(void)
{
	return siiTXDisableInfoFrame(VENDORSPEC_TYPE);
}

static int hdmi_main(void *unused)
{
	unsigned int timingvideomode_temp = 0;
	unsigned int siiTXInputAudioFs_temp = HDMI_AUDIO_48KHz; // default = 48kHz;//0:44.1kHZ//3:32kH
	unsigned int siiTXInputAudioType_temp =1;
	unsigned int siiTXInputAudioSampleLength_temp =0xb;
	unsigned int siiTXInputAudioChannel_temp      =0x1;
	//char plug_check;

	siiTXInputAudioFs           =HDMI_AUDIO_48KHz;
	siiTXInputAudioType         =HDMI_AUDIO_I2S;
	siiTXInputAudioSampleLength =HDMI_AUDIO_24BIT;
	siiTXInputAudioChannel      =HDMI_AUDIO_2_CHANNEL;

	DEBUG_PRINTK("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);
	siiTXInitParameter();
	if(siiTX.is_first_time == 1)
		siiTX.siiTXInputVideoMode = timingvideomode;

	siiTX.siiTXHDCPEnable=siiTXHDCPEnable;

#if 0
	plug_check = (siiTX.siiTXOutputState>0)?1:0;

	if (!(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN))
	{
#if 0
		if(hpd_callback != NULL)
		{
			actual_event = CSTVOUT_HDMI_PLUG_OUT;
			hpd_callback(&actual_event);
		}
		clock_tms_clockena(0);
#endif
	}
#endif

    while (!kthread_should_stop()){
		//DEBUG_PRINTK("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);

		siiTX.siiTXHDCPEnable=siiTXHDCPEnable;
		siiTXTaskHandler();

#if 0
		if(hpd_callback != NULL){
			plugging = plug_check;
			plug_check = (siiTX.siiTXOutputState>0)?1:0;
			//		printk("\nHDMI cable plugged , plug_check = %d, plugging = %d, pid 0x%x\n",plug_check, plugging,getpid());
			if(plugging^plug_check){
				if(plug_check==1 ){
					actual_event = CSTVOUT_HDMI_PLUG_IN;
					printk("\nHDMI cable plugged in, plug_check = %d, plugging = %d, pid 0x%x\n",plug_check, plugging,getpid());
					hpd_callback(&actual_event);
				}else{
					actual_event = CSTVOUT_HDMI_PLUG_OUT;
					printk("\nHDMI cable plugged out, plug_check = %d, plugging = %d, pid 0x%x\n",plug_check, plugging,getpid());
					hpd_callback(&actual_event);
				}
			}
		}
#endif

		timingvideomode_temp             = timingvideomode;
		siiTXInputAudioType_temp         = siiTXInputAudioType;
		siiTXInputAudioFs_temp           = siiTXInputAudioFs;
		siiTXInputAudioSampleLength_temp = siiTXInputAudioSampleLength;
		siiTXInputAudioChannel_temp      = siiTXInputAudioChannel;

		//When change HDMI output video resolution,
		//we must set "siiTX.siiTXInputVideoMode" firstly, then call function "siiTX_HDMI_Init()"

		if((timingvideomode_temp != timingvideomode_current)||(hdmi_wakeup_flag))
		{
			DEBUG_PRINTK("timingvideomode_temp = %d,timingvideomode_current = %d\n",timingvideomode_temp,timingvideomode_current);
			DEBUG_PRINTK("................................................\n");
			DEBUG_PRINTK(".....change timingvideomode......:\n");
			DEBUG_PRINTK("................................................\n");
			hdmi_wakeup_flag = 0;
			if(siiTX.siiTXOutputState != CABLE_UNPLUG_){
				siiTXWakeUp( OFF );
				timingvideomode_current = timingvideomode_temp;
				ChangeInitVideoMode(timingvideomode_current);
				siiTX_HDMI_Init();
			}
		}

		if((siiTX.siiTXOutputMode == HDMI_Mode) &&
				((siiTXInputAudioFs_temp != siiTX.siiTXInputAudioFs)
				 || (siiTXInputAudioType_temp!=siiTX.siiTXInputAudioType)
				 || (siiTXInputAudioSampleLength_temp!=siiTX.siiTXInputAudioSampleLength)
				 || (siiTXInputAudioChannel_temp!=siiTX.siiTXInputAudioChannel)))
		{
			DEBUG_PRINTK("change Audio Format : %d,%d,%d,%d\n",siiTXInputAudioFs_temp,siiTX.siiTXInputAudioFs,siiTXInputAudioType_temp,siiTX.siiTXInputAudioType);
			DEBUG_PRINTK(".................................................\n");
			DEBUG_PRINTK("change Audio Format :%d,%d,%d,%d\n",siiTXInputAudioSampleLength_temp,siiTX.siiTXInputAudioSampleLength,siiTXInputAudioChannel_temp,siiTX.siiTXInputAudioChannel);
			DEBUG_PRINTK(".....change Audio Format.......................\n");
			DEBUG_PRINTK(".................................................\n");
			siiTX.siiTXInputAudioFs           = siiTXInputAudioFs_temp;
			siiTX.siiTXInputAudioType         = siiTXInputAudioType_temp;
			siiTX.siiTXInputAudioSampleLength = siiTXInputAudioSampleLength_temp;
			siiTX.siiTXInputAudioChannel      = siiTXInputAudioChannel_temp;

			DEBUG_PRINTK("siiTXInputAudioFs_temp = %d\n",siiTXInputAudioFs_temp);
			DEBUG_PRINTK("siiTXInputAudioType_temp = %d\n",siiTXInputAudioType_temp);
			DEBUG_PRINTK("siiTXInputAudioSampleLength_temp = %d\n",siiTXInputAudioSampleLength_temp);
			DEBUG_PRINTK("siiTXInputAudioChannel_temp = %d\n",siiTXInputAudioChannel_temp);

			siiTXInitAudioPart();
			siiTXSetAudioPath();
			siiTXSetAudioInfoFramePacket();
		}

		msleep(10);
	}

	return 1;
}

#define SECURITY_CFG_ADDR         0xFFFFF010
#define SECURITY_CFG_UID_OFFSET   0x4

// Determine whether the chip has HDCP 1.x security.
//
// This is kind of a kludge, but all chips that have a
// Unique ID programmed also have HDCP 1.x keys.
static unsigned int HDMISecureChip(void)
{
	unsigned int base_addr = SECURITY_CFG_ADDR;
	unsigned int map_size = 0x10;
	void __iomem *security_cfg_addr;
	unsigned int result = 0;

	if (NULL == request_mem_region(base_addr, map_size, "security mem")) {
		return -1;
	}

	security_cfg_addr = ioremap(base_addr, map_size);
	if (security_cfg_addr == NULL)
	{
		return -1;
	}

	result = (*(unsigned int *)((unsigned int)security_cfg_addr + SECURITY_CFG_UID_OFFSET));

	iounmap(security_cfg_addr);

	release_mem_region(base_addr, map_size);

	return (result > 0);
}

int HDMIEnableHDCP(unsigned int Ena)
{
	if(Ena)
	{
		siiTXHDCPEnable =1;
	}
	else
	{
		siiTXHDCPEnable =0;
	}
	return 0;
}
#if 1
void HDMI_PowerDown(void)
{
	siiTXWakeUp( OFF );

	udelay(1000);
}
#endif

void hdmi_wake_up(int flag)
{
    down(&hdmi_dev->hdmi_sem);
    hdmi_events |= flag;
    up(&hdmi_dev->hdmi_sem);
    wake_up(&hdmi_wait_queue);
}

module_init(cnc_hdmi_init);
module_exit(cnc_hdmi_exit);

MODULE_AUTHOR("peng.xu@caviumnetworks.com");
MODULE_DESCRIPTION("HDMI driver for CNC1800L");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

