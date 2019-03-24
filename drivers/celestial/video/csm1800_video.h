#ifndef __CSM1800_VIDEO_H__
#define __CSM1800_VIDEO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VIDEO_MAJOR 300

#define MAX_CHANNEL_NUM (8)

#define CSM_VIDEO_REG_BASE (0x41600000)
#define CSM_VIDEO_REG_SIZE	(0x100000)

#define BIT_CODE_SIZE           0x00061000
#define BIT_PARA_SIZE           0x00002000
#define BIR_WORK_SIZE           0x000dd000

#define VIDEO_MCU_REGION (VIDEO_STUFF_REGION) // video firmware
#define VIDEO_MCU_SIZE (0x80000)
#define VIDEO_CODA_OFFSET ((VIDEO_MCU_SIZE+BIT_PARA_SIZE +BIR_WORK_SIZE)>> 2)
#define VIDEO_CODA_REGION (VIDEO_MCU_REGION + VIDEO_MCU_SIZE + BIT_PARA_SIZE + BIR_WORK_SIZE)  //coda firmware
#define VIDEO_CODA_SIZE (0x180000)
#define VIDEO_HOST_OFFSET ((VIDEO_CODA_SIZE + VIDEO_MCU_SIZE) >> 2)
#define VIDEO_HOST_IF_REGION (VIDEO_CODA_REGION +VIDEO_CODA_SIZE -BIT_PARA_SIZE -BIR_WORK_SIZE) // handshake between host and firmware
#define VIDEO_HOST_IF_SIZE (0x100000)

#define MIPS_ISRAM_MEM_SIZE	(1024 * 9)	/* 9KB */
#define MIPS_DSRAM_MEM_SIZE	(1024 * 16)	/* 16KB */

#define INST_OUT_MEM_SIZE	(1024 * 1)
#define INST_MIPS_MEM_SIZE	(1024 * 8)

#define MIPS_ISRAM_OFFSET	((0x41690000 - CSM_VIDEO_REG_BASE) >> 2)
#define MIPS_DSRAM_OFFSET	((0x416A0000 - CSM_VIDEO_REG_BASE) >> 2)
#define MIPS_RESET_OFFSET	((0x416B0000 - CSM_VIDEO_REG_BASE) >> 2)

#define MEM_SWAP32(x)	( (((x) & 0xff) << 24) | (((x) & 0xff00) <<8) |	(((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24) )

#define CSVID_IOC_GET_VIDEO_CPB_ADDR	_IOW('z', 0x01, int)
#define CSVID_IOC_GET_VIDEO_CPB_SIZE	_IOW('z', 0x02, int)
#define CSVID_IOC_GET_VIDEO_CPB_DIR_ADDR	_IOW('z', 0x03, int)
#define CSVID_IOC_GET_VIDEO_CPB_DIR_SIZE	_IOW('z', 0x04, int)

#define CSVID_IOC_GET_VIDEO_HOST_IF_ADDR	_IOW('z', 0x05, int)
#define CSVID_IOC_GET_VIDEO_HOST_IF_SIZE	_IOW('z', 0x06, int)
#define CSVID_IOC_GET_VIDEO_REG_ADDR		_IOW('z', 0x07, int)
#define CSVID_IOC_GET_VIDEO_REG_SIZE		_IOW('z', 0x08, int)

#define CSVID_IOC_GET_NOTIFY_TYPE	_IOW('z', 0x09, int)
#define CSVID_IOC_SET_NOTIFY_TYPE	_IOW('z', 0x0a, int)

#define CSVID_IOC_GET_XPORT_VIDEO	_IOW('z', 0x0b, int)
#define CSVID_IOC_SET_VIB_PINMUX _IOW('z', 0x0c, int)

#define CSVID_IOC_GET_VIDEO_UD_ADDR _IOW('z', 0x0d, int)
#define CSVID_IOC_GET_VIDEO_UD_SIZE _IOW('z', 0x0e, int)

#define CSVID_IOC_RELOAD_FIRMWARE _IOW('z', 0x0f, int)

// test
#define CSVID_IOC_SET_VIDEO_START		_IOW('z', 0xfe, int)

