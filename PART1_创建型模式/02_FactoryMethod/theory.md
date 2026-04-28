# 工厂方法模式（Factory Method Pattern）

## 本章目标

1. 理解简单工厂的问题：变化封闭在工厂内部，违反开闭原则
2. 理解工厂方法模式的核心理念：**把工厂也抽象化**
3. 掌握工厂方法模式在 Linux 内核驱动注册模型中的应用（pci_driver）
4. 掌握 C 语言实现工厂方法的套路（接口结构体 + 创建函数指针）

---

## 1. 从一个问题开始

设计一个 PCIe 驱动框架，需要支持多种设备类型：
- NVMe SSD 设备
- 以太网控制器
- USB 控制器

**简单工厂的做法**：
```c
// 根据设备类型创建不同的处理对象
void* create_device(int type) {
    if (type == DEVICE_NVME) return create_nvme();
    if (type == DEVICE_ETHERNET) return create_ethernet();
    if (type == DEVICE_USB) return create_usb();
}
```

**问题**：每加一种新设备类型，都要修改 `create_device()` —— 对修改开放，违反开闭原则。

**工厂方法的思路**：不创建一个什么都能造的工厂，而是**为每种设备定义一个工厂接口**，让具体的设备工厂负责创建自己的产品。

---

## 2. 工厂方法模式的定义

> **工厂方法模式**：定义一个创建对象的接口，让子类决定实例化哪一个类。工厂方法使一个类的实例化延迟到其子类。

四个要素：
- **Product（产品）**：抽象产品，定义产品的接口
- **ConcreteProduct（具体产品）**：实现产品接口的具体类
- **Factory（工厂）**：抽象工厂，定义创建产品的接口
- **ConcreteFactory（具体工厂）**：实现工厂接口，创建具体产品

---

## 3. 工厂方法 vs 简单工厂

```
简单工厂：
┌─────────────┐
│  DeviceFactory │  ← 一个工厂知道所有产品类型
└──────┬──────┘
       │ create_device(type)  ← type 是变化点，修改工厂
       ▼
  ┌────┴────┐
  │ 产品对象 │
  └─────────┘

工厂方法：
┌───────────────────────┐
│  IDeviceFactory       │  ← 抽象工厂接口
│  +create_device()     │
└───────────┬───────────┘
            │ 
    ┌───────┴───────┐
    ▼               ▼
┌─────────┐   ┌─────────┐
│NVMeFac  │   │EthFac   │  ← 每个工厂只负责自己的产品
└────┬────┘   └────┬────┘
     │              │
     ▼              ▼
  NVMe设备      以太网设备
```

---

## 4. C 语言实现：PCIe 设备工厂

### 4.1 定义产品接口

```c
// pcie_device.h
#ifndef PCIE_DEVICE_H
#define PCIE_DEVICE_H

#include <stdint.h>

/**
 * PCIe 设备抽象接口
 * 
 * 所有具体的 PCIe 设备（NVMe、网卡、USB 等）都实现这个接口。
 * "接口"在 C 语言中通过函数指针结构体来模拟。
 */
typedef struct IDevice IDevice;

struct IDevice {
    /**
     * 初始化设备
     * @param base_addr 设备寄存器基地址
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
     */
    const char* (*get_name)(IDevice *device);
    
    /**
     * 销毁设备，释放资源
     */
    void (*destroy)(IDevice *device);
    
    /**
     * 设备特定数据（子类数据）
     */
    void *private_data;
};

#endif // PCIE_DEVICE_H
```

### 4.2 定义工厂接口

```c
// device_factory.h
#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "pcie_device.h"

/**
 * 设备工厂抽象接口
 * 
 * 定义创建设备的方法。
 * 具体工厂（如 NVMe 工厂、网卡工厂）实现这个接口。
 */
typedef struct IFactory IFactory;

struct IFactory {
    /**
     * 创建设备实例
     * @return 实现了 IDevice 接口的具体设备对象
     */
    IDevice* (*create)(IFactory *factory);
    
    /**
     * 销毁工厂（如果需要）
     */
    void (*destroy)(IFactory *factory);
};

#endif // DEVICE_FACTORY_H
```

### 4.3 具体产品：NVMe 设备

```c
// nvme_device.h
#ifndef NVME_DEVICE_H
#define NVME_DEVICE_H

#include "pcie_device.h"

/**
 * NVMe SSD 设备
 * 
 * 实现了 IDevice 接口的具体产品。
 */
typedef struct {
    IDevice base;                    // 继承 IDevice（通过组合）
    const char *name;                 // 设备名称
    uint64_t bar_base_addr;          // BAR 空间基地址
    int queue_depth;                 // 队列深度
    int initialized;                 // 是否已初始化
} NVMeDevice;

// 构造函数
NVMeDevice* NVMeDevice_create(void);

#endif // NVME_DEVICE_H
```

