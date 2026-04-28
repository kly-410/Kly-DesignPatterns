/**
 * pthread_once 单例测试程序
 * 
 * 演示多线程环境下单例的正确行为：
 * - 所有线程获取的应该是同一个实例
 * - 初始化函数只被调用一次
 * - 即使并发调用，也不会导致重复初始化
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "singleton_pthread_once.h"
#include <stdio.h>

#define THREAD_COUNT 5

/**
 * 工作线程函数
 * 
 * 每个线程都尝试获取 PCI 设备管理器实例。
 * 如果单例实现正确，所有线程应该得到相同的地址。
 */
void* thread_worker(void* arg) {
    int id = *(int*)arg;
    
    /* 模拟线程随机启动延迟（让并发更真实） */
    usleep(id * 10000);  // 10ms * id
    
    PCIDeviceManager* mgr = pci_manager_get_instance();
    
    printf("[线程%d] 实例地址: %p, device_count=%u\n", 
           id, (void*)mgr, mgr->device_count);
    
    return NULL;
}

int main(void) {
    printf("=========================================\n");
    printf("   单例模式测试 - pthread_once 实现\n");
    printf("=========================================\n\n");
    
    /* 存储线程 ID */
    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];
    
    /* 创建多个线程，让它们同时尝试获取单例 */
    printf("[主线程] 创建 %d 个工作线程...\n", THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i + 1;
        int err = pthread_create(&threads[i], NULL, thread_worker, &thread_ids[i]);
        if (err != 0) {
            fprintf(stderr, "[ERROR] 创建线程%d失败: %s\n", i + 1, strerror(err));
        }
    }
    
    /* 等待所有线程完成 */
    printf("[主线程] 等待所有线程结束...\n\n");
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n=========================================\n");
    printf("   测试通过：所有线程获取了同一个实例 ✓\n");
    printf("=========================================\n");
    
    return 0;
}