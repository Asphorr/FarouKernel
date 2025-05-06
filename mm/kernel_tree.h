/*
 * kernel_tree.h - Высокооптимизированная библиотека бинарных деревьев для ядра ОС
 *
 * Copyright (C) 2025 Kernel Systems
 * SPDX-License-Identifier: GPL-2.0
 *
 * Версия: 1.0
 * Компиляция: gcc -O3 -march=native -flto -fno-exceptions -fno-rtti
 */

#ifndef _KERNEL_TREE_H
#define _KERNEL_TREE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================================
 * КОНФИГУРАЦИЯ И АРХИТЕКТУРНО-ЗАВИСИМЫЕ ОПТИМИЗАЦИИ
 * ============================================================================
 */

/* Архитектурно-специфичные оптимизации */
#if defined(__x86_64__) || defined(_M_X64)
  #include <immintrin.h>
  #define KTREE_ARCH_X86_64
  #define KTREE_CACHE_LINE_SIZE 64
  #define KTREE_PREFETCH(addr) _mm_prefetch((const char *)(addr), _MM_HINT_T0)
#elif defined(__aarch64__) || defined(_M_ARM64)
  #include <arm_neon.h>
  #define KTREE_ARCH_ARM64
  #define KTREE_CACHE_LINE_SIZE 64
  #define KTREE_PREFETCH(addr) __builtin_prefetch((addr), 0, 3)
#else
  #define KTREE_ARCH_GENERIC
  #define KTREE_CACHE_LINE_SIZE 64
  #define KTREE_PREFETCH(addr) __builtin_prefetch((addr), 0, 3)
#endif

/* Конфигурируемые параметры */
#ifndef KTREE_MAX_NODES
  #define KTREE_MAX_NODES 4096
#endif

#ifndef KTREE_MAX_TREES
  #define KTREE_MAX_TREES 32
#endif

#ifndef KTREE_SPINLOCK_RETRIES
  #define KTREE_SPINLOCK_RETRIES 1000
#endif

#ifndef KTREE_NUMA_AWARE
  #define KTREE_NUMA_AWARE 0
#endif

/* Компиляторно-специфичные оптимизации */
#if defined(__GNUC__) || defined(__clang__)
  #define KTREE_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define KTREE_UNLIKELY(x) __builtin_expect(!!(x), 0)
  #define KTREE_INLINE      static inline __attribute__((always_inline))
  #define KTREE_PACKED      __attribute__((packed))
  #define KTREE_ALIGNED(x)  __attribute__((aligned(x)))
  #define KTREE_NOINLINE    __attribute__((noinline))
  #define KTREE_PURE        __attribute__((pure))
  #define KTREE_CONST       __attribute__((const))
  #define KTREE_UNUSED      __attribute__((unused))
  #define KTREE_NORETURN    __attribute__((noreturn))
  #define KTREE_COLD        __attribute__((cold))
  #define KTREE_HOT         __attribute__((hot))
#else
  #define KTREE_LIKELY(x)   (x)
  #define KTREE_UNLIKELY(x) (x)
  #define KTREE_INLINE      static inline
  #define KTREE_PACKED
  #define KTREE_ALIGNED(x)
  #define KTREE_NOINLINE
  #define KTREE_PURE
  #define KTREE_CONST
  #define KTREE_UNUSED
  #define KTREE_NORETURN
  #define KTREE_COLD
  #define KTREE_HOT
#endif

/* Предзаполненные константы */
#define KTREE_INVALID_NODE UINT16_MAX
#define KTREE_MAX_DEPTH    32

/* Индикация ошибок */
typedef enum {
    KTREE_SUCCESS = 0,
    KTREE_ERR_FULL,
    KTREE_ERR_INVALID,
    KTREE_ERR_NOT_FOUND,
    KTREE_ERR_EXISTS,
    KTREE_ERR_EMPTY,
    KTREE_ERR_LOCK_FAILED,
    KTREE_ERR_MAX_DEPTH,
    KTREE_ERR_OOM,
    KTREE_ERR_CORRUPTED
} ktree_error_t;

