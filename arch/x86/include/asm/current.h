#ifndef _ASM_X86_CURRENT_H
#define _ASM_X86_CURRENT_H

#include <linux/compiler.h>
#include <asm/percpu.h>
#include <asm/thread_info.h>

struct task_struct;

DECLARE_PER_CPU(struct task_struct *, current_task);

static __always_inline struct task_struct *get_current(void)
{
    return percpu_read_stable(current_task);
}

#define current get_current()

static inline struct thread_info *current_thread_info(void)
{
    return (struct thread_info *)current;
}

#endif /* _ASM_X86_CURRENT_H */
