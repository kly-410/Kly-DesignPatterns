/**
 * 以太网控制器设备工厂实现
 */

#include "eth_factory.h"
#include <stdio.h>
#include <stdlib.h>

static IDevice* factory_create(IFactory *factory);
static const char* factory_get_name(IFactory *factory);
static void factory_destroy(IFactory *factory);

EthFactory* EthFactory_create(void) {
    EthFactory *factory = malloc(sizeof(EthFactory));
    if (factory == NULL) {
        fprintf(stderr, "[EthFactory] 错误：分配内存失败\n");
        return NULL;
    }
    
    factory->base.create = factory_create;
    factory->base.get_name = factory_get_name;
    factory->base.destroy = factory_destroy;
    factory->name = "Ethernet Factory";
    
    printf("[EthFactory] 以太网工厂实例创建完成\n");
    return factory;
}

static IDevice* factory_create(IFactory *factory) {
    (void)factory;
    
    printf("[EthFactory] 工厂方法 create() 被调用，创建以太网设备\n");
    
    EthDevice *eth_dev = EthDevice_create();
    if (eth_dev == NULL) {
        return NULL;
    }
    
    return &eth_dev->base;
}

static const char* factory_get_name(IFactory *factory) {
    EthFactory *eth_factory = (EthFactory*)factory;
    return eth_factory->name;
}

static void factory_destroy(IFactory *factory) {
    printf("[EthFactory] 销毁工厂实例\n");
    free(factory);
}