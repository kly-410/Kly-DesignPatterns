/**
 * 以太网控制器设备实现
 */

#include "eth_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void eth_init(IDevice *device, uint64_t base_addr);
static void eth_start(IDevice *device);
static void eth_stop(IDevice *device);
static const char* eth_get_name(IDevice *device);
static const char* eth_get_type(IDevice *device);
static void eth_destroy(IDevice *device);

EthDevice* EthDevice_create(void) {
    EthDevice *dev = malloc(sizeof(EthDevice));
    if (dev == NULL) {
        fprintf(stderr, "[Eth] 错误：分配内存失败\n");
        return NULL;
    }
    
    memset(dev, 0, sizeof(EthDevice));
    
    /* 设置基类接口 */
    dev->base.init = eth_init;
    dev->base.start = eth_start;
    dev->base.stop = eth_stop;
    dev->base.get_name = eth_get_name;
    dev->base.get_type = eth_get_type;
    dev->base.destroy = eth_destroy;
    dev->base.private_data = dev;
    
    /* 初始化子类数据 */
    dev->name = "Gigabit Ethernet Controller";
    dev->type = "Ethernet";
    dev->bar_base_addr = 0;
    dev->link_speed = 1000;
    dev->max_link_speed = 10000;
    dev->initialized = 0;
    dev->running = 0;
    dev->tx_queue_count = 16;
    dev->rx_queue_count = 16;
    
    printf("[Eth] 以太网控制器实例创建完成\n");
    return dev;
}

static void eth_init(IDevice *device, uint64_t base_addr) {
    EthDevice *dev = (EthDevice*)device->private_data;
    dev->bar_base_addr = base_addr;
    
    printf("[Eth] 初始化以太网控制器\n");
    printf("       BAR 地址: 0x%016lx\n", base_addr);
    printf("       链路速度: %d Mbps\n", dev->link_speed);
    printf("       TX 队列数: %d\n", dev->tx_queue_count);
    printf("       RX 队列数: %d\n", dev->rx_queue_count);
    
    dev->initialized = 1;
    printf("[Eth] 初始化完成 ✓\n");
}

static void eth_start(IDevice *device) {
    EthDevice *dev = (EthDevice*)device->private_data;
    
    if (!dev->initialized) {
        printf("[Eth] 错误：设备未初始化！请先调用 init()\n");
        return;
    }
    
    printf("[Eth] 启动以太网控制器\n");
    printf("       协商链路速度: %d Mbps\n", dev->link_speed);
    printf("       使能 TX/RX 队列...\n");
    printf("       开始收发数据包\n");
    
    dev->running = 1;
    printf("[Eth] 设备已启动 ✓\n");
}

static void eth_stop(IDevice *device) {
    (void)device;
    printf("[Eth] 停止以太网控制器\n");
    printf("       停止 TX/RX 队列...\n");
    printf("       设备已停止 ✓\n");
}

static const char* eth_get_name(IDevice *device) {
    EthDevice *dev = (EthDevice*)device->private_data;
    return dev->name;
}

static const char* eth_get_type(IDevice *device) {
    EthDevice *dev = (EthDevice*)device->private_data;
    return dev->type;
}

static void eth_destroy(IDevice *device) {
    printf("[Eth] 销毁以太网控制器实例\n");
    free(device->private_data);
}