#ifndef __DF_REG_FMT_H__
#define __DF_REG_FMT_H__

typedef struct _DF_CTRL_REG_PARA_
{
	union{
		struct {
			unsigned int iUpdateReg:1;
			unsigned int reserved:31;
		}bits;

		unsigned int val;
	}df_update_reg;

	union{
		struct {
			unsigned int oOutIF1Err:1;
			unsigned int oOutIF1ErrType:7;
			unsigned int oOutIF2Err:1;
			unsigned int oOutIF2ErrType:7;
			unsigned int oOutIF1FrmSyncInterrupt:1;
			unsigned int oOutIF2FrmSyncInterrupt:1;
			unsigned int reserved_0:6;
			unsigned int oHd2sdErr:1;
			unsigned int reserved_1:3;
			unsigned int oHd2sdErrType:4;
		}bits;

		unsigned int val;
	}df_status_reg;

	union{
		struct{
			unsigned int outif0_int_for_risc:1;
			unsigned int outif1_int_for_risc:1;
			unsigned int outif0_int_for_host:1;
			unsigned int outif1_int_for_host:1;
			unsigned int reserved:28;
		}bits;

		unsigned int val;
	}df_int_clear;

	int iOutIF1IntClr;
	int iOutIF2IntClr;
	int iOutIF1ErrClr;
	int iOutIF2ErrClr;
	int iHd2sdErrClr;

	union{
		struct {
			unsigned int iScalerCoefTapIdx:2;
			unsigned int reserved_1:6;
			unsigned int iScalerCoefPhaseIdx:4;
			unsigned int reserved_2:4;
			unsigned int iScalerCoefType:3;
			unsigned int reserved_3:13;
		}bits;

		unsigned int val;
	}df_sca_coef_idx_reg;

	int iScalerCoefData;

}df_ctrl_reg;

typedef struct _DF_GFX_REG_PARA_
{
	union{
		struct{
			unsigned int iGfxEnable:2;
			unsigned int iGfxScalerEnable:1;
			unsigned int iColorKeyEnable:1;
			unsigned int iFetchType:2;
			unsigned int iRGB2YUVConvertEna:1;
			unsigned int reserved:25;
		}bits;

		unsigned int val;
	}df_gfx_control_reg;

	union {
		struct{
			unsigned int iColorFormat:3;
			unsigned int reserved_1:5;
			unsigned int iByteEndian:1;
			unsigned int  i128BitEndian:1;
			unsigned int i16BitEndian:1;
			unsigned int iNibbleEndian:1;
			unsigned int reserved_2:20;
		}bits;
		
		unsigned int val;
	}df_gfx_format_reg;

	union {
		struct{
			unsigned int iDefaultAlpha:8;
			unsigned int iKeyAlpha:8;
			unsigned int iArgb1555Alpha0:8;
			unsigned int iArgb1555Alpha1:8;
		}bits;
		
		unsigned int val;
	}df_gfx_alpha_control_reg;

	union {
		struct{
			unsigned int iKeyRedMin:8;
			unsigned int iKeyRedMax:8;
			unsigned int reserved:16;
		}bits;

		unsigned int val;
	}df_gfx_key_red_reg;

	union {
		struct{
			unsigned int iKeyBlueMin:8;
			unsigned int iKeyBlueMax:8;
			unsigned int reserved:16;
		}bits;

		unsigned int val;
	}df_gfx_key_blue_reg;

	union {
		struct{
			unsigned int iKeyGreenMin:8;
			unsigned int iKeyGreenMax:8;
			unsigned int reserved:16;
		}bits;

		unsigned int val;
	}df_gfx_key_green_reg;

	union {
		struct{
			unsigned int iStartAddr:24;
			unsigned int reserved:8;
		}bits;

		unsigned int val;
	}df_gfx_buf_start_addr_reg;

	union {
		struct{
			unsigned int iLinePitch:20;
			unsigned int reserved_1:4;
			unsigned int iBlankPixel:5;
			unsigned int reserved_2:3;
		}bits;

		unsigned int val;
	}df_gfx_line_pitch_reg;

	union {
		struct{
			unsigned int iXStart:11;
			unsigned int reserved_1:5;
			unsigned int iXEnd:11;
			unsigned int reserved_2:5;
		}bits;
		
		unsigned int val;
	}df_gfx_x_position_reg;

	union {
		struct{
			unsigned int iYStart:11;
			unsigned int reserved_1:5;
			unsigned int iYEnd:11;
			unsigned int reserved_2:5;
		}bits;
		
		unsigned int val;
	}df_gfx_y_position_reg;

	union {
		struct{
			unsigned int iScaleXStart:11;
			unsigned int reserved_1:5;
			unsigned int iScaleXEnd:11;
			unsigned int reserved_2:5;
		}bits;
		
		unsigned int val;
	}df_gfx_scl_x_position_reg;

	union {
		struct{
			unsigned int iClutAddr:8;
			unsigned int reserved:24;
		}bits;

		unsigned int val;
	}df_gfx_clut_addr_reg;

	union {
		struct{
			unsigned int iBlue:8;
			unsigned int iGreen:8;
			unsigned int iRed:8;
			unsigned int iAlpha:8;
		}bits;

		unsigned int val;
	}df_gfx_clut_data_reg;

}df_gfx_reg;

