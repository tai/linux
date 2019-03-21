#ifndef __CNCFB_HW_H__
#define __CNCFB_HW_H__

void df_writel(int addr, int val);
int df_readl(int addr);

int cncfb_hw_init(struct fb_info *info, int en_flags, struct cnc_info *);
void cncfb_hw_exit(void);

int cncfb_hw_set_par(struct fb_info *info, struct cnc_info *);
int cncfb_hw_pan_display(struct fb_info *info, struct cnc_info *);
int cncfb_hw_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                       u_int transp, struct fb_info *info, struct cnc_info *);

int cncfb_hw_blank(int blank);
int cncfb_hw_waitforvsync(struct fb_info *info);

int cncfb_hw_gfx_on(struct fb_info *info, unsigned int args, struct cnc_info *);
int cncfb_hw_gfx_alpha(struct fb_info *info, unsigned int args, struct cnc_info *);

int cncfb_hw_z_order(struct fb_info *info,unsigned int arg);

int cncfb_hw_colorkey_val(struct fb_info *info, gfx_colorkey *col_key, struct cnc_info *);
int cncfb_hw_colorkey_on(struct fb_info *info, unsigned int arg, struct cnc_info *);


int cncfb_hw_setbgcolor(struct fb_info *info, unsigned int arg);
int cncfb_hw_waitforvsync(struct fb_info *info);
#endif /* __CNCFB_HW_H__ */