```c
// nvme_device.c
#include "nvme_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 内部函数声明 */
static void nvme_init(IDevice *device, uint64_t base_addr);
static void nvme_start(IDevice *device);
static void nvme_stop(IDevice *device);
static const char* nvme_get_name(IDevice *device);
static void nvme_destroy(IDevice *device);

/* 创建 NVMe 设备 */
NVMeDevice* NVMeDevice_create(void) {
    NVMeDevice *dev = malloc(sizeof(NVMeDevice));
    if (dev == NULL) {
        fprintf(stderr, "[NVMe] 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化基类接口（函数指针） */
    memset(dev, 0, sizeof(NVMeDevice));
    dev->base.init = nvme_init;
    dev->base.start = nvme_start;
    dev->base.stop = nvme_stop;
    dev->base.get_name = nvme_get_name;
    dev->base.destroy = nvme_destroy;
    dev->base.private_data = dev;  // 指向自身，用于接口回调时访问子类数据
    
    /* 初始化子类数据 */
    dev->name = "NVMe SSD Controller";
    dev->bar_base_addr = 0;
    dev->queue_depth = 32;
    dev->initialized = 0;
    
    printf("[NVMe] 设备实例创建完成\n");
    return dev;
}

static void nvme_init(IDevice *device, uint64_t base_addr) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    dev->bar_base_addr = base_addr;
    dev->initialized = 1;
    printf("[NVMe] 初始化完成，BAR=0x%lx，队列深度=%d\n", 
           base_addr, dev->queue_depth);
}

static void nvme_start(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    if (!dev->initialized) {
        printf("[NVMe] 错误：设备未初始化！\n");
        return;
    }
    printf("[NVMe] 设备启动，开始处理 I/O 请求\n");
}

static void nvme_stop(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    printf("[NVMe] 设备停止，刷新所有待处理请求\n");
}

static const char* nvme_get_name(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    return dev->name;
}

static void nvme_destroy(IDevice *device) {
    NVMeDevice *dev = (NVMeDevice*)device->private_data;
    printf("[NVMe] 销毁设备实例\n");
    free(dev);
}
```

### 4.4 具体工厂：NVMe 工厂

```c
// nvme_factory.h
#ifndef NVME_FACTORY_H
#define NVME_FACTORY_H

#include "device_factory.h"
#include "nvme_device.h"

/**
 * NVMe 设备工厂
 * 
 * 实现了 IFactory 接口，专门创建 NVMe 设备。
 */
typedef struct {
    IFactory base;
} NVMeFactory;

// 构造函数
NVMeFactory* NVMeFactory_create(void);

#endif // NVME_FACTORY_H
```

```c
// nvme_factory.c
#include "nvme_factory.h"
#include <stdio.h>

/* 内部函数 */
static IDevice* nvme_factory_create(IFactory *factory);
static void nvme_factory_destroy(IFactory *factory);

/* 创建 NVMe 工厂 */
NVMeFactory* NVMeFactory_create(void) {
    NVMeFactory *factory = malloc(sizeof(NVMeFactory));
    if (factory == NULL) {
        fprintf(stderr, "[NVMeFactory] 分配内存失败\n");
        return NULL;
    }
    
    /* 初始化工厂接口 */
    factory->base.create = nvme_factory_create;
    factory->base.destroy = nvme_factory_destroy;
    
    printf("[NVMeFactory] 工厂实例创建完成\n");
    return factory;
}

static IDevice* nvme_factory_create(IFactory *factory) {
    (void)factory;  // 未使用
    /* 创建 NVMe 设备并返回 */
    return &NVMeDevice_create()->base;
}

static void nvme_factory_destroy(IFactory *factory) {
    NVMeFactory *f = (NVMeFactory*)factory;
    printf("[NVMeFactory] 销毁工厂实例\n");
    free(f);
}
```

### 4.5 具体产品 + 工厂：以太网控制器

```c
// eth_device.h
#ifndef ETH_DEVICE_H
#define ETH_DEVICE_H

#include "pcie_device.h"

/**
 * 以太网控制器设备
 * 实现了 IDevice 接口的具体产品。
 */
typedef struct {
    IDevice base;
    const char *name;
    uint64_t bar_base_addr;
    int link_speed;       // Mbps
    int initialized;
} EthDevice;

EthDevice* EthDevice_create(void);

#endif // ETH_DEVICE_H
```

