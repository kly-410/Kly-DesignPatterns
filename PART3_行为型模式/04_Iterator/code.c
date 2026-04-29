/**
 * ===========================================================================
 * 迭代器模式 — Iterator Pattern
 * ===========================================================================
 *
 * 核心思想：提供一种方法顺序访问集合元素，不暴露底层表示
 *
 * 本代码演示（嵌入式视角）：
 * 1. 数组和链表如何用同一个接口遍历
 * 2. PCIe 数据包缓冲区的遍历（队列、环形buffer）
 * 3. 嵌套结构（树）的深度/广度优先遍历
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* ============================================================================
 * 第一部分：统一迭代器接口
 *
 * 迭代器模式的核心：
 *   容器提供一个 create_iterator() 方法，返回迭代器
 *   迭代器提供 hasNext() 和 next() 两个操作
 *   客户端代码只和迭代器接口打交道，不关心容器内部结构
 * ============================================================================*/

/** 迭代器抽象接口：两个方法搞定遍历 */
typedef struct _IIterator {
    int (*hasNext)(struct _IIterator*);   /**< 还有下一个元素吗 */
    void* (*next)(struct _IIterator*);    /**< 返回下一个元素 */
} IIterator;

/* ============================================================================
 * 第二部分：数组迭代器
 *
 * 场景：PCIe 驱动中有大量寄存器数组、配置表
 *       用迭代器统一遍历方式
 * ============================================================================*/

typedef struct {
    IIterator base;  /**< 继承迭代器接口 */
    int* array;      /**< 指向底层数组 */
    int size;        /**< 数组长度 */
    int index;       /**< 当前遍历位置 */
} ArrayIterator;

/** 还有下一个元素吗 */
static int ArrayIterator_hasNext(IIterator* base) {
    ArrayIterator* it = (ArrayIterator*)base;
    return it->index < it->size;  /**< index 还没到末尾就还有元素 */
}

/** 返回下一个元素 */
static void* ArrayIterator_next(IIterator* base) {
    ArrayIterator* it = (ArrayIterator*)base;
    if (!ArrayIterator_hasNext(base)) return NULL;  /**< 越界保护 */
    return &it->array[it->index++];  /**< 返回元素后 index++ */
}

/** 数组迭代器构造函数 */
static IIterator* ArrayIterator_create(int* array, int size) {
    ArrayIterator* it = malloc(sizeof(ArrayIterator));
    it->base.hasNext = ArrayIterator_hasNext;
    it->base.next = ArrayIterator_next;
    it->array = array;
    it->size = size;
    it->index = 0;
    return &it->base;
}

/* ============================================================================
 * 第三部分：链表节点 & 链表迭代器
 *
 * 场景：DMA 描述符链表、PCIe 事务链表
 *       链表节点不能随机访问，只能顺序遍历
 * ============================================================================*/

typedef struct _ListNode {
    int data;                 /**< 节点数据（可以是任意类型，这里用 int 简化） */
    struct _ListNode* next;  /**< 指向下一个节点，NULL 表示链表结尾 */
} ListNode;

typedef struct {
    IIterator base;       /**< 继承迭代器接口 */
    ListNode* current;    /**< 当前遍历到的节点 */
} ListIterator;

static int ListIterator_hasNext(IIterator* base) {
    ListIterator* it = (ListIterator*)base;
    return it->current != NULL;  /**< current 不为 NULL 就还有下一个 */
}

static void* ListIterator_next(IIterator* base) {
    ListIterator* it = (ListIterator*)base;
    if (it->current == NULL) return NULL;  /**< 已遍历完 */
    void* result = &it->current->data;     /**< 保存当前节点数据 */
    it->current = it->current->next;        /**< 移动到下一个节点 */
    return result;
}

/** 链表迭代器构造函数 */
static IIterator* ListIterator_create(ListNode* head) {
    ListIterator* it = malloc(sizeof(ListIterator));
    it->base.hasNext = ListIterator_hasNext;
    it->base.next = ListIterator_next;
    it->current = head;
    return &it->base;
}

/* ============================================================================
 * 第四部分：统一遍历函数
 *
 * 关键：C 语言没有泛型，迭代器返回 void*，
 *       调用方需要自行转换成正确类型
 * ============================================================================*/

/**
 * 通用遍历函数：打印所有整数
 * @param it       迭代器指针
 * @param type_str 集合类型名（用于打印提示）
 *
 * 注意：这个函数完全不关心底层是数组还是链表
 *      只要迭代器实现了 hasNext/next，就能正确遍历
 */
