#include <stdint.h>
#include <stddef.h> // For size_t, ssize_t
// #include <sys/types.h> // For mode_t, off_t (in a real kernel, these come from internal headers)
// #include <sys/stat.h>  // For struct stat  (in a real kernel, these come from internal headers)

//==================================================================
// ЗАГЛУШКИ / ПРЕДПОЛОЖЕНИЯ:
// Предполагается, что syscalls.h определяет:
// 1. Константы SYS_WRITE, SYS_READ, ..., SYS_MAX
// 2. Значение ENOSYS
// 3. Прототипы для sys_write, sys_read и т.д. (хотя они static, это не обязательно)
// 4. Типы mode_t, off_t, struct stat (если не подключать системные)
#include "syscalls.h" 

// Заглушка для kernel logging - РЕАЛИЗАЦИЯ ЗАВИСИТ ОТ ЯДРА!
// Убираем ассемблерную вставку для int 0x80, т.к. она несовместима
// с инструкцией 'syscall' для 64-bit и, скорее всего, неверна.
static void kernel_log_impl(const char* func, const char* msg) {
   // ЗДЕСЬ ДОЛЖНА БЫТЬ РЕАЛИЗАЦИЯ ЛОГИРОВАНИЯ ВАШЕГО ЯДРА
   // Например, вызов printk()
   (void)func; // Подавить unused warning
   (void)msg;  // Подавить unused warning
}
#define KERNEL_LOG(fn, msg) kernel_log_impl(fn, msg)
//==================================================================


#define SYSCALL_INLINE __attribute__((always_inline, flatten)) static inline
#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

// Для ясности лучше использоватьstdint типы или системные, если доступны
// typedef long ssize_t; 
// typedef unsigned long size_t;

typedef struct {
    union {
        // Аргументы согласно ABI (rdi, rsi, rdx, r10, ...)
        struct { uint64_t arg0, arg1, arg2, arg3, arg4, arg5; }; 
        uint64_t args[6]; // Расширим до 6 для общности
    };
} syscall_args_t;

// Объявим прототипы здесь, чтобы таблица была чище
SYSCALL_INLINE uint64_t sys_write(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_read(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_open(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_close(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_lseek(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_fstat(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_exit(const syscall_args_t *a);
//---

typedef uint64_t (*syscall_handler_t)(const syscall_args_t*);

// Таблица обработчиков
static const syscall_handler_t syscall_table[SYS_MAX] __attribute__((aligned(64))) = {
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
    [SYS_OPEN]  = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_LSEEK] = sys_lseek,
    [SYS_FSTAT] = sys_fstat,
    [SYS_EXIT]  = sys_exit
};

/**
 * @brief Точка входа диспетчера системных вызовов.
 * 
 * @param num Номер системного вызова (должен быть в rax при входе).
 * @param args Указатель на структуру с аргументами.
 * @return uint64_t Результат системного вызова. 
 *         В случае ошибки, это будет отрицательное число (-errno).
 *         Вызывающая сторона должна интерпретировать результат.
 */
uint64_t syscall_entry(uint64_t num, const syscall_args_t *args) {
    // Проверка остается такой же, она корректна.
    if (UNLIKELY(num >= SYS_MAX || !syscall_table[num])) {
        KERNEL_LOG(__func__, "Invalid or unimplemented syscall");
        return (uint64_t)-ENOSYS; // Возвращаем -ENOSYS, приведенный к uint64_t
    }
     
    // Вызов обработчика
    return syscall_table[num](args);
}

//----------------------------------------------------------------
// РЕАЛИЗАЦИИ ОБРАБОТЧИКОВ
//----------------------------------------------------------------

SYSCALL_INLINE uint64_t sys_write(const syscall_args_t *a) {
    // Используем более строгие и подходящие типы
    int         fd    = (int)a->arg0;
    const void* buf   = (const void*)a->arg1;
    size_t      count = (size_t)a->arg2;

    register int64_t ret __asm__("rax"); // syscall возвращает знаковый тип!
    
    __asm__ volatile (
        "syscall"
        : "=a"(ret) // Выход: rax
        : "a"(SYS_WRITE), "D"(fd), "S"(buf), "d"(count) // Входы: rax, rdi, rsi, rdx
        : "rcx", "r11", "memory" // Разрушаемые регистры + память
    );
     // ПРАВИЛЬНАЯ ОБРАБОТКА: Просто возвращаем то, что вернул syscall.
     // Вызывающий код должен проверить, < 0 ли результат.
    return (uint64_t)ret;
}

SYSCALL_INLINE uint64_t sys_read(const syscall_args_t *a) {
     int         fd    = (int)a->arg0;
     void*       buf   = (void*)a->arg1;
     size_t      count = (size_t)a->arg2;

    register int64_t ret __asm__("rax");
    
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_READ), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
     return (uint64_t)ret; // Возвращаем как есть
}

SYSCALL_INLINE uint64_t sys_open(const syscall_args_t *a) {
    const char *path  = (const char*)a->arg0;
    int        flags = (int)a->arg1;
    mode_t     mode  = (mode_t)a->arg2; // mode_t - обычно unsigned

    register int64_t ret __asm__("rax");
     
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_OPEN), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
     return (uint64_t)ret;
}

SYSCALL_INLINE uint64_t sys_close(const syscall_args_t *a) {
    int fd = (int)a->arg0;
    
    register int64_t ret __asm__("rax");
     
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_CLOSE), "D"(fd)
        : "rcx", "r11" // Память не трогается
    );
     return (uint64_t)ret;
}

SYSCALL_INLINE uint64_t sys_lseek(const syscall_args_t *a) {
    int   fd     = (int)a->arg0; 
    off_t offset = (off_t)a->arg1; 
    int   whence = (int)a->arg2;

    register int64_t ret __asm__("rax"); // lseek может вернуть большое значение или -1
    
     __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_LSEEK), "D"(fd), "S"(offset), "d"(whence)
        : "rcx", "r11"
    );
     return (uint64_t)ret;
}
 
 SYSCALL_INLINE uint64_t sys_fstat(const syscall_args_t *a) {
    int fd = (int)a->arg0;
    struct stat *st = (struct stat*)a->arg1;

    register int64_t ret __asm__("rax");
    
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_FSTAT), "D"(fd), "S"(st)
        : "rcx", "r11", "memory"
    );
     return (uint64_t)ret;
}

// Исправленная версия:
SYSCALL_INLINE uint64_t sys_exit(const syscall_args_t *a) {
     int status = (int)a->arg0;
     
    __asm__ volatile (
        "syscall"
        : /* Нет выходных операндов, т.к. вызов не возвращается */ 
        : "a"(SYS_EXIT), "D"(status) /* syscall номер в RAX, первый арг. в RDI */
        : "rcx", "r11" // Технически, они тоже могут быть изменены
    );
    __builtin_unreachable(); // Сообщаем компилятору, что сюда управление не вернется.
     // return 0; // Недостижимо, но может потребоваться, чтобы компилятор не ругался
}