typedef struct _DF_VIDEO_REG_PARA_
{
	union {
		struct{
			unsigned int iVideoEna:2;
			unsigned int iLumaKeyEna:1;
			unsigned int iAutoCorrectTopBottomField:1;
			unsigned int iLumaScleVFIRTapNumSel:1;
			unsigned int reserved:25;
		}bits;

		unsigned int val;
	}df_video_control_reg;

	union {
		struct{
			unsigned int iDefaultAlpha:8;
			unsigned int reserved:8;
			unsigned int iLumaKeyAlpha0:8;
			unsigned int iLumaKeyAlpha1:8;
		}bits;

		unsigned int val;
	}df_video_alpha_control_reg;

	union {
		struct{
			unsigned int iLumaKeyMin:8;
			unsigned int iLumaKeyMax:8;
			unsigned int reserved:16;
		}bits;

		unsigned int val;
	}df_video_luma_key_reg;

	union {
		struct{
			unsigned int iDispXStart:11;
			unsigned int iDispXStartCropPixelNum:4;
			unsigned int reserved:1;
			unsigned int iDispXEnd:11;
			unsigned int iDispXEndCropPixelNum:4;
			unsigned int iDispXCropEnable:1;
		}bits;

		unsigned int val;
	}df_video_x_position_reg;

	union {
		struct{
			unsigned int iDispYStart:11;
			unsigned int reserved_1:5;
			unsigned int iDispYEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_y_position_reg;

	union {
		struct{
			unsigned int iSrcCropXOff:11;
			unsigned int reserved_1:5;
			unsigned int iSrcCropXWidth:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_x_crop_reg;

	union {
		struct{
			unsigned int iSrcCropYOff:11;
			unsigned int reserved_1:5;
			unsigned int iSrcCropYHeight:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_y_crop_reg;

	int df_video_cmd;
	int df_video_status_addr;
	int df_video_status_0;
	int df_video_status_1;
	int df_video_status_2;
	
	union {
		struct{
			unsigned int oSrcFrameWidth:11;
			unsigned int reserved_1:5;
			unsigned int oSrcFrameHeight:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_status_frame_size;

	union {
		struct{
			unsigned int oSrcIsHD:1;
			unsigned int oSrcVideoType:1;
			unsigned int oSrcH264Map:1;
			unsigned int oIsCMDFifoFull:1;
			unsigned int oSrcIsProgressSeq:1;
			unsigned int oSrcIsTopFld:1;
			unsigned int reserved_1:2;
			unsigned int oScalerVInitPhase:4;
			unsigned int oScalerHInitPhase:4;
			unsigned int oRepeatCnt:5;
			unsigned int reserved_2:3;
			unsigned int oCMDFIFOSize:5;
			unsigned int reserved_3:3;
		}bits;

		unsigned int val;
	}df_video_status_frame_info;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcYTopAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_y_topaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcYBotAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_y_botaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcCTopAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_c_topaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcCBotAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_c_botaddr;

	unsigned int oVideoDispNum;	
}df_video_reg;

typedef struct _DF_COMPOSITOR_REG_PARA_
{
	union {
		struct{
			unsigned int iBGY:8;
			unsigned int iBGU:8;
			unsigned int iBGV:8;
			unsigned int reserved:8;
		}bits;

		unsigned int val;
	}df_comp_back_ground;

	union{
		struct{
			unsigned int iVideo1ZOrder:2;
			unsigned int reserved_1:2;
			unsigned int iVideo2ZOrder:2;
			unsigned int reserved_2:2;
			unsigned int iGfx1ZOrder:2;
			unsigned int reserved_3:2;
			unsigned int iGfx2ZOrder:2;
			unsigned int reserved_4:18;
		}bits;

		unsigned int val;
	}df_comp_z_order;
}df_compositor_reg;

typedef struct _DF_HD2SD_REG_PARA_
{
	union{
		struct{
			unsigned int iHD2SDEna:1;
			unsigned int iSourceSel:1;
			unsigned int iByPassScaler:1;
			unsigned int reserved_1:1;
			unsigned int iVerticalReverseStore:1;
			unsigned int iHorizontalReverseStore:1;
			unsigned int iIsFrame:1;
			unsigned int iIsHD:1;
			unsigned int iDramFIFODepthMinus1:4;
			unsigned int reserved_2:4;
			unsigned int iVInitPhase:4;
			unsigned int iHInitPhase:4;
			unsigned int reserved_3:8;
		}bits;

		unsigned int val;
	}df_hd2sd_control;

	union {
		struct{
			unsigned int iDesWidth:11;
			unsigned int reserved_1:5;
			unsigned int iDesHeight:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_hd2sd_des_size;

	unsigned int iDesYAddr;
	unsigned int  iDesCAddr;
	unsigned int  iDesBufPith;

	int oCurBufIdx;
}df_hd2sd_reg;

typedef struct _DF_OUTIF_REG_PARA
{
	union {
		struct{
			unsigned int iDispEna:1;
			//unsigned int iClkOutSel:3;
			unsigned int iClockOutPhase:1;
			unsigned int iRepeatTimes:2;
			unsigned int iIsHD:1;
			unsigned int iIsInterlaced:1;
			unsigned int iYCMux:1;
			unsigned int iIsPal:1;
			unsigned int iTopOrFrameIntEna:1;
			unsigned int iBotIntEna:1;
			unsigned int iChoromaDrop:1;
			unsigned int iColorSpace:1;
			unsigned int iHSyncPolarity:1;
			unsigned int iVSyncPolarity:1;
			unsigned int iColorModulator:1;
			unsigned int reserved:17;
		}bits;

		unsigned int val;
	}df_outif_control;

	union {
		struct{
			unsigned int iXTotal:12;
			unsigned int reserved_1:4;
			unsigned int iXActiveStart:12;
			unsigned int reserved_2:4;
		}bits;

		unsigned int val;
	}df_outif_x_size;

	union {
		struct{
			unsigned int iYTotal:12;
			unsigned int reserved:4;
			unsigned int iYActiveStart:12;
			unsigned int reserved_2:4;
		}bits;

		unsigned int val;
	}df_outif_y_size;

	union {
		struct{
			unsigned int iHSyncFrontProch:10;
			unsigned int iHSyncWidth:10;
			unsigned int iHSyncBackProch:10;
			unsigned int reserved_2:2;
		}bits;

		unsigned int val;
	}df_outif_hsync;

	union {
		struct{
			unsigned int iVSyncFrontProch:8;
			unsigned int iVSyncWidth:8;
			unsigned int iVSyncBackProch:8;
			unsigned int reserved_2:2;
		}bits;

		unsigned int val;
	}df_outif_vsync;

	union {
		struct{
			unsigned int iClipEnable:1;
			unsigned int reserved:7;
			unsigned int iYLow:8;
			unsigned int iYRange:8;
			unsigned int iCRange:8;
		}bits;

		unsigned int val;
	}df_outif_clip;

	union {
		struct{
			unsigned int iCoef00:9;
			unsigned int reserved_0:1;
			unsigned int iCoef01:9;
			unsigned int reserved_1:1;
			unsigned int iCoef02:9;
			unsigned int reserved:3;
		}bits;

		unsigned int val;
	}df_outif_cm_coef0_012;

	union {
		struct{
			unsigned int iCoef03:13;
			unsigned int reserved:19;
		}bits;

		unsigned int val;
	}df_outif_cm_coef0_3;

	union {
		struct{
			unsigned int iCoef10:9;
			unsigned int reserved_0:1;
			unsigned int iCoef11:9;
			unsigned int reserved_1:1;
			unsigned int iCoef12:9;
			unsigned int reserved:3;
		}bits;

		unsigned int val;
	}df_outif_cm_coef1_012;

	union {
		struct{
			unsigned int iCoef13:13;
			unsigned int reserved:19;
		}bits;

		unsigned int val;
	}df_outif_cm_coef1_3;

	union {
		struct{
			unsigned int iCoef20:9;
			unsigned int reserved_0:1;
			unsigned int iCoef21:9;
			unsigned int reserved_1:1;
			unsigned int iCoef22:9;
			unsigned int reserved:3;
		}bits;

		unsigned int val;
	}df_outif_cm_coef2_012;

	union {
		struct{
			unsigned int iCoef23:13;
			unsigned int reserved:19;
		}bits;

		unsigned int val;
	}df_outif_cm_coef2_3;

	union {
		struct{
			unsigned int iWidth:12;
			unsigned int reserved_0:4;
			unsigned int iHeight:12;
			unsigned int reserved_1:4;
		}bits;

		unsigned int val;
	}df_outif_disp_size;

	union {
		struct{
			unsigned int oBotFlag:1;
			unsigned int oFrameSync:1;
			unsigned int oVSync:1;
			unsigned int oActiveLine:1;
			unsigned int reserved_1:12;
			unsigned int oLineCnt:11;
			unsigned int reserved_2:4;
		}bits;

		unsigned int val;
	}df_outif_status_line;

}df_outif_reg;

typedef struct _DF_VIDEOAUX_REG_PARA
{
	union {
		struct{
			unsigned int iVideoEna:2;
			unsigned int reserved_0:3;
			unsigned int iAutoCorrectTopBottomField:1;
			unsigned int reserved_1:26;
		}bits;

		unsigned int val;
	}df_video_control_reg;

	union {
		struct{
			unsigned int iDefaultAlpha:8;
			unsigned int reserved:24;
		}bits;

		unsigned int val;
	}df_video_alpha_control_reg;

	union {
		struct{
			unsigned int iDispXStart:11;
			unsigned int iDispXStartCropPixelNum:4;
			unsigned int reserved:1;
			unsigned int iDispXEnd:11;
			unsigned int iDispXEndCropPixelNum:4;
			unsigned int iDispXCropEnable:1;
		}bits;

		unsigned int val;
	}df_video_x_position_reg;

	union {
		struct{
			unsigned int iDispYStart:11;
			unsigned int reserved_1:5;
			unsigned int iDispYEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_y_position_reg;

	union {
		struct{
			unsigned int iSrcCropXOff:11;
			unsigned int reserved:21;
		}bits;

		unsigned int val;
	}df_video_src_x_crop_reg;

	union {
		struct{
			unsigned int iSrcCropYOff:11;
			unsigned int reserved:21;
		}bits;

		unsigned int val;
	}df_video_src_y_crop_reg;

	int df_video_cmd;
	int df_video_status_addr;
	int df_video_status_0;
	int df_video_status_1;
	int df_video_status_2;
	
	union {
		struct{
			unsigned int oSrcFrameWidth:11;
			unsigned int reserved_1:5;
			unsigned int oSrcFrameHeight:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_status_frame_size;

	union {
		struct{
			unsigned int oSrcIsHD:1;
			unsigned int oSrcVideoType:1;
			unsigned int oSrcH264Map:1;
			unsigned int oIsCMDFifoFull:1;
			unsigned int oSrcIsProgressSeq:1;
			unsigned int oSrcIsTopFld:1;
			unsigned int reserved_1:2;
			unsigned int oScalerVInitPhase:4;
			unsigned int oScalerHInitPhase:4;
			unsigned int oRepeatCnt:5;
			unsigned int reserved_2:3;
			unsigned int oCMDFIFOSize:5;
			unsigned int reserved_3:3;
		}bits;

		unsigned int val;
	}df_video_status_frame_info;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcYTopAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_y_topaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcYBotAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_y_botaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcCTopAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_c_topaddr;

	union {
		struct{
			unsigned int reserved_1:4;
			unsigned int oSrcCBotAddr:25;
			unsigned int reserved_2:3;
		}bits;
		
		unsigned int val;
	}df_video_status_c_botaddr;

	unsigned int oVideoDispNum;	
}df_videoaux_reg;

typedef struct _DF_REG_PARA_
{
	df_ctrl_reg  Ctrl;
	df_gfx_reg   Gfx[2];
	df_video_reg Video[2];
	df_compositor_reg Comp[2];
	df_hd2sd_reg HD2SD;
	df_outif_reg OutIF[2];
	df_videoaux_reg VideoAUX[1];
}df_reg_para;

#endif
