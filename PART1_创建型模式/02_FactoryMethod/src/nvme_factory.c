/**
 * NVMe 设备工厂实现
 */

#include "nvme_factory.h"
#include <stdio.h>
#include <stdlib.h>

/* 内部函数 */
static IDevice* factory_create(IFactory *factory);
static const char* factory_get_name(IFactory *factory);
static void factory_destroy(IFactory *factory);

/* ========== 构造函数 ========== */
NVMeFactory* NVMeFactory_create(void) {
    NVMeFactory *factory = malloc(sizeof(NVMeFactory));
    if (factory == NULL) {
        fprintf(stderr, "[NVMeFactory] 错误：分配内存失败\n");
        return NULL;
    }
    
    /* 初始化基类接口 */
    factory->base.create = factory_create;
    factory->base.get_name = factory_get_name;
    factory->base.destroy = factory_destroy;
    
    factory->name = "NVMe Factory";
    
    printf("[NVMeFactory] 工厂实例创建完成\n");
    return factory;
}

/* ========== IFactory 接口实现 ========== */

/**
 * 工厂方法：创建 NVMe 设备
 * 
 * 这里体现了工厂方法模式的核心理念：
 * "延迟到子类决定创建哪个产品"
 * ——具体工厂决定创建具体产品
 */
static IDevice* factory_create(IFactory *factory) {
    NVMeFactory *nvme_factory = (NVMeFactory*)factory;
    (void)nvme_factory;
    
    printf("[NVMeFactory] 工厂方法 create() 被调用，创建 NVMe 设备\n");
    
    /* 创建 NVMe 设备并返回接口指针 */
    NVMeDevice *nvme_dev = NVMeDevice_create();
    if (nvme_dev == NULL) {
        return NULL;
    }
    
    /* 返回 IDevice 接口指针（隐藏具体类型）*/
    return &nvme_dev->base;
}

static const char* factory_get_name(IFactory *factory) {
    NVMeFactory *nvme_factory = (NVMeFactory*)factory;
    return nvme_factory->name;
}

static void factory_destroy(IFactory *factory) {
    printf("[NVMeFactory] 销毁工厂实例\n");
    free(factory);
}