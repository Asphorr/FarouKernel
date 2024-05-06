#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>

#define PAGE_SIZE 4096
#define IDT_SIZE 256
#define MAX_PROCESSORS 4

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

struct idt_entry idt[IDT_SIZE];
struct idt_pointer idtp;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile bool keep_running = true;

void load_idt(void *idt_ptr);
void load_gdt(void *gdt_ptr);

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
    // Setup default handler for all vectors
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, (uintptr_t)default_interrupt_handler, 0x08, 0x8E);
    }

    load_idt(&idtp);
}

void enable_interrupts() {
    asm volatile("sti");
}

void disable_interrupts() {
    asm volatile("cli");
}

void default_interrupt_handler(void) {
    syslog(LOG_INFO, "Default interrupt handler called.");
}

void custom_interrupt_handler(void) {
    syslog(LOG_INFO, "Custom interrupt handler executed.");
}

void signal_handler(int sig) {
    pthread_mutex_lock(&lock);
    keep_running = false;
    pthread_mutex_unlock(&lock);
    syslog(LOG_INFO, "Signal received, shutting down.");
}

void* processor_main(void* arg) {
    int processor_id = *(int*)arg;
    free(arg);

    setup_idt();
    enable_interrupts();

    while (keep_running) {
        pause();
    }

    disable_interrupts();
    syslog(LOG_INFO, "Processor %d: Interrupts disabled, shutting down safely.", processor_id);
    return NULL;
}

int main(int argc, char* argv[]) {
    openlog("system_program", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    pthread_t threads[MAX_PROCESSORS];
    int i;

    signal(SIGINT, signal_handler);

    for (i = 0; i < MAX_PROCESSORS; i++) {
        int* arg = malloc(sizeof(int));
        *arg = i;
        if (pthread_create(&threads[i], NULL, processor_main, arg) != 0) {
            syslog(LOG_ERR, "Failed to create thread for processor %d", i);
            free(arg);
        }
    }

    for (i = 0; i < MAX_PROCESSORS; i++) {
        pthread_join(threads[i], NULL);
    }

    closelog();
    return 0;
}

__asm__(
"load_idt:\n"
"   lidt [rdi]\n"
"   ret\n"

"load_gdt:\n"
"   lgdt [rdi]\n"
"   ret\n"
);
