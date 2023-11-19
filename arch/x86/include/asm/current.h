#ifndef _ASM_X86_CURRENT_H
#define _ASM_X86_CURRENT_H

#include <linux/compiler.h>
#include <asm/percpu.h>
#include <asm/thread_info.h>

struct task_struct;

DECLARE_PER_CPU(struct task_struct *, current_task);

/*
 * get_current - Get the current task.
 *
 * This function returns the current task. It disables preemption to ensure that the
 * current task doesn't get preempted while it's being accessed.
 */
static __always_inline struct task_struct *get_current(void)
{
    struct task_struct *task;

    preempt_disable();
    task = percpu_read_begin();
    task = percpu_read(current_task);
    percpu_read_end();
    preempt_enable();

    return task;
}

#define current get_current()

/*
 * current_thread_info - Get the current thread info.
 *
 * This function returns the current thread info. It simply casts the current task to
 * a thread info struct.
 */
static inline struct thread_info *current_thread_info(void)
{
    return (struct thread_info *)current;
}

#endif /* _ASM_X86_CURRENT_H */
