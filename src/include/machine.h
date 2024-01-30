#ifndef __MACHINE_H__
#define __MACHINE_H__


#define MAX_HARTS (8)

#define XREG_ZERO (0)
#define XREG_RA   (1)
#define XREG_SP   (2)
#define XREG_GP   (3)
#define XREG_TP   (4)
#define XREG_T0   (5)
#define XREG_T1   (6)
#define XREG_T2   (7)
#define XREG_S0   (8)
#define XREG_S1   (9)
#define XREG_A0   (10)
#define XREG_A1   (11)
#define XREG_A2   (12)
#define XREG_A3   (13)
#define XREG_A4   (14)
#define XREG_A5   (15)
#define XREG_A6   (16)
#define XREG_A7   (17)
#define XREG_S2   (18)
#define XREG_S3   (19)
#define XREG_S4   (20)
#define XREG_S5   (21)
#define XREG_S6   (22)
#define XREG_S7   (23)
#define XREG_S8   (24)
#define XREG_S9   (25)
#define XREG_S10  (26)
#define XREG_S11  (27)
#define XREG_T3   (28)
#define XREG_T4   (29)
#define XREG_T5   (30)
#define XREG_T6   (31)
#define XREG_NUM  (32)
#define FREG_NUM  (32)


#define CSR_READ(var, csr)    asm volatile("csrr %0, " csr : "=r"(var))
#define CSR_WRITE(csr, var)   asm volatile("csrw " csr ", %0" :: "r"(var))
#define SFENCE()              asm volatile("sfence.vma");
#define SFENCE_ASID(x)        asm volatile("sfence.vma zero, %0" :: "r"(x))
#define SFENCE_VMA(x)         asm volatile("sfence.vma %0, zero" :: "r"(x))
#define SFENCE_ALL(vma, asid) asm volatile("sfence.vma %0, %1" :: "r"(vma), "r"(asid))
#define WAIT_FOR_INTERRUPT()  asm volatile("wfi")
#define MRET()                asm volatile("mret")


#define USER_MODE       (0)
#define SUPERVISOR_MODE (1)
#define MACHINE_MODE    (3)


#define MSTATUS_UIE        (1   << 0)
#define MSTATUS_SIE        (1   << 1)
/* #define MSTATUS_WPRI    (1   << 2) */
#define MSTATUS_MIE        (1   << 3)
#define MSTATUS_UPIE       (1   << 4)
#define MSTATUS_SPIE       (1   << 5)
#define MSTATUS_WPRI       (1   << 6)
#define MSTATUS_MPIE       (1   << 7)
#define MSTATUS_SPP        (1   << 8)
/* #define MSTATUS_WPRI(x) ((x) << 9) */
#define MSTATUS_MPP(x)     ((x) << 11)
#define MSTATUS_FS(x)      ((x) << 13)
#define MSTATUS_XS(x)      ((x) << 15)
#define MSTATUS_MPRV       (1   << 16)
#define MSTATUS_SUM        (1   << 17)
#define MSTATUS_MXR        (1   << 18)
#define MSTATUS_TVM        (1   << 19)
#define MSTATUS_TW         (1   << 20)
#define MSTATUS_TSR        (1   << 21)
/* #define MSTATUS_WPRI(x) ((x) << 22) */
#define MSTATUS_UXL(x)     ((x) << 31)
#define MSTATUS_SXL(x)     ((x) << 33)
/* #define MSTATUS_WPRI(x) ((x) << 34) */
#define MSTATUS_SD         (1   << 63)

#define SSTATUS_UIE        (1   << 0)
#define SSTATUS_SIE        (1   << 1)
/* #define SSTATUS_WPRI    (1   << 2) */
#define SSTATUS_MIE        (1   << 3)
#define SSTATUS_UPIE       (1   << 4)
#define SSTATUS_SPIE       (1   << 5)
#define SSTATUS_WPRI       (1   << 6)
#define SSTATUS_MPIE       (1   << 7)
/* #define SSTATUS_SPP        (1   << 8) */
/* #define SSTATUS_WPRI(x) ((x) << 9) */
#define SSTATUS_SPP(x)     ((x) << 8)
#define SSTATUS_FS(x)      ((x) << 13)
#define SSTATUS_XS(x)      ((x) << 15)
#define SSTATUS_MPRV       (1   << 16)
#define SSTATUS_SUM        (1   << 17)
#define SSTATUS_MXR        (1   << 18)
#define SSTATUS_TVM        (1   << 19)
#define SSTATUS_TW         (1   << 20)
#define SSTATUS_TSR        (1   << 21)
/* #define SSTATUS_WPRI(x) ((x) << 22) */
#define SSTATUS_UXL(x)     ((x) << 31)
#define SSTATUS_SXL(x)     ((x) << 33)
/* #define SSTATUS_WPRI(x) ((x) << 34) */
#define SSTATUS_SD         (1   << 63)

