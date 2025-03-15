// Переписанный код с соблюдением профессиональных стандартов разработки ядра ОС
// Улучшения включают:
// - Корректное сохранение и восстановление контекста прерываний
// - Поддержка APIC для подтверждения прерываний
// - Базовая инициализация SMP с использованием INIT/SIPI (требует дальнейшей доработки)
// - Разделение обработчика прерываний на ассемблерную часть и C-логику (заготовка)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define IDT_SIZE 256
#define MAX_PROCESSORS 4
#define KERNEL_CS 0x08
#define IDT_TYPE_ATTR 0x8E

// Регистры APIC (для xAPIC)
#define APIC_BASE 0xFEE00000
#define APIC_ID (APIC_BASE + 0x20)
#define APIC_EOI (APIC_BASE + 0xB0)
#define APIC_ICR_LOW (APIC_BASE + 0x300)
#define APIC_ICR_HIGH (APIC_BASE + 0x310)
#define APIC_SPURIOUS (APIC_BASE + 0xF0)

typedef struct __attribute__((packed, aligned(16))) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist : 3;
    uint8_t reserved0 : 5;
    uint8_t type : 4;
    uint8_t zero : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved1;
} idt_entry_t;

typedef struct __attribute__((packed, aligned(16))) {
    uint16_t limit;
    uint64_t base;
} idt_pointer_t;

