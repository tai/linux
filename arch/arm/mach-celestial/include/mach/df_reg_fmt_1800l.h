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
			unsigned int iUpdateRtReg:1;
			unsigned int reserved:31;
		}bits;

		unsigned int val;
	}df_update_rt_reg;

	union{
		struct{
			unsigned int oCopIntStaDisp1Err:1;  
			unsigned int oCopIntStaDisp2Err:1;    
			unsigned int oCopIntStaHd2sdErr:1;    
			unsigned int oCopIntStaDisp1TopVSync:1;
			unsigned int oCopIntStaDisp1BotVSync:1;
			unsigned int oCopIntStaDisp2TopVSync:1;
			unsigned int oCopIntStaDisp2BotVSync:1;
			unsigned int oCopIntStaHd2sdFinish:1;    
			unsigned int oCopIntStaF2fFinish:1;
			unsigned int reserved:23;
		}bits;

		unsigned int val;
	}df_cop_int_status_reg;

	union{
		struct{
			unsigned int iCopIntMskDisp1Err:1;    
			unsigned int iCopIntMskDisp2Err:1;    
			unsigned int iCopIntMskHd2sdErr:1;    
			unsigned int iCopIntMskDisp1TopVSync:1;
			unsigned int iCopIntMskDisp1BotVSync:1;
			unsigned int iCopIntMskDisp2TopVSync:1;
			unsigned int iCopIntMskDisp2BotVSync:1;
			unsigned int iCopIntMskHd2sdFinish:1;    
			unsigned int iCopIntMskF2fFinish:1;
			unsigned int reserved:23;
		}bits;

		unsigned int val;
	}df_cop_int_mask_reg;

	union{
		struct{
			unsigned int oHostIntStaDisp1Err:1;  
			unsigned int oHostIntStaDisp2Err:1;    
			unsigned int oHostIntStaHd2sdErr:1;    
			unsigned int oHostIntStaDisp1TopVSync:1;
			unsigned int oHostIntStaDisp1BotVSync:1;
			unsigned int oHostIntStaDisp2TopVSync:1;
			unsigned int oHostIntStaDisp2BotVSync:1;
			unsigned int oHostIntStaHd2sdFinish:1;    
			unsigned int oHostIntStaF2fFinish:1;
			unsigned int reserved:23;
		}bits;

		unsigned int val;
	}df_host_int_status_reg;

	union{
		struct{
			unsigned int iHostIntMskDisp1Err:1;    
			unsigned int iHostIntMskDisp2Err:1;    
			unsigned int iHostIntMskHd2sdErr:1;    
			unsigned int iHostIntMskDisp1TopVSync:1;
			unsigned int iHostIntMskDisp1BotVSync:1;
			unsigned int iHostIntMskDisp2TopVSync:1;
			unsigned int iHostIntMskDisp2BotVSync:1;
			unsigned int iHostIntMskHd2sdFinish:1;    
			unsigned int iHostIntMskF2fFinish:1;
			unsigned int reserved:23;
		}bits;

		unsigned int val;
	}df_host_int_mask_reg;

	union{
		struct{
			unsigned int iArbiterHd2sdLimit:16;
            unsigned int iArbiterVideoLimit:8;
            unsigned int iArbiterVideoBurstLimit:5;
			unsigned int reserved:3;
		}bits;
		unsigned int val;
	}df_arbiter_quantum_reg;

}df_ctrl_reg;

