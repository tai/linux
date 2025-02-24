/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  PARISC TLB and cache flushing support
 *  Copyright (C) 2000-2001 Hewlett-Packard (John Marvin)
 *  Copyright (C) 2001 Matthew Wilcox (willy at parisc-linux.org)
 *  Copyright (C) 2002 Richard Hirst (rhirst with parisc-linux.org)
 */

/*
 * NOTE: fdc,fic, and pdc instructions that use base register modification
 *       should only use index and base registers that are not shadowed,
 *       so that the fast path emulation in the non access miss handler
 *       can be used.
 */

#ifdef CONFIG_64BIT
	.level	2.0w
#else
	.level	2.0
#endif

#include <asm/psw.h>
#include <asm/assembly.h>
#include <asm/cache.h>
#include <asm/ldcw.h>
#include <asm/alternative.h>
#include <linux/linkage.h>
#include <linux/init.h>
#include <linux/pgtable.h>

	.section .text.hot
	.align	16

#ifdef CONFIG_PA20
#define ALT_COND_PACACHE	ALT_COND_ALWAYS
#else
#define ALT_COND_PACACHE	ALT_COND_NO_SMP
#endif

ENTRY_CFI(flush_tlb_all_local)
	/*
	 * The pitlbe and pdtlbe instructions should only be used to
	 * flush the entire tlb. Also, there needs to be no intervening
	 * tlb operations, e.g. tlb misses, so the operation needs
	 * to happen in real mode with all interruptions disabled.
	 */

	/* pcxt_ssm_bug	- relied upon translation! PA 2.0 Arch. F-4 and F-5 */
	rsm		PSW_SM_I, %r19		/* save I-bit state */
	load32		PA(1f), %r1
	nop
	nop
	nop
	nop
	nop

	rsm		PSW_SM_Q, %r0		/* prep to load iia queue */
	mtctl		%r0, %cr17		/* Clear IIASQ tail */
	mtctl		%r0, %cr17		/* Clear IIASQ head */
	mtctl		%r1, %cr18		/* IIAOQ head */
	ldo		4(%r1), %r1
	mtctl		%r1, %cr18		/* IIAOQ tail */
	load32		REAL_MODE_PSW, %r1
	mtctl           %r1, %ipsw
	rfi
	nop

1:      load32		PA(cache_info), %r1

	/* Flush Instruction Tlb */

88:	LDREG		ITLB_SID_BASE(%r1), %r20
	LDREG		ITLB_SID_STRIDE(%r1), %r21
	LDREG		ITLB_SID_COUNT(%r1), %r22
	LDREG		ITLB_OFF_BASE(%r1), %arg0
	LDREG		ITLB_OFF_STRIDE(%r1), %arg1
	LDREG		ITLB_OFF_COUNT(%r1), %arg2
	LDREG		ITLB_LOOP(%r1), %arg3

	addib,COND(=)		-1, %arg3, fitoneloop	/* Preadjust and test */
	movb,<,n	%arg3, %r31, fitdone	/* If loop < 0, skip */
	copy		%arg0, %r28		/* Init base addr */

fitmanyloop:					/* Loop if LOOP >= 2 */
	mtsp		%r20, %sr1
	add		%r21, %r20, %r20	/* increment space */
	copy		%arg2, %r29		/* Init middle loop count */

fitmanymiddle:					/* Loop if LOOP >= 2 */
	addib,COND(>)		-1, %r31, fitmanymiddle	/* Adjusted inner loop decr */
	pitlbe		%r0(%sr1, %r28)
	pitlbe,m	%arg1(%sr1, %r28)	/* Last pitlbe and addr adjust */
	addib,COND(>)		-1, %r29, fitmanymiddle	/* Middle loop decr */
	copy		%arg3, %r31		/* Re-init inner loop count */

	movb,tr		%arg0, %r28, fitmanyloop /* Re-init base addr */
	addib,COND(<=),n	-1, %r22, fitdone	/* Outer loop count decr */

fitoneloop:					/* Loop if LOOP = 1 */
	mtsp		%r20, %sr1
	copy		%arg0, %r28		/* init base addr */
	copy		%arg2, %r29		/* init middle loop count */

fitonemiddle:					/* Loop if LOOP = 1 */
	addib,COND(>)		-1, %r29, fitonemiddle	/* Middle loop count decr */
	pitlbe,m	%arg1(%sr1, %r28)	/* pitlbe for one loop */

	addib,COND(>)		-1, %r22, fitoneloop	/* Outer loop count decr */
	add		%r21, %r20, %r20		/* increment space */

