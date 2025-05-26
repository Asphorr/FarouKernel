/* idt.c  –  x86‑64 Interrupt Descriptor Table
 *
 * Assemble with -mcmodel=kernel -mno-red-zone (kernel) or suitable flags.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>     /* for static_assert */

/* ────────────────────────────────────────────
 *  Constants / macros
 * ────────────────────────────────────────────*/
#define IDT_ENTRIES 256
#define KERNEL_CS   0x08u   /* GDT selector: kernel code segment */

/* IDT attribute flags (Intel SDM Vol. 3A 6.14.1) */
enum {
    IDT_FLAG_PRESENT   = 0x80,
    IDT_FLAG_INT_GATE  = 0x0E,
    IDT_FLAG_TRAP_GATE = 0x0F,
    IDT_FLAG_DPL0      = 0x00,
    IDT_FLAG_DPL3      = 0x60
};

#define INT_GATE_KERNEL (IDT_FLAG_PRESENT | IDT_FLAG_INT_GATE | IDT_FLAG_DPL0)

/* ────────────────────────────────────────────
 *  16‑byte IDT gate entry (Intel layout)
 * ────────────────────────────────────────────*/
typedef struct __attribute__((packed, aligned(16))) {
    uint16_t offset_low;    /* bits 0‑15  */
    uint16_t selector;      /* code‑segment selector */
    uint8_t  ist;           /* bits 0‑2 = IST, 3‑7 = zero */
    uint8_t  type_attr;     /* gate type, DPL, P */
    uint16_t offset_mid;    /* bits 16‑31 */
    uint32_t offset_high;   /* bits 32‑63 */
    uint32_t zero;          /* reserved */
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

/* ────────────────────────────────────────────
 *  Forward declarations: ISR stubs in assembly
 *  We generate them via a macro to avoid typos.
 * ────────────────────────────────────────────*/
#define DECLARE_ISR(n) extern void isr##n(void);
FOR_EACH_VECTOR(DECLARE_ISR)     /* see macro below */

/* Helper: apply macro M to 0..255 */
#define FOR_EACH_VECTOR(M) \
    M(0)   M(1)   M(2)   M(3)   M(4)   M(5)   M(6)   M(7)   \
    M(8)   M(9)   M(10)  M(11)  M(12)  M(13)  M(14)  M(15)  \
    M(16)  M(17)  M(18)  M(19)  M(20)  M(21)  M(22)  M(23)  \
    M(24)  M(25)  M(26)  M(27)  M(28)  M(29)  M(30)  M(31)  \
    M(32)  M(33)  M(34)  M(35)  M(36)  M(37)  M(38)  M(39)  \
    M(40)  M(41)  M(42)  M(43)  M(44)  M(45)  M(46)  M(47)  \
    M(48)  M(49)  M(50)  M(51)  M(52)  M(53)  M(54)  M(55)  \
    M(56)  M(57)  M(58)  M(59)  M(60)  M(61)  M(62)  M(63)  \
    M(64)  M(65)  M(66)  M(67)  M(68)  M(69)  M(70)  M(71)  \
    M(72)  M(73)  M(74)  M(75)  M(76)  M(77)  M(78)  M(79)  \
    M(80)  M(81)  M(82)  M(83)  M(84)  M(85)  M(86)  M(87)  \
    M(88)  M(89)  M(90)  M(91)  M(92)  M(93)  M(94)  M(95)  \
    M(96)  M(97)  M(98)  M(99)  M(100) M(101) M(102) M(103) \
    M(104) M(105) M(106) M(107) M(108) M(109) M(110) M(111) \
    M(112) M(113) M(114) M(115) M(116) M(117) M(118) M(119) \
    M(120) M(121) M(122) M(123) M(124) M(125) M(126) M(127) \
    M(128) M(129) M(130) M(131) M(132) M(133) M(134) M(135) \
    M(136) M(137) M(138) M(139) M(140) M(141) M(142) M(143) \
    M(144) M(145) M(146) M(147) M(148) M(149) M(150) M(151) \
    M(152) M(153) M(154) M(155) M(156) M(157) M(158) M(159) \
    M(160) M(161) M(162) M(163) M(164) M(165) M(166) M(167) \
    M(168) M(169) M(170) M(171) M(172) M(173) M(174) M(175) \
    M(176) M(177) M(178) M(179) M(180) M(181) M(182) M(183) \
    M(184) M(185) M(186) M(187) M(188) M(189) M(190) M(191) \
    M(192) M(193) M(194) M(195) M(196) M(197) M(198) M(199) \
    M(200) M(201) M(202) M(203) M(204) M(205) M(206) M(207) \
    M(208) M(209) M(210) M(211) M(212) M(213) M(214) M(215) \
    M(216) M(217) M(218) M(219) M(220) M(221) M(222) M(223) \
    M(224) M(225) M(226) M(227) M(228) M(229) M(230) M(231) \
    M(232) M(233) M(234) M(235) M(236) M(237) M(238) M(239) \
    M(240) M(241) M(242) M(243) M(244) M(245) M(246) M(247) \
    M(248) M(249) M(250) M(251) M(252) M(253) M(254) M(255)

/* ────────────────────────────────────────────
 *  Static storage
 * ────────────────────────────────────────────*/
static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(16)));
static idt_ptr_t   idt_ptr;

/* ────────────────────────────────────────────
 *  Helper: fill a gate
 * ────────────────────────────────────────────*/
static inline void idt_set_gate(int vec, uintptr_t handler,
                                uint16_t sel, uint8_t flags)
{
    idt_entry_t *d = &idt[vec];
    d->offset_low  = handler & 0xFFFF;
    d->selector    = sel;
    d->ist         = 0;                /* no IST for now */
    d->type_attr   = flags;
    d->offset_mid  = (handler >> 16) & 0xFFFF;
    d->offset_high = (handler >> 32) & 0xFFFFFFFF;
    d->zero        = 0;
}

/* ────────────────────────────────────────────
 *  Loader (LIDT)
 * ────────────────────────────────────────────*/
static inline void lidt(const idt_ptr_t *p)
{
    __asm__ __volatile__("lidt %0" : : "m"(*p));
}

/* ────────────────────────────────────────────
 *  Public API
 * ────────────────────────────────────────────*/
void idt_init(void)
{
    /* 1. Blank‑init the table */
    memset(idt, 0, sizeof(idt));

    /* 2. One line – all 256 vectors: */
#define INSTALL_GATE(n) \
    idt_set_gate(n, (uintptr_t)isr##n, KERNEL_CS, INT_GATE_KERNEL);
    FOR_EACH_VECTOR(INSTALL_GATE)
#undef INSTALL_GATE

    /* 3. Build pointer & load */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uintptr_t)idt;
    lidt(&idt_ptr);
}

/* ────────────────────────────────────────────
 *  Compile‑time guarantee: each isrN must exist
 * ────────────────────────────────────────────*/
#define PTR_OF_ISR(n) ((void*)&isr##n)
static const void * const __attribute__((unused)) isr_presence_check[] = {
    FOR_EACH_VECTOR(PTR_OF_ISR)
};
#undef PTR_OF_ISR
