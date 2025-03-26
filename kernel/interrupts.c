#include <stdint.h>
#include <stddef.h> // Для offsetof, size_t
#include <stdbool.h>
#include <string.h> // Для memset (хотя в ядре часто своя реализация)

// --- Конфигурация и Константы ---
#define IDT_SIZE 256         // Количество векторов в IDT
#define KERNEL_CS 0x08       // Селектор сегмента кода ядра (предполагается плоская модель)
#define MAX_PROCESSORS 8     // Максимальное поддерживаемое количество процессоров для нашего статического массива
#define SPURIOUS_VECTOR_NUM 0xFF // Вектор для ложных прерываний APIC (рекомендуется 0xFF или 39)

// --- Атрибуты и Выравнивание ---
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#define NORETURN __attribute__((noreturn))
#define NAKED __attribute__((naked))
#define NOINLINE __attribute__((noinline))
#define USED __attribute__((used))
#define COLD __attribute__((cold))
#define INLINE static inline __attribute__((always_inline))

// --- Регистры APIC (xAPIC MMIO) ---
#define APIC_DEFAULT_BASE 0xFEE00000 // Стандартный физический адрес LAPIC
// Офсеты регистров относительно базового адреса APIC
#define APIC_REG_ID 0x0020                 // APIC ID Register
#define APIC_REG_VERSION 0x0030            // APIC Version Register
#define APIC_REG_EOI 0x00B0                // End Of Interrupt Register (Write-Only)
#define APIC_REG_SPURIOUS 0x00F0           // Spurious Interrupt Vector Register
#define APIC_REG_ICR_LOW 0x0300            // Interrupt Command Register (Low)
#define APIC_REG_ICR_HIGH 0x0310           // Interrupt Command Register (High)
// Биты в APIC_REG_SPURIOUS
#define APIC_SPURIOUS_VECTOR_MASK 0x00FF   // Маска для номера вектора
#define APIC_SPURIOUS_APIC_ENABLE 0x0100   // Бит для включения APIC
#define APIC_SPURIOUS_FOCUS_DISABLE 0x0200 // Не передавать прерывания процессору с наименьшим приоритетом
// Биты и команды для APIC_REG_ICR_LOW
#define APIC_DELIVERY_MODE_FIXED 0x00000000
#define APIC_DELIVERY_MODE_INIT 0x00000500
#define APIC_DELIVERY_MODE_STARTUP 0x00000600
#define APIC_DESTINATION_PHYSICAL 0x00000000
#define APIC_DESTINATION_LOGICAL 0x00000800
#define APIC_DELIVERY_STATUS_IDLE 0x00000000
#define APIC_DELIVERY_STATUS_PENDING 0x00001000
#define APIC_LEVEL_DEASSERT 0x00000000
#define APIC_LEVEL_ASSERT 0x00004000
#define APIC_TRIGGER_MODE_EDGE 0x00000000
#define APIC_TRIGGER_MODE_LEVEL 0x00008000
#define APIC_DESTINATION_SELF 0x00040000
#define APIC_DESTINATION_ALL_INCL_SELF 0x00080000
#define APIC_DESTINATION_ALL_EXCL_SELF 0x000C0000

// --- Структуры ---

// Структура дескриптора прерывания (IDT Entry) для x86-64 Long Mode
typedef struct PACKED ALIGNED(16) {
    uint16_t offset_low;    // Младшие 16 бит адреса обработчика
    uint16_t selector;      // Селектор сегмента кода (обычно KERNEL_CS)
    uint8_t ist : 3;        // Interrupt Stack Table index (0 = none)
    uint8_t reserved0 : 5;  // Зарезервировано
    uint8_t type : 4;       // Тип дескриптора (e.g., 0xE = 64-bit Interrupt Gate)
    uint8_t zero : 1;       // Должен быть 0 для Interrupt/Trap Gates
    uint8_t dpl : 2;        // Descriptor Privilege Level (0 = kernel)
    uint8_t present : 1;    // Флаг присутствия (1 = действителен)
    uint16_t offset_mid;    // Средние 16 бит адреса обработчика
    uint32_t offset_high;   // Старшие 32 бита адреса обработчика
    uint32_t reserved1;     // Зарезервировано
} idt_entry_t;