fitdone:
	ALTERNATIVE(88b, fitdone, ALT_COND_NO_SPLIT_TLB, INSN_NOP)

	/* Flush Data Tlb */

	LDREG		DTLB_SID_BASE(%r1), %r20
	LDREG		DTLB_SID_STRIDE(%r1), %r21
	LDREG		DTLB_SID_COUNT(%r1), %r22
	LDREG		DTLB_OFF_BASE(%r1), %arg0
	LDREG		DTLB_OFF_STRIDE(%r1), %arg1
	LDREG		DTLB_OFF_COUNT(%r1), %arg2
	LDREG		DTLB_LOOP(%r1), %arg3

	addib,COND(=)		-1, %arg3, fdtoneloop	/* Preadjust and test */
	movb,<,n	%arg3, %r31, fdtdone	/* If loop < 0, skip */
	copy		%arg0, %r28		/* Init base addr */

fdtmanyloop:					/* Loop if LOOP >= 2 */
	mtsp		%r20, %sr1
	add		%r21, %r20, %r20	/* increment space */
	copy		%arg2, %r29		/* Init middle loop count */

fdtmanymiddle:					/* Loop if LOOP >= 2 */
	addib,COND(>)		-1, %r31, fdtmanymiddle	/* Adjusted inner loop decr */
	pdtlbe		%r0(%sr1, %r28)
	pdtlbe,m	%arg1(%sr1, %r28)	/* Last pdtlbe and addr adjust */
	addib,COND(>)		-1, %r29, fdtmanymiddle	/* Middle loop decr */
	copy		%arg3, %r31		/* Re-init inner loop count */

	movb,tr		%arg0, %r28, fdtmanyloop /* Re-init base addr */
	addib,COND(<=),n	-1, %r22,fdtdone	/* Outer loop count decr */

fdtoneloop:					/* Loop if LOOP = 1 */
	mtsp		%r20, %sr1
	copy		%arg0, %r28		/* init base addr */
	copy		%arg2, %r29		/* init middle loop count */

fdtonemiddle:					/* Loop if LOOP = 1 */
	addib,COND(>)		-1, %r29, fdtonemiddle	/* Middle loop count decr */
	pdtlbe,m	%arg1(%sr1, %r28)	/* pdtlbe for one loop */

	addib,COND(>)		-1, %r22, fdtoneloop	/* Outer loop count decr */
	add		%r21, %r20, %r20	/* increment space */


fdtdone:
	/*
	 * Switch back to virtual mode
	 */
	/* pcxt_ssm_bug */
	rsm		PSW_SM_I, %r0
	load32		2f, %r1
	nop
	nop
	nop
	nop
	nop

	rsm		PSW_SM_Q, %r0		/* prep to load iia queue */
	mtctl		%r0, %cr17		/* Clear IIASQ tail */
	mtctl		%r0, %cr17		/* Clear IIASQ head */
	mtctl		%r1, %cr18		/* IIAOQ head */
	ldo		4(%r1), %r1
	mtctl		%r1, %cr18		/* IIAOQ tail */
	load32		KERNEL_PSW, %r1
	or		%r1, %r19, %r1	/* I-bit to state on entry */
	mtctl		%r1, %ipsw	/* restore I-bit (entire PSW) */
	rfi
	nop

2:      bv		%r0(%r2)
	nop

	/*
	 * When running in qemu, drop whole flush_tlb_all_local function and
	 * replace by one pdtlbe instruction, for which QEMU will drop all
	 * local TLB entries.
	 */
3:	pdtlbe		%r0(%sr1,%r0)
	bv,n		%r0(%r2)
	ALTERNATIVE_CODE(flush_tlb_all_local, 2, ALT_COND_RUN_ON_QEMU, 3b)
ENDPROC_CFI(flush_tlb_all_local)

	.import cache_info,data

ENTRY_CFI(flush_instruction_cache_local)
88:	load32		cache_info, %r1

	/* Flush Instruction Cache */

	LDREG		ICACHE_BASE(%r1), %arg0
	LDREG		ICACHE_STRIDE(%r1), %arg1
	LDREG		ICACHE_COUNT(%r1), %arg2
	LDREG		ICACHE_LOOP(%r1), %arg3
	rsm		PSW_SM_I, %r22		/* No mmgt ops during loop*/
	mtsp		%r0, %sr1
	addib,COND(=)		-1, %arg3, fioneloop	/* Preadjust and test */
	movb,<,n	%arg3, %r31, fisync	/* If loop < 0, do sync */

fimanyloop:					/* Loop if LOOP >= 2 */
	addib,COND(>)		-1, %r31, fimanyloop	/* Adjusted inner loop decr */
	fice            %r0(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)	/* Last fice and addr adjust */
	movb,tr		%arg3, %r31, fimanyloop	/* Re-init inner loop count */
	addib,COND(<=),n	-1, %arg2, fisync	/* Outer loop decr */

fioneloop:					/* Loop if LOOP = 1 */
	/* Some implementations may flush with a single fice instruction */
	cmpib,COND(>>=),n	15, %arg2, fioneloop2

fioneloop1:
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	fice,m		%arg1(%sr1, %arg0)
	addib,COND(>)	-16, %arg2, fioneloop1
	fice,m		%arg1(%sr1, %arg0)

	/* Check if done */
	cmpb,COND(=),n	%arg2, %r0, fisync	/* Predict branch taken */

