/**
 * ===========================================================================
 * Iterator Pattern — 迭代器模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* ============================================================================
 * 第一部分：Array + LinkedList Iterator
 * ============================================================================*/

typedef struct _IIterator {
    int (*hasNext)(struct _IIterator*);
    void* (*next)(struct _IIterator*);
} IIterator;

typedef struct _ArrayIterator {
    IIterator base;
    int* array;
    int size;
    int index;
} ArrayIterator;

static int ArrayIterator_hasNext(IIterator* base) {
    ArrayIterator* it = (ArrayIterator*)base;
    return it->index < it->size;
}

static void* ArrayIterator_next(IIterator* base) {
    ArrayIterator* it = (ArrayIterator*)base;
    if (!ArrayIterator_hasNext(base)) return NULL;
    return &it->array[it->index++];
}

static ArrayIterator* ArrayIterator_new(int* array, int size) {
    ArrayIterator* it = malloc(sizeof(ArrayIterator));
    it->base.hasNext = ArrayIterator_hasNext;
    it->base.next = ArrayIterator_next;
    it->array = array;
    it->size = size;
    it->index = 0;
    return it;
}

typedef struct _ListNode {
    int data;
    struct _ListNode* next;
} ListNode;

typedef struct _LinkedListIterator {
    IIterator base;
    ListNode* current;
} LinkedListIterator;

static int LinkedListIterator_hasNext(IIterator* base) {
    LinkedListIterator* it = (LinkedListIterator*)base;
    return it->current != NULL;
}

static void* LinkedListIterator_next(IIterator* base) {
    LinkedListIterator* it = (LinkedListIterator*)base;
    if (!LinkedListIterator_hasNext(base)) return NULL;
    ListNode* node = it->current;
    it->current = it->current->next;
    return &node->data;
}

static LinkedListIterator* LinkedListIterator_new(ListNode* head) {
    LinkedListIterator* it = malloc(sizeof(LinkedListIterator));
    it->base.hasNext = LinkedListIterator_hasNext;
    it->base.next = LinkedListIterator_next;
    it->current = head;
    return it;
}

/* ============================================================================
 * 第二部分：Linux list_head
 * ============================================================================*/

#define container_of(ptr, T, member) \
    ((T*)((char*)(ptr) - offsetof(T, member)))

typedef struct _list_head {
    struct _list_head *next;
    struct _list_head *prev;
} list_head;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

static inline void list_add(struct _list_head* new_node, struct _list_head* head) {
    struct _list_head* next = head->next;
    new_node->prev = head;
    new_node->next = next;
    next->prev = new_node;
    head->next = new_node;
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

typedef struct _PCIDriver {
    list_head node;
    int irq;
    char name[32];
} PCIDriver;

static PCIDriver* PCIDriver_new(const char* name, int irq) {
    PCIDriver* drv = malloc(sizeof(PCIDriver));
    drv->irq = irq;
    strncpy(drv->name, name, 31);
    return drv;
}

static list_head driver_list = LIST_HEAD_INIT(driver_list);

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                    Iterator Pattern — 迭代器模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Array + LinkedList Iterator */
    printf("【示例1】Array + LinkedList Iterator\n");
    printf("-----------------------------------------------------------------\n");
    int arr[] = {10, 20, 30, 40, 50};
    ArrayIterator* it = ArrayIterator_new(arr, 5);
    printf("  Array traversal: ");
    while (it->base.hasNext(&it->base)) {
        int* val = (int*)it->base.next(&it->base);
        printf("%d ", *val);
    }
    printf("\n");
    free(it);

    ListNode nodes[5];
    for (int i = 0; i < 5; i++) {
        nodes[i].data = (i + 1) * 10;
        nodes[i].next = (i < 4) ? &nodes[i + 1] : NULL;
    }
    LinkedListIterator* lit = LinkedListIterator_new(&nodes[0]);
    printf("  LinkedList traversal: ");
    while (lit->base.hasNext(&lit->base)) {
        int* val = (int*)lit->base.next(&lit->base);
        printf("%d ", *val);
    }
    printf("\n");
    free(lit);

    /* 示例2：Linux list_head */
    printf("\n【示例2】Linux list_head — 内核双向链表\n");
    printf("-----------------------------------------------------------------\n");
    PCIDriver* drv1 = PCIDriver_new("RTL8111", 17);
    PCIDriver* drv2 = PCIDriver_new("X710", 18);
    PCIDriver* drv3 = PCIDriver_new("AXI_DMA", 19);

    list_add(&drv1->node, &driver_list);
    list_add(&drv2->node, &driver_list);
    list_add(&drv3->node, &driver_list);

    printf("  Registered drivers:\n");
    list_head* pos;
    list_for_each(pos, &driver_list) {
        PCIDriver* drv = container_of(pos, PCIDriver, node);
        printf("    - %s: IRQ=%d\n", drv->name, drv->irq);
    }

    free(drv1); free(drv2); free(drv3);

    printf("\n=================================================================\n");
    printf(" Iterator 模式核心：hasNext() + next() 函数指针接口\n");
    printf(" Linux 内核应用：list_head 遍历、hlist、radix-tree\n");
    printf("=================================================================\n");
    return 0;
}
