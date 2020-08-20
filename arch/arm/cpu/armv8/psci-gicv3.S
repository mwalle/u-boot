/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/macro.h>
#include <asm/gic.h>

.pushsection ._secure.text, "ax"

/* void psci_gic_get_base(u64 mpidr) */
ENTRY(psci_gic_get_base)
	lsr	x1, x0, #32
	bfi	x0, x1, #24, #8
	mov	x1, x0			/* w1 is aff3:aff2:aff1:aff0 */
	ldr	x0, =GICR_BASE
1:	ldr	x2, [x0, GICR_TYPER]
	lsr	x2, x2, #32		/* w2 is aff3:aff2:aff1:aff0 */
	cmp	w1, w2
	b.eq	2f
	add	x0, x0, #0x20000
	b	1b
2:
	ret
ENDPROC(psci_gic_get_base)

/* void psci_gic_setup(void) */
ENTRY(psci_gic_setup)
	mov	x9, x30

	mrs	x0, mpidr_el1
	bl	psci_gic_get_base

	/* x0: ReDistributor Base Address of Current CPU */
2:	mov	w10, #~0x2
	ldr	w11, [x0, GICR_WAKER]
	and	w11, w11, w10		/* Clear ProcessorSleep */
	str	w11, [x0, GICR_WAKER]
	dsb	st
	isb
3:	ldr	w10, [x0, GICR_WAKER]
	tbnz	w10, #2, 3b		/* Wait Children be Alive */

	add	x10, x0, #(1 << 16)	/* SGI_Base */
	mov	w11, #~0
	str	w11, [x10, GICR_IGROUPRn]
	str	wzr, [x10, GICR_IGROUPMODRn]	/* SGIs|PPIs Group1NS */
	mov	w11, #0x1		/* Enable SGI 0 */
	str	w11, [x10, GICR_ISENABLERn]

	/* Initialize Cpu Interface */
	mrs	x10, ICC_SRE_EL3
	orr	x10, x10, #0xf		/* SRE & Disable IRQ/FIQ Bypass & */
					/* Allow EL2 access to ICC_SRE_EL2 */
	msr	ICC_SRE_EL3, x10
	isb

	mov	x10, #0x3		/* EnableGrp1NS | EnableGrp1S */
	msr	ICC_IGRPEN1_EL3, x10
	isb

	msr	ICC_CTLR_EL3, xzr
	isb

	mrs	x10, ICC_SRE_EL2
	orr	x10, x10, #0xf		/* SRE & Disable IRQ/FIQ Bypass & */
					/* Allow EL1 access to ICC_SRE_EL1 */
	msr	ICC_SRE_EL2, x10
	isb

	msr	ICC_CTLR_EL1, xzr	/* NonSecure ICC_CTLR_EL1 */
	isb

	mov	x10, #0x1 << 7		/* Non-Secure access to ICC_PMR_EL1 */
	msr	ICC_PMR_EL1, x10
	isb

	mov	x30, x9
	ret
ENDPROC(psci_gic_setup)

/* void psci_gic_kick_cpu_core(u64 mpidr) */
ENTRY(psci_gic_kick_cpu_core)
	mov	x1, #GICD_BASE
	ldr	w2, [x1, #GICD_CTLR]
	orr	w2, w2, #(1<<0)
	str	w2, [x1, #GICD_CTLR]
	dsb	sy
	isb

0:	/* poll RWP bit */
	ldr	w2, [x1, #GICD_CTLR]
	tst	w2, #(1<<31)
	b.ne	0b

	/* target list */
	and	x1, x0, #0xf
	mov	x2, #1
	lsl	x2, x2, x1
	and	x1, x0, #0xff00
	lsl	x1, x1, #8
	orr	x2, x2, x1
	orr	x2, x2, #0x0F000000 /* ICC_SGI0R_EL1_INTID */
	msr	ICC_SGI0R_EL1, x2
	dsb	sy
	isb
	ret
ENDPROC(psci_gic_kick_cpu_core)

/* void psci_gic_pre_off(u64 mpidr) */
ENTRY(psci_gic_pre_off)
	mov	x9, x30

	/* x0 is gicr base, x1 is sgi base */
	bl	psci_gic_get_base
	add	x1, x0, #(1 << 16)

	/* disable SGI15 */
	mov	w3, #(1<<15)
	str	w3, [x1, GICR_ICENABLERn]

0:	/* poll RWP bit */
	ldr	w4, [x0, #GICR_CTLR]
	tst	w4, #(1<<3)
	b.ne	0b

	/* disable GRP0 and GPR1 */
	msr	ICC_IGRPEN0_EL1, xzr
	msr	ICC_IGRPEN1_EL3, xzr

	/* set SGI15 as G0S */
	ldr	w4, [x1, #GICR_IGROUPRn]
	bic	w4, w4, #(1<<15)
	str	w4, [x1, #GICR_IGROUPRn]
	ldr	w4, [x1, #GICR_IGROUPMODRn]
	bic	w4, w4, #(1<<15)
	str	w4, [x1, #GICR_IGROUPMODRn]

	/* set highest priority for SGI15 */
	ldr	w4, [x1, #GICR_IPRIORITYR(3)]
	bic	w4, w4, #(0xff << 24)
	str	w4, [x1, #GICR_IPRIORITYR(3)]

	/* enable SGI15 again */
	str	w3, [x1, GICR_ISENABLERn]
	dsb	sy
	isb

0:	/* poll RWP bit */
	ldr	w4, [x0, #GICR_CTLR]
	tst	w4, #(1<<3)
	b.ne	0b

	/* enable GRP0 */
	mov	x4, 1
	msr	ICC_IGRPEN0_EL1, x4

	mvn	w4, wzr
	str	w4, [x1, #GICR_ICPENDRn]

	mov	x30, x9
	ret
ENDPROC(psci_gic_pre_off)

/* void psci_gic_post_off(u64 mpidr) */
ENTRY(psci_gic_post_off)
	mov	x9, x30

	bl	psci_gic_get_base
	add	x1, x0, #(1 << 16)

	/* disable GRP0 */
	msr	ICC_IGRPEN0_EL1, xzr

	/* disable SGI15 */
	mov	w3, #(1<<15)
	str	w3, [x1, GICR_ICENABLERn]

	/* clear pending interrupts */
	mvn	w4, wzr
	str	w4, [x1, #GICR_ICPENDRn]

	mov	x30, x9
	ret
ENDPROC(psci_gic_post_off)

.popsection
