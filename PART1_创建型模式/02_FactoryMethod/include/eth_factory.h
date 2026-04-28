/**
 * 以太网控制器设备工厂 - 具体工厂
 */
#ifndef ETH_FACTORY_H
#define ETH_FACTORY_H

#include "device_factory.h"
#include "eth_device.h"

typedef struct {
    IFactory base;
    const char *name;
} EthFactory;

EthFactory* EthFactory_create(void);

#endif // ETH_FACTORY_H