// Структура для регистра IDTR (указатель на IDT)
typedef struct PACKED {
    uint16_t limit; // Размер IDT в байтах - 1
    uint64_t base;  // Базовый адрес IDT
} idt_pointer_t;

// Структура, сохраняемая на стеке ассемблерной оберткой
// Добавляем номер вектора и код ошибки
typedef struct PACKED {
    // Регистры, сохраняемые вручную
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Номер прерывания и код ошибки (если есть)
    uint64_t vector_number;
    uint64_t error_code; // Может быть фиктивным, если CPU не пушит код ошибки
    // Сохраняется аппаратно при прерывании
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

// Информация о процессоре
typedef struct ALIGNED(64) { // Выравнивание по линии кэша
    uint32_t acpi_processor_id; // ID из ACPI MADT
    uint32_t apic_id;           // ID из регистра LAPIC
    volatile bool active;       // Флаг, устанавливаемый процессором после инициализации
    volatile bool bsp;          // Это Bootstrap Processor?
    // Сюда можно добавить другие данные, специфичные для процессора
} processor_info_t;

// --- Глобальные Переменные ---
// Сама таблица IDT, выровненная по странице для удобства
static ALIGNED(4096) idt_entry_t idt[IDT_SIZE];
// Указатель на IDT для загрузки в IDTR
static idt_pointer_t idtr USED;
// Флаг для управления основным циклом процессоров
static volatile bool global_keep_running = true;
// Массив для хранения информации о найденных процессорах
static processor_info_t processors[MAX_PROCESSORS];
// Счетчик активных процессоров
static volatile uint32_t active_processor_count = 0;
// Базовый адрес MMIO для локального APIC текущего процессора (обычно одинаков)
static uintptr_t local_apic_base = APIC_DEFAULT_BASE;

// --- Статические проверки ---
static_assert(sizeof(idt_entry_t) == 16, "Invalid IDT entry size");
static_assert(offsetof(idt_entry_t, offset_high) == 8, "IDT layout mismatch (offset_high)");
static_assert(sizeof(interrupt_frame_t) == (25 * 8), "Invalid interrupt frame size");

// --- Утилиты APIC ---
INLINE uint32_t apic_read(uint32_t reg_offset) {
    return *((volatile uint32_t *)(local_apic_base + reg_offset));
}

INLINE void apic_write(uint32_t reg_offset, uint32_t value) {
    *((volatile uint32_t *)(local_apic_base + reg_offset)) = value;
}

// Ожидание завершения отправки IPI
INLINE void apic_wait_ipi_idle() {
    while (apic_read(APIC_REG_ICR_LOW) & APIC_DELIVERY_STATUS_PENDING) {
        __asm__ volatile("pause");
    }
}

// Отправка EOI (End of Interrupt)
INLINE void apic_send_eoi() {
    apic_write(APIC_REG_EOI, 0); // Значение не важно
}

// --- Функции задержки (ПРИМИТИВНЫЕ!) ---
// !!! ВНИМАНИЕ: Эта задержка КРАЙНЕ НЕТОЧНА и зависит от CPU.
// !!! В реальной системе используйте калиброванный таймер (PIT, HPET, TSC).
void platform_udelay(uint64_t microseconds) {
    // Очень грубый цикл задержки, настройте константу под вашу цель/частоту
    volatile uint64_t i;
    uint64_t loops_per_us = 100; // Примерное значение, требует калибровки!
    for (i = 0; i < microseconds * loops_per_us; ++i) {
        __asm__ volatile("pause"); // Уменьшает энергопотребление в цикле ожидания
    }
}

// --- Настройка IDT ---

// Прототип C-обработчика
void generic_interrupt_handler_c(interrupt_frame_t *frame);

// Макрос для генерации ассемблерных заглушек
// _isr_stub_no_errcode: для векторов без кода ошибки CPU
// _isr_stub_errcode: для векторов с кодом ошибки CPU
#define ISR_STUB(vector_num, has_error_code) \
    NAKED void isr_stub_##vector_num(void) { \
        __asm__ volatile ( \
            "pushq $0\n\t" /* Фиктивный код ошибки, если его нет */ \
            : : : "memory" \
        ); \
        if (!has_error_code) { \
            __asm__ volatile ("popq %%rax\n\t" : : : "rax"); /* Убираем фиктивный код*/ \
             __asm__ volatile ("pushq $0\n\t"); /* И кладем настоящий 0 */ \
        }\
        __asm__ volatile ( \
            "pushq %[vec_num]\n\t" /* Кладем номер вектора */ \
            /* Сохраняем все регистры общего назначения */ \
            "pushq %%rax\n\t" \
            "pushq %%rbx\n\t" \
            "pushq %%rcx\n\t" \
            "pushq %%rdx\n\t" \
            "pushq %%rsi\n\t" \
            "pushq %%rdi\n\t" \
            "pushq %%rbp\n\t" \
            "pushq %%r8\n\t"  \
            "pushq %%r9\n\t"  \
            "pushq %%r10\n\t" \
            "pushq %%r11\n\t" \
            "pushq %%r12\n\t" \
            "pushq %%r13\n\t" \
            "pushq %%r14\n\t" \
            "pushq %%r15\n\t" \
            \
            "movq %%rsp, %%rdi\n\t" /* Первый аргумент (frame*) в RDI */ \
            "call generic_interrupt_handler_c\n\t" /* Вызов C-обработчика */ \
            \
            /* Восстанавливаем регистры */ \
            "popq %%r15\n\t" \
            "popq %%r14\n\t" \
            "popq %%r13\n\t" \
            "popq %%r12\n\t" \
            "popq %%r11\n\t" \
            "popq %%r10\n\t" \
            "popq %%r9\n\t"  \
            "popq %%r8\n\t"  \
            "popq %%rbp\n\t" \
            "popq %%rdi\n\t" \
            "popq %%rsi\n\t" \
            "popq %%rdx\n\t" \
            "popq %%rcx\n\t" \
            "popq %%rbx\n\t" \
            "popq %%rax\n\t" \
            \
            "addq $16, %%rsp\n\t" /* Пропускаем номер вектора и код ошибки */ \
            "iretq\n\t" /* Возврат из прерывания */ \
            : \
            : [vec_num] "i"(vector_num) \
            : "memory" \
        ); \
    }

