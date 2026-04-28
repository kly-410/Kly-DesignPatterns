/**
 * 设备工厂抽象接口
 * 
 * 本文件定义创建设备的工厂接口（抽象工厂角色）。
 * 
 * 工厂方法模式的核心：抽象工厂把产品创建延迟到子类。
 */
#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "pcie_device.h"

/* 前向声明 */
typedef struct IFactory IFactory;

/**
 * 工厂抽象接口
 * 
 * 定义创建 PCIe 设备的方法。
 * 具体工厂（NVMe 工厂、网卡工厂等）实现这个接口。
 */
struct IFactory {
    /**
     * 创建设备实例
     * 
     * 这是工厂方法模式的核心方法。
     * 每个具体工厂实现这个方法，创建对应的具体产品。
     * 
     * @return 实现了 IDevice 接口的具体设备对象
     */
    IDevice* (*create)(IFactory *factory);
    
    /**
     * 获取工厂名称
     */
    const char* (*get_name)(IFactory *factory);
    
    /**
     * 销毁工厂
     */
    void (*destroy)(IFactory *factory);
};

#endif // DEVICE_FACTORY_H