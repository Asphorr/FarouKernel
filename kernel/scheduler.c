#define _DEFAULT_SOURCE // For usleep
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

//===================================================================
// 1. ЗАМЕНА/ОБЕРТКИ ПРИМИТИВОВ
//    Используем правильные примитивы user-space (мьютексы)
//===================================================================

// --- Обертка для Мьютекса ---
typedef struct {
     pthread_mutex_t mutex;
} KernelMutex;

static inline void mutex_init(KernelMutex* m) {
     pthread_mutex_init(&m->mutex, NULL);
}
static inline void mutex_destroy(KernelMutex* m) {
     pthread_mutex_destroy(&m->mutex);
}
static inline void mutex_lock(KernelMutex* m) {
      pthread_mutex_lock(&m->mutex);
}
 static inline void mutex_unlock(KernelMutex* m) {
      pthread_mutex_unlock(&m->mutex);
}

// --- Заглушки для Kernel API (для совместимости) ---
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void* kmalloc(size_t size) { return malloc(size); }
void kfree(void* ptr) { free(ptr); }
void ksleep_ms(unsigned int ms) { usleep(ms * 1000); }

// --- Логгер ---
KernelMutex g_log_mutex;

void log_message(const char* func, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    mutex_lock(&g_log_mutex);
     printf("[KERN:%s:%d] ", func, line);
    vprintf(format, args);
     printf("\n");
    mutex_unlock(&g_log_mutex);
    
    va_end(args);
}
// Макрос для удобства
#define LOG(...) log_message(__func__, __LINE__, __VA_ARGS__)


//===================================================================
// 2. КОНФИГУРАЦИЯ И ТИПЫ
//===================================================================

#define MAX_TASKS 64
#define MIN_PRIORITY 1
#define MAX_PRIORITY 99
#define TASK_NAME_LEN 32

typedef uint32_t ktid_t;

typedef enum {
   TASK_STATE_INVALID = 0, // Слот не используется
   TASK_STATE_RUNNING,     // Задача выполняется
   TASK_STATE_DONE         // Задача завершила работу (готова к удалению)
} TaskState;

// "Блок Управления Задачей" (Task Control Block)
typedef struct {
    ktid_t      tid;
    pthread_t   pthread_handle; // Храним хендл для возможности join (хотя здесь не используется)
    TaskState   state;
    int         priority;      // <-- Симулируемый приоритет
    char        name[TASK_NAME_LEN];
     // Здесь можно хранить состояние регистров, стек и т.д. в реальном ядре
} TCB;

typedef struct Scheduler_tag Scheduler; // Предварительное объявление

// Аргументы, передаваемые в реальный поток при старте
typedef struct {
     Scheduler* scheduler;
     ktid_t     tid;         // ID задачи, которую поток представляет
     void (*user_entry)(void*); // Функция пользователя
     void* user_arg;            // Аргумент для функции пользователя
     char  name[TASK_NAME_LEN]; // Копия имени для логирования
} ThreadStartArgs;


//===================================================================
// 3. ПЛАНИРОВЩИК (Структуры данных)
//===================================================================

struct Scheduler_tag {
    TCB         task_list[MAX_TASKS];
    size_t      task_count; // Количество АКТИВНЫХ (RUNNING+DONE) задач
    KernelMutex task_list_lock;
    volatile bool terminate_manager; // Флаг для остановки потока-менеджера
    ktid_t      next_tid;
};


//===================================================================
// 4. УПРАВЛЕНИЕ ЗАДАЧАМИ И ЖИЗНЕННЫМ ЦИКЛОМ
//===================================================================

// Внутренняя функция-обертка, запускаемая через pthread_create
static void* thread_runner(void* arg) {
     ThreadStartArgs* start_args = (ThreadStartArgs*)arg;
     Scheduler* sched = start_args->scheduler;
     ktid_t tid = start_args->tid;
     
     // 1. Выполняем пользовательский код
     LOG("Task '%s' (TID %u) STARTED.", start_args->name, tid);
     
     start_args->user_entry(start_args->user_arg); // Вызов функции пользователя
     
     LOG("Task '%s' (TID %u) FINISHED execution.", start_args->name, tid);

    // 2. Обновляем статус в TCB на DONE
     mutex_lock(&sched->task_list_lock);
     bool found = false;
     for(size_t i = 0; i < MAX_TASKS; ++i) {
        // Ищем по TID, а не по индексу, т.к. массив может уплотняться
         if(sched->task_list[i].state != TASK_STATE_INVALID && sched->task_list[i].tid == tid)
         {
             sched->task_list[i].state = TASK_STATE_DONE;
             found = true;
             break;
         }
     }
     mutex_unlock(&sched->task_list_lock);
     
     if (!found) {
        LOG("ERROR: Task '%s' (TID %u) could not find its TCB after finishing!", start_args->name, tid);
     }

     kfree(start_args); // Освобождаем память, выделенную для аргументов
     return NULL;
}

