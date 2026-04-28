/**
 * pthread_once 单例实现 - 线程安全的懒汉式单例
 * 
 * 本实现使用 pthread_once 来保证初始化函数只被执行一次。
 * pthread_once 是 Linux/POSIX 系统中线程安全的标准实现方式。
 * 
 * 对比三种实现方式：
 * - 饿汉式：程序加载时创建，天然线程安全，但有启动延迟
 * - 懒汉式（DCL）：延迟加载，但双检锁实现复杂且容易出错
 * - pthread_once：延迟加载 + 线程安全 + 实现简单（推荐）
 */

#include "singleton_pthread_once.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* 模块级静态变量 */
static PCIDeviceManager *g_manager = NULL;

/**
 * pthread_once 控制块
 * 
 * PTHREAD_ONCE_INIT 是标准初始值，ensure_init() 只会被调用一次，
 * 无论有多少个线程同时调用 pci_manager_get_instance()。
 */
static pthread_once_t g_once_control = PTHREAD_ONCE_INIT;

/**
 * 单例初始化函数（仅执行一次）
 * 
 * 这是实际创建单例实例的函数，由 pthread_once 保证只调用一次。
 * 在这个函数里通常做：
 * - 分配内存
 * - 初始化成员变量
 * - 打开硬件资源（打开设备文件、映射寄存器等）
 */
static void init_manager(void) {
    g_manager = malloc(sizeof(PCIDeviceManager));
    if (g_manager == NULL) {
        fprintf(stderr, "[ERROR] Failed to allocate PCIDeviceManager: %s\n", 
                strerror(errno));
        return;
    }
    
    /* 初始化成员变量 */
    memset(g_manager, 0, sizeof(PCIDeviceManager));
    g_manager->device_count = 0;
    g_manager->driver_data = NULL;
    
    printf("[INFO] PCI 设备管理器初始化完成（线程安全，延迟加载）\n");
}

/**
 * 获取 PCI 设备管理器实例
 * 
 * 这是单例模式的唯一全局访问点。
 * 线程安全：pthread_once 保证即使多个线程同时调用，也不会重复初始化。
 * 
 * @return PCIDeviceManager* 全局唯一实例指针
 */
PCIDeviceManager* pci_manager_get_instance(void) {
    int err = pthread_once(&g_once_control, init_manager);
    if (err != 0) {
        fprintf(stderr, "[ERROR] pthread_once failed: %s\n", strerror(err));
        return NULL;
    }
    return g_manager;
}