enum
{
	SDK_FW_IF_PARA_SIZE		= (16*1024),	/* 16KB */
	DINT_BACK_GROUD_SIZE		= (64*1024),	/* 64KB */
	DINT_SCALER_COFF_SIZE		= (64*1024),	/* 64KB */
	ENCODER_TABLE_SIZE		= (2*1024),	/* 2KB */
	DEC_SWITCH_DATA_SIZE		= (64*1024),	/* 64KB */
	DINT_SWITCH_DATA_SIZE	= (64*1024),	/* 64KB */
	ENC_PARA_DATA_SIZE		= (1*1024),	/* 1KB */ 
};

enum
{
	SDK_FW_IF_PARA_OFFSET   = 0,       
	DINT_BACK_GROUD_OFFSET  = SDK_FW_IF_PARA_OFFSET + SDK_FW_IF_PARA_SIZE,
	DINT_SCALER_COFF_OFFSET = DINT_BACK_GROUD_OFFSET + DINT_BACK_GROUD_SIZE,
	ENCODER_TABLE_OFFSET    = DINT_SCALER_COFF_OFFSET + DINT_SCALER_COFF_SIZE,
	DEC_SWITCH_DATA_OFFSET  = ENCODER_TABLE_OFFSET + ENCODER_TABLE_SIZE,
	DINT_SWITCH_DATA_OFFSET = DEC_SWITCH_DATA_OFFSET + DEC_SWITCH_DATA_SIZE,
	ENC_PARA_DATA_OFFSET    = DINT_SWITCH_DATA_OFFSET + ENC_PARA_DATA_SIZE,
};

typedef struct _CPB_CFG_PARA_t_
{
	unsigned int cpb_buffer_low;
	unsigned int cpb_buffer_up;
	unsigned int cpb_dir_low;
	unsigned int cpb_dir_up;
	unsigned int cpb_dir_rp;
	unsigned int cpb_dir_wp;
    	unsigned int stream_type;
}_CPB_PARA_t;

typedef struct _UDP_CFG_PARA_t_
{
	unsigned int udp_buffer_low;
	unsigned int udp_buffer_up;
	unsigned char udp_wp_index; // bit7: toggle bit
	unsigned char udp_rp_index; // bit7: toggle bit
	unsigned short udp_block_size;
	unsigned int reserved;
}_UDP_PARA_t;

typedef struct _video_channel_cfg_para_t_
{
	_CPB_PARA_t	cpb_cfg_para;		// CPB configure
#if 0
	unsigned int	udp_region;			// user data start address
#else
	_UDP_PARA_t  udp_cfg_para; 			// user data configure
#endif
	unsigned int	host_cfg_para;		// host configure
	unsigned int	fw_running_sta;		// firmware running status
	unsigned int	vid_src_format;		// video source format
	unsigned int	vid_aspect_ratio;	// video aspect ratio
	unsigned int	vid_dint_format;		// video format after deinterlacer
	unsigned int	vid_frame_pts;		// current frame PTS
	unsigned int	host_set_time_code;	// host set the mark time code
	unsigned int	vid_time_code;		// current frame time code
	unsigned int	vid_start_delay;		// unit : ms
	unsigned int	vid_display_info;		// 31~16: sync is ok; 15~0: has display context
	unsigned int	reserved;
}_VIDEO_CHANNEL_CFG_PARA_t;

typedef struct _VIDEO_RECT_t_
{
	unsigned short x_start;
	unsigned short x_end;
	unsigned short y_start;
	unsigned short y_end;
}_VIDEO_RECT_t;

typedef struct _WINDOW_CROP_t_
{
    unsigned short CropXOff;
    unsigned short CropXWidth;
    unsigned short CropYOff;
    unsigned short CropYHeight;
}_WINDOW_CROP_t;

typedef enum
{
	VIDEO_LAYER_TYPE_CONFERENCE = 0,
	VIDEO_LAYER_TYPE_PIP,
}_VIDEO_LAYER_TYPE_e;

typedef struct _HOST_VIDEO_LAYER_INFO_t_
{
	unsigned char			WinNum;		
	unsigned char			type;
	unsigned short		BackGroundNum;
	_VIDEO_RECT_t		LayerPosition;
	unsigned char			WinOrder[MAX_CHANNEL_NUM];
	unsigned char			WinAlpha[MAX_CHANNEL_NUM];// for display
	_VIDEO_RECT_t		WinPosition[MAX_CHANNEL_NUM];
	_WINDOW_CROP_t		WinCrop[MAX_CHANNEL_NUM];
	unsigned int  reserved1;
	unsigned int  reserved2;
	unsigned int  reserved3;
	unsigned int  reserved4;
}_HOST_VIDEO_LAYER_INFO_t;