// Генерируем заглушки для векторов 0-31 (исключения CPU)
// Вектора с кодом ошибки: 8, 10, 11, 12, 13, 14, 17, 21(*), 29(*), 30(*)
ISR_STUB(0, false); ISR_STUB(1, false); ISR_STUB(2, false); ISR_STUB(3, false);
ISR_STUB(4, false); ISR_STUB(5, false); ISR_STUB(6, false); ISR_STUB(7, false);
ISR_STUB(8, true);  ISR_STUB(9, false); ISR_STUB(10, true); ISR_STUB(11, true);
ISR_STUB(12, true); ISR_STUB(13, true); ISR_STUB(14, true); ISR_STUB(15, false);
ISR_STUB(16, false); ISR_STUB(17, true); ISR_STUB(18, false); ISR_STUB(19, false);
ISR_STUB(20, false); ISR_STUB(21, true); ISR_STUB(22, false); ISR_STUB(23, false);
ISR_STUB(24, false); ISR_STUB(25, false); ISR_STUB(26, false); ISR_STUB(27, false);
ISR_STUB(28, false); ISR_STUB(29, true); ISR_STUB(30, true); ISR_STUB(31, false);
// Добавим заглушку и для ложного вектора
ISR_STUB(SPURIOUS_VECTOR_NUM, false);

