#ifndef _CSM18XX_SOCKET_H
#define _CSM18XX_SOCKET_H

/*
 * This __REG() version gives the same results as the one above,  except
 * that we are fooling gcc somehow so it generates far better and smaller
 * assembly code for access to contigous registers.  It's a shame that gcc
 * doesn't guess this by itself.
 */
#include <asm/types.h>
#include <asm/io.h>

/* CSM PCMCIA Register Map */
#define TIMECFG     (VA_PCMCIA_BASE + 0x100)
#define OPMODE      (VA_PCMCIA_BASE + 0x104)
#define RAWSTAT     (VA_PCMCIA_BASE + 0x108)
#define INTSTAT     (VA_PCMCIA_BASE + 0x10C)
#define INTENA      (VA_PCMCIA_BASE + 0x110)
#define PINCTL      (VA_PCMCIA_BASE + 0x114)
#define ATTRBASE    (VA_PCMCIA_BASE + 0x118)
#define COMMBASE    (VA_PCMCIA_BASE + 0x11C)
#define IOBASE      (VA_PCMCIA_BASE + 0x120)
#define WAITTMR     (VA_PCMCIA_BASE + 0x124)

/* CSM PCMCIA Interrupt Status */
#define INT_IREQ    (1 << 0)
#define INT_CDCHG   (1 << 2)
#define INT_WER     (1 << 3)

/* CSM PCMCIA Pin Status */
#define PIN_RST     (1 << 0)
#define PIN_CD1     (1 << 1)
#define PIN_CD2     (1 << 2)
#define PIN_RDYIREQ (1 << 4)
#define PIN_WAIT    (1 << 5)

#define PCMCIAPrtSp     (0x8000)     /* PCMCIA Partition Space [byte] */
#define PCMCIASp        (0x800000)   /* PCMCIA Space [byte]           */
#define PCMCIAIOSp      PCMCIAPrtSp  /* PCMCIA I/O Space [byte]       */
#define PCMCIAAttrSp    PCMCIAPrtSp  /* PCMCIA Attribute Space [byte] */
#define PCMCIAMemSp     PCMCIAPrtSp  /* PCMCIA Memory Space [byte]    */

#define _PCMCIA(Nb)     (0x35700000) /* not really                    */
#define _PCMCIAIO(Nb)   _PCMCIA(Nb)
#define _PCMCIAAttr(Nb) (0x35800000)
#define _PCMCIAMem(Nb)  (0x35A00000)


//#define RESET_BY_GPIO12	// actually reset by socket bus chip, pmux is gpio12
#define RESET_BY_GPIO15		// really reset by gpio15

#endif