static void foreach_int(IIterator* it, const char* type_str) {
    printf("  遍历 [%s]: ", type_str);
    int count = 0;
    while (it->hasNext(it)) {
        int* val = (int*)it->next(it);  /**< void* 转成 int* 再解引用 */
        printf("%d ", *val);
        count++;
    }
    printf("(共 %d 个元素)\n", count);
}

/* ============================================================================
 * 第五部分：环形缓冲区迭代器
 *
 * 场景：UART DMA 环形缓冲区、网络包接收队列
 *       写入是顺序的，读取也是顺序的
 * ============================================================================*/

typedef struct {
    IIterator base;
    int* buffer;     /**< 环形缓冲区底层数组 */
    int capacity;    /**< 缓冲区容量 */
    int read_pos;    /**< 当前读位置 */
    int count;       /**< 缓冲区中有效元素数量 */
    int iter_count;  /**< 迭代器已遍历计数 */
} RingBufferIterator;

static int RingBufferIterator_hasNext(IIterator* base) {
    RingBufferIterator* it = (RingBufferIterator*)base;
    return it->iter_count < it->count;  /**< 还没遍历完所有有效数据 */
}

static void* RingBufferIterator_next(IIterator* base) {
    RingBufferIterator* it = (RingBufferIterator*)base;
    if (it->iter_count >= it->count) return NULL;
    /**< 计算实际数组下标：读位置 + 已读数量，折返到容量 */
    int idx = (it->read_pos + it->iter_count) % it->capacity;
    it->iter_count++;
    return &it->buffer[idx];
}

static IIterator* RingBufferIterator_create(int* buffer, int capacity, int read_pos, int count) {
    RingBufferIterator* it = malloc(sizeof(RingBufferIterator));
    it->base.hasNext = RingBufferIterator_hasNext;
    it->base.next = RingBufferIterator_next;
    it->buffer = buffer;
    it->capacity = capacity;
    it->read_pos = read_pos;
    it->count = count;
    it->iter_count = 0;
    return &it->base;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                 Iterator Pattern — 迭代器模式\n");
    printf("=================================================================\n\n");

    /* -------------------- 示例1：数组遍历 -------------------- */
    printf("【示例1】数组迭代器 — PCIe BAR 配置表遍历\n");
    printf("-----------------------------------------------------------------\n");
    int pcie_bars[] = { 0xFE400000, 0xFE402000, 0x0, 0x0, 0xFE403000 };  /**< 模拟 BAR0~BAR4 */
    int bar_count = (int)(sizeof(pcie_bars) / sizeof(pcie_bars[0]));
    IIterator* arr_it = ArrayIterator_create(pcie_bars, bar_count);
    foreach_int(arr_it, "PCIe BARs");
    free(arr_it);

    /* -------------------- 示例2：链表遍历 -------------------- */
    printf("\n【示例2】链表迭代器 — DMA 描述符链表\n");
    printf("-----------------------------------------------------------------\n");
    /**< 构建 DMA 描述符链表：手动创建节点 */
    ListNode n3 = { .data = 30, .next = NULL };
    ListNode n2 = { .data = 20, .next = &n3 };
    ListNode n1 = { .data = 10, .next = &n2 };
    IIterator* list_it = ListIterator_create(&n1);  /**< 链表头是 n1 */
    foreach_int(list_it, "DMA Descriptors");
    free(list_it);

    /* -------------------- 示例3：环形缓冲区遍历 -------------------- */
    printf("\n【示例3】环形缓冲区迭代器 — UART 接收队列\n");
    printf("-----------------------------------------------------------------\n");
    /**< 模拟 UART DMA 环形缓冲区，容量 8，写入 5 个字节 */
    int rx_buffer[8] = { 0x55, 0xAA, 0x12, 0x34, 0xDE, 0xAD, 0xBE, 0xEF };
    int rx_read_pos = 1;   /**< 读指针在 index=1 */
    int rx_valid_count = 5; /**< 有 5 个有效字节 */
    IIterator* ring_it = RingBufferIterator_create(rx_buffer, 8, rx_read_pos, rx_valid_count);
    foreach_int(ring_it, "UART RX Buffer");
    free(ring_it);

    printf("\n=================================================================\n");
    printf(" 迭代器核心：遍历逻辑和容器结构分离\n");
    printf(" 嵌入式场景：DMA描述符链、环形缓冲区、配置表\n");
    printf("=================================================================\n");
    return 0;
}
