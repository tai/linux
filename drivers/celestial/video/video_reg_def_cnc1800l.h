#ifndef __VIDEO_REG_DEF_H__
#define __VIDEO_REG_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CSM_VIDEO_REG_BASE (0xb1600000)
#define CSM_VIDEO_REG_SIZE	(0x100000)

// mailbox
#define HOST_IF_BASE_ADDR_REG  		 (0x00)//VID_MIPS_MAILBOX_0
//#define HOST_PARA_UPDATE_FLAG_REG  	(0x01)//VID_MIPS_MAILBOX_1
//#define HOST_CMD_REG  				(0x02)//VID_MIPS_MAILBOX_2
#define DF_STATUS_SWITCH_REG  		(0x03)//VID_MIPS_MAILBOX_3
#define VID_INT_TYPE_REG  			(0x04)//VID_MIPS_MAILBOX_4
#define HOST_DPB_STATUS  			(0x05)//VID_MIPS_MAILBOX_5
#define VIB_PARA_REG  				(0x06)//VID_MIPS_MAILBOX_6
#define MIPS_DPB_STATUS  			(0x07)//VID_MIPS_MAILBOX_7
#define VID_MIPS_MAILBOX_8  		(0x08)
#define VID_MIPS_MAILBOX_9  		(0x09)
#define VID_MIPS_MAILBOX_10 		(0x0A)
#define VID_MIPS_MAILBOX_11 		(0x0B)
#define VID_MIPS_MAILBOX_12 		(0x0C)
#define VID_MIPS_MAILBOX_13 		(0x0D)
#define VID_MIPS_MAILBOX_14 		(0x0E)
#define VID_MIPS_MAILBOX_15 		(0x0F)
#define VID_MIPS_MAILBOX_16  		(0x10)
#define VID_MIPS_MAILBOX_17  		(0x11)
#define VID_MIPS_MAILBOX_18  		(0x12)
#define VID_MIPS_MAILBOX_19  		(0x13)
#define VID_MIPS_MAILBOX_20  		(0x14)
#define VID_MIPS_MAILBOX_21  		(0x15)
#define VID_MIPS_MAILBOX_22  		(0x16)
#define VID_MIPS_MAILBOX_23  		(0x17)
#define VID_MIPS_MAILBOX_24  		(0x18)
#define VID_MIPS_MAILBOX_25  		(0x19)
#define VID_MIPS_MAILBOX_26 		(0x1A)
#define VID_MIPS_MAILBOX_27 		(0x1B)
#define VID_MIPS_MAILBOX_28 		(0x1C)
#define VID_MIPS_MAILBOX_29 		(0x1D)
#define VID_MIPS_MAILBOX_30 		(0x1E)
#define VID_MIPS_MAILBOX_31 		(0x1F)
#define VID_MIPS_MAILBOX_32  		(0x20)
#define VID_MIPS_MAILBOX_33  		(0x21)
#define VID_MIPS_MAILBOX_34  		(0x22)
#define VID_MIPS_MAILBOX_35  		(0x23)
#define VID_MIPS_MAILBOX_36  		(0x24)
#define VID_MIPS_MAILBOX_37  		(0x25)
#define VID_MIPS_MAILBOX_38  		(0x26) 
#define VID_MIPS_MAILBOX_39  		(0x27)
#define VID_MIPS_MAILBOX_40  		(0x28)
#define VID_MIPS_MAILBOX_41  		(0x29)
#define VID_MIPS_MAILBOX_42 		(0x2A)
#define VID_MIPS_MAILBOX_43 		(0x2B)
#define VID_MIPS_MAILBOX_44 		(0x2C)
#define VID_MIPS_MAILBOX_45 		(0x2D)
#define VID_MIPS_MAILBOX_46 		(0x2E)
#define VID_MIPS_MAILBOX_47 		(0x2F)
#define VID_MIPS_MAILBOX_48  		(0x30)
#define VID_MIPS_MAILBOX_49  		(0x31)
#define VID_MIPS_MAILBOX_50  		(0x32)
#define VID_MIPS_MAILBOX_51  		(0x33)
#define VID_MIPS_MAILBOX_52  		(0x34)
#define VID_MIPS_MAILBOX_53  		(0x35)
#define VID_MIPS_MAILBOX_54  		(0x36)
#define VID_MIPS_MAILBOX_55  		(0x37)
#define VID_MIPS_MAILBOX_56  		(0x38)
#define HOST_PARA_UPDATE_FLAG_REG  		(0x39)//HOST_PARA_UPDATE_FLAG_REG
#define HOST_CMD_REG 		(0x3A)//HOST_CMD_REG

#define VID_UDB						(0x3B)//VID_MIPS_MAILBOX_59
#define FW_VERSION_REG 			(0x3C)//VID_MIPS_MAILBOX_60(VID_MIPS_STA0)
#define MIPS_STATUS_REG 			(0x3D)//VID_MIPS_MAILBOX_61(VID_MIPS_STA1)
#define VID_MIPS_STA2 				(0x3E)//VID_MIPS_MAILBOX_62
#define KEY_CONTROL_REG			(0x3F)//VID_MIPS_MAILBOX_63
#define VID_CPBRP0					(0x40)
#define VID_CPBWP0					(0x41)
#define VID_HISTC					(0x42)
#define VID_STC0						(0x43)
#define VID_CPBRP1					(0x44)
#define VID_CPBWP1					(0x45)
#define VID_WR_TO_AUDIO			(0x46)
#define VID_RD_FROM_AUDIO			(0x47)
#define VID_WR_TO_AUDIO1			(0x48)
#define VID_RD_FROM_AUDIO1		(0x49)
#define VID_VERSION					(0x4A)

// interrupt control registers
#define RISC_INT_ADDR_BASE		(0xB1C10000)
#define RISC_INT_ADDR_SIZE		(0x1000)

#define VID_RISC_ENA         		(0x00>>2)
#define VID_RISC_MASK			(0x08>>2)
#define VID_RISC_INT         		(0x10>>2)
#define VID_RISC_STA         		(0x30>>2)
#define VID_HOST_ENA         		(0xC0>>2)
#define VID_HOST_MASK    		(0xC4>>2)
#define VID_HOST_INT         		(0xC8>>2)
#define VID_HOST_STA         		(0xD4>>2)

#define CPB_DIR_POINTER_BASE_ADDR	(VID_CPBWP0)

#ifdef __cplusplus
}
#endif

#endif