// Добавим прототипы для линковщика
#define ISR_STUB_PROTO(vector_num) extern void isr_stub_##vector_num(void)
ISR_STUB_PROTO(0); ISR_STUB_PROTO(1); ISR_STUB_PROTO(2); ISR_STUB_PROTO(3);
ISR_STUB_PROTO(4); ISR_STUB_PROTO(5); ISR_STUB_PROTO(6); ISR_STUB_PROTO(7);
ISR_STUB_PROTO(8); ISR_STUB_PROTO(9); ISR_STUB_PROTO(10); ISR_STUB_PROTO(11);
ISR_STUB_PROTO(12); ISR_STUB_PROTO(13); ISR_STUB_PROTO(14); ISR_STUB_PROTO(15);
ISR_STUB_PROTO(16); ISR_STUB_PROTO(17); ISR_STUB_PROTO(18); ISR_STUB_PROTO(19);
ISR_STUB_PROTO(20); ISR_STUB_PROTO(21); ISR_STUB_PROTO(22); ISR_STUB_PROTO(23);
ISR_STUB_PROTO(24); ISR_STUB_PROTO(25); ISR_STUB_PROTO(26); ISR_STUB_PROTO(27);
ISR_STUB_PROTO(28); ISR_STUB_PROTO(29); ISR_STUB_PROTO(30); ISR_STUB_PROTO(31);
ISR_STUB_PROTO(SPURIOUS_VECTOR_NUM);

// Указатели на функции-заглушки
void* isr_stubs[] = {
    [0] = &isr_stub_0,   [1] = &isr_stub_1,   [2] = &isr_stub_2,   [3] = &isr_stub_3,
    [4] = &isr_stub_4,   [5] = &isr_stub_5,   [6] = &isr_stub_6,   [7] = &isr_stub_7,
    [8] = &isr_stub_8,   [9] = &isr_stub_9,   [10] = &isr_stub_10, [11] = &isr_stub_11,
    [12] = &isr_stub_12, [13] = &isr_stub_13, [14] = &isr_stub_14, [15] = &isr_stub_15,
    [16] = &isr_stub_16, [17] = &isr_stub_17, [18] = &isr_stub_18, [19] = &isr_stub_19,
    [20] = &isr_stub_20, [21] = &isr_stub_21, [22] = &isr_stub_22, [23] = &isr_stub_23,
    [24] = &isr_stub_24, [25] = &isr_stub_25, [26] = &isr_stub_26, [27] = &isr_stub_27,
    [28] = &isr_stub_28, [29] = &isr_stub_29, [30] = &isr_stub_30, [31] = &isr_stub_31,
    // Устанавливаем заглушку и для ложного вектора
    [SPURIOUS_VECTOR_NUM] = &isr_stub_SPURIOUS_VECTOR_NUM
    // Остальные вектора пока не настроены (будут NULL)
};

// Функция установки одной записи в IDT
static void set_idt_entry(uint8_t vec, void *handler, uint8_t dpl) {
    if (handler == NULL) {
        // Если обработчик NULL, делаем запись недействительной (Present = 0)
         idt[vec] = (idt_entry_t){ .present = 0 };
         return;
    }

    uintptr_t addr = (uintptr_t)handler;
    idt[vec] = (idt_entry_t){
        .offset_low = (uint16_t)(addr & 0xFFFF),
        .selector = KERNEL_CS,
        .ist = 0, // IST не используется пока
        .reserved0 = 0,
        .type = 0xE, // 0b1110 - 64-bit Interrupt Gate
        .zero = 0,
        .dpl = dpl, // Уровень привилегий (0 для ядра)
        .present = 1, // Запись действительна
        .offset_mid = (uint16_t)((addr >> 16) & 0xFFFF),
        .offset_high = (uint32_t)(addr >> 32),
        .reserved1 = 0
    };
}

