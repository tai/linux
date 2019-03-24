/*******************************************************************************

File name   : orion_gfxobj.h

Description : 

COPYRIGHT (C) CelestialSemi 2007.

Date               Modification                               Name
----               ------------                               ----
2007-03-30         Created                                    xm.chen
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __ORION_GFXOBJ_H____
#define __ORION_GFXOBJ_H____

/* Includes ----------------------------------------------------------------- */
#include <linux/types.h>

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
typedef u8 boolean;
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

typedef enum CSBLIT_ColorType_e
{
  CSBLIT_COLOR_TYPE_A0 = 2,       /*one single color, can't be set to output colortype*/
  CSBLIT_COLOR_TYPE_RGB565 = 0,
  CSBLIT_COLOR_TYPE_ARGB1555 = 3,
  CSBLIT_COLOR_TYPE_ARGB4444 = 1,
  CSBLIT_COLOR_TYPE_ARGB8888 = 6,
  CSBLIT_COLOR_TYPE_CLUT4 = 4,
  CSBLIT_COLOR_TYPE_CLUT8 = 5,
} CSBLIT_ColorType_t;

typedef struct CSBLIT_Bitmap_s
{
  CSBLIT_ColorType_t			ColorType;
  unsigned int                                   Width;
  unsigned int                                   Height;
  unsigned int                                   Pitch;
  unsigned int                                   Offset;
  void*                                 Data_p;
  unsigned int                                   Size;
} CSBLIT_Bitmap_t;

typedef struct CSBLIT_ColorARGB_s
{
  unsigned char Alpha;
  unsigned char R;
  unsigned char G;
  unsigned char B;
} CSBLIT_ColorARGB_t;

typedef struct CSBLIT_ColorRGB_s
{
  unsigned char R;
  unsigned char G;
  unsigned char B;
} CSBLIT_ColorRGB_t;

typedef struct CSBLIT_ColorKeyRGB_s
{
  unsigned char      RMin;
  unsigned char      RMax;
  unsigned char    ROut;
  
  unsigned char      GMin;
  unsigned char      GMax;
  unsigned char    GOut;

  unsigned char      BMin;
  unsigned char      BMax;
  unsigned char    BOut;

  unsigned char    RGBEnable;
} CSBLIT_ColorKeyRGB_t;

typedef union CSBLIT_ColorValue_u
{
  CSBLIT_ColorARGB_t           ARGB8888;
  CSBLIT_ColorRGB_t            RGB565;
} CSBLIT_ColorValue_t;

typedef struct CSBLIT_Color_s
{
  CSBLIT_ColorType_t            Type;
  CSBLIT_ColorValue_t           Value;
} CSBLIT_Color_t;

typedef struct CSBLIT_Rectangle_s
{
  int PositionX;
  int PositionY;
  unsigned int Width;
  unsigned int Height;
} CSBLIT_Rectangle_t;
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __ORION_GFXOBJ_H */

/* End of orion_gxobj.h */

