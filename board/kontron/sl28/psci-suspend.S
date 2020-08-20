#include <config.h>
#include <linux/linkage.h>
#include <asm/macro.h>
#include <asm/gic.h>

#define NXP_PMU_ADDR                    0x01E30000
#define NXP_EPU_ADDR                    0x700060000
 /* epu register offsets and values */
#define EPU_EPGCR_OFFSET              0x0
#define EPU_EPIMCR10_OFFSET           0x128
#define EPU_EPCTR10_OFFSET            0xa28
#define EPU_EPCCR10_OFFSET            0x828
#define EPU_EPCCR10_VAL               0xb2800000
#define EPU_EPIMCR10_VAL              0xba000000
#define EPU_EPCTR10_VAL               0x0
#define EPU_EPGCR_VAL                 (1 << 31)
#define NXP_GICD_ADDR                   0x06000000

#define GICD_IROUTER60_OFFSET   0x61e0
#define GICD_IROUTER76_OFFSET   0x6260
#define GICD_IROUTER89_OFFSET   0x62C8
#define GICD_IROUTER112_OFFSET  0x6380
#define GICD_IROUTER113_OFFSET  0x6388
#define GICD_IROUTER_VALUE      0x100
#define NXP_DCFG_ADDR                   0x01E00000
#define DCFG_DEVDISR1_OFFSET          0x70
#define DCFG_DEVDISR2_OFFSET          0x74
#define DCFG_DEVDISR4_OFFSET          0x7c
#define DCFG_DEVDISR1_SEC             (1 << 22)
#define DCFG_DEVDISR4_SPI_QSPI        (1 << 4 | 1 << 5)
#define DCFG_DEVDISR2_ENETC           (1 << 31)
#define TPMWAKEMR0_ADDR               0x700123c50
#define NXP_CCI_ADDR                    0x04090000

#define  CPUECTLR_SMPEN_EN      0x40
#define  CPUECTLR_TIMER_2TICKS  0x1

#define GICR_WAKER_OFFSET       0x14
#define GICR_WAKER_SLEEP_BIT    0x2
#define GICR_WAKER_ASLEEP       (1 << 2 | 1 << 1)


#define PMU_PCPW20SR_OFFSET           0x830
#define PMU_CLAINACTSETR_OFFSET       0x1100
#define PMU_CLAINACTCLRR_OFFSET       0x1104
#define PMU_CLSINACTSETR_OFFSET       0x1108
#define PMU_CLSINACTCLRR_OFFSET       0x110C
#define PMU_CLL2FLUSHSETR_OFFSET      0x1110
#define PMU_CLSL2FLUSHCLRR_OFFSET     0x1114
#define PMU_CLL2FLUSHSR_OFFSET        0x1118
#define PMU_POWMGTCSR_OFFSET          0x4000
#define PMU_POWMGTCSR_VAL             (1 << 20)
#define PMU_IDLE_CLUSTER_MASK         0x0
#define PMU_FLUSH_CLUSTER_MASK        0x0
#define PMU_IDLE_CORE_MASK            0xfe

#define PMU_POWMGTCSR_OFFSET          0x4000
#define PMU_POWMGTCSR_VAL             (1 << 20)
#define NXP_GICR_ADDR                 0x06040000
#define GIC_RD_OFFSET       0x00020000

#define CPUECTLR_EL1  S3_1_C15_C2_1
#define CPUACTLR_EL1  S3_1_C15_C2_0

.pushsection ._secure.text, "ax"

