/**
 * 工厂方法模式测试程序
 * 
 * 演示如何使用工厂方法创建不同类型的 PCIe 设备。
 * 
 * 关键点：
 * 1. 通过抽象工厂接口操作具体工厂（解耦）
 * 2. 通过抽象产品接口操作具体产品（多态）
 * 3. 新增设备类型时，不需要修改已有代码（开闭原则）
 */

#include "device_factory.h"
#include "nvme_factory.h"
#include "eth_factory.h"
#include <stdio.h>

/**
 * 通用设备初始化函数
 * 
 * 这个函数只依赖 IDevice 接口，不知道具体是什么设备。
 * 这就是"针对接口编程，而不是针对实现编程"。
 */
void init_device(IDevice *device, uint64_t bar_addr) {
    printf("----------------------------------------\n");
    printf("初始化设备: %s (类型: %s)\n", 
           device->get_name(device), device->get_type(device));
    device->init(device, bar_addr);
}

/**
 * 通用设备启动函数
 */
void start_device(IDevice *device) {
    device->start(device);
    printf("----------------------------------------\n\n");
}

int main(void) {
    printf("========================================\n");
    printf("   工厂方法模式 - PCIe 设备示例\n");
    printf("========================================\n\n");
    
    /* ========================================
     * 第一步：创建工厂
     * ======================================== */
    printf("=== 创建工厂 ===\n");
    
    NVMeFactory *nvme_factory = NVMeFactory_create();
    IFactory *factory = &nvme_factory->base;  // 抽象工厂指针
    
    EthFactory *eth_factory = EthFactory_create();
    IFactory *eth_base = &eth_factory->base;
    
    printf("\n");
    
    /* ========================================
     * 第二步：通过工厂创建产品
     * ======================================== */
    printf("=== 通过工厂创建设备 ===\n");
    
    /* 工厂创建产品，返回的是抽象产品接口 */
    IDevice *nvme = factory->create(factory);
    printf("创建的产品: %s\n", nvme->get_name(nvme));
    
    IDevice *eth = eth_base->create(eth_base);
    printf("创建的产品: %s\n", eth->get_name(eth));
    
    printf("\n");
    
    /* ========================================
     * 第三步：使用设备
     * ======================================== */
    printf("=== 使用设备（多态调用）===\n");
    
    init_device(nvme, 0x10000000000UL);
    start_device(nvme);
    
    init_device(eth, 0x20000000000UL);
    start_device(eth);
    
    /* ========================================
     * 第四步：销毁资源
     * ======================================== */
    printf("=== 清理资源 ===\n");
    
    nvme->destroy(nvme);
    eth->destroy(eth);
    
    factory->destroy(factory);  // 销毁工厂
    eth_base->destroy(eth_base);
    
    printf("\n========================================\n");
    printf("   测试完成 ✓\n");
    printf("========================================\n");
    
    return 0;
}