// Инициализация IDT
static void init_idt(void) {
    // Настройка указателя IDTR
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;

    // Заполнение IDT заглушками (с DPL=0)
    for (size_t i = 0; i < IDT_SIZE; i++) {
        if (i < sizeof(isr_stubs)/sizeof(isr_stubs[0]) && isr_stubs[i] != NULL) {
            set_idt_entry(i, isr_stubs[i], 0);
        } else {
             // Для неустановленных векторов (особенно > 31) можно либо:
             // 1. Поставить NULL обработчик (Present=0) - безопасно, но тихо.
             // 2. Поставить общую заглушку типа isr_stub_unhandled.
             // Выберем вариант 1
             set_idt_entry(i, NULL, 0);
        }
    }

    // Настройка вектора Spurious Interrupt
    // Включаем APIC (бит 8) и устанавливаем вектор (биты 0-7)
    apic_write(APIC_REG_SPURIOUS, SPURIOUS_VECTOR_NUM | APIC_SPURIOUS_APIC_ENABLE);

    // Загрузка IDT
    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

// --- Обработчик прерываний (C-часть) ---
// Временная примитивная функция вывода для ядра (требует реализации!)
void kprintf(const char *fmt, ...); // Объявление

// Общий C-обработчик прерываний
void generic_interrupt_handler_c(interrupt_frame_t *frame) {
    uint64_t vec = frame->vector_number;

    // Базовая диагностика для исключений CPU
    if (vec < 32) {
        kprintf("!!! CPU EXCEPTION %lld (ERROR CODE: 0x%llx) !!!\n", vec, frame->error_code);
        kprintf("  RIP=0x%016llx CS=0x%llx RFLAGS=0x%016llx\n", frame->rip, frame->cs, frame->rflags);
        kprintf("  RAX=0x%016llx RBX=0x%016llx RCX=0x%016llx RDX=0x%016llx\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
        kprintf("  RSI=0x%016llx RDI=0x%016llx RBP=0x%016llx RSP=0x%016llx\n", frame->rsi, frame->rdi, frame->rbp, frame->rsp);
        kprintf("  R8 =0x%016llx R9 =0x%016llx R10=0x%016llx R11=0x%016llx\n", frame->r8, frame->r9, frame->r10, frame->r11);
        kprintf("  R12=0x%016llx R13=0x%016llx R14=0x%016llx R15=0x%016llx\n", frame->r12, frame->r13, frame->r14, frame->r15);

        // Для фатальных исключений останавливаем систему
        if (vec != 8 && vec != 14) { // Пример: Page Fault (14) или Double Fault (8) могут быть исправимы
             kprintf("Unhandled CPU exception. Halting.\n");
             global_keep_running = false; // Сигнал всем CPU остановиться
             for (;;) __asm__ volatile("cli; hlt");
        }
    } else if (vec == SPURIOUS_VECTOR_NUM) {
        // Ложное прерывание - Игнорируем и НЕ отправляем EOI!
        kprintf("Spurious interrupt (vector 0x%llx) received.\n", vec);
        return; // Не отправляем EOI для ложных прерываний!
    }
    else {
         // Другие прерывания (от устройств и т.д.)
         kprintf("IRQ received: vector %lld\n", vec);
         // Здесь должна быть диспетчеризация к соответствующему драйверу/обработчику
    }

    // Отправка EOI (End of Interrupt) для всех прерываний, КРОМЕ ложных
    apic_send_eoi();
}

// --- Управление прерываниями ---
INLINE void enable_interrupts(void) {
    __asm__ volatile("sti" ::: "memory");
}

INLINE void disable_interrupts(void) {
    __asm__ volatile("cli" ::: "memory");
}

// --- Инициализация SMP (с Симуляцией MADT) ---

// Заголовок ACPI таблицы (упрощенный)
typedef struct PACKED {
    char signature[4];
    uint32_t length;
    // ... другие поля ACPI ...
} acpi_sdt_header_t;

// Запись о локальном APIC в MADT
typedef struct PACKED {
    uint8_t type; // Тип записи (0 = Local APIC)
    uint8_t length; // Длина записи
    uint8_t acpi_processor_id; // ID процессора от ACPI
    uint8_t apic_id;           // ID локального APIC
    uint32_t flags;            // Флаги (1 = Processor Enabled)
} madt_lapic_entry_t;

// --- Симуляция ACPI MADT ---
// Создадим "фальшивую" таблицу MADT для демонстрации
#define FAKE_MADT_LAPIC_ENTRY(proc_id, apic_id_val, enabled) \
    { .type = 0, .length = sizeof(madt_lapic_entry_t), .acpi_processor_id = proc_id, .apic_id = apic_id_val, .flags = enabled }

ALIGNED(16) static uint8_t fake_madt_table[] = {
    // Заголовок (упрощенный)
    'A', 'P', 'I', 'C', // Signature "APIC" (MADT)
    sizeof(fake_madt_table), 0, 0, 0, // Length
    1, // Revision
    0, // Checksum (игнорируем)
    'F', 'A', 'K', 'E', ' ', ' ', // OEMID
    'F', 'A', 'K', 'E', 'T', 'A', 'B', 'L', // OEM Table ID
    1, 0, 0, 0, // OEM Revision
    'F', 'A', 'K', 'E', // Creator ID
    1, 0, 0, 0, // Creator Revision
    // Данные MADT (Local APIC Address, Flags)
    0x00, 0x00, 0xE0, 0xFE, // Local APIC Address (0xFEE00000)
    1, 0, 0, 0, // Flags (PCAT_COMPAT=1)
    // Записи о процессорах (LAPIC)
    FAKE_MADT_LAPIC_ENTRY(0, 0, 1), // CPU 0, APIC ID 0, Enabled
    FAKE_MADT_LAPIC_ENTRY(1, 2, 1), // CPU 1, APIC ID 2, Enabled
    FAKE_MADT_LAPIC_ENTRY(2, 4, 1), // CPU 2, APIC ID 4, Enabled
    FAKE_MADT_LAPIC_ENTRY(3, 1, 0), // CPU 3, APIC ID 1, Disabled (не будет запускаться)
    FAKE_MADT_LAPIC_ENTRY(4, 5, 1), // CPU 4, APIC ID 5, Enabled
    // Можно добавить другие типы записей (I/O APIC, Interrupt Source Override, etc.)
};
// --- Конец Симуляции ACPI MADT ---

// Точка входа для Application Processors (APs)
// В реальной системе здесь будет 16-битный код ("трамплин")
// Мы симулируем, что AP сразу попадает сюда в 64-битном режиме.
NORETURN void ap_entry_point(void) {
    uint32_t my_apic_id = apic_read(APIC_REG_ID) >> 24;
    uint32_t my_processor_index = 0xFFFFFFFF;

    // Найдем индекс этого процессора в нашем массиве
    for(uint32_t i = 0; i < active_processor_count; ++i) {
        if (processors[i].apic_id == my_apic_id) {
            my_processor_index = i;
            break;
        }
    }

    if (my_processor_index == 0xFFFFFFFF) {
         kprintf("AP Error: Could not find processor entry for APIC ID %u\n", my_apic_id);
         for (;;) __asm__ volatile("cli; hlt");
    }

    // Основная функция процессора
    cpu_main(my_processor_index);
}


// Инициализация SMP
COLD void smp_init(void) {
    memset(processors, 0, sizeof(processors));
    active_processor_count = 0;

    // --- 1. Найти BSP (Bootstrap Processor) ---
    uint32_t bsp_apic_id = apic_read(APIC_REG_ID) >> 24;
    bool bsp_found_in_madt = false;

    // --- 2. "Парсинг" MADT (используем нашу фальшивую таблицу) ---
    acpi_sdt_header_t *header = (acpi_sdt_header_t *)fake_madt_table;
    uintptr_t madt_end = (uintptr_t)fake_madt_table + header->length;
    uintptr_t current_ptr = (uintptr_t)fake_madt_table + 44; // Пропускаем заголовок и первые поля

    kprintf("SMP: Parsing MADT (simulated)...\n");
    while (current_ptr < madt_end) {
        uint8_t entry_type = *(uint8_t *)current_ptr;
        uint8_t entry_length = *(uint8_t *)(current_ptr + 1);

        if (entry_length == 0) {
             kprintf("SMP Error: Zero length MADT entry found.\n");
             break; // Предотвращение бесконечного цикла
        }

        if (entry_type == 0) { // Тип 0: Local APIC
            if (active_processor_count >= MAX_PROCESSORS) {
                kprintf("SMP Warning: Found more processors than MAX_PROCESSORS limit.\n");
                current_ptr += entry_length;
                continue;
            }
            madt_lapic_entry_t *lapic = (madt_lapic_entry_t *)current_ptr;

            if (lapic->flags & 1) { // Проверяем флаг Enabled
                processor_info_t *proc = &processors[active_processor_count];
                proc->acpi_processor_id = lapic->acpi_processor_id;
                proc->apic_id = lapic->apic_id;
                proc->active = false; // Пока не активен
                proc->bsp = (lapic->apic_id == bsp_apic_id);

                 kprintf("  Found LAPIC: ACPI ID %u, APIC ID %u, %s\n",
                         proc->acpi_processor_id, proc->apic_id, proc->bsp ? "BSP" : "AP");

          