ENTRY(psci_system_suspend)
    mov  x18, x30

    mov  x3, #NXP_PMU_ADDR

     /* x3 = pmu base addr */

     /* backup epu registers to stack */
    ldr  x2, =NXP_EPU_ADDR
    ldr  w4, [x2, #EPU_EPIMCR10_OFFSET]
    ldr  w5, [x2, #EPU_EPCCR10_OFFSET]
    ldr  w6, [x2, #EPU_EPCTR10_OFFSET]
    ldr  w7, [x2, #EPU_EPGCR_OFFSET]
    stp  x4,  x5,  [sp, #-16]!
    stp  x6,  x7,  [sp, #-16]!

     /* x2 = epu base addr
      * x3 = pmu base addr
      */

     /* set up EPU event to receive the wake signal from PMU */
    mov  w4, #EPU_EPIMCR10_VAL
    mov  w5, #EPU_EPCCR10_VAL
    mov  w6, #EPU_EPCTR10_VAL
    mov  w7, #EPU_EPGCR_VAL
    str  w4, [x2, #EPU_EPIMCR10_OFFSET]
    str  w5, [x2, #EPU_EPCCR10_OFFSET]
    str  w6, [x2, #EPU_EPCTR10_OFFSET]
    str  w7, [x2, #EPU_EPGCR_OFFSET]

    ldr  x2, =NXP_GICD_ADDR

     /* x2 = gicd base addr
      * x3 = pmu base addr
      */

     /* backup flextimer/mmc/usb interrupt router */
    ldr  x0, =GICD_IROUTER60_OFFSET
    ldr  x1, =GICD_IROUTER76_OFFSET
    ldr  w4, [x2, x0]
    ldr  w5, [x2, x1]
    ldr  x0, =GICD_IROUTER112_OFFSET
    ldr  x1, =GICD_IROUTER113_OFFSET
    ldr  w6, [x2, x0]
    ldr  w7, [x2, x1]
    stp  x4,  x5,  [sp, #-16]!
    stp  x6,  x7,  [sp, #-16]!

     /* x2 = gicd base addr
      * x3 = pmu base addr
      * x0 = GICD_IROUTER112_OFFSET
      * x1 = GICD_IROUTER113_OFFSET
      */

     /* re-route interrupt to cluster 1 */
    ldr  w4, =GICD_IROUTER_VALUE
    str  w4, [x2, x0]
    str  w4, [x2, x1]
    ldr  x0, =GICD_IROUTER60_OFFSET
    ldr  x1, =GICD_IROUTER76_OFFSET
    str  w4, [x2, x0]
    str  w4, [x2, x1]
    dsb  sy
    isb

     /* x3 = pmu base addr */

     /* disable sec, Check for eNETC, spi and qspi */
    ldr  x2, =NXP_DCFG_ADDR
    ldr  x0, =DCFG_DEVDISR1_OFFSET
    ldr  w1, =DCFG_DEVDISR1_SEC
    str  w1, [x2, x0]
#if 0
    ldr  x0, =DCFG_DEVDISR2_OFFSET
    ldr  w1, =DCFG_DEVDISR2_ENETC
    str  w1, [x2, x0]
#endif
    ldr  x0, =DCFG_DEVDISR4_OFFSET
    ldr  w1, =DCFG_DEVDISR4_SPI_QSPI
    str  w1, [x2, x0]

     /* x3 = pmu base addr */

     /* set TPMWAKEMR0 */
    ldr  x0, =TPMWAKEMR0_ADDR
    mov  w1, #0x1
    str  w1, [x0]

     /* disable CCI snoop domain */
    ldr  x0, =NXP_CCI_ADDR
    mov  w1, #0x1
    str  w1, [x0]

     // setup retention control
    mrs  x0, CPUECTLR_EL1
    orr  x0, x0, #CPUECTLR_SMPEN_EN
    orr  x0, x0, #CPUECTLR_TIMER_2TICKS
    msr  CPUECTLR_EL1, x0
    dsb  sy
    isb

	/* DONE */
#if 0
    bl get_pmu_idle_core_mask
#else
	mov x0, 1
#endif
    mov  x3, #NXP_PMU_ADDR

8:
    ldr  w1, [x3, #PMU_PCPW20SR_OFFSET]
    cmp  w1, w0
    b.ne 8b

	mov  x3, #0x2310000
	mov	 w4, #0x01000000
	str  w4, [x3]
     /* x3 = NXP_PMU_ADDR */
     /* 1 cluster SoC */

#if 0
    bl get_pmu_idle_cluster_mask
#else
	mov x0, 1
#endif
    mov  x3, #NXP_PMU_ADDR

    str  w0, [x3, #PMU_CLAINACTSETR_OFFSET]

#if 0
    bl get_pmu_idle_core_mask
#else
	mov x0, 1
#endif
     /* x3 = NXP_PMU_ADDR */
    mov  x3, #NXP_PMU_ADDR

1:
    ldr  w1, [x3, #PMU_PCPW20SR_OFFSET]
    cmp  w1, w0
    b.ne 1b


     /* x3 = NXP_PMU_ADDR */
#if 0
    bl get_pmu_flush_cluster_mask
#else
	mov x0, 1
#endif
    mov  x3, #NXP_PMU_ADDR

    str  w0, [x3, #PMU_CLL2FLUSHSETR_OFFSET]

     /* x3 = NXP_PMU_ADDR */

2:
    ldr  w1, [x3, #PMU_CLL2FLUSHSR_OFFSET]
    cmp  w1, w0
    b.ne 2b

     /* x3 = NXP_PMU_ADDR */

    str  w0, [x3, #PMU_CLSL2FLUSHCLRR_OFFSET]

    str  w0, [x3, #PMU_CLSINACTSETR_OFFSET]

     /* force the debug interface to be quiescent */
    mrs  x0, osdlr_el1
    orr  x0, x0, #0x1
    msr  osdlr_el1, x0

     /* enable the WakeRequest signal
      * x3 is cpu mask starting from cpu1 to cpu0
      */
#if 0
    bl get_tot_num_cores
#else
	mov x0, 2
#endif
    sub  x0, x0, #1
    mov  x3, #0x1
    lsl  x3, x3, x0
2:
    mov  x0, x3
    bl   get_gic_rd_base  // 0-2
    ldr  w1, [x0, #GICR_WAKER_OFFSET]
    orr  w1, w1, #GICR_WAKER_SLEEP_BIT
    str  w1, [x0, #GICR_WAKER_OFFSET]
1:
    ldr  w1, [x0, #GICR_WAKER_OFFSET]
    cmp  w1, #GICR_WAKER_ASLEEP
    b.ne 1b

    lsr  x3, x3, #1
    cbnz x3, 2b

     /* invalidate all TLB entries at all 3 exception levels */
    tlbi alle1
    tlbi alle2
    tlbi alle3

     /* request lpm20 */

    mov  x3, #NXP_PMU_ADDR
    ldr  x0, =PMU_POWMGTCSR_OFFSET
    ldr  w1, =PMU_POWMGTCSR_VAL
    str  w1, [x3, x0]

    ldr  x5, =NXP_EPU_ADDR
4:
    wfe
    ldr  w1, [x5, #EPU_EPCTR10_OFFSET]
    cmp  w1, #0
    b.eq  4b

0:  b 0b

ENDPROC(psci_system_suspend)

ENTRY(get_gic_rd_base)
     /* get the 0-based core number */
    clz  w1, w0
    mov  w2, #0x20
    sub  w2, w2, w1
    sub  w2, w2, #1

     /* x2 = core number / loop counter */

    ldr  x0, =NXP_GICR_ADDR
    mov  x1, #GIC_RD_OFFSET
2:
    cbz  x2, 1f
    add  x0, x0, x1
    sub  x2, x2, #1
    b    2b
1:
    ret
ENDPROC(get_gic_rd_base)

ENTRY(psci_system_suspend_64)
	b psci_system_suspend
ENDPROC(psci_system_suspend_64)

.popsection