// API: Создание потока
ktid_t kernel_create_thread(
    Scheduler* sched,
    const char* name,
    int priority,
    void (*entry)(void*),
    void* arg
    )
{
    ThreadStartArgs* start_args = NULL;
    ktid_t tid = 0;
    int rc = 0;
    
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
       LOG("ERROR: Invalid priority %d for task '%s'", priority, name);
       return 0; // 0 = invalid TID
    }
       
    start_args = kmalloc(sizeof(ThreadStartArgs));
     if (!start_args) {
        LOG("ERROR: Failed to allocate memory for thread args ('%s')", name);
        return 0;
     }
     
    // --- Критическая секция: Добавление задачи в список ---
    mutex_lock(&sched->task_list_lock);

    // Ищем свободный слот
     int free_slot = -1;
     for(int i=0; i < MAX_TASKS; ++i) {
       if (sched->task_list[i].state == TASK_STATE_INVALID) {
         free_slot = i;
         break;
       }
     }
   
    if (free_slot == -1) {
        LOG("ERROR: Task limit (%d) reached. Cannot create '%s'.", MAX_TASKS, name);
        mutex_unlock(&sched->task_list_lock);
        kfree(start_args);
        return 0;
    }
    
    // Заполняем TCB
     tid = sched->next_tid++;
     TCB* tcb = &sched->task_list[free_slot];
     
     tcb->tid = tid;
     tcb->state = TASK_STATE_RUNNING; // Сразу считаем запущенной
     tcb->priority = priority;
     strncpy(tcb->name, name, TASK_NAME_LEN - 1);
     tcb->name[TASK_NAME_LEN - 1] = '\0';
     
    // Готовим аргументы для потока-обертки
     start_args->scheduler = sched;
     start_args->tid = tid;
     start_args->user_entry = entry;
     start_args->user_arg = arg;
     strncpy(start_args->name, name, TASK_NAME_LEN - 1);
     start_args->name[TASK_NAME_LEN - 1] = '\0';

     // Создаем реальный поток ОС
     rc = pthread_create(&tcb->pthread_handle, NULL, thread_runner, start_args);
     if (rc != 0) {
         LOG("ERROR: pthread_create failed for '%s' with code %d (%s)", name, rc, strerror(rc));
         tcb->state = TASK_STATE_INVALID; // Откатываем - слот снова свободен
         tid = 0; // Сигнал об ошибке
     } else {
        pthread_detach(tcb->pthread_handle); // По-прежнему не ждем потоки, 
                                             // но теперь они сами выставят статус DONE.
        LOG("Created task '%s' (TID %u, Prio %d) in slot %d.", name, tid, priority, free_slot);
        sched->task_count++;
     }
     
    mutex_unlock(&sched->task_list_lock);
    // --- Конец критической секции ---

    if (tid == 0) { // Если pthread_create не удался
       kfree(start_args);
    }
    
    return tid;
}


// API: Установка СИМУЛИРУЕМОГО приоритета
// ВНИМАНИЕ: НЕ влияет на планировщик ОС!
void kernel_set_priority(Scheduler* sched, ktid_t tid, int priority)
{
     mutex_lock(&sched->task_list_lock);
      bool found = false;
       for(size_t i = 0; i < MAX_TASKS; ++i) {
         if(sched->task_list[i].state == TASK_STATE_RUNNING && sched->task_list[i].tid == tid)
         {
             sched->task_list[i].priority = priority;
             found = true;
             break;
         }
     }
     mutex_unlock(&sched->task_list_lock);
     
    //  if (found) {
    //      LOG("API: Set simulated priority for TID %u to %d", tid, priority);
    //  }
}

//===================================================================
// 5. ФОНОВЫЙ ПОТОК (Менеджер / Сборщик мусора)
//===================================================================

// Уплотняет массив, удаляя записи DONE
static void reap_tasks(Scheduler* sched)
{
   // Работает только под блокировкой!
   size_t write_idx = 0;
   size_t read_idx = 0;
   size_t tasks_reaped = 0;
   
   // Используем алгоритм уплотнения (compacting)
   for(read_idx = 0; read_idx < MAX_TASKS; ++read_idx)
   {
      // Копируем валидные (RUNNING) записи в начало (если нужно)
      if (sched->task_list[read_idx].state == TASK_STATE_RUNNING)
      {
         if (read_idx != write_idx) {
           sched->task_list[write_idx] = sched->task_list[read_idx];
         }
         write_idx++;
      }
       else if (sched->task_list[read_idx].state == TASK_STATE_DONE)
      {
          // Эту задачу нужно убрать
          LOG("Reaping task '%s' (TID %u)", sched->task_list[read_idx].name, sched->task_list[read_idx].tid);
          tasks_reaped++;
      }
   }
   
   sched->task_count -= tasks_reaped; // Уменьшаем счетчик живых задач

   // Очищаем оставшиеся слоты в конце массива
   for(size_t i = write_idx; i < MAX_TASKS; ++i) {
       sched->task_list[i].state = TASK_STATE_INVALID;
       sched->task_list[i].tid = 0;
   }
    if (tasks_reaped > 0) {
      LOG("Reaped %zu tasks. Active tasks now: %zu", tasks_reaped, sched->task_count);
    }
}


