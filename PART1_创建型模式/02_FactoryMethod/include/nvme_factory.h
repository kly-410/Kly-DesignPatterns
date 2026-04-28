/**
 * NVMe 设备工厂 - 具体工厂
 * 
 * 专门创建 NVMe SSD 设备的工厂。
 * 
 * 工厂方法模式：具体工厂负责创建具体产品。
 */
#ifndef NVME_FACTORY_H
#define NVME_FACTORY_H

#include "device_factory.h"
#include "nvme_device.h"

/**
 * NVMe 工厂结构体
 */
typedef struct {
    IFactory base;           // 基类工厂接口
    const char *name;        // 工厂名称
} NVMeFactory;

/**
 * 创建 NVMe 工厂实例
 */
NVMeFactory* NVMeFactory_create(void);

/**
 * 创建 NVMe 设备（工厂方法）
 * 
 * 这是工厂方法模式的核心：
 * 具体的工厂方法实现，决定创建哪个具体产品。
 */
IDevice* NVMeFactory_create_device(NVMeFactory *factory);

#endif // NVME_FACTORY_H