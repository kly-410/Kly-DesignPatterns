/**
 * 以太网控制器设备 - 具体产品
 */
#ifndef ETH_DEVICE_H
#define ETH_DEVICE_H

#include "pcie_device.h"

typedef struct {
    IDevice base;
    const char *name;
    const char *type;
    uint64_t bar_base_addr;
    int link_speed;          // Mbps: 100, 1000, 10000
    int max_link_speed;
    int initialized;
    int running;
    int tx_queue_count;
    int rx_queue_count;
} EthDevice;

EthDevice* EthDevice_create(void);

#endif // ETH_DEVICE_H