/* Уровни отладки */
#ifdef KTREE_DEBUG
  #define KTREE_DEBUG_NONE     0
  #define KTREE_DEBUG_ERROR    1
  #define KTREE_DEBUG_WARN     2
  #define KTREE_DEBUG_INFO     3
  #define KTREE_DEBUG_VERBOSE  4
  #define KTREE_DEBUG_TRACE    5

  #ifndef KTREE_DEBUG_LEVEL
    #define KTREE_DEBUG_LEVEL KTREE_DEBUG_ERROR
  #endif

  #define KTREE_DEBUG_LOG(level, fmt, ...) \
    do { \
        if (level <= KTREE_DEBUG_LEVEL) { \
            kernel_printk(KERN_DEBUG "[KTREE] " fmt, ##__VA_ARGS__); \
        } \
    } while (0)
#else
  #define KTREE_DEBUG_LOG(level, fmt, ...)
#endif

/*
 * ============================================================================
 * ТИПЫ И СТРУКТУРЫ ДАННЫХ
 * ============================================================================
 */

/* Тип произвольных данных для узлов */
typedef int32_t ktree_data_t;

/* Тип хэша для быстрого поиска */
typedef uint32_t ktree_hash_t;

/* Тип индекса узла */
typedef uint16_t ktree_node_idx_t;

/* Тип атомик-счетчика */
typedef struct {
    volatile uint32_t counter;
} ktree_atomic_t;

/* Тип спин-лока */
typedef struct {
    volatile uint32_t lock;
} ktree_spinlock_t;

/* Выровненная на кэш-линию структура узла для предотвращения false sharing */
typedef struct KTREE_ALIGNED(8) {
    ktree_data_t data;
    ktree_hash_t hash;
    ktree_node_idx_t left;
    ktree_node_idx_t right;
    ktree_node_idx_t parent;
    uint8_t height;
    uint8_t flags;
} ktree_node_t;

/* Статический пул узлов */
typedef struct KTREE_ALIGNED(KTREE_CACHE_LINE_SIZE) {
    ktree_node_t nodes[KTREE_MAX_NODES];
    ktree_node_idx_t free_list;
    ktree_atomic_t node_count;
    ktree_spinlock_t pool_lock;
    uint8_t padding[KTREE_CACHE_LINE_SIZE - sizeof(ktree_spinlock_t) 
                   - sizeof(ktree_atomic_t) - sizeof(ktree_node_idx_t)];
} ktree_node_pool_t;

/* Битовая карта для отслеживания дубликатов/посещений */
typedef struct KTREE_ALIGNED(KTREE_CACHE_LINE_SIZE) {
    uint64_t bits[KTREE_MAX_NODES / 64 + (KTREE_MAX_NODES % 64 ? 1 : 0)];
} ktree_bitmap_t;

/* Кольцевой буфер с фиксированным размером для обходов дерева */
typedef struct KTREE_ALIGNED(KTREE_CACHE_LINE_SIZE) {
    ktree_node_idx_t buffer[KTREE_MAX_NODES];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} ktree_ring_buffer_t;

/* Структура дерева */
typedef struct KTREE_ALIGNED(KTREE_CACHE_LINE_SIZE) {
    ktree_node_idx_t root;
    ktree_atomic_t size;
    ktree_spinlock_t tree_lock;
    uint32_t flags;
    uint8_t height;
    uint8_t padding[KTREE_CACHE_LINE_SIZE - sizeof(ktree_node_idx_t) 
                  - sizeof(ktree_atomic_t) - sizeof(ktree_spinlock_t) 
                  - sizeof(uint32_t) - sizeof(uint8_t)];
} ktree_tree_t;

/* Управление пулом деревьев */
typedef struct KTREE_ALIGNED(KTREE_CACHE_LINE_SIZE) {
    ktree_node_pool_t node_pool;
    ktree_tree_t trees[KTREE_MAX_TREES];
    ktree_atomic_t tree_count;
    ktree_spinlock_t mgr_lock;
    uint32_t flags;
} ktree_manager_t;

/* Итератор для обхода деревьев */
typedef struct {
    ktree_node_idx_t stack[KTREE_MAX_DEPTH];
    int32_t top;
    ktree_node_pool_t* pool;
    ktree_tree_t* tree;
    uint32_t flags;
    ktree_node_idx_t current;
} ktree_iterator_t;

/* Флаги для узлов */
#define KTREE_FLAG_RED       (1 << 0)  /* Для красно-черного дерева */
#define KTREE_FLAG_MARKED    (1 << 1)  /* Для алгоритмов обхода */
#define KTREE_FLAG_DELETED   (1 << 2)  /* Помечен для удаления */
#define KTREE_FLAG_DIRTY     (1 << 3)  /* Данные изменены */
#define KTREE_FLAG_LOCKED    (1 << 4)  /* Узел заблокирован */
#define KTREE_FLAG_ROOT      (1 << 5)  /* Корневой узел */
#define KTREE_FLAG_LEAF      (1 << 6)  /* Листовой узел */

/* Флаги для деревьев */
#define KTREE_TREE_FLAG_AVL       (1 << 0)  /* АВЛ дерево */
#define KTREE_TREE_FLAG_RB        (1 << 1)  /* Красно-черное дерево */
#define KTREE_TREE_FLAG_BST       (1 << 2)  /* Простое бинарное дерево */
#define KTREE_TREE_FLAG_THREADED  (1 << 3)  /* Нитевидное дерево */
#define KTREE_TREE_FLAG_BALANCED  (1 << 4)  /* Сбалансированное */
#define KTREE_TREE_FLAG_SORTED    (1 << 5)  /* Сортированное */
#define KTREE_TREE_FLAG_LOCKED    (1 << 6)  /* Дерево заблокировано */

/* Флаги для итераторов */
#define KTREE_ITER_FLAG_PREORDER   (1 << 0)
#define KTREE_ITER_FLAG_INORDER    (1 << 1)
#define KTREE_ITER_FLAG_POSTORDER  (1 << 2)
#define KTREE_ITER_FLAG_LEVELORDER (1 << 3)

/*
 * ============================================================================
 * АТОМАРНЫЕ ОПЕРАЦИИ И СИНХРОНИЗАЦИЯ
 * ============================================================================
 */

/* Атомарное увеличение */
KTREE_INLINE uint32_t ktree_atomic_inc(ktree_atomic_t* atom) {
    return __atomic_add_fetch(&atom->counter, 1, __ATOMIC_SEQ_CST);
}

/* Атомарное уменьшение */
KTREE_INLINE uint32_t ktree_atomic_dec(ktree_atomic_t* atom) {
    return __atomic_sub_fetch(&atom->counter, 1, __ATOMIC_SEQ_CST);
}

/* Атомарное чтение */
KTREE_INLINE uint32_t ktree_atomic_read(const ktree_atomic_t* atom) {
    return __atomic_load_n(&atom->counter, __ATOMIC_SEQ_CST);
}

/* Атомарная запись */
KTREE_INLINE void ktree_atomic_set(ktree_atomic_t* atom, uint32_t value) {
    __atomic_store_n(&atom->counter, value, __ATOMIC_SEQ_CST);
}

/* Атомарный compare-and-swap */
KTREE_INLINE bool ktree_atomic_cas(ktree_atomic_t* atom, 
                                 uint32_t expected, 
                                 uint32_t desired) {
    return __atomic_compare_exchange_n(&atom->counter, &expected, desired, 
                                     false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/* Инициализация спин-лока */
KTREE_INLINE void ktree_spinlock_init(ktree_spinlock_t* lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELAXED);
}

/* Захват спин-лока */
KTREE_INLINE ktree_error_t ktree_spinlock_lock(ktree_spinlock_t* lock) {
    uint32_t i;
    for (i = 0; i < KTREE_SPINLOCK_RETRIES; i++) {
        if (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE) == 0)
            return KTREE_SUCCESS;
        
        /* Пауза для снижения контендации в гипер-потоках */
#ifdef KTREE_ARCH_X86_64
        _mm_pause();
#elif defined(KTREE_ARCH_ARM64)
        __asm__ volatile("yield" ::: "memory");
#endif
    }
    
    return KTREE_ERR_LOCK_FAILED;
}

/* Попытка захвата спин-лока */
KTREE_INLINE bool ktree_spinlock_trylock(ktree_spinlock_t* lock) {
    return __atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE) == 0;
}

