#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <syslog.h>

#define PAGE_SIZE 4096
#define IDT_SIZE 256
#define MAX_PROCESSORS 4

#define INTERRUPT_TYPE_TIMER 0x20
#define INTERRUPT_TYPE_KEYBOARD 0x21
#define INTERRUPT_TYPE_DISK 0x22
#define INTERRUPT_TYPE_NETWORK 0x23

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

struct cpu_state {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eflags;
};

struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

struct idt_entry idt[IDT_SIZE];
struct idt_pointer idtp;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile bool keep_running = true;

void load_idt(void *idt_ptr) {
    __asm__("lidt [%0]" ::"r"(idt_ptr));
}

void set_idt_entry(int num, uintptr_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
}

void setup_idt() {
    idtp.limit = sizeof(struct idt_entry) * IDT_SIZE - 1;
    idtp.base = (uintptr_t)&idt;

    memset(&idt, 0, sizeof(struct idt_entry) * IDT_SIZE);
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, (uintptr_t)default_interrupt_handler, 0x08, 0x8E);
    }

    load_idt(&idtp);
}

void enable_interrupts() {
    __asm__("sti");
}

void disable_interrupts() {
    __asm__("cli");
}

void default_interrupt_handler(struct interrupt_frame* frame, struct cpu_state* cpu) {
    int interrupt_number = frame->ip & 0xFF; // Simulated interrupt number extraction
    syslog(LOG_INFO, "Interrupt 0x%X handled", interrupt_number);
    syslog(LOG_INFO, "CPU State: EAX=0x%X, EBX=0x%X, ECX=0x%X, EDX=0x%X", cpu->eax, cpu->ebx, cpu->ecx, cpu->edx);
    syslog(LOG_INFO, "Frame: IP=0x%X, CS=0x%X, FLAGS=0x%X, SP=0x%X, SS=0x%X", frame->ip, frame->cs, frame->flags, frame->sp, frame->ss);

    switch (interrupt_number) {
        case INTERRUPT_TYPE_TIMER:
            syslog(LOG_INFO, "Timer interrupt occurred.");
            break;
        case INTERRUPT_TYPE_KEYBOARD:
            syslog(LOG_INFO, "Keyboard interrupt occurred.");
            break;
        case INTERRUPT_TYPE_DISK:
            syslog(LOG_INFO, "Disk interrupt occurred.");
            break;
        case INTERRUPT_TYPE_NETWORK:
            syslog(LOG_INFO, "Network interrupt occurred.");
            break;
        default:
            syslog(LOG_WARNING, "Unhandled interrupt 0x%X occurred.", interrupt_number);
            break;
    }
}

void signal_handler(int sig) {
    pthread_mutex_lock(&lock);
    keep_running = false;
    pthread_mutex_unlock(&lock);
    syslog(LOG_INFO, "Signal received, shutting down.");
}

void* processor_main(void* arg) {
    setup_idt();
    enable_interrupts();

    while (keep_running) {
        pause();
    }

    disable_interrupts();
    syslog(LOG_INFO, "Interrupts disabled, processor shutting down safely.");
    return NULL;
}

int main(int argc, char* argv[]) {
    openlog("system_interrupts", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    pthread_t threads[MAX_PROCESSORS];
    signal(SIGINT, signal_handler);  // Set up signal handler for graceful shutdown

    // Create threads to simulate handling on different processors
    for (int i = 0; i < MAX_PROCESSORS; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            syslog(LOG_ERR, "Failed to allocate memory for thread argument.");
            continue;
        }
        *arg = i;
        if (pthread_create(&threads[i], NULL, processor_main, arg) != 0) {
            syslog(LOG_ERR, "Failed to create thread for processor %d", i);
            free(arg); // Cleanup if thread creation fails
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < MAX_PROCESSORS; i++) {
        pthread_join(threads[i], NULL);
    }

    closelog();  // Close the logging system

    return 0;
}

__asm__(
"load_idt:\n"
"   lidt [%0]\n"
"   ret\n"
);