typedef enum
{
	FBS_FREE     = 0,	//free
	FBS_USING    = 1,     
	FBS_USED     = 2,
	FBS_WFD      = 3,
}_DPB_STATUS;

typedef struct{
	char	sliceMode;
	char	sliceSizeMode;
	unsigned short sliceSize;
}_EncSliceMode;

#define	ENC_DPB_BUFFER_SIZE 6

typedef struct{
    unsigned int status[ENC_DPB_BUFFER_SIZE]; // DPB_STATUS typedef
} _EncInputBuf;

typedef struct _yuv_para_
{
	unsigned short width;
	unsigned short height;
	unsigned int pts_value; //0:invalid value
}_YUV_PARA;//each yuv input para

typedef struct _coda_enc_para_
{
	char		channel_idx;
	char		encode_idx;
	char		encoder_type; // ENC_MP4 = 1,ENC_H263 = 2,ENC_AVC = 3,ENC_MJPG = 4, 
	char		cbcr_interleaved; // 0: uv in seperate mode 1: uv in interleave mode
	char		input_mode; // 0: vib input 1: yuv input
//	char		use_encode;
//	char		use_display;
	char		encoder_format; // except MODE420 mode is only for mjpeg encoder
	char		rcIntraQp; // avc:0-51; h263/mpeg4:1-31
	char		gopSize; // 0-60
//	char		encode_buffer_size;
	unsigned int	frame_rate; // 0: free bitrate
	unsigned int	bitrate;
	_EncInputBuf	input_buf;
	_EncSliceMode	slicemode;
	_YUV_PARA		yuv_para[ENC_DPB_BUFFER_SIZE];
	unsigned int		yuv_start_address;
}_CODA_ENC_PARA;

typedef struct _yuv_dec_para_
{
	unsigned int yuv_dec_start_address;
	unsigned short width;
	unsigned short height;
	unsigned int pts_value[ENC_DPB_BUFFER_SIZE]; // 0: invalid value
	_DPB_STATUS status[ENC_DPB_BUFFER_SIZE]; // DPB_STATUS typedef
	char frame_rate;
	unsigned int reserved;
}_YUV_DEC_PARA; // each yuv input para

typedef struct _sdk_fw_interface_
{
	unsigned int					BackGroudBaseAddr;
	unsigned int					channel_num;			// bit enable
	unsigned int					dpb_region;				// DPB buffer start address
	unsigned int					dpb_size;				// DPB sum size
	unsigned int					coda_bit_code_addr;		// Coda firmware start address
	unsigned int					vib_host_cfg;			// VIB control patameter
	_VIDEO_CHANNEL_CFG_PARA_t	video_channel_para[MAX_CHANNEL_NUM];
	_HOST_VIDEO_LAYER_INFO_t	video_display_layer[2];
	_CODA_ENC_PARA				enc_para;
	int							host_set_frame_rate;
	_YUV_DEC_PARA  				yuv_dec_para;
}_HOST_FW_IF_t;

#define VIDEO_NOTIFY_TYPE_ENABLE 0x80000000
#define VIDEO_NOTIFY_TYPE_TIMECODE 0x1
#define VIDEO_NOTIFY_TYPE_DECODERERROR 0x2
#define VIDEO_NOTIFY_TYPE_ASPECTRATIO 0x4
#define VIDEO_NOTIFY_TYPE_PANSCANCROP 0x8
#define VIDEO_NOTIFY_TYPE_SYNC1 0x10
#define VIDEO_NOTIFY_TYPE_SYNC2 0x20
#define VIDEO_NOTIFY_TYPE_UNDERFLOW 0x40
#define VIDEO_NOTIFY_TYPE_FORMATCHANGE 0x80
#define VIDEO_NOTIFY_TYPE_SRC_FORMATCHANGE 0x100
#define VIDEO_NOTIFY_TYPE_DECODE_FINISHED 0x200


#ifdef __cplusplus
}
#endif

#endif

