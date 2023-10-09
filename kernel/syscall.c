#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_PATH_LENGTH 4096

typedef struct _file_t {
    int fd;
    char filename[MAX_FILENAME_LENGTH];
    unsigned long inode;
    off_t offset;
    size_t length;
    time_t mtime;
    bool deleted;
} file_t;

typedef enum _mode_t {
    O_RDONLY = 0x0000,   /* Open for reading only */
    O_WRONLY = 0x0001,   /* Open for writing only */
    O_RDWR = 0x0002,     /* Open for reading and writing */
    O_APPEND = 0x0008,   /* If set, append to the end of the file */
    O_CREAT = 0x0200,    /* If set, create the file if it does not exist */
    O_EXCL = 0x0800      /* If set, fail if the file already exists */
} mode_t;

typedef struct _process_t {
    pid_t pid;           /* Process ID */
    uid_t uid;           /* User ID */
    gid_t gid;           /* Group ID */
    char name[32];       /* Name of the process */
    char cwd[MAX_PATH_LENGTH]; /* Current working directory */
    char **argv;         /* Command line arguments */
    char **envp;         /* Environment variables */
    file_t files[32];    /* File descriptors */
    int num_files;       /* Number of file descriptors */
    int max_files;       /* Maximum number of file descriptors */
    sigset_t signal_mask;/* Signal mask for the process */
    volatile int state; /* State of the process (running, waiting, etc.) */
    volatile int wakeup; /* Wake-up reason for the process */
    volatile int error; /* Error code for the process */
    volatile int retval; /* Return value for the process */
} process_t;

typedef struct _syscall_args_t {
    union {
        int arg1;
        int arg2;
        int arg3;
        int arg4;
        int arg5;
        int arg6;
        int arg7;
        int arg8;
        int arg9;
        int arg10;
    };
} syscall_args_t;

typedef struct _syscall_result_t {
    int result;          /* Result of the system call */
    int errnum;          /* Errno associated with the result */
} syscall_result_t;

typedef struct _syscall_entry_t {
    int number;          /* System call number */
    const char *name;    /* Name of the system call */
    void (*handler)(struct _syscall_args_t *args, struct _syscall_result_t *res); /* Handler function for the system call */
} syscall_entry_t;

extern syscall_entry_t syscall_table[];

/* Function prototypes */
void init_syscalls(void);
void handle_system_call(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_exit(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_read(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_write(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_open(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_close(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_creat(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_link(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_unlink(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_execve(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_chdir(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_fork(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_wait(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_pipe(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_dup(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_dup2(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_getpid(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_brk(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_sleep(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_signal(struct _syscall_args_t *args, struct _syscall_result_t *res);
void handle_kill(struct _syscall_args_t *args, struct _syscall_result_t *res);