fioneloop2:
	addib,COND(>)	-1, %arg2, fioneloop2	/* Outer loop count decr */
	fice,m		%arg1(%sr1, %arg0)	/* Fice for one loop */

fisync:
	sync
	mtsm		%r22			/* restore I-bit */
89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_ICACHE, INSN_NOP)
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_instruction_cache_local)


	.import cache_info, data
ENTRY_CFI(flush_data_cache_local)
88:	load32		cache_info, %r1

	/* Flush Data Cache */

	LDREG		DCACHE_BASE(%r1), %arg0
	LDREG		DCACHE_STRIDE(%r1), %arg1
	LDREG		DCACHE_COUNT(%r1), %arg2
	LDREG		DCACHE_LOOP(%r1), %arg3
	rsm		PSW_SM_I, %r22		/* No mmgt ops during loop*/
	mtsp		%r0, %sr1
	addib,COND(=)		-1, %arg3, fdoneloop	/* Preadjust and test */
	movb,<,n	%arg3, %r31, fdsync	/* If loop < 0, do sync */

fdmanyloop:					/* Loop if LOOP >= 2 */
	addib,COND(>)		-1, %r31, fdmanyloop	/* Adjusted inner loop decr */
	fdce		%r0(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)	/* Last fdce and addr adjust */
	movb,tr		%arg3, %r31, fdmanyloop	/* Re-init inner loop count */
	addib,COND(<=),n	-1, %arg2, fdsync	/* Outer loop decr */

fdoneloop:					/* Loop if LOOP = 1 */
	/* Some implementations may flush with a single fdce instruction */
	cmpib,COND(>>=),n	15, %arg2, fdoneloop2

fdoneloop1:
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	fdce,m		%arg1(%sr1, %arg0)
	addib,COND(>)	-16, %arg2, fdoneloop1
	fdce,m		%arg1(%sr1, %arg0)

	/* Check if done */
	cmpb,COND(=),n	%arg2, %r0, fdsync	/* Predict branch taken */

fdoneloop2:
	addib,COND(>)	-1, %arg2, fdoneloop2	/* Outer loop count decr */
	fdce,m		%arg1(%sr1, %arg0)	/* Fdce for one loop */

fdsync:
	sync
	mtsm		%r22			/* restore I-bit */
89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_data_cache_local)

/* Clear page using kernel mapping.  */

ENTRY_CFI(clear_page_asm)
#ifdef CONFIG_64BIT

	/* Unroll the loop.  */
	ldi		(PAGE_SIZE / 128), %r1

1:
	std		%r0, 0(%r26)
	std		%r0, 8(%r26)
	std		%r0, 16(%r26)
	std		%r0, 24(%r26)
	std		%r0, 32(%r26)
	std		%r0, 40(%r26)
	std		%r0, 48(%r26)
	std		%r0, 56(%r26)
	std		%r0, 64(%r26)
	std		%r0, 72(%r26)
	std		%r0, 80(%r26)
	std		%r0, 88(%r26)
	std		%r0, 96(%r26)
	std		%r0, 104(%r26)
	std		%r0, 112(%r26)
	std		%r0, 120(%r26)

	/* Note reverse branch hint for addib is taken.  */
	addib,COND(>),n	-1, %r1, 1b
	ldo		128(%r26), %r26

#else

	/*
	 * Note that until (if) we start saving the full 64-bit register
	 * values on interrupt, we can't use std on a 32 bit kernel.
	 */
	ldi		(PAGE_SIZE / 64), %r1

1:
	stw		%r0, 0(%r26)
	stw		%r0, 4(%r26)
	stw		%r0, 8(%r26)
	stw		%r0, 12(%r26)
	stw		%r0, 16(%r26)
	stw		%r0, 20(%r26)
	stw		%r0, 24(%r26)
	stw		%r0, 28(%r26)
	stw		%r0, 32(%r26)
	stw		%r0, 36(%r26)
	stw		%r0, 40(%r26)
	stw		%r0, 44(%r26)
	stw		%r0, 48(%r26)
	stw		%r0, 52(%r26)
	stw		%r0, 56(%r26)
	stw		%r0, 60(%r26)

	addib,COND(>),n	-1, %r1, 1b
	ldo		64(%r26), %r26
#endif
	bv		%r0(%r2)
	nop
ENDPROC_CFI(clear_page_asm)

/* Copy page using kernel mapping.  */

ENTRY_CFI(copy_page_asm)
#ifdef CONFIG_64BIT
	/* PA8x00 CPUs can consume 2 loads or 1 store per cycle.
	 * Unroll the loop by hand and arrange insn appropriately.
	 * Prefetch doesn't improve performance on rp3440.
	 * GCC probably can do this just as well...
	 */

	ldi		(PAGE_SIZE / 128), %r1

