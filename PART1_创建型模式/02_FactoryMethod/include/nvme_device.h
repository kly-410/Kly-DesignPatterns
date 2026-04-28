/**
 * NVMe SSD 设备 - 具体产品
 * 
 * 实现了 IDevice 接口的 NVMe 固态硬盘控制器。
 * 
 * 设计说明：
 * - 使用"组合"方式实现接口（包含 IDevice base）
 * - 通过 private_data 指向自身来实现接口方法的多态调用
 */
#ifndef NVME_DEVICE_H
#define NVME_DEVICE_H

#include "pcie_device.h"

/**
 * NVMe 设备结构体
 * 
 * 包含两部分：
 * 1. IDevice base：继承接口
 * 2. NVMe 特有数据
 */
typedef struct {
    IDevice base;                    // 基类接口（组合）
    const char *name;                // 设备名称
    const char *type;                // 设备类型
    uint64_t bar_base_addr;         // BAR 空间基地址
    int queue_depth;                 // 提交队列深度
    int max_queue_depth;            // 最大队列深度
    int initialized;                // 是否已初始化
    int running;                    // 是否已启动
} NVMeDevice;

/**
 * 创建 NVMe 设备实例
 * 
 * 对应工厂方法的"构造函数"。
 * 在具体工厂的 create() 方法中调用此函数创建产品。
 * 
 * @return NVMeDevice* 分配好的设备实例（已设置好接口）
 */
NVMeDevice* NVMeDevice_create(void);

#endif // NVME_DEVICE_H