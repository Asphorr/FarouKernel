#ifndef _ASM_X86_THREAD_INFO_H
#define _ASM_X86_THREAD_INFO_H

#include <linux/compiler.h>
#include <asm/percpu.h>

struct task_struct;

struct thread_info {
    struct task_struct *task;
    struct restart_block restart_block;
    unsigned long flags;
    int cpu;
    int preempt_count;
    mm_segment_t addr_limit;
    struct exec_domain *exec_domain;
    __u32 flags;
    unsigned long debugreg[8];
};

DECLARE_PER_CPU(struct thread_info *, current_thread_info);

#endif /* _ASM_X86_THREAD_INFO_H */