#define MEIE_BIT (11)
#define MEIP_BIT (MEIE_BIT)
#define SEIE_BIT (9)
#define SEIP_BIT (SEIE_BIT)
#define MIE_MEIE (1UL << MEIE_BIT)
#define MIP_MEIP (MIE_MEIE)
#define MIE_SEIE (1UL << SEIE_BIT)
#define MIP_SEIP (MIE_SEIE)
#define SIE_SEIE (MIE_SEIE)
#define SIP_SEIP (SIE_SEIE)
#define MTIE_BIT (7)
#define MTIP_BIT (MTIE_BIT)
#define STIE_BIT (5)
#define STIP_BIT (STIE_BIT)
#define MIE_MTIE (1UL << MTIE_BIT)
#define MIP_MTIP (MIE_MTIE)
#define MIE_STIE (1UL << STIE_BIT)
#define MIP_STIP (MIE_STIE)
#define SIE_STIE (MIE_STIE)
#define SIP_STIP (SIE_STIE)
#define MSIE_BIT (3)
#define MSIP_BIT (MSIE_BIT)
#define SSIE_BIT (1)
#define SSIP_BIT (SSIE_BIT)
#define MIE_MSIE (1UL << MSIE_BIT)
#define MIP_MSIP (MIE_MSIE)
#define MIE_SSIE (1UL << SSIE_BIT)
#define MIP_SSIP (MIE_SSIE)
#define SIE_SSIE (MIE_SSIE)
#define SIP_SSIP (SIE_SSIE)


#define CAUSE_INTERRUPT(cause) ((cause) & 0x7fffffffffffffff)
#define CAUSE_ASYNC(cause)     ((cause) >> 63ULL)


#define INT_USWI     (0)
#define INT_SSWI     (1)
#define INT____2     (2)
#define INT_MSWI     (3)
#define INT_UTIM     (4)
#define INT_STIM     (5)
#define INT____6     (6)
#define INT_MTIM     (7)
#define INT_UEXT     (8)
#define INT_SEXT     (9)
#define INT___10     (10)
#define INT_MEXT     (11)
#define INT_UART     (12)
#define INT_RCLK     (13)
#define INT_PCIA     (14)
#define INT_PCIB     (15)
#define INT_PCIC     (16)
#define INT_PCID     (17)
#define INT_UNKN     (18)
#define INT_BITS_ALL (0xffff)

#define EXC_IMIS     (0)
#define EXC_IFLT     (1)
#define EXC_IILL     (2)
#define EXC_BRPT     (3)
#define EXC_LMIS     (4)
#define EXC_LFLT     (5)
#define EXC_SMIS     (6)
#define EXC_SFLT     (7)
#define EXC_USYS     (8)
#define EXC_SSYS     (9)
#define EXC___10     (10)
#define EXC_MSYS     (11)
#define EXC_IPFT     (12)
#define EXC_LPFT     (13)
#define EXC___14     (14)
#define EXC_SPFT     (15)
#define EXC_UNKN     (19)
#define EXC_BITS_ALL (0xffff)

#define MEDELEG_ALL (0xB1F7UL)


#define CLINT_BASE              (0x2000000)
#define CLINT_MTIMECMP_OFFSET   (0x4000)
#define CLINT_MTIMECMP_INFINITE (0x7FFFFFFFFFFFFFFFUL)


#define PLIC_BASE            0x0c000000UL
#define PLIC_PRIORITY_BASE   0x4
#define PLIC_PENDING_BASE    0x1000
#define PLIC_ENABLE_BASE     0x2000
#define PLIC_ENABLE_STRIDE   0x80
#define PLIC_CONTEXT_BASE    0x200000
#define PLIC_CONTEXT_STRIDE  0x1000

#define PLIC_MODE_MACHINE    0x0
#define PLIC_MODE_SUPERVISOR 0x1

#define PLIC_INT_UART     (10)
#define PLIC_INT_RT_CLOCK (11)
#define PLIC_INT_PCIEA    (32)
#define PLIC_INT_PCIEB    (33)
#define PLIC_INT_PCIEC    (34)
#define PLIC_INT_PCIED    (35)

#define PLIC_PRIORITY(interrupt) \
    (PLIC_BASE + PLIC_PRIORITY_BASE * interrupt)

#define PLIC_THRESHOLD(hartid, mode) \
    (PLIC_BASE + PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * (2 * hartid + mode))

#define PLIC_CLAIM(hartid, mode) \
    (PLIC_THRESHOLD(hartid, mode) + 4)

#define PLIC_ENABLE(hartid, mode) \
    (PLIC_BASE + PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * (2 * hartid + mode))



#define PMPCFG_RD    (1 << 0)
#define PMPCFG_WR    (1 << 1)
#define PMPCFG_EX    (1 << 2)
#define PMPCFG_AM(x) ((x) << 3)

#define PMPAM_OFF   (0)
#define PMPAM_TOR   (1)
#define PMPAM_NA4   (2)
#define PMPAM_NAPOT (3)

#define PAGE_SIZE (4096)

#define ECAM_BASE (0x30000000ULL)
#define BAR_BASE  (0x40000000ULL)

#endif