1:	ldd		0(%r25), %r19
	ldd		8(%r25), %r20

	ldd		16(%r25), %r21
	ldd		24(%r25), %r22
	std		%r19, 0(%r26)
	std		%r20, 8(%r26)

	ldd		32(%r25), %r19
	ldd		40(%r25), %r20
	std		%r21, 16(%r26)
	std		%r22, 24(%r26)

	ldd		48(%r25), %r21
	ldd		56(%r25), %r22
	std		%r19, 32(%r26)
	std		%r20, 40(%r26)

	ldd		64(%r25), %r19
	ldd		72(%r25), %r20
	std		%r21, 48(%r26)
	std		%r22, 56(%r26)

	ldd		80(%r25), %r21
	ldd		88(%r25), %r22
	std		%r19, 64(%r26)
	std		%r20, 72(%r26)

	ldd		 96(%r25), %r19
	ldd		104(%r25), %r20
	std		%r21, 80(%r26)
	std		%r22, 88(%r26)

	ldd		112(%r25), %r21
	ldd		120(%r25), %r22
	ldo		128(%r25), %r25
	std		%r19, 96(%r26)
	std		%r20, 104(%r26)

	std		%r21, 112(%r26)
	std		%r22, 120(%r26)

	/* Note reverse branch hint for addib is taken.  */
	addib,COND(>),n	-1, %r1, 1b
	ldo		128(%r26), %r26

#else

	/*
	 * This loop is optimized for PCXL/PCXL2 ldw/ldw and stw/stw
	 * bundles (very restricted rules for bundling).
	 * Note that until (if) we start saving
	 * the full 64 bit register values on interrupt, we can't
	 * use ldd/std on a 32 bit kernel.
	 */
	ldw		0(%r25), %r19
	ldi		(PAGE_SIZE / 64), %r1

1:
	ldw		4(%r25), %r20
	ldw		8(%r25), %r21
	ldw		12(%r25), %r22
	stw		%r19, 0(%r26)
	stw		%r20, 4(%r26)
	stw		%r21, 8(%r26)
	stw		%r22, 12(%r26)
	ldw		16(%r25), %r19
	ldw		20(%r25), %r20
	ldw		24(%r25), %r21
	ldw		28(%r25), %r22
	stw		%r19, 16(%r26)
	stw		%r20, 20(%r26)
	stw		%r21, 24(%r26)
	stw		%r22, 28(%r26)
	ldw		32(%r25), %r19
	ldw		36(%r25), %r20
	ldw		40(%r25), %r21
	ldw		44(%r25), %r22
	stw		%r19, 32(%r26)
	stw		%r20, 36(%r26)
	stw		%r21, 40(%r26)
	stw		%r22, 44(%r26)
	ldw		48(%r25), %r19
	ldw		52(%r25), %r20
	ldw		56(%r25), %r21
	ldw		60(%r25), %r22
	stw		%r19, 48(%r26)
	stw		%r20, 52(%r26)
	ldo		64(%r25), %r25
	stw		%r21, 56(%r26)
	stw		%r22, 60(%r26)
	ldo		64(%r26), %r26
	addib,COND(>),n	-1, %r1, 1b
	ldw		0(%r25), %r19
#endif
	bv		%r0(%r2)
	nop
ENDPROC_CFI(copy_page_asm)

/*
 * NOTE: Code in clear_user_page has a hard coded dependency on the
 *       maximum alias boundary being 4 Mb. We've been assured by the
 *       parisc chip designers that there will not ever be a parisc
 *       chip with a larger alias boundary (Never say never :-) ).
 *
 *       Yah, what about the PA8800 and PA8900 processors?
 *
 *       Subtle: the dtlb miss handlers support the temp alias region by
 *       "knowing" that if a dtlb miss happens within the temp alias
 *       region it must have occurred while in clear_user_page. Since
 *       this routine makes use of processor local translations, we
 *       don't want to insert them into the kernel page table. Instead,
 *       we load up some general registers (they need to be registers
 *       which aren't shadowed) with the physical page numbers (preshifted
 *       for tlb insertion) needed to insert the translations. When we
 *       miss on the translation, the dtlb miss handler inserts the
 *       translation into the tlb using these values:
 *
 *          %r26 physical address of "to" translation
 *          %r23 physical address of "from" translation
 */

	/*
	 * copy_user_page_asm() performs a page copy using mappings
	 * equivalent to the user page mappings.  It can be used to
	 * implement copy_user_page() but unfortunately both the `from'
	 * and `to' pages need to be flushed through mappings equivalent
	 * to the user mappings after the copy because the kernel accesses
	 * the `from' page through the kmap kernel mapping and the `to'
	 * page needs to be flushed since code can be copied.  As a
	 * result, this implementation is less efficient than the simpler
	 * copy using the kernel mapping.  It only needs the `from' page
	 * to flushed via the user mapping.  The kunmap routines handle
	 * the flushes needed for the kernel mapping.
	 *
	 * I'm still keeping this around because it may be possible to
	 * use it if more information is passed into copy_user_page().
	 * Have to do some measurements to see if it is worthwhile to
	 * lobby for such a change.
	 *
	 */