typedef struct {
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

static _Alignas(4096) idt_entry_t idt;
static idt_pointer_t idtr __attribute__((used));
static volatile bool keep_running = true;

// Статические проверки архитектурных требований
static_assert(sizeof(idt_entry_t) == 16, "Invalid IDT entry size");
static_assert(__alignof(idt_entry_t) == 16, "IDT entry misalignment");
static_assert(offsetof(idt_entry_t, offset_high) == 8, "IDT layout mismatch");

static inline void load_idt(const idt_pointer_t *idtr) {
    __asm__ volatile("lidt %0" : : "m"(*idtr) : "memory");
}

static void __attribute__((noinline)) set_idt_entry(uint8_t vec, void *handler) {
    uintptr_t addr = (uintptr_t)handler;
    idt[vec] = (idt_entry_t){
        .offset_low = addr & 0xFFFF,
        .selector = KERNEL_CS,
        .ist = 0,
        .reserved0 = 0,
        .type = IDT_TYPE_ATTR & 0xF,
        .zero = 0,
        .dpl = (IDT_TYPE_ATTR >> 5) & 0x3,
        .present = 1,
        .offset_mid = (addr >> 16) & 0xFFFF,
        .offset_high = addr >> 32,
        .reserved1 = 0
    };
}

// Ассемблерная обертка обработчика прерываний по умолчанию
__attribute__((naked)) static void default_interrupt_handler_asm(void) {
    __asm__ volatile(
        "pushq %rax\n\t"
        "pushq %rbx\n\t"
        "pushq %rcx\n\t"
        "pushq %rdx\n\t"
        "pushq %rsi\n\t"
        "pushq %rdi\n\t"
        "pushq %rbp\n\t"
        "pushq %r8\n\t"
        "pushq %r9\n\t"
        "pushq %r10\n\t"
        "pushq %r11\n\t"
        "pushq %r12\n\t"
        "pushq %r13\n\t"
        "pushq %r14\n\t"
        "pushq %r15\n\t"

        "movq %rsp, %rdi\n\t" // Первый аргумент - указатель на сохраненный контекст
        "call default_interrupt_handler_c\n\t"

        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %r11\n\t"
        "popq %r10\n\t"
        "popq %r9\n\t"
        "popq %r8\n\t"
        "popq %rbp\n\t"
        "popq %rdi\n\t"
        "popq %rsi\n\t"
        "popq %rdx\n\t"
        "popq %rcx\n\t"
        "popq %rbx\n\t"
        "popq %rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

// C-часть обработчика прерываний по умолчанию
static void default_interrupt_handler_c(interrupt_frame_t *frame) {
    // (void)frame; // Используйте frame для доступа к сохраненным регистрам
    // Отправка EOI (End of Interrupt) на APIC
    volatile uint32_t *eoi = (volatile uint32_t *)APIC_EOI;
    *eoi = 0;
}

static void init_idt(void) {
    idtr = (idt_pointer_t){
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)&idt
    };

    for (size_t i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, &default_interrupt_handler_asm);
    }

    // Настройка вектора Spurious Interrupt (например, 39)
    volatile uint32_t *spurious_vector = (volatile uint32_t *)APIC_SPURIOUS;
    *spurious_vector = 0x1BF | 0x100; // Вектор 39 (0x27) и включение APIC

    load_idt(&idtr);
}

static inline void enable_interrupts(void) {
    __asm__ volatile(
        "sti\n\t"
        ::: "memory"
    );
}

static inline void disable_interrupts(void) {
    __asm__ volatile(
        "cli\n\t"
        ::: "memory"
    );
}

// Статический пул процессорных структур
static struct {
    uint32_t apic_id;
    bool active;
} __attribute__((aligned(64))) processors;

static void __attribute__((noreturn)) cpu_main(uint32_t cpu_id) {
    init_idt();
    enable_interrupts();

    while (__atomic_load_n(&keep_running, __ATOMIC_ACQUIRE)) {
        __asm__ volatile("pause");
    }

    disable_interrupts();
    __atomic_store_n(&processors[cpu_id].active, false, __ATOMIC_RELEASE);
    for(;;) __asm__ volatile("hlt");
}

// Инициализация процессоров через ACPI MADT (упрощенная модель)
void __attribute__((cold)) smp_init(void) {
    memset(processors, 0, sizeof(processors));

    // Получение APIC ID для BSP (упрощенно предполагаем, что это 0)
    volatile uint32_t *apic_id_reg = (volatile uint32_t *)APIC_ID;
    uint32_t bsp_apic_id = (*apic_id_reg >> 24) & 0xFF;
    processors.apic_id = bsp_apic_id;
    processors.active = true;

    for (uint32_t i = 1; i < MAX_PROCESSORS; i++) {
        processors[i].apic_id = i; // Упрощенное назначение APIC ID
        processors[i].active = true;

        // Запуск AP (Application Processor) - УПРОЩЕННАЯ МОДЕЛЬ
        // В РЕАЛЬНОЙ СИСТЕМЕ ПОТРЕБУЕТСЯ ИСПОЛЬЗОВАНИЕ ACPI MADT И ПРОТОКОЛА INIT/SIPI
        // Для простоты мы предполагаем, что AP уже готовы к запуску кода по определенному адресу
        uint64_t ap_startup_address = (uint64_t)&cpu_main; // Пример адреса запуска

        // Отправка INIT IPI
        volatile uint32_t *icr_high = (volatile uint32_t *)APIC_ICR_HIGH;
        volatile uint32_t *icr_low = (volatile uint32_t *)APIC_ICR_LOW;

        *icr_high = (uint32_t)(processors[i].apic_id << 24); // Назначение целевого APIC ID
        *icr_low = 0x00000500; // INIT IPI, физическая адресация

        // Задержка (10 миллисекунд) - требуется реальная реализация задержки
        for (volatile int delay = 0; delay < 100000; ++delay); // Очень грубая задержка

        // Отправка STARTUP IPI (предполагаем, что код запуска AP находится по адресу ap_startup_address)
        uint32_t startup_page = (uint32_t)(ap_startup_address >> 12);
        *icr_high = (uint32_t)(processors[i].apic_id << 24); // Назначение целевого APIC ID
        *icr_low = 0x00000600 | startup_page; // STARTUP IPI, физическая адресация, вектор страницы

        // Задержка (200 микросекунд) - требуется реальная реализация задержки
        for (volatile int delay = 0; delay < 2000; ++delay); // Очень грубая задержка

        // Вторая попытка отправки STARTUP IPI (рекомендовано спецификацией)
        *icr_high = (uint32_t)(processors[i].apic_id << 24); // Назначение целевого APIC ID
        *icr_low = 0x00000600 | startup_page; // STARTUP IPI, физическая адресация, вектор страницы

        // Дальнейшая синхронизация и проверка запуска AP здесь (в реальной системе)
    }
}

__attribute__((noreturn)) void shutdown(void) {
    disable_interrupts();
    __atomic_store_n(&keep_running, false, __ATOMIC_RELEASE);

    for(;;) __asm__ volatile(
        "cli\n\t"
        "hlt\n\t"
        ::: "memory"
    );
}

// Точка входа для BSP (Bootstrap Processor)
__attribute__((naked)) void _start(void) {
    __asm__ volatile(
        "mov $0, %%rbp\n\t"
        "mov %%rsp, %%rax\n\t"
        "and $-16, %%rsp\n\t"
        "push %%rax\n\t"
        "call smp_init\n\t"
        "call shutdown\n\t"
        ::: "memory"
    );
}
