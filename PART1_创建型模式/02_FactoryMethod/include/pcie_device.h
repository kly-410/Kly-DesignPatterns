/**
 * PCIe 设备抽象接口
 * 
 * 本文件定义所有 PCIe 设备的统一接口（抽象产品角色）。
 * 在 C 语言中，"接口"通过函数指针结构体来模拟。
 * 
 * 工厂方法模式的产品角色（Product）
 */
#ifndef PCIE_DEVICE_H
#define PCIE_DEVICE_H

#include <stdint.h>

/* 前向声明 */
typedef struct IDevice IDevice;

/**
 * 设备抽象接口
 * 
 * 所有具体的 PCIe 设备（NVMe、网卡、USB 等）都实现这个接口。
 * 使用"组合"方式：C++ 中的继承在 C 中通过包含基类结构体来实现。
 */
struct IDevice {
    /**
     * 初始化设备
     * @param base_addr 设备寄存器基地址（BAR 地址）
     */
    void (*init)(IDevice *device, uint64_t base_addr);
    
    /**
     * 启动设备
     */
    void (*start)(IDevice *device);
    
    /**
     * 停止设备
     */
    void (*stop)(IDevice *device);
    
    /**
     * 获取设备名称
     * @return 设备名称字符串
     */
    const char* (*get_name)(IDevice *device);
    
    /**
     * 获取设备类型
     * @return 设备类型标识
     */
    const char* (*get_type)(IDevice *device);
    
    /**
     * 销毁设备，释放资源
     */
    void (*destroy)(IDevice *device);
    
    /**
     * 设备特定数据（子类数据的通用入口）
     * 
     * 这是实现"多态"的关键：
     * 每个具体设备把自己的指针存在这里，
     * 接口方法通过这个指针访问具体的子类数据。
     */
    void *private_data;
};

#endif // PCIE_DEVICE_H