ENTRY_CFI(copy_user_page_asm)
	/* Convert virtual `to' and `from' addresses to physical addresses.
	   Move `from' physical address to non shadowed register.  */
	ldil		L%(__PAGE_OFFSET), %r1
	sub		%r26, %r1, %r26
	sub		%r25, %r1, %r23

	ldil		L%(TMPALIAS_MAP_START), %r28
	dep_safe	%r24, 31,TMPALIAS_SIZE_BITS, %r28	/* Form aliased virtual address 'to' */
	depi_safe	0, 31,PAGE_SHIFT, %r28			/* Clear any offset bits */
	copy		%r28, %r29
	depi_safe	1, 31-TMPALIAS_SIZE_BITS,1, %r29	/* Form aliased virtual address 'from' */

	/* Purge any old translations */

0:	pdtlb		%r0(%r28)
1:	pdtlb		%r0(%r29)
	ALTERNATIVE(0b, 0b+4, ALT_COND_PACACHE, INSN_PxTLB)
	ALTERNATIVE(1b, 1b+4, ALT_COND_PACACHE, INSN_PxTLB)

#ifdef CONFIG_64BIT
	/* PA8x00 CPUs can consume 2 loads or 1 store per cycle.
	 * Unroll the loop by hand and arrange insn appropriately.
	 * GCC probably can do this just as well.
	 */

	ldd		0(%r29), %r19
	ldi		(PAGE_SIZE / 128), %r1

1:	ldd		8(%r29), %r20

	ldd		16(%r29), %r21
	ldd		24(%r29), %r22
	std		%r19, 0(%r28)
	std		%r20, 8(%r28)

	ldd		32(%r29), %r19
	ldd		40(%r29), %r20
	std		%r21, 16(%r28)
	std		%r22, 24(%r28)

	ldd		48(%r29), %r21
	ldd		56(%r29), %r22
	std		%r19, 32(%r28)
	std		%r20, 40(%r28)

	ldd		64(%r29), %r19
	ldd		72(%r29), %r20
	std		%r21, 48(%r28)
	std		%r22, 56(%r28)

	ldd		80(%r29), %r21
	ldd		88(%r29), %r22
	std		%r19, 64(%r28)
	std		%r20, 72(%r28)

	ldd		 96(%r29), %r19
	ldd		104(%r29), %r20
	std		%r21, 80(%r28)
	std		%r22, 88(%r28)

	ldd		112(%r29), %r21
	ldd		120(%r29), %r22
	std		%r19, 96(%r28)
	std		%r20, 104(%r28)

	ldo		128(%r29), %r29
	std		%r21, 112(%r28)
	std		%r22, 120(%r28)
	ldo		128(%r28), %r28

	/* conditional branches nullify on forward taken branch, and on
	 * non-taken backward branch. Note that .+4 is a backwards branch.
	 * The ldd should only get executed if the branch is taken.
	 */
	addib,COND(>),n	-1, %r1, 1b		/* bundle 10 */
	ldd		0(%r29), %r19		/* start next loads */

#else
	ldi		(PAGE_SIZE / 64), %r1

	/*
	 * This loop is optimized for PCXL/PCXL2 ldw/ldw and stw/stw
	 * bundles (very restricted rules for bundling). It probably
	 * does OK on PCXU and better, but we could do better with
	 * ldd/std instructions. Note that until (if) we start saving
	 * the full 64 bit register values on interrupt, we can't
	 * use ldd/std on a 32 bit kernel.
	 */

1:	ldw		0(%r29), %r19
	ldw		4(%r29), %r20
	ldw		8(%r29), %r21
	ldw		12(%r29), %r22
	stw		%r19, 0(%r28)
	stw		%r20, 4(%r28)
	stw		%r21, 8(%r28)
	stw		%r22, 12(%r28)
	ldw		16(%r29), %r19
	ldw		20(%r29), %r20
	ldw		24(%r29), %r21
	ldw		28(%r29), %r22
	stw		%r19, 16(%r28)
	stw		%r20, 20(%r28)
	stw		%r21, 24(%r28)
	stw		%r22, 28(%r28)
	ldw		32(%r29), %r19
	ldw		36(%r29), %r20
	ldw		40(%r29), %r21
	ldw		44(%r29), %r22
	stw		%r19, 32(%r28)
	stw		%r20, 36(%r28)
	stw		%r21, 40(%r28)
	stw		%r22, 44(%r28)
	ldw		48(%r29), %r19
	ldw		52(%r29), %r20
	ldw		56(%r29), %r21
	ldw		60(%r29), %r22
	stw		%r19, 48(%r28)
	stw		%r20, 52(%r28)
	stw		%r21, 56(%r28)
	stw		%r22, 60(%r28)
	ldo		64(%r28), %r28

	addib,COND(>)		-1, %r1,1b
	ldo		64(%r29), %r29
#endif

	bv		%r0(%r2)
	nop
ENDPROC_CFI(copy_user_page_asm)

