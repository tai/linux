#ifndef __ARCH_MEM_DEF_H
#define __ARCH_MEM_DEF_H

#if (CONFIG_CELESTIAL_MEM_SIZE == 256 || CONFIG_CELESTIAL_MEM_SIZE == 512 || CONFIG_CELESTIAL_MEM_SIZE == 1024)

#define        BASE_DDR_ADDR          0x10000000

#ifdef CONFIG_FB0_SIZE
#define        FB0_SIZE       		  CONFIG_FB0_SIZE
#else
#define        FB0_SIZE       		  0x1000000
#endif
#define        FB0_REGION             (BASE_DDR_ADDR - FB0_SIZE)
#ifdef CONFIG_FB1_SIZE
#define        FB1_SIZE               CONFIG_FB1_SIZE
#else
#define        FB1_SIZE               0x0
#endif
#define        FB1_REGION             (FB0_REGION - FB1_SIZE)
#ifdef CONFIG_FB2_SIZE
#define        FB2_SIZE               CONFIG_FB2_SIZE
#else
#define        FB2_SIZE               0x800000
#endif
#define        FB2_REGION             (FB0_REGION + FB0_SIZE - FB2_SIZE)
#ifdef CONFIG_FB3_SIZE
#define        FB3_SIZE               CONFIG_FB3_SIZE
#else
#define        FB3_SIZE               0x0
#endif
#define        FB3_REGION             (FB1_REGION + FB1_SIZE - FB3_SIZE)

#define        BLOB_SIZE              0x1000
#define        BLOB_REGION            (FB1_REGION - BLOB_SIZE)

#define        ETHERNET_SIZE          0xFF000
#define        ETHERNET_REGION        (BLOB_REGION - ETHERNET_SIZE)

#define        CRYPTO_MEM_SIZE        0x300000
#define        CRYPTO_MEM_REGION      (ETHERNET_REGION - CRYPTO_MEM_SIZE)

#ifdef CONFIG_HD2SD_ENABLE
#define        HD2SD_DATA_SIZE        (0x5a0000)
#else
#define        HD2SD_DATA_SIZE        (0)
#endif  // CONFIG_HD2SD_ENABLE
#define        HD2SD_DATA_REGION      (CRYPTO_MEM_REGION - HD2SD_DATA_SIZE)

#define        VIDEO_STUFF_SIZE       (0x300000)
#define        VIDEO_STUFF_REGION     (HD2SD_DATA_REGION - VIDEO_STUFF_SIZE)

#ifdef CONFIG_VIDEO_NUM
#define        VIDEO_NUM              CONFIG_VIDEO_NUM
#else
#define        VIDEO_NUM              1
#endif
#define        VIDEO_DPB_SIZE         (0x4200000) //add HLS feature. before is 0x3700000
#define        VIDEO_DPB_REGION       (VIDEO_STUFF_REGION - VIDEO_DPB_SIZE)
#ifdef CONFIG_VIDEO_CPB_SIZE
#define        VIDEO_CPB_SIZE         (CONFIG_VIDEO_CPB_SIZE * VIDEO_NUM)
#else
#define        VIDEO_CPB_SIZE         (0x400000 * VIDEO_NUM)
#endif
#define        VIDEO_CPB_REGION       (VIDEO_DPB_REGION - VIDEO_CPB_SIZE)
#define        VIDEO_CPB_DIR_SIZE     (CONFIG_VIDEO_CPB_SIZE/0x100 * VIDEO_NUM)
#define        VIDEO_CPB_DIR_REGION   (VIDEO_CPB_REGION - VIDEO_CPB_DIR_SIZE)
#ifdef CONFIG_CELESTIAL_TIGA_MINI
#define        VIDEO_USER_DATA_SIZE   (0x0)
#else
#define        VIDEO_USER_DATA_SIZE   (0x80000 * VIDEO_NUM)
#endif  // CONFIG_CELESTIAL_TIGA_MINI
#define        VIDEO_USER_DATA_REGION (VIDEO_CPB_DIR_REGION - VIDEO_USER_DATA_SIZE)


#ifdef CONFIG_AUDIO_NUM
#define        AUDIO_NUM		CONFIG_AUDIO_NUM
#else
#define        AUDIO_NUM		1
#endif
#ifdef CONFIG_AUDIO_CAB_SIZE
#define        AUDIO_CAB_SIZE	(CONFIG_AUDIO_CAB_SIZE * AUDIO_NUM)
#else
#define        AUDIO_CAB_SIZE	(0x18000 * AUDIO_NUM)
#endif
#define        AUDIO_CAB_REGION	(VIDEO_USER_DATA_REGION - AUDIO_CAB_SIZE)
#define        AUDIO_PTS_SIZE	(0x3000* AUDIO_NUM)
#define        AUDIO_PTS_REGION	(AUDIO_CAB_REGION - AUDIO_PTS_SIZE)
#ifdef CONFIG_CELESTIAL_TIGA_MINI
#define        AUDIO_MIX_SIZE	(0x0)
#else
#define        AUDIO_MIX_SIZE	(0x40000)
#endif  // CONFIG_CELESTIAL_TIGA_MINI
#define        AUDIO_MIX_REGION	(AUDIO_PTS_REGION - AUDIO_MIX_SIZE)