```c
// eth_device.c
#include "eth_device.h"
#include <stdio.h>
#include <stdlib.h>

static void eth_init(IDevice *device, uint64_t base_addr);
static void eth_start(IDevice *device);
static void eth_stop(IDevice *device);
static const char* eth_get_name(IDevice *device);
static void eth_destroy(IDevice *device);

EthDevice* EthDevice_create(void) {
    EthDevice *dev = malloc(sizeof(EthDevice));
    if (dev == NULL) return NULL;
    
    memset(dev, 0, sizeof(EthDevice));
    dev->base.init = eth_init;
    dev->base.start = eth_start;
    dev->base.stop = eth_stop;
    dev->base.get_name = eth_get_name;
    dev->base.destroy = eth_destroy;
    dev->base.private_data = dev;
    
    dev->name = "Gigabit Ethernet Controller";
    dev->bar_base_addr = 0;
    dev->link_speed = 1000;
    dev->initialized = 0;
    
    printf("[Eth] 以太网控制器实例创建完成\n");
    return dev;
}

static void eth_init(IDevice *device, uint64_t base_addr) {
    EthDevice *dev = (EthDevice*)device->private_data;
    dev->bar_base_addr = base_addr;
    dev->initialized = 1;
    printf("[Eth] 初始化完成，BAR=0x%lx，链路速度=%d Mbps\n", 
           base_addr, dev->link_speed);
}

static void eth_start(IDevice *device) {
    EthDevice *dev = (EthDevice*)device->private_data;
    if (!dev->initialized) {
        printf("[Eth] 错误：设备未初始化！\n");
        return;
    }
    printf("[Eth] 设备启动，开始收发数据包\n");
}

static void eth_stop(IDevice *device) {
    printf("[Eth] 设备停止\n");
}

static const char* eth_get_name(IDevice *device) {
    EthDevice *dev = (EthDevice*)device->private_data;
    return dev->name;
}

static void eth_destroy(IDevice *device) {
    printf("[Eth] 销毁以太网控制器实例\n");
    free(device->private_data);
}
```

```c
// eth_factory.h
#ifndef ETH_FACTORY_H
#define ETH_FACTORY_H

#include "device_factory.h"
#include "eth_device.h"

typedef struct {
    IFactory base;
} EthFactory;

EthFactory* EthFactory_create(void);

#endif // ETH_FACTORY_H
```

```c
// eth_factory.c
#include "eth_factory.h"
#include <stdio.h>

static IDevice* eth_factory_create(IFactory *factory);
static void eth_factory_destroy(IFactory *factory);

EthFactory* EthFactory_create(void) {
    EthFactory *factory = malloc(sizeof(EthFactory));
    if (factory == NULL) return NULL;
    
    factory->base.create = eth_factory_create;
    factory->base.destroy = eth_factory_destroy;
    
    printf("[EthFactory] 以太网工厂实例创建完成\n");
    return factory;
}

static IDevice* eth_factory_create(IFactory *factory) {
    (void)factory;
    return &EthDevice_create()->base;
}

static void eth_factory_destroy(IFactory *factory) {
    free(factory);
}
```

### 4.6 主程序

```c
// main.c
#include "device_factory.h"
#include "nvme_factory.h"
#include "eth_factory.h"
#include <stdio.h>

int main(void) {
    printf("=========================================\n");
    printf("   工厂方法模式 - PCIe 设备示例\n");
    printf("=========================================\n\n");
    
    /* 创建 NVMe 工厂，创建 NVMe 设备 */
    NVMeFactory *nvme_factory = NVMeFactory_create();
    IDevice *nvme = nvme_factory->base.create(&nvme_factory->base);
    printf("创建的产品: %s\n\n", nvme->get_name(nvme));
    
    /* 创建以太网工厂，创建以太网设备 */
    EthFactory *eth_factory = EthFactory_create();
    IDevice *eth = eth_factory->base.create(&eth_factory->base);
    printf("创建的产品: %s\n\n", eth->get_name(eth));
    
    /* 使用设备 */
    printf("=== 使用设备 ===\n");
    nvme->init(nvme, 0x10000000000UL);
    nvme->start(nvme);
    printf("\n");
    
    eth->init(eth, 0x20000000000UL);
    eth->start(eth);
    printf("\n");
    
    /* 销毁设备 */
    printf("=== 清理资源 ===\n");
    nvme->destroy(nvme);
    eth->destroy(eth);
    nvme_factory->base.destroy(&nvme_factory->base);
    eth_factory->base.destroy(&eth_factory->base);
    
    return 0;
}
```

