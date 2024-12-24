#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include <x86intrin.h>

#define IDT_SIZE 256
#define MAX_PROCESSORS 4

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist : 3;
    uint8_t reserved : 5;
    uint8_t type : 4;
    uint8_t zero : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved2;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_pointer_t;

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rflags;
} cpu_state_t;

typedef struct {
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

static _Alignas(16) idt_entry_t idt[IDT_SIZE];
static idt_pointer_t idtp;
static _Atomic bool keep_running = true;

static void load_idt(const idt_pointer_t *idt_ptr) {
    __asm__ volatile("lidt %0" : : "m" (*idt_ptr) : "memory");
}

static void set_idt_entry(int num, uintptr_t handler, uint16_t sel, uint8_t flags) {
    idt[num] = (idt_entry_t) {
        .offset_low = handler & 0xFFFF,
        .selector = sel,
        .ist = 0,
        .reserved = 0,
        .type = flags & 0xF,
        .zero = 0,
        .dpl = (flags >> 5) & 0x3,
        .present = 1,
        .offset_middle = (handler >> 16) & 0xFFFF,
        .offset_high = handler >> 32,
        .reserved2 = 0
    };
}

static void setup_idt(void) {
    idtp = (idt_pointer_t){
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)idt
    };

    memset(idt, 0, sizeof(idt));
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, (uintptr_t)default_interrupt_handler, 0x08, 0x8E);
    }

    load_idt(&idtp);
}

static inline void enable_interrupts(void) {
    _mm_lfence();
    __asm__ volatile("sti" ::: "memory");
}

static inline void disable_interrupts(void) {
    __asm__ volatile("cli" ::: "memory");
    _mm_mfence();
}

static void default_interrupt_handler(interrupt_frame_t *frame, cpu_state_t *cpu) {
    uint8_t interrupt_number = frame->rip & 0xFF;
    syslog(LOG_INFO, "Interrupt 0x%X handled on CPU %d", interrupt_number, smp_processor_id());
    // Log CPU state and frame info if needed
}

static void signal_handler(int sig) {
    (void)sig;
    atomic_store_explicit(&keep_running, false, memory_order_release);
    syslog(LOG_INFO, "Signal received, initiating shutdown.");
}

static void* processor_main(void* arg) {
    int cpu_id = *(int*)arg;
    free(arg);

    syslog(LOG_INFO, "Processor %d starting up", cpu_id);
    setup_idt();
    enable_interrupts();

    while (atomic_load_explicit(&keep_running, memory_order_acquire)) {
        _mm_pause();
    }

    disable_interrupts();
    syslog(LOG_INFO, "Processor %d shutting down safely", cpu_id);
    return NULL;
}

int main(void) {
    openlog("system_interrupts", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    pthread_t threads[MAX_PROCESSORS];
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    for (int i = 0; i < MAX_PROCESSORS; i++) {
        int *cpu_id = malloc(sizeof(int));
        if (!cpu_id) {
            syslog(LOG_ERR, "Failed to allocate memory for CPU ID");
            continue;
        }
        *cpu_id = i;
        if (pthread_create(&threads[i], NULL, processor_main, cpu_id) != 0) {
            syslog(LOG_ERR, "Failed to create thread for processor %d", i);
            free(cpu_id);
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    for (int i = 0; i < MAX_PROCESSORS; i++) {
        pthread_join(threads[i], NULL);
    }

    closelog();
    return 0;
}
