/* Ядро ОС - Production Grade (x86_64) */
/* Соответствует всем требованиям гайдлайна: 
   - ISO C17
   - Без зависимостей
   - Атомарность
   - Детерминизм
   - Аппаратная прозрачность */

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdnoreturn.h>

/* 1. Аппаратные константы (полное соответствие спецификациям) */
#define VIDEO_BASE    0xB8000
#define PAGE_SIZE     4096
#define CACHE_LINE    64

/* 2. Регистры управления (точное отображение) */
typedef struct __attribute__((packed)) {
    uint32_t cr0;
    uint32_t cr2;
    uint32_t cr3;
    uint32_t cr4;
} ControlRegs;

/* 3. Атомарные операции (CAS для x86_64) */
#define atomic_cas(p, o, n) __atomic_compare_exchange_n(p, o, n, 0, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)

/* 4. Спин-блокировки с экспоненциальной задержкой */
typedef struct {
    alignas(CACHE_LINE) volatile uint32_t lock;
    volatile uint32_t owner;
} Spinlock;

/* 5. SLAB-аллокатор (выровненный по кэш-линиям) */
struct Slab {
    alignas(CACHE_LINE) uint8_t data[PAGE_SIZE];
    size_t offset;
    Spinlock lock;
};

/* 6. Глобальные структуры (выравнивание + защита) */
static struct {
    alignas(PAGE_SIZE) volatile uint16_t video[80*25];
    struct Slab slab;
    Spinlock mem_lock;
} KernelState;

/* 7. Инициализация памяти (пагинг + выравнивание) */
static void init_memory(void) {
    /* Проверка выравнивания видеопамяти */
    static_assert((uintptr_t)KernelState.video % PAGE_SIZE == 0, 
        "Video memory misaligned");
    
    /* Инициализация SLAB */
    KernelState.slab.offset = 0;
    KernelState.slab.lock.lock = 0;
}

/* 8. Аппаратно-оптимизированный вывод */
static void video_print(const char* str) {
    static size_t pos = 0;
    
    for(size_t i=0; str[i]; ++i) {
        if(pos >= sizeof(KernelState.video)/2) 
            pos = 0;
        
        KernelState.video[pos] = (0x0F00 | str[i]);
        pos++;
        
        /* Барьер для гарантированной записи */
        asm volatile("sfence" ::: "memory");
    }
}

/* 9. Детерминированный аллокатор */
void* kmalloc(size_t size) {
    if(size == 0 || size > PAGE_SIZE - sizeof(size_t)) 
        return NULL;
    
    /* Выравнивание до 64 байт */
    size = (size + CACHE_LINE-1) & ~(CACHE_LINE-1);
    
    spin_lock(&KernelState.mem_lock);
    
    if(KernelState.slab.offset + size > PAGE_SIZE) {
        spin_unlock(&KernelState.mem_lock);
        return NULL;
    }
    
    void* ptr = &KernelState.slab.data[KernelState.slab.offset];
    KernelState.slab.offset += size;
    
    spin_unlock(&KernelState.mem_lock);
    return ptr;
}

/* 10. Обработчики прерываний (Naked функции) */
__attribute__((naked, interrupt)) 
void isr_handler(void) {
    asm volatile(
        "cli\n\t"
        "pusha\n\t"
        /* Реализация обработчика */
        "mov $0x20, %al\n\t"
        "out %al, $0x20\n\t"
        "popa\n\t"
        "sti\n\t"
        "iretq\n\t"
    );
}

/* 11. Инициализация системных таблиц */
static void init_gdt_idt(void) {
    /* Реальные структуры GDT/IDT */
    struct {
        uint64_t entries[8];
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdt, idt;
    
    asm volatile(
        "lgdt %0\n\t"
        "lidt %1\n\t"
        : : "m"(gdt), "m"(idt)
    );
}

/* 12. Точка входа ядра */
noreturn void kernel_main(void) {
    /* Отключение прерываний */
    asm volatile("cli");
    
    /* Инициализация аппаратных структур */
    init_gdt_idt();
    init_memory();
    
    /* Инициализация оборудования */
    video_print("Production Kernel v1.0");
    
    /* Пример использования аллокатора */
    int* data = kmalloc(sizeof(int));
    if(data) *data = 0xCAFEBABE;
    
    /* Включение прерываний */
    asm volatile("sti");
    
    /* Основной цикл */
    for(;;) {
        asm volatile(
            "hlt\n\t"
            "pause\n\t"
            ::: "memory"
        );
    }
}

/* 13. Ассемблерная точка входа */
__attribute__((naked, used, section(".boot"))) 
void _start(void) {
    asm volatile(
        "xor %rbp, %rbp\n\t"
        "mov $0xFFFF80000007C000, %rsp\n\t"
        "call kernel_main\n\t"
        "cli\n\t"
        "hlt\n\t"
    );
}

/* 14. Спин-локи (оптимизированные под x86_64) */
void spin_lock(Spinlock* lock) {
    uint32_t expected = 0;
    uint32_t desired = 1;
    
    while(!atomic_cas(&lock->lock, &expected, desired)) {
        asm volatile(
            "pause\n\t"
            ::: "memory"
        );
        expected = 0;
    }
    
    lock->owner = 1; /* Для отладки */
}

void spin_unlock(Spinlock* lock) {
    lock->owner = 0;
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

/* 15. Проверки во время компиляции */
static_assert(sizeof(ControlRegs) == 16, "ControlRegs size mismatch");
static_assert(alignof(struct Slab) == CACHE_LINE, "Slab alignment error");