---

## 5. Linux 内核里的工厂方法：pci_driver 注册模型

Linux 内核的驱动模型本质上就是工厂方法模式的应用：

```c
// 简化自 include/linux/pci.h

/* 抽象产品：pci_device_id - 描述支持的设备 */
#define PCI_DEVICE(vendor, device) \
    .vendor = vendor, .device = device

/* 抽象工厂接口：pci_driver */
struct pci_driver {
    const char *name;                    // 驱动名称
    const struct pci_device_id *id_table; // 支持的设备列表（工厂的"产品线"）
    
    /* 工厂方法：probe - 当发现支持的设备时调用 */
    int (*probe)(struct pci_dev *dev,    // 具体产品：PCI 设备
                 const struct pci_device_id *id);  // 匹配的设备 ID
    
    /* 工厂方法：remove - 当设备移除时调用 */
    void (*remove)(struct pci_dev *dev);
    
    /* ... 其他方法 ... */
};

/* 注册驱动 = 将工厂加入内核设备管理体系 */
int pci_register_driver(struct pci_driver *drv);
```

**这就是工厂方法模式**：
- `pci_driver` 是抽象工厂
- `probe()` 是工厂方法，创建并初始化一个 PCI 设备实例
- 具体的驱动（如 `nvme_driver`、`ixgbe_driver`）是具体工厂

```c
// 内核中的具体工厂示例（简化）
static struct pci_driver nvme_driver = {
    .name = "nvme",
    .id_table = nvme_id_table,       // 支持的设备列表
    .probe = nvme_probe,             // 创建 NVMe 设备实例
    .remove = nvme_remove,           // 销毁设备实例
};
```

内核在枚举 PCI 总道时，对每个设备调用所有驱动的 `probe()` —— 这就是工厂方法的"让子类决定实例化哪个类"。

---

## 6. 工厂方法模式的适用场景

**应该用：**
- 需要支持多种设备/产品，每种产品的创建逻辑不同
- 需要解耦"谁创建"和"创建什么"
- 框架需要扩展新的产品类型，而不影响已有代码

**不应该用：**
- 产品种类单一，不会扩展（用简单工厂就够了）
- 产品创建逻辑非常简单（直接 new 更直接）
- 产品之间没有统一的接口（没有抽象的必要）

---

## 7. 工厂方法 vs 抽象工厂

| 对比 | 工厂方法 | 抽象工厂 |
|:---|:---|:---|
| 解决的问题 | 创建一个产品 | 创建**一系列**相关产品 |
| 工厂粒度 | 一个工厂 → 一个产品 | 一个工厂 → 产品族（多个相关产品） |
| 扩展方式 | 增加新产品 → 增加具体工厂 | 增加产品族 → 增加抽象工厂 + 所有具体工厂 |
| 典型应用 | Linux pci_driver（一个驱动创建一个设备） | Linux ACPI（一套初始化序列创建多个资源） |

---

## 8. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 抽象产品 | `typedef struct { void (*method)(); ... } IProduct;` |
| 具体产品 | 包含 `IProduct base` + 子类数据 + 实现函数 |
| 抽象工厂 | `typedef struct { IProduct* (*create)(); ... } IFactory;` |
| 具体工厂 | 实现 `create()` 方法，返回具体产品 |

**工厂方法的核心**：把"创建哪个产品"延迟到具体工厂子类决定。

---

## 9. 练习

1. 在上面的 PCIe 设备框架中添加一个 USB 控制器设备（USBControllerDevice）和对应的工厂（USBControllerFactory）

2. 实现一个"形状工厂"系统：ShapeFactory 创建 Circle、Square、Triangle，每个形状有 `draw()` 和 `area()` 方法

3. 读 Linux 内核 `drivers/pci/pci-driver.c`，找出 `pci_register_driver()` 的实现，理解驱动注册的工厂方法模型

4. （思考题）工厂方法和抽象工厂的核心区别是什么？什么情况下应该用抽象工厂而不是工厂方法？

---

## 代码

`include/pcie_device.h` — 设备抽象接口
`include/device_factory.h` — 工厂抽象接口
`include/nvme_device.h` + `src/nvme_device.c` — NVMe 设备实现
`include/nvme_factory.h` + `src/nvme_factory.c` — NVMe 工厂实现
`include/eth_device.h` + `src/eth_device.c` — 以太网设备实现
`include/eth_factory.h` + `src/eth_factory.c` — 以太网工厂实现
`src/main.c` — 测试主程序
`Makefile` — 编译脚本