#define        AUDIO_STUFF_SIZE       (0x280000)
#define        AUDIO_STUFF_REGION     (AUDIO_MIX_REGION - AUDIO_STUFF_SIZE)
#ifdef CONFIG_CELESTIAL_XPORT
#define        XPORT_SIZE             (0xd00000)
#else
#define        XPORT_SIZE             (0)
#endif // CONFIG_CELESTIAL_XPORT

#define        XPORT_REGION           (AUDIO_STUFF_REGION - XPORT_SIZE)

#elif (CONFIG_CELESTIAL_MEM_SIZE == 128) // setting to 128MB of Memory size

#define        BASE_DDR_ADDR          0x8000000

#define        FB0_SIZE               0x1000000 // FB0_SIZE
#define        FB0_REGION             (BASE_DDR_ADDR - FB0_SIZE) //

#define        FB1_SIZE               0x00000
#define        FB1_REGION             (FB0_REGION - FB1_SIZE)
#define        FB2_SIZE               0x800000
#define        FB2_REGION             (FB0_REGION + FB0_SIZE - FB2_SIZE)
#define        FB3_SIZE               0x000000
#define        FB3_REGION             (FB1_REGION + FB1_SIZE - FB3_SIZE)

#define        BLOB_SIZE              0x1000
#define        BLOB_REGION            (FB1_REGION - BLOB_SIZE)

#define        ETHERNET_SIZE          0xFE000
#define        ETHERNET_REGION        (BLOB_REGION - ETHERNET_SIZE)

#define        CRYPTO_MEM_SIZE        0x0
#define        CRYPTO_MEM_REGION      (ETHERNET_REGION - CRYPTO_MEM_SIZE)

#define        HD2SD_DATA_SIZE        (0x5A0000) //(0x420000)
#define        HD2SD_DATA_REGION      (CRYPTO_MEM_REGION - HD2SD_DATA_SIZE)

#define        VIDEO_STUFF_SIZE       (0x300000) //0x100000
#define        VIDEO_STUFF_REGION     (HD2SD_DATA_REGION - VIDEO_STUFF_SIZE)

#define        VIDEO_NUM               1

#define        VIDEO_DPB_SIZE         (0x2c10000) //0x3000000
#define        VIDEO_DPB_REGION       (VIDEO_STUFF_REGION - VIDEO_DPB_SIZE)
#define        VIDEO_CPB_SIZE         (0x400000 * VIDEO_NUM)
#define        VIDEO_CPB_REGION       (VIDEO_DPB_REGION - VIDEO_CPB_SIZE)
#define        VIDEO_CPB_DIR_SIZE     (0x4000 * VIDEO_NUM)
#define        VIDEO_CPB_DIR_REGION   (VIDEO_CPB_REGION - VIDEO_CPB_DIR_SIZE)
#define        VIDEO_USER_DATA_SIZE   (0x80000 * VIDEO_NUM)
#define        VIDEO_USER_DATA_REGION (VIDEO_CPB_DIR_REGION - VIDEO_USER_DATA_SIZE)

#define        AUDIO_NUM		1
#define        AUDIO_CAB_SIZE	(0x18000* AUDIO_NUM)
#define        AUDIO_CAB_REGION	(VIDEO_USER_DATA_REGION - AUDIO_CAB_SIZE)
#define        AUDIO_PTS_SIZE	(0x3000* AUDIO_NUM)
#define        AUDIO_PTS_REGION	(AUDIO_CAB_REGION - AUDIO_PTS_SIZE)
#define        AUDIO_MIX_SIZE	(0x40000)
#define        AUDIO_MIX_REGION	(AUDIO_PTS_REGION - AUDIO_MIX_SIZE)

#define        AUDIO_STUFF_SIZE       (0x190000)
#define        AUDIO_STUFF_REGION     (AUDIO_MIX_REGION - AUDIO_STUFF_SIZE)

#ifdef (CONFIG_CELESTIAL_XPORT)
#define        XPORT_SIZE             (0x9c0000)
#else
#define        XPORT_SIZE             (0)
#endif // CONFIG_CNC1800L_NO_DEMUX

#define        XPORT_REGION           (AUDIO_STUFF_REGION-XPORT_SIZE)

#endif

#endif