ENTRY_CFI(clear_user_page_asm)
	tophys_r1	%r26

	ldil		L%(TMPALIAS_MAP_START), %r28
	dep_safe	%r25, 31,TMPALIAS_SIZE_BITS, %r28	/* Form aliased virtual address 'to' */
	depi_safe	0, 31,PAGE_SHIFT, %r28			/* Clear any offset bits */

	/* Purge any old translation */

0:	pdtlb		%r0(%r28)
	ALTERNATIVE(0b, 0b+4, ALT_COND_PACACHE, INSN_PxTLB)

#ifdef CONFIG_64BIT
	ldi		(PAGE_SIZE / 128), %r1

	/* PREFETCH (Write) has not (yet) been proven to help here */
	/* #define	PREFETCHW_OP	ldd		256(%0), %r0 */

1:	std		%r0, 0(%r28)
	std		%r0, 8(%r28)
	std		%r0, 16(%r28)
	std		%r0, 24(%r28)
	std		%r0, 32(%r28)
	std		%r0, 40(%r28)
	std		%r0, 48(%r28)
	std		%r0, 56(%r28)
	std		%r0, 64(%r28)
	std		%r0, 72(%r28)
	std		%r0, 80(%r28)
	std		%r0, 88(%r28)
	std		%r0, 96(%r28)
	std		%r0, 104(%r28)
	std		%r0, 112(%r28)
	std		%r0, 120(%r28)
	addib,COND(>)		-1, %r1, 1b
	ldo		128(%r28), %r28

#else	/* ! CONFIG_64BIT */
	ldi		(PAGE_SIZE / 64), %r1

1:	stw		%r0, 0(%r28)
	stw		%r0, 4(%r28)
	stw		%r0, 8(%r28)
	stw		%r0, 12(%r28)
	stw		%r0, 16(%r28)
	stw		%r0, 20(%r28)
	stw		%r0, 24(%r28)
	stw		%r0, 28(%r28)
	stw		%r0, 32(%r28)
	stw		%r0, 36(%r28)
	stw		%r0, 40(%r28)
	stw		%r0, 44(%r28)
	stw		%r0, 48(%r28)
	stw		%r0, 52(%r28)
	stw		%r0, 56(%r28)
	stw		%r0, 60(%r28)
	addib,COND(>)		-1, %r1, 1b
	ldo		64(%r28), %r28
#endif	/* CONFIG_64BIT */

	bv		%r0(%r2)
	nop
ENDPROC_CFI(clear_user_page_asm)

ENTRY_CFI(flush_dcache_page_asm)
	ldil		L%(TMPALIAS_MAP_START), %r28
	dep_safe	%r25, 31,TMPALIAS_SIZE_BITS, %r28	/* Form aliased virtual address 'to' */
	depi_safe	0, 31,PAGE_SHIFT, %r28			/* Clear any offset bits */

	/* Purge any old translation */

0:	pdtlb		%r0(%r28)
	ALTERNATIVE(0b, 0b+4, ALT_COND_PACACHE, INSN_PxTLB)

88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), r31

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r28, %r25, %r25
	sub		%r25, r31, %r25

1:	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	fdc,m		r31(%r28)
	cmpb,COND(>>)	%r25, %r28, 1b /* predict taken */
	fdc,m		r31(%r28)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_dcache_page_asm)

ENTRY_CFI(purge_dcache_page_asm)
	ldil		L%(TMPALIAS_MAP_START), %r28
	dep_safe	%r25, 31,TMPALIAS_SIZE_BITS, %r28	/* Form aliased virtual address 'to' */
	depi_safe	0, 31,PAGE_SHIFT, %r28			/* Clear any offset bits */

	/* Purge any old translation */

0:	pdtlb		%r0(%r28)
	ALTERNATIVE(0b, 0b+4, ALT_COND_PACACHE, INSN_PxTLB)

88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), r31

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r28, %r25, %r25
	sub		%r25, r31, %r25

1:      pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	pdc,m		r31(%r28)
	cmpb,COND(>>)	%r25, %r28, 1b /* predict taken */
	pdc,m		r31(%r28)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(purge_dcache_page_asm)

ENTRY_CFI(flush_icache_page_asm)
	ldil		L%(TMPALIAS_MAP_START), %r28
	dep_safe	%r25, 31,TMPALIAS_SIZE_BITS, %r28	/* Form aliased virtual address 'to' */
	depi_safe	0, 31,PAGE_SHIFT, %r28			/* Clear any offset bits */

	/* Purge any old translation.  Note that the FIC instruction
	 * may use either the instruction or data TLB.  Given that we
	 * have a flat address space, it's not clear which TLB will be
	 * used.  So, we purge both entries.  */

0:	pdtlb		%r0(%r28)
1:	pitlb           %r0(%sr4,%r28)
	ALTERNATIVE(0b, 0b+4, ALT_COND_PACACHE, INSN_PxTLB)
	ALTERNATIVE(1b, 1b+4, ALT_COND_PACACHE, INSN_PxTLB)
	ALTERNATIVE(1b, 1b+4, ALT_COND_NO_SPLIT_TLB, INSN_NOP)