/* Освобождение спин-лока */
KTREE_INLINE void ktree_spinlock_unlock(ktree_spinlock_t* lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

/*
 * ============================================================================
 * ОПЕРАЦИИ С БИТОВЫМИ КАРТАМИ
 * ============================================================================
 */

/* Инициализация битовой карты */
KTREE_INLINE void ktree_bitmap_init(ktree_bitmap_t* bitmap) {
    size_t i;
    size_t size = KTREE_MAX_NODES / 64 + (KTREE_MAX_NODES % 64 ? 1 : 0);
    
    for (i = 0; i < size; i++)
        bitmap->bits[i] = 0;
}

/* Установка бита */
KTREE_INLINE void ktree_bitmap_set(ktree_bitmap_t* bitmap, uint32_t bit) {
    uint32_t idx = bit >> 6;      /* div 64 */
    uint32_t offset = bit & 0x3F; /* mod 64 */
    bitmap->bits[idx] |= (1ULL << offset);
}

/* Проверка бита */
KTREE_INLINE bool ktree_bitmap_test(const ktree_bitmap_t* bitmap, uint32_t bit) {
    uint32_t idx = bit >> 6;      /* div 64 */
    uint32_t offset = bit & 0x3F; /* mod 64 */
    return (bitmap->bits[idx] & (1ULL << offset)) != 0;
}

/* Проверка и установка бита атомарно */
KTREE_INLINE bool ktree_bitmap_test_and_set(ktree_bitmap_t* bitmap, uint32_t bit) {
    uint32_t idx = bit >> 6;      /* div 64 */
    uint32_t offset = bit & 0x3F; /* mod 64 */
    uint64_t mask = 1ULL << offset;
    uint64_t old;
    
    old = __atomic_fetch_or(&bitmap->bits[idx], mask, __ATOMIC_SEQ_CST);
    return (old & mask) != 0;
}

/* Сброс бита */
KTREE_INLINE void ktree_bitmap_clear(ktree_bitmap_t* bitmap, uint32_t bit) {
    uint32_t idx = bit >> 6;      /* div 64 */
    uint32_t offset = bit & 0x3F; /* mod 64 */
    bitmap->bits[idx] &= ~(1ULL << offset);
}

/* Подсчет установленных битов - оптимизированная версия */
KTREE_PURE KTREE_INLINE uint32_t ktree_bitmap_count(const ktree_bitmap_t* bitmap) {
    uint32_t count = 0;
    size_t i;
    size_t size = KTREE_MAX_NODES / 64 + (KTREE_MAX_NODES % 64 ? 1 : 0);
    
    for (i = 0; i < size; i++)
        count += __builtin_popcountll(bitmap->bits[i]);
    
    return count;
}

/*
 * ============================================================================
 * ОПЕРАЦИИ С КОЛЬЦЕВЫМИ БУФЕРАМИ
 * ============================================================================
 */

/* Инициализация кольцевого буфера */
KTREE_INLINE void ktree_ring_init(ktree_ring_buffer_t* rb) {
    rb->head = rb->tail = rb->count = 0;
}

/* Проверка на пустоту */
KTREE_PURE KTREE_INLINE bool ktree_ring_empty(const ktree_ring_buffer_t* rb) {
    return rb->count == 0;
}

/* Проверка на заполненность */
KTREE_PURE KTREE_INLINE bool ktree_ring_full(const ktree_ring_buffer_t* rb) {
    return rb->count == KTREE_MAX_NODES;
}

/* Добавление элемента */
KTREE_INLINE ktree_error_t ktree_ring_push(ktree_ring_buffer_t* rb, ktree_node_idx_t idx) {
    if (KTREE_UNLIKELY(ktree_ring_full(rb)))
        return KTREE_ERR_FULL;
    
    rb->buffer[rb->tail] = idx;
    rb->tail = (rb->tail + 1) & (KTREE_MAX_NODES - 1); /* Быстрый % через битовую операцию */
    rb->count++;
    
    return KTREE_SUCCESS;
}

/* Извлечение элемента */
KTREE_INLINE ktree_error_t ktree_ring_pop(ktree_ring_buffer_t* rb, ktree_node_idx_t* idx) {
    if (KTREE_UNLIKELY(ktree_ring_empty(rb)))
        return KTREE_ERR_EMPTY;
    
    *idx = rb->buffer[rb->head];
    rb->head = (rb->head + 1) & (KTREE_MAX_NODES - 1);
    rb->count--;
    
    return KTREE_SUCCESS;
}

/* Очистка буфера */
KTREE_INLINE void ktree_ring_clear(ktree_ring_buffer_t* rb) {
    rb->head = rb->tail = rb->count = 0;
}

/* Получение размера */
KTREE_PURE KTREE_INLINE uint16_t ktree_ring_size(const ktree_ring_buffer_t* rb) {
    return rb->count;
}

/*
 * ============================================================================
 * МАНИПУЛЯЦИИ С УЗЛАМИ И ПУЛОМ УЗЛОВ
 * ============================================================================
 */

/* Инициализация пула узлов */
KTREE_INLINE void ktree_pool_init(ktree_node_pool_t* pool) {
    uint16_t i;
    
    /* Инициализация цепочки свободных узлов */
    for (i = 0; i < KTREE_MAX_NODES - 1; i++) {
        pool->nodes[i].left = i + 1;
        pool->nodes[i].right = KTREE_INVALID_NODE;
        pool->nodes[i].parent = KTREE_INVALID_NODE;
        pool->nodes[i].height = 0;
        pool->nodes[i].flags = 0;
    }
    
    /* Последний узел */
    pool->nodes[KTREE_MAX_NODES - 1].left = KTREE_INVALID_NODE;
    pool->nodes[KTREE_MAX_NODES - 1].right = KTREE_INVALID_NODE;
    pool->nodes[KTREE_MAX_NODES - 1].parent = KTREE_INVALID_NODE;
    pool->nodes[KTREE_MAX_NODES - 1].height = 0;
    pool->nodes[KTREE_MAX_NODES - 1].flags = 0;
    
    pool->free_list = 0;
    ktree_atomic_set(&pool->node_count, 0);
    ktree_spinlock_init(&pool->pool_lock);
}

/* Выделение узла из пула */
KTREE_INLINE ktree_error_t ktree_pool_alloc_node(ktree_node_pool_t* pool, 
                                               ktree_node_idx_t* idx) {
    ktree_error_t err = KTREE_SUCCESS;
    ktree_node_idx_t new_idx;
    
    if (KTREE_UNLIKELY(ktree_spinlock_lock(&pool->pool_lock) != KTREE_SUCCESS))
        return KTREE_ERR_LOCK_FAILED;
    
    if (KTREE_UNLIKELY(pool->free_list == KTREE_INVALID_NODE)) {
        err = KTREE_ERR_FULL;
        goto unlock;
    }
    
    new_idx = pool->free_list;
    pool->free_list = pool->nodes[new_idx].left;
    
    /* Подготавливаем узел */
    pool->nodes[new_idx].left = KTREE_INVALID_NODE;
    pool->nodes[new_idx].right = KTREE_INVALID_NODE;
    pool->nodes[new_idx].parent = KTREE_INVALID_NODE;
    pool->nodes[new_idx].height = 1;
    pool->nodes[new_idx].flags = 0;
    
    ktree_atomic_inc(&pool->node_count);
    *idx = new_idx;
    
unlock:
    ktree_spinlock_unlock(&pool->pool_lock);
    return err;
}

/* Инициализация узла с данными */
KTREE_INLINE void ktree_node_init(ktree_node_pool_t* pool, 
                                ktree_node_idx_t idx, 
                                ktree_data_t data) {
    ktree_node_t* node = &pool->nodes[idx];
    node->data = data;
    node->hash = (ktree_hash_t)data; /* Простой хэш для примера */
}

/* Освобождение узла и возврат в пул */
KTREE_INLINE void ktree_pool_free_node(ktree_node_pool_t* pool, 
                                     ktree_node_idx_t idx) {
    if (KTREE_UNLIKELY(idx == KTREE_INVALID_NODE))
        return;
    
    ktree_spinlock_lock(&pool->pool_lock);
    
    /* Возврат в список свободных */
    pool->nodes[idx].left = pool->free_list;
    pool->nodes[idx].right = KTREE_INVALID_NODE;
    pool->nodes[idx].parent = KTREE_INVALID_NODE;
    pool->nodes[idx].flags = 0;
    pool->free_list = idx;
    
    ktree_atomic_dec(&pool->node_count);
    
    ktree_spinlock_unlock(&pool->pool_lock);
}

/* Получение узла по индексу */
KTREE_INLINE ktree_node_t* ktree_pool_get_node(ktree_node_pool_t* pool, 
                                             ktree_node_idx_t idx) {
    if (KTREE_UNLIKELY(idx == KTREE_INVALID_NODE))
        return NULL;
    
    KTREE_PREFETCH(&pool->nodes[idx]);
    return &pool->nodes[idx];
}

/* Получить количество активных узлов */
KTREE_PURE KTREE_INLINE uint32_t ktree_pool_node_count(ktree_node_pool_t* pool) {
    return ktree_atomic_read(&pool->node_count);
}

/*
 * ============================================================================
 * ОПЕРАЦИИ С ДЕРЕВЬЯМИ
 * ============================================================================
 */

/* Инициализация дерева */
KTREE_INLINE void ktree_init(ktree_tree_t* tree, uint32_t flags) {
    tree->root = KTREE_INVALID_NODE;
    ktree_atomic_set(&tree->size, 0);
    ktree_spinlock_init(&tree->tree_lock);
    tree->flags = flags;
    tree->height = 0;
}

/* Создание нового дерева в менеджере */
KTREE_INLINE ktree_error_t ktree_manager_create_tree(ktree_manager_t* mgr, 
                                                  uint32_t flags, 
                                                  uint32_t* tree_id) {
    uint32_t i;
    ktree_error_t err = KTREE_SUCCESS;
    
    if (KTREE_UNLIKELY(ktree_spinlock_lock(&mgr->mgr_lock) != KTREE_SUCCESS))
        return KTREE_ERR_LOCK_FAILED;
    
    uint32_t count = ktree_atomic_read(&mgr->tree_count);
    if (KTREE_UNLIKELY(count >= KTREE_MAX_TREES)) {
        err = KTREE_ERR_FULL;
        goto unlock;
    }
    
    /* Находим свободный слот */
    for (i = 0; i < KTREE_MAX_TREES; i++) {
        if (mgr->trees[i].root == KTREE_INVALID_NODE && 
            ktree_atomic_read(&mgr->trees[i].size) == 0) {
            
            ktree_init(&mgr->trees[i], flags);
            ktree_atomic_inc(&mgr->tree_count);
            *tree_id = i;
            goto unlock;
        }
    }
    
    err = KTREE_ERR_FULL;
    
unlock:
    ktree_spinlock_unlock(&mgr->mgr_lock);
    return err;
}

/* Инициализация менеджера деревьев */
KTREE_INLINE void ktree_manager_init(ktree_manager_t* mgr) {
    uint32_t i;
    
    ktree_pool_init(&mgr->node_pool);
    
    for (i = 0; i < KTREE_MAX_TREES; i++)
        ktree_init(&mgr->trees[i], 0);
    
    ktree_atomic_set(&mgr->tree_count, 0);
    ktree_spinlock_init(&mgr->mgr_lock);
    mgr->flags = 0;
}

/* Добавление узла в дерево (базовая BST вставка) */
KTREE_INLINE ktree_error_t ktree_insert(ktree_manager_t* mgr,
                                      uint32_t tree_id,
                                      ktree_data_t data) {
    ktree_node_idx_t new_idx, parent_idx, curr_idx;
    ktree_error_t err;
    ktree_tree_t* tree;
    
    if (KTREE_UNLIKELY(tree_id >= KTREE_MAX_TREES))
        return KTREE_ERR_INVALID;
    
    tree = &mgr->trees[tree_id];
    
    if (KTREE_UNLIKELY(ktree_spinlock_lock(&tree->tree_lock) != KTREE_SUCCESS))
        return KTREE_ERR_LOCK_FAILED;
    
    /* Выделяем новый узел */
    err = ktree_pool_alloc_node(&mgr->node_pool, &new_idx);
    if (KTREE_UNLIKELY(err != KTREE_SUCCESS))
        goto unlock_tree;
    
    ktree_node_init(&mgr->node_pool, new_idx, data);
    
    /* Если дерево пустое, устанавливаем корень */
    if (KTREE_UNLIKELY(tree->root == KTREE_INVALID_NODE)) {
        tree->root = new_idx;
        ktree_atomic_set(&tree->size, 1);
        tree->height = 1;
        
        ktree_node_t* root_node = ktree_pool_get_node(&mgr->node_pool, new_idx);
        root_node->flags |= KTREE_FLAG_ROOT;
        
        goto unlock_tree;
    }
    
    /* Вставка в непустое дерево */
    curr_idx = tree->root;
    parent_idx = KTREE_INVALID_NODE;
    
    while (curr_idx != KTREE_INVALID_NODE) {
        ktree_node_t* curr = ktree_pool_get_node(&mgr->node_pool, curr_idx);
        
        parent_idx = curr_idx;
     
