#ifndef SINGLETON_PTHREAD_ONCE_H
#define SINGLETON_PTHREAD_ONCE_H

#include <stdint.h>

/**
 * PCI 设备管理器 - 单例模式示例
 * 
 * 本模块演示如何使用 pthread_once 实现线程安全的懒汉式单例。
 * 在 Linux 内核中，类似的模式用于管理全局设备列表。
 */
typedef struct {
    uint32_t device_count;   // 已扫描到的设备数量
    void *driver_data;       // 驱动私有数据指针
} PCIDeviceManager;

/**
 * 获取 PCI 设备管理器实例
 * 
 * 使用 pthread_once 保证初始化函数在整个程序生命周期只执行一次。
 * 第一次调用时创建实例，后续调用直接返回已有实例。
 * 
 * @return PCIDeviceManager* 全局唯一实例指针
 */
PCIDeviceManager* pci_manager_get_instance(void);

#endif // SINGLETON_PTHREAD_ONCE_H