88:	ldil		L%icache_stride, %r1
	ldw		R%icache_stride(%r1), %r31

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r28, %r25, %r25
	sub		%r25, %r31, %r25

	/* fic only has the type 26 form on PA1.1, requiring an
	 * explicit space specification, so use %sr4 */
1:      fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	fic,m		%r31(%sr4,%r28)
	cmpb,COND(>>)	%r25, %r28, 1b /* predict taken */
	fic,m		%r31(%sr4,%r28)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_ICACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_icache_page_asm)

ENTRY_CFI(flush_kernel_dcache_page_asm)
88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), %r23

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r26, %r25, %r25
	sub		%r25, %r23, %r25

1:      fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	cmpb,COND(>>)	%r25, %r26, 1b /* predict taken */
	fdc,m		%r23(%r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_kernel_dcache_page_asm)

ENTRY_CFI(purge_kernel_dcache_page_asm)
88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), %r23

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r26, %r25, %r25
	sub		%r25, %r23, %r25

1:      pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	cmpb,COND(>>)	%r25, %r26, 1b /* predict taken */
	pdc,m		%r23(%r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(purge_kernel_dcache_page_asm)

ENTRY_CFI(flush_user_dcache_range_asm)
88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), %r23
	ldo		-1(%r23), %r21
	ANDCM		%r26, %r21, %r26

#ifdef CONFIG_64BIT
	depd,z		%r23, 59, 60, %r21
#else
	depw,z		%r23, 27, 28, %r21
#endif
	add		%r26, %r21, %r22
	cmpb,COND(>>),n	%r22, %r25, 2f /* predict not taken */
1:	add		%r22, %r21, %r22
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	fdc,m		%r23(%sr3, %r26)
	cmpb,COND(<<=)	%r22, %r25, 1b /* predict taken */
	fdc,m		%r23(%sr3, %r26)

2:	cmpb,COND(>>),n	%r25, %r26, 2b
	fdc,m		%r23(%sr3, %r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_user_dcache_range_asm)

ENTRY_CFI(flush_kernel_dcache_range_asm)
88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), %r23
	ldo		-1(%r23), %r21
	ANDCM		%r26, %r21, %r26

#ifdef CONFIG_64BIT
	depd,z		%r23, 59, 60, %r21
#else
	depw,z		%r23, 27, 28, %r21
#endif
	add		%r26, %r21, %r22
	cmpb,COND(>>),n	%r22, %r25, 2f /* predict not taken */
1:	add		%r22, %r21, %r22
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	fdc,m		%r23(%r26)
	cmpb,COND(<<=)	%r22, %r25, 1b /* predict taken */
	fdc,m		%r23(%r26)

2:	cmpb,COND(>>),n	%r25, %r26, 2b /* predict taken */
	fdc,m		%r23(%r26)

	sync
89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_kernel_dcache_range_asm)

ENTRY_CFI(purge_kernel_dcache_range_asm)
88:	ldil		L%dcache_stride, %r1
	ldw		R%dcache_stride(%r1), %r23
	ldo		-1(%r23), %r21
	ANDCM		%r26, %r21, %r26

#ifdef CONFIG_64BIT
	depd,z		%r23, 59, 60, %r21
#else
	depw,z		%r23, 27, 28, %r21
#endif
	add		%r26, %r21, %r22
	cmpb,COND(>>),n	%r22, %r25, 2f /* predict not taken */
1:	add		%r22, %r21, %r22
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	pdc,m		%r23(%r26)
	cmpb,COND(<<=)	%r22, %r25, 1b /* predict taken */
	pdc,m		%r23(%r26)

2:	cmpb,COND(>>),n	%r25, %r26, 2b /* predict taken */
	pdc,m		%r23(%r26)

	sync
89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_DCACHE, INSN_NOP)
	bv		%r0(%r2)
	nop
ENDPROC_CFI(purge_kernel_dcache_range_asm)

ENTRY_CFI(flush_user_icache_range_asm)
88:	ldil		L%icache_stride, %r1
	ldw		R%icache_stride(%r1), %r23
	ldo		-1(%r23), %r21
	ANDCM		%r26, %r21, %r26

#ifdef CONFIG_64BIT
	depd,z		%r23, 59, 60, %r21
#else
	depw,z		%r23, 27, 28, %r21
#endif
	add		%r26, %r21, %r22
	cmpb,COND(>>),n	%r22, %r25, 2f /* predict not taken */
1:	add		%r22, %r21, %r22
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	fic,m		%r23(%sr3, %r26)
	cmpb,COND(<<=)	%r22, %r25, 1b /* predict taken */
	fic,m		%r23(%sr3, %r26)

2:	cmpb,COND(>>),n	%r25, %r26, 2b
	fic,m		%r23(%sr3, %r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_ICACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_user_icache_range_asm)

ENTRY_CFI(flush_kernel_icache_page)
88:	ldil		L%icache_stride, %r1
	ldw		R%icache_stride(%r1), %r23

