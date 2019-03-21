#ifndef _HDMI_API_H_
#define _HDMI_API_H_

#include <linux/semaphore.h>
#include "si_cec_enums.h"

#define HDMI_SETMODE               _IOW('h', 1, int)
#define HDMI_CHECKEVENT            _IOW('h', 2, int)
#define HDMI_GETEDIDDATALEN        _IOW('h', 3, int)
#define HDMI_GETEDIDDATA           _IOW('h', 4, int)
#define HDMI_CEC_ENABLE            _IOW('h', 5, int)
#define HDMI_ENA_VENDORSPECIFICIF  _IOW('h', 6, int)
#define HDMI_DIS_VENDORSPECIFICIF  _IOW('h', 7, int)
#define HDMI_CHECKCECSUPPORT       _IOW('h', 8, int)
#define HDMI_CHECKCECSTATUS        _IOW('h', 9, int)
#define HDMI_CEC_ONETOUCHPLAY      _IOW('h', 10, int)
#define HDMI_CEC_SETLA             _IOW('h', 11, int)
#define HDMI_CEC_SETCAPTUREID      _IOW('h', 12, int)
#define HDMI_CEC_GETACKSTATUS      _IOW('h', 13, int)
#define HDMI_CEC_CLEARACKSTATUS    _IOW('h', 14, int)
#define HDMI_CEC_SENDMESSAGE       _IOW('h', 15, int)
#define HDMI_CEC_READMESSAGE       _IOW('h', 16, int)
#define HDMI_SET_AUDIO             _IOW('h', 17, int)
#define HDMI_ENA                   _IOW('h', 18, int)

#define HDMI_EVENT_HPD_HIGH        0x1
#define HDMI_EVENT_HPD_LOW         0x2
#define HDMI_EVENT_CEC_MSG         0x4

typedef struct
{
    int task_running;
    struct semaphore hdmi_sem;
    struct task_struct *task;
}HDMI_DEV;

struct HDMI_Audio_Param
{
      unsigned int AudioFsId;
      unsigned int AudioTypeId;
      unsigned int AudioLenId;
      unsigned int AudioChannelId;
};

extern unsigned char *Raw_Data;
extern unsigned char EDID_ERROR;

enum _HDMI_VIDEO_TIMING_
{
	HDMI_NONE_STD_VID_FORMAT       =0,
	HDMI_V640x480p_60Hz            =1,
	HDMI_V720x480p_60Hz_4x3        =2,
	HDMI_V720x480p_60Hz_16x9       =3,
	HDMI_V720x480i_60Hz_4x3        =4,
	HDMI_V720x480i_60Hz_16x9       =5,
	HDMI_V720x576i_50Hz_4x3        =6,
	HDMI_V720x576i_50Hz_16x9       =7,
	HDMI_V720x576p_50Hz_4x3        =8,
	HDMI_V720x576p_50Hz_16x9       =9,
	HDMI_V1280x720p_50Hz           =10,
	HDMI_V1280x720p_60Hz           =11,
	HDMI_V1920x1080i_50Hz          =12,
	HDMI_V1920x1080i_60Hz          =13,
	HDMI_V1920x1080p_50Hz          =14,
	HDMI_V1920x1080p_60Hz          =15,
	HDMI_V1920x1080p_24Hz          =16,

	HDMI_V800x600p_60Hz		=17,
	HDMI_V1024x768p_60Hz		= 18,
	HDMI_V1280x720p_RGB_60Hz           =19,
	HDMI_V800x480p_60Hz		=20,
	HDMI_V1440x900p_60Hz		=21,
	HDMI_V1280x1024p_60Hz	=22,
	HDMI_V1360x768p_60Hz		=23,
	HDMI_V1920x1080p_RGB_30Hz          =24,
	HDMI_V1920x1080p_RGB_60Hz          =25,
	HDMI_V1920x1080i_RGB_30Hz          =26,
	HDMI_V1366x768p_60Hz		=27,
};

enum _HDMI_VIDEO_ASPECT_
{
	HDMI_ASPECT_4x3,
	HDMI_ASPECT_16x9,
};

enum _HDMI_AUDIO_TYPE_
{
	HDMI_AUDIO_SPDIF =0,
	HDMI_AUDIO_I2S   =1,
	HDMI_AUDIO_DSD   =2,
	HDMI_AUDIO_HBR   =3,

};

enum _HDMI_AUDIO_FS_
{
	HDMI_AUDIO_44_1KHz   =0,
	HDMI_AUDIO_48KHz     =2,
	HDMI_AUDIO_32KHz     =3,
	HDMI_AUDIO_88_2KHz   =8,
	HDMI_AUDIO_768KHz    =9,
	HDMI_AUDIO_96KHz     =0xA,
	HDMI_AUDIO_176_4KHz  =0xC,
	HDMI_AUDIO_192KHz    =0xE,
};

enum _HDMI_AUDIO_LEN_
{
	HDMI_AUDIO_16BIT     =2,
	HDMI_AUDIO_18BIT     =8,
	HDMI_AUDIO_19BIT     =9,
	HDMI_AUDIO_20BIT     =0xA,
	HDMI_AUDIO_17BIT     =0xB,
	HDMI_AUDIO_22BIT     =0x5,
	HDMI_AUDIO_23BIT     =0x9,
	HDMI_AUDIO_24BIT     =0xB,
	HDMI_AUDIO_21BIT     =0xD,
};

enum _HDMI_AUDIO_CHANNEL_
{
	HDMI_AUDIO_2_CHANNEL =1,
	HDMI_AUDIO_3_CHANNEL =2,
	HDMI_AUDIO_4_CHANNEL =3,
	HDMI_AUDIO_5_CHANNEL =4,
	HDMI_AUDIO_6_CHANNEL =5,
	HDMI_AUDIO_7_CHANNEL =6,
	HDMI_AUDIO_8_CHANNEL =7,
};

int HDMISetVideo(unsigned int VideoTimingId,unsigned int PictureAspectId);
int HDMISetAR(unsigned int ar);
int HDMISetAudio(unsigned int AudioFsId,unsigned int AudioTypeId,unsigned int AudioLenId,unsigned int AudioChannelId);
int HDMIEnableHDCP(unsigned int Ena);
char HDMI_PlugInNotify(void );
int HDMI_Suspend(void);
int HDMI_Resume(void);
int HDMI_Enable_VendorSpecificIF(int format);
int HDMI_Disable_VendorSpecificIF(void);
void hdmi_wake_up(int flag);

#endif