typedef struct _DF_GFX_REG_PARA_
{
	union{
		struct{
			unsigned int iGfxEnable:2;
			unsigned int reserved_1:1;
			unsigned int iColorKeyEnable:1;
			unsigned int iFetchType:2;
			unsigned int iRGB2YUVConvertEna:1;
			unsigned int reserved_2:25;
		}bits;

		unsigned int val;
	}df_gfx_control_reg;

	union {
		struct{
			unsigned int iColorFormat:3;
            unsigned int reserved_1:1;
			unsigned int iAlphaIsValid:1;
			unsigned int reserved_2:27;
		}bits;
		
		unsigned int val;
	}df_gfx_format_reg;

	union {
		struct{
			unsigned int iDefaultAlpha:10;
			unsigned int iArgb1555Alpha0:10;
			unsigned int iArgb1555Alpha1:10;
			unsigned int reserved:2;
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

	unsigned int iStartAddr;
	unsigned int iLinePitch;

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
}df_gfx_reg;

typedef struct _DF_VIDEO1_REG_PARA_
{
	union {
		struct{
			unsigned int iVideoEnable:1;
			unsigned int iF2FMode:1;
			unsigned int iYVScalerIs2Taps:1;
			unsigned int iSrcYUV422:1;
			unsigned int iStoreFrame:1;
			unsigned int iStoreTopField:1;	
			unsigned int iEnableLumaDi:1;
            		unsigned int iEnableChromaDi:1;
			unsigned int iLumaDeintMode:2;
			unsigned int iChromaDeintMode:2;
			unsigned int iStoreLinePitch:4;
			unsigned int iFetchLinePitch:8;
			unsigned int iMaxDiff3D:8;
		}bits;

		unsigned int val;
	}df_video_control_reg;

	union {
		struct{
			unsigned int iDefaultAlpha:10;
			unsigned int reserved_1:6;
            unsigned int iF2FMode:1;
            unsigned int reserved_2:15;
		}bits;

		unsigned int val;
	}df_video_alpha_control_reg;
	
	union {
		struct{
			unsigned int iDispXStart:11;
			unsigned int reserved_1:5;
			unsigned int iDispXEnd:11;
			unsigned int reserved_2:5;
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
			unsigned int iSrcXStart:11;
			unsigned int reserved_1:5;
			unsigned int iSrcXEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_x_crop_reg;

	union {
		struct{
			unsigned int iSrcYStart:11;
			unsigned int reserved_1:5;
			unsigned int iSrcYEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_y_crop_reg;

	union{
		struct{
			unsigned int iScalerMin0:5;
			unsigned int reserved_1:3;
			unsigned int iScalerMin1:5;
			unsigned int reserved_2:3;
			unsigned int iScalerK0:4;
			unsigned int reserved_3:4;
			unsigned int iScalerK1:4;
			unsigned int iScalerDirMax:4;
		}bits;
		unsigned int val;
	}df_video_lai_reg;

	union{
		struct{
			unsigned int iScalerYVRatio:17;
			unsigned int reserved_1:7;
			unsigned int iYCenterLineTop:4;
			unsigned int iYCenterLineBot:4;
		}bits;
		unsigned int val;
	}df_video_luma_vratio_reg;

	union{
		struct{
			unsigned int iScalerYHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_video_luma_hratio_reg;

	union{
		struct{
			unsigned int iScalerCVRatio:17;
			unsigned int reserved_1:7;
			unsigned int iCCenterLineTop:4;
			unsigned int iCCenterLineBot:4;
		}bits;
		unsigned int val;
	}df_video_chroma_vratio_reg;

	union{
		struct{
			unsigned int iScalerCHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_video_chroma_hratio_reg;

	union{
		struct{
			unsigned int iScalerYD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_video_luma_noline_d_reg;

	union{
		struct{
			unsigned int iScalerCD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_video_chroma_noline_d_reg;
	

	union{
	 	struct{
			unsigned int iScalerYHKAbs:13;
			unsigned int reserved_1	:2;
			unsigned int iScalerYHKSign:1;
			unsigned int iScalerYHKRadix:5;
			unsigned int iScalerYHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_video_luma_noline_k_reg;

	union{
	 	struct{
			unsigned int iScalerCHKAbs:13;
			unsigned int reserved_1	:2;
			unsigned int iScalerCHKSign:1;
			unsigned int iScalerCHKRadix:5;
			unsigned int iScalerCHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_video_chroma_noline_k_reg;

	union{
		struct{
			unsigned int iYHPhase:6;
			unsigned int reserved_1:10;
			unsigned int iCHPhase:6;
			unsigned int reserved_2:10;
		}bits;
		unsigned int val;
	}df_video_hor_phase_reg;

	union{
		struct{
			unsigned int iYTVPhase:6;
			unsigned int reserved_1:2;
			unsigned int iYBVPhase:6;
			unsigned int reserved_2:2;
			unsigned int iCTVPhase:6;
			unsigned int reserved_3:2;
			unsigned int iCBVPhase:6;
			unsigned int reserved_4:2;
		}bits;
		unsigned int val;
	}df_video_ver_phase_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCurY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_cur_yt_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCurC128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_cur_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCurY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_cur_yb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCurC128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_cur_cb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPreY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_pre_yt_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPreC128AddrTop:25;
        }bits;
        unsigned int val;
    }df_video_pre_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPreY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_pre_yb_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPreC128AddrBot:25;
        }bits;
        unsigned int val;
    }df_video_pre_cb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPosY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_pos_yt_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPosC128AddrTop:25;
        }bits;
        unsigned int val;
    }df_video_pos_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPosY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_pos_yb_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPosC128AddrBot:25;
        }bits;
        unsigned int val;
    }df_video_pos_cb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPprY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_ppr_yt_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPprC128AddrTop:25;
        }bits;
        unsigned int val;
    }df_video_ppr_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iPprY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_ppr_yb_addr_reg;

    union{
        struct{
            unsigned int reserved:7;
            unsigned int iPprC128AddrBot:25;
        }bits;
        unsigned int val;
    }df_video_ppr_cb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iStoreY128Addr:25;
		}bits;
		unsigned int val;
	}df_video_store_y_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iStoreC128Addr:25;
		}bits;
		unsigned int val;
	}df_video_store_c_addr_reg;

	union{
		struct{
			unsigned int iSrcIsFrame:1;
			unsigned int reserved_1:7;
			unsigned int iSrcIsTop:1;
			unsigned int reserved_2:23;
		}bits;
		unsigned int val;
	}df_video_src_ctrl_reg;

	union{
		struct{
			unsigned int iCoeffUpdate:1;
			unsigned int reserved:31;
		}bits;
		unsigned int val;
	}df_video_scaler_coeff_up_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCoeff128Addr:25;
		}bits;
		unsigned int val;
	}df_video_scaler_coeff_addr_reg;

	union{
		struct{
			unsigned int iF2FStart:1;
			unsigned int reserved:31;
		}bits;
		unsigned int val;
	}df_video_f2f_start_reg;

	union{
		struct{
			unsigned int oStoreFinish:1;
			unsigned int reserved:31;
		}bits;
		unsigned int val;
	}df_video_f2f_status_reg;

	unsigned int oDispNum;
}df_video1_reg;

typedef struct _DF_VIDEO2_REG_PARA_
{
	union{
		struct{
			unsigned int iVideoEnable:1;
			unsigned int reserved_1:15;
			unsigned int iFetchLinePitch:4;
			unsigned int reserved_2:12;
		}bits;
		unsigned int val;
	}df_video_control_reg;

    union {
        struct{
            unsigned int iDispXStart:11;
            unsigned int reserved_1:5;
            unsigned int iDispXEnd:11;
            unsigned int reserved_2:5;
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

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_yt_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_yb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_cb_addr_reg;

	union{
		struct{
			unsigned int iSrcIsFrame:1;
			unsigned int reserved_1:7;
			unsigned int iSrcIsTop:1;
			unsigned int reserved_2:23;
		}bits;
		unsigned int val;
	}df_video_src_ctrl_reg;
	
	unsigned int oDispNum;
}df_video2_reg;

typedef struct _DF_VIDEO3_REG_PARA_
{
	union {
		struct{
			unsigned int iVideoEnable:1;
			unsigned int reserved_1:15;
			unsigned int iFetchLinePitch:4;
			unsigned int reserved_2:12;
		}bits;

		unsigned int val;
	}df_video_control_reg;
	
	union {
		struct{
			unsigned int iDispXStart:11;
			unsigned int reserved_1:5;
			unsigned int iDispXEnd:11;
			unsigned int reserved_2:5;
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
			unsigned int iSrcXStart:11;
			unsigned int reserved_1:5;
			unsigned int iSrcXEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_x_crop_reg;

	union {
		struct{
			unsigned int iSrcYStart:11;
			unsigned int reserved_1:5;
			unsigned int iSrcYEnd:11;
			unsigned int reserved_2:5;
		}bits;

		unsigned int val;
	}df_video_src_y_crop_reg;

    union{
        struct{
            unsigned int iScalerYVRatio:17;
            unsigned int reserved_1:7;
            unsigned int iYCenterLineTop:4;
            unsigned int iYCenterLineBot:4;
        }bits;
        unsigned int val;
    }df_video_luma_vratio_reg;


	union{
		struct{
			unsigned int iScalerYHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_video_luma_hratio_reg;

    union{
        struct{
            unsigned int iScalerCVRatio:17;
            unsigned int reserved_1:7;
            unsigned int iCCenterLineTop:4;
            unsigned int iCCenterLineBot:4;
        }bits;
        unsigned int val;
    }df_video_chroma_vratio_reg;

	union{
		struct{
			unsigned int iScalerCHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_video_chroma_hratio_reg;

	union{
		struct{
			unsigned int iScalerYD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_video_luma_noline_d_reg;

	union{
		struct{
			unsigned int iScalerCD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_video_chroma_noline_d_reg;
	

	union{
	 	struct{
			unsigned int iScalerYHKAbs:13;
			unsigned int reserved_1	:2;
			unsigned int iScalerYHKSign:1;
			unsigned int iScalerYHKRadix:5;
			unsigned int iScalerYHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_video_luma_noline_k_reg;

	union{
	 	struct{
			unsigned int iScalerCHKAbs:13;
			unsigned int reserved_1	:2;
			unsigned int iScalerCHKSign:1;
			unsigned int iScalerCHKRadix:5;
			unsigned int iScalerCHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_video_chroma_noline_k_reg;

	union{
		struct{
			unsigned int iYHPhase:6;
			unsigned int reserved_1:10;
			unsigned int iCHPhase:6;
			unsigned int reserved_2:10;
		}bits;
		unsigned int val;
	}df_video_hor_phase_reg;

	union{
		struct{
			unsigned int iYTVPhase:6;
			unsigned int reserved_1:2;
			unsigned int iYBVPhase:6;
			unsigned int reserved_2:2;
			unsigned int iCTVPhase:6;
			unsigned int reserved_3:2;
			unsigned int iCBVPhase:6;
			unsigned int reserved_4:2;
		}bits;
		unsigned int val;
	}df_video_ver_phase_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_yt_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrTop:25;
		}bits;
		unsigned int val;
	}df_video_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_yb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrBot:25;
		}bits;
		unsigned int val;
	}df_video_cb_addr_reg;

	union{
		struct{
			unsigned int iSrcIsFrame:1;
			unsigned int reserved_1:7;
			unsigned int iSrcIsTop:1;
			unsigned int reserved_2:23;
		}bits;
		unsigned int val;
	}df_video_src_ctrl_reg;

	unsigned int oDispNum;
}df_video3_reg;

typedef struct _DF_COMPOSITOR_REG_PARA_
{
	union {
		struct{
			unsigned int iBGY:10;
			unsigned int iBGU:10;
			unsigned int iBGV:10;
			unsigned int reserved:2;
		}bits;

		unsigned int val;
	}df_comp_back_ground_reg;

	union{
		struct{
			unsigned int iVideo1ZOrder:2;
			unsigned int reserved_1:6;
			unsigned int iGfx1ZOrder:2;
			unsigned int reserved_2:2;
			unsigned int iGfx2ZOrder:2;
			unsigned int reserved_3:18;
		}bits;

		unsigned int val;
	}df_comp_z_order_reg;

	union{
		struct{
			unsigned int iBright:8;
			unsigned int iContrast:6;

			unsigned int reserved_1:2;
			unsigned int iSaturation:8;
			unsigned int reserved_2:8;
		}bits;
		unsigned int val;
	}df_comp_color_adjust_reg;

	union{
		struct{
			unsigned int iYSharpEn:1;
			unsigned int iYMedianEn:1;
			unsigned int iYCoringSoft:1;
			unsigned int reserved_1:1;
			unsigned int iYPeakingShrink:3;
			unsigned int reserved_2:1;
			unsigned int iYCoring:6;
			unsigned int reserved_3:2;
			unsigned int iYbpw:8;
			unsigned int iYhpw:8;
		}bits;
		unsigned int val;
	}df_comp_luma_sharp_reg;

	union{
		struct{
			unsigned int iCSharpEn:1;
			unsigned int iCMedianEn:1;
			unsigned int iCCoringSoft:1;
			unsigned int reserved_1:1;
			unsigned int iCPeakingShrink:3;
			unsigned int reserved_2:1;
			unsigned int iCCoring:6;
			unsigned int reserved_3:2;
			unsigned int iCbpw:8;
			unsigned int iChpw:8;
		}bits;
		unsigned int val;
	}df_comp_chroma_sharp_reg;
}df_compositor_reg;

typedef struct _DF_HD2SD_REG_PARA_
{
	union{
		struct{
			unsigned int iHD2SDEna:1;
			unsigned int reserved_1:3;
			unsigned int iStoreFrame:1;
			unsigned int iStoreTopField:1;
			unsigned int reserved_2:10;
			unsigned int iStoreLinePitch:4;
			unsigned int reserved_3:12;
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

	union{
		struct{
			unsigned int iScalerYVRatio:17;
			unsigned int reserved_1:7;
			unsigned int iYCenterLineTop:4;
			unsigned int iYCenterLineBot:4;
		}bits;
		unsigned int val;
	}df_hd2sd_luma_vratio_reg;

	union{
		struct{
			unsigned int iScalerYHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_hd2sd_luma_hratio_reg;

	union{
		struct{
			unsigned int iScalerCVRatio:17;
			unsigned int reserved_1:7;
			unsigned int iCCenterLineTop:4;
			unsigned int iCCenterLineBot:4;
		}bits;
		unsigned int val;
	}df_hd2sd_chroma_vratio_reg;

	union{
		struct{
			unsigned int iScalerCHRatio:17;
			unsigned int reserved:15;
		}bits;
		unsigned int val;
	}df_hd2sd_chroma_hratio_reg;

	union{
		struct{
			unsigned int iScalerYD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_hd2sd_luma_noline_d_reg;

	union{
		struct{
			unsigned int iScalerCD0Abs:24;
			unsigned int reserved:8;
		}bits;
		unsigned int val;
	}df_hd2sd_chroma_noline_d_reg;
	

	union{
		struct{
			unsigned int iScalerYHKAbs:13;
			unsigned int reserved_1 :2;
			unsigned int iScalerYHKSign:1;
			unsigned int iScalerYHKRadix:5;
			unsigned int iScalerYHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_hd2sd_luma_noline_k_reg;

	union{
		struct{
			unsigned int iScalerCHKAbs:13;
			unsigned int reserved_1 :2;
			unsigned int iScalerCHKSign:1;
			unsigned int iScalerCHKRadix:5;
			unsigned int iScalerCHNoneLinearStart:11;
		}bits;
		unsigned int val;
	}df_hd2sd_chroma_noline_k_reg;

	union{
		struct{
			unsigned int iYHPhase:6;
			unsigned int reserved_1:10;
			unsigned int iCHPhase:6;
			unsigned int reserved_2:10;
		}bits;
		unsigned int val;
	}df_hd2sd_hor_phase_reg;

	union{
		struct{
			unsigned int iYTVPhase:6;
			unsigned int reserved_1:2;
			unsigned int iYBVPhase:6;
			unsigned int reserved_2:2;
			unsigned int iCTVPhase:6;
			unsigned int reserved_3:2;
			unsigned int iCBVPhase:6;
			unsigned int reserved_4:2;
		}bits;
		unsigned int val;
	}df_hd2sd_ver_phase_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrTop:25;
		}bits;
		unsigned int val;
	}df_hd2sd_yt_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrTop:25;
		}bits;
		unsigned int val;
	}df_hd2sd_ct_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iY128AddrBot:25;
		}bits;
		unsigned int val;
	}df_hd2sd_yb_addr_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iC128AddrBot:25;
		}bits;
		unsigned int val;
	}df_hd2sd_cb_addr_reg;

	union{
		struct{
			unsigned int iCoeffUpdate:1;
			unsigned int reserved:31;
		}bits;
		unsigned int val;
	}df_hd2sd_scaler_coeff_up_reg;

	union{
		struct{
			unsigned int reserved:7;
			unsigned int iCoeff128Addr:25;
		}bits;
		unsigned int val;
	}df_hd2sd_scaler_coeff_addr_reg;

	unsigned int oStoreNum;
}df_hd2sd_reg;

typedef struct _DF_OUTIF_REG_PARA
{
	union {
		struct{
			unsigned int iDispEn:1;
			unsigned int reserved_1:1;
			unsigned int iIsHD:1;
			unsigned int reserved_2:2;
			unsigned int iIsInterlaced:1;
			unsigned int reserved_3:1;
			unsigned int iPALFmt:1;
			unsigned int iIntEnaTop:1;
			unsigned int iIntEnaBot:1;
			unsigned int iDownMode:1;
			unsigned int iDitherMode:1;
			unsigned int iHSyncPol:1;
			unsigned int iVSyncPol:1;
			unsigned int iDitherEna:1;
			unsigned int reserved:17;
		}bits;

		unsigned int val;
	}df_outif_control_reg;

	union {
		struct{
			unsigned int iHTotal:12;
			unsigned int reserved_1:4;
			unsigned int iActPixNum:12;
			unsigned int reserved_2:4;
		}bits;

		unsigned int val;
	}df_outif_x_size_reg;

	union {
		struct{
			unsigned int iVTotal:12;
			unsigned int reserved:4;
			unsigned int iActLineNum:12;
			unsigned int reserved_2:4;
		}bits;

		unsigned int val;
	}df_outif_y_size_reg;

	union {
		struct{
			unsigned int iHSyncFP:10;
			unsigned int iHSyncWidth:10;
			unsigned int iHSyncBP:10;
			unsigned int reserved_2:2;
		}bits;

		unsigned int val;
	}df_outif_hsync_reg;

	union {
		struct{
			unsigned int iVSyncFP:8;
			unsigned int iVSyncWidth:8;
			unsigned int iVSyncBP:8;
			unsigned int reserved_2:8;
		}bits;

		unsigned int val;
	}df_outif_vsync_reg;

	union {
		struct{
			unsigned int iBlackLevel:10;
			unsigned int reserved_1:6;
			unsigned int iWhiteLevel:10;
			unsigned int reserved_2:6;
		}bits;
		unsigned int val;
	}df_outif_vga_level_reg;

	union {
		struct{
			unsigned int oDispWidth:12;
			unsigned int reserved_1:4;
			unsigned int oDispHeight:12;
			unsigned int reserved_2:4;
		}bits;
		unsigned int val;
	}df_outif_size_status_reg;
	
	union {
		struct{
			unsigned int oLineCnt:12;
			unsigned int reserved_1:4;
			unsigned int oActiveLine:1;
			unsigned int reserved_2:3;
			unsigned int oBottomFlag:1;
			unsigned int reserved_3:3;
			unsigned int oVSync:1;
			unsigned int reserved_4:7;
		}bits;

		unsigned int val;
	}df_outif_status_line_reg;
}df_outif_reg;

typedef struct _DF_REG_PARA_
{
	df_ctrl_reg		Ctrl;
	df_gfx_reg		Gfx[2];
	df_video1_reg		Video1;
	df_video2_reg		Video2;
	df_video3_reg		Video3;
	df_compositor_reg	Comp;
	df_hd2sd_reg		HD2SD;
	df_outif_reg		OutIF[2];
}df_reg_para;

#endif