#ifdef CONFIG_64BIT
	depdi,z		1, 63-PAGE_SHIFT,1, %r25
#else
	depwi,z		1, 31-PAGE_SHIFT,1, %r25
#endif
	add		%r26, %r25, %r25
	sub		%r25, %r23, %r25


1:      fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	cmpb,COND(>>)	%r25, %r26, 1b /* predict taken */
	fic,m		%r23(%sr4, %r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_ICACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_kernel_icache_page)

ENTRY_CFI(flush_kernel_icache_range_asm)
88:	ldil		L%icache_stride, %r1
	ldw		R%icache_stride(%r1), %r23
	ldo		-1(%r23), %r21
	ANDCM		%r26, %r21, %r26

#ifdef CONFIG_64BIT
	depd,z		%r23, 59, 60, %r21
#else
	depw,z		%r23, 27, 28, %r21
#endif
	add		%r26, %r21, %r22
	cmpb,COND(>>),n	%r22, %r25, 2f /* predict not taken */
1:	add		%r22, %r21, %r22
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	fic,m		%r23(%sr4, %r26)
	cmpb,COND(<<=)	%r22, %r25, 1b /* predict taken */
	fic,m		%r23(%sr4, %r26)

2:	cmpb,COND(>>),n	%r25, %r26, 2b /* predict taken */
	fic,m		%r23(%sr4, %r26)

89:	ALTERNATIVE(88b, 89b, ALT_COND_NO_ICACHE, INSN_NOP)
	sync
	bv		%r0(%r2)
	nop
ENDPROC_CFI(flush_kernel_icache_range_asm)

	.text

	/* align should cover use of rfi in disable_sr_hashing_asm and
	 * srdis_done.
	 */
	.align	256
ENTRY_CFI(disable_sr_hashing_asm)
	/*
	 * Switch to real mode
	 */
	/* pcxt_ssm_bug */
	rsm		PSW_SM_I, %r0
	load32		PA(1f), %r1
	nop
	nop
	nop
	nop
	nop

	rsm		PSW_SM_Q, %r0		/* prep to load iia queue */
	mtctl		%r0, %cr17		/* Clear IIASQ tail */
	mtctl		%r0, %cr17		/* Clear IIASQ head */
	mtctl		%r1, %cr18		/* IIAOQ head */
	ldo		4(%r1), %r1
	mtctl		%r1, %cr18		/* IIAOQ tail */
	load32		REAL_MODE_PSW, %r1
	mtctl		%r1, %ipsw
	rfi
	nop

1:      cmpib,=,n	SRHASH_PCXST, %r26,srdis_pcxs
	cmpib,=,n	SRHASH_PCXL, %r26,srdis_pcxl
	cmpib,=,n	SRHASH_PA20, %r26,srdis_pa20
	b,n		srdis_done

srdis_pcxs:

	/* Disable Space Register Hashing for PCXS,PCXT,PCXT' */

	.word		0x141c1a00		/* mfdiag %dr0, %r28 */
	.word		0x141c1a00		/* must issue twice */
	depwi		0,18,1, %r28		/* Clear DHE (dcache hash enable) */
	depwi		0,20,1, %r28		/* Clear IHE (icache hash enable) */
	.word		0x141c1600		/* mtdiag %r28, %dr0 */
	.word		0x141c1600		/* must issue twice */
	b,n		srdis_done

srdis_pcxl:

	/* Disable Space Register Hashing for PCXL */

	.word		0x141c0600		/* mfdiag %dr0, %r28 */
	depwi           0,28,2, %r28		/* Clear DHASH_EN & IHASH_EN */
	.word		0x141c0240		/* mtdiag %r28, %dr0 */
	b,n		srdis_done

srdis_pa20:

	/* Disable Space Register Hashing for PCXU,PCXU+,PCXW,PCXW+,PCXW2 */

	.word		0x144008bc		/* mfdiag %dr2, %r28 */
	depdi		0, 54,1, %r28		/* clear DIAG_SPHASH_ENAB (bit 54) */
	.word		0x145c1840		/* mtdiag %r28, %dr2 */


srdis_done:
	/* Switch back to virtual mode */
	rsm		PSW_SM_I, %r0		/* prep to load iia queue */
	load32 	   	2f, %r1
	nop
	nop
	nop
	nop
	nop

	rsm		PSW_SM_Q, %r0		/* prep to load iia queue */
	mtctl		%r0, %cr17		/* Clear IIASQ tail */
	mtctl		%r0, %cr17		/* Clear IIASQ head */
	mtctl		%r1, %cr18		/* IIAOQ head */
	ldo		4(%r1), %r1
	mtctl		%r1, %cr18		/* IIAOQ tail */
	load32		KERNEL_PSW, %r1
	mtctl		%r1, %ipsw
	rfi
	nop

2:      bv		%r0(%r2)
	nop
ENDPROC_CFI(disable_sr_hashing_asm)

	.end
