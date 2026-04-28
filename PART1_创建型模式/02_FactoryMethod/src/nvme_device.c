/**
 * NVMe SSD 设备实现
 * 
 * 实现了 IDevice 接口的所有方法。
 * 展示了如何在 C 语言中用函数指针模拟"继承"。
 */

#include "nvme_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 内部函数声明 ========== */
static void nvme_init(IDevice *device, uint64_t base_addr);
static void nvme_start(IDevice *device);
static void nvme_stop(IDevice *device);
static const char* nvme_get_name(IDevice *device);
static const char* nvme_get_type(IDevice *device);
static void nvme_destroy(IDevice *device);

/* ========== 构造函数 ========== */
NVMeDevice* NVMeDevice_create(void) {
    NVMeDevice *dev = malloc(sizeof(NVMeDevice));
    if (dev == NULL) {
        fprintf(stderr, "[NVMe] 错误：分配内存失败\n");
        return NULL;
    }
    
    /* 初始化为 0 */
    memset(dev, 0, sizeof(NVMeDevice));
    
    /* ========== 设置基类接口（实现继承）========== */
    dev->base.init = nvme_init;
    dev->base.start = nvme_start;
    dev->base.stop = nvme_stop;
    dev->base.get_name = nvme_get_name;
    dev->base.get_type = nvme_get_type;
    dev->base.destroy = nvme_destroy;
    dev->base.private_data = dev;  /* 多态关键：指向子类自身 */
    
    /* ========== 初始化子类数据 ========== */
    dev->name = "NVMe SSD Controller";
    dev->type = "NVMe";
    dev->bar_base_addr = 0;
    dev->queue_depth = 32;
    dev->max_queue_depth = 64;
    dev->initialized = 0;
    dev->running = 0;
    
    printf("[NVMe] 设备实例创建完成\n");
    return dev;
}

/* ========== IDevice 接口实现 ========== */

/**
 * 初始化 NVMe 控制器
 * 
 * 执行：
 * 1. 映射 BAR 空间
 * 2. 读取并配置 Controller Capabilities 寄存器
 * 3. 初始化提交队列和完成队列
 */
static void nvme_init(IDevice *device, uint64_t base_addr) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    
    dev->bar_base_addr = base_addr;
    
    /* 模拟 NVMe 初始化序列 */
    printf("[NVMe] 初始化 NVMe SSD 控制器\n");
    printf("       BAR 地址: 0x%016lx\n", base_addr);
    printf("       队列深度: %d\n", dev->queue_depth);
    printf("       最大队列深度: %d\n", dev->max_queue_depth);
    
    /* 模拟寄存器读取 */
    printf("       [寄存器] CAP = 0x%lx\n", base_addr + 0x00);
    printf("       [寄存器] VS  = 0x%lx\n", base_addr + 0x08);
    printf("       [寄存器] CSTS = 0x%lx\n", base_addr + 0x14);
    
    dev->initialized = 1;
    printf("[NVMe] 初始化完成 ✓\n");
}

/**
 * 启动 NVMe 控制器
 * 
 * 执行：
 * 1. 启用控制器
 * 2. 设置 Admin 队列
 * 3. 扫描并报告命名空间
 */
static void nvme_start(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    
    if (!dev->initialized) {
        printf("[NVMe] 错误：设备未初始化！请先调用 init()\n");
        return;
    }
    
    if (dev->running) {
        printf("[NVMe] 警告：设备已经在运行\n");
        return;
    }
    
    printf("[NVMe] 启动 NVMe SSD 控制器\n");
    printf("       [寄存器] CSTS.RDY = 1 (设置就绪位)\n");
    printf("       开始处理 I/O 请求...\n");
    
    dev->running = 1;
    printf("[NVMe] 设备已启动 ✓\n");
}

/**
 * 停止 NVMe 控制器
 * 
 * 执行：
 * 1. 刷新所有待处理 I/O
 * 2. 停止控制器
 */
static void nvme_stop(IDevice *device) {
    (void)device;
    printf("[NVMe] 停止 NVMe SSD 控制器\n");
    printf("       [寄存器] CSTS.RDY = 0 (清除就绪位)\n");
    printf("       刷新所有待处理 I/O 请求...\n");
    printf("       设备已停止 ✓\n");
}

/**
 * 获取设备名称
 */
static const char* nvme_get_name(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    return dev->name;
}

/**
 * 获取设备类型
 */
static const char* nvme_get_type(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    return dev->type;
}

/**
 * 销毁 NVMe 设备
 */
static void nvme_destroy(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    printf("[NVMe] 销毁设备实例\n");
    free(dev);
}