void* manager_thread_entry(void* arg)
{
     Scheduler* sched = (Scheduler*)arg;
     LOG("Manager/Reaper thread started.");
     
     while (!sched->terminate_manager)
     {
        ksleep_ms(2000); // Пауза между циклами работы

        mutex_lock(&sched->task_list_lock);
        
        // --- Фаза 1: Обновление симулируемых приоритетов (как в оригинале) ---
        for (size_t i = 0; i < MAX_TASKS; i++)
        {
             // Работаем ТОЛЬКО с живыми задачами
             if (sched->task_list[i].state != TASK_STATE_RUNNING) {
                 continue;
             }

            // Меняем симулируемый приоритет
            int old_prio = sched->task_list[i].priority;
            int new_prio = old_prio + (rand() % 3 - 1); // -1, 0, or +1
            
            // Ограничение
             new_prio = (new_prio < MIN_PRIORITY) ? MIN_PRIORITY : new_prio;
             new_prio = (new_prio > MAX_PRIORITY) ? MAX_PRIORITY : new_prio;
            
            if (new_prio != old_prio) {
               sched->task_list[i].priority = new_prio;
                LOG("Manager: Updated '%s' (TID %u) simulated prio to %d", 
                   sched->task_list[i].name, sched->task_list[i].tid, new_prio);
            }
        }
        
         // --- Фаза 2: Сборка мусора (удаление DONE задач) ---
         reap_tasks(sched);
         
        mutex_unlock(&sched->task_list_lock);
     }
     
      LOG("Manager/Reaper thread shutting down.");
     return NULL;
}


//===================================================================
// 6. ИНИЦИАЛИЗАЦИЯ И MAIN
//===================================================================

void initialize_scheduler(Scheduler* scheduler) {
    scheduler->task_count = 0;
    scheduler->next_tid = 1; // TIDы начинаются с 1
    scheduler->terminate_manager = false;
    mutex_init(&scheduler->task_list_lock);
    
    for(size_t i=0; i < MAX_TASKS; ++i) {
      scheduler->task_list[i].state = TASK_STATE_INVALID;
       scheduler->task_list[i].tid = 0;
    }
    LOG("Scheduler initialized.");
}

 void destroy_scheduler(Scheduler* scheduler) {
      mutex_destroy(&scheduler->task_list_lock);
      LOG("Scheduler destroyed.");
 }

// --- Пример задачи пользователя ---
void example_task_func(void* arg) {
    const char* task_name = (const char*) arg;
    
    for (int i = 0; i < 3; ++i) {
        LOG("   Task %s working... (%d/3)", task_name, i + 1);
        ksleep_ms(500 + (rand() % 1000)); // Работаем случайное время
    }
     // НЕ нужно трогать планировщик отсюда,
     // обертка thread_runner сделает это сама.
}
//---------------------------------

int main() {
    srand(time(NULL));
    mutex_init(&g_log_mutex);
    
    Scheduler global_scheduler;
    initialize_scheduler(&global_scheduler);

    // Запускаем поток-менеджер
    pthread_t manager_thread_handle;
    int rc = pthread_create(&manager_thread_handle, NULL, manager_thread_entry, &global_scheduler);
     if (rc != 0) {
       LOG("FATAL: Could not create manager thread!");
        mutex_destroy(&g_log_mutex);
       return 1;
     }

    // Создаем задачи через наше "API"
    kernel_create_thread(&global_scheduler, "HighPrio", 90, example_task_func, "HighPrio");
     ksleep_ms(50);
    kernel_create_thread(&global_scheduler, "MidPrio",  50, example_task_func, "MidPrio");
     ksleep_ms(50);
    kernel_create_thread(&global_scheduler, "LowPrio",  10, example_task_func, "LowPrio");
     kernel_create_thread(&global_scheduler, "LowPrio_2",  10, example_task_func, "LowPrio_2");

    LOG("------- Running for 15 seconds -------");
    ksleep_ms(15000);

    // --- Корректное Завершение ---
    LOG("------- Signalling shutdown -------");
    global_scheduler.terminate_manager = true; // Сигнал менеджеру на выход

    // Ждем, пока менеджер завершится
    pthread_join(manager_thread_handle, NULL); 
    LOG("Manager thread joined.");
    
    destroy_scheduler(&global_scheduler);
    mutex_destroy(&g_log_mutex);
     
    printf("\nMain execution completed.\n");
    return 0;
}
