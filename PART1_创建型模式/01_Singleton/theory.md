# 单例模式（Singleton Pattern）

## 本章目标

1. 理解单例模式解决的问题：**全局唯一实例如何保证**
2. 掌握饿汉式 vs 懒汉式的区别与适用场景
3. 掌握 C 语言实现单例的多种方式（模块级静态变量、DCL、pthread_once）
4. 理解 Linux 内核中单例的应用（net_device_devices、init_task 等）
5. 理解单例模式在固件开发中的常见陷阱

---

## 1. 从一个问题开始

设计一个 PCIe 驱动，需要：
- 一个全局配置结构体，供所有函数访问
- 这个结构体在整个程序生命周期只能有一份
- 不能出现两份配置（会导致状态不一致）

**糟糕的设计**：全局变量 → 任何文件都可以修改，无法控制访问

**好的设计**：把构造过程封装，只暴露一个访问点，保证只创建一个实例

---

## 2. 单例模式的定义

> **单例模式**：确保一个类只有一个实例，并提供一个全局访问点。

三个要点：
- **唯一实例**：程序运行期间，只会有一个对象
- **全局访问**：任何代码都可以通过统一入口获取这个实例
- **构造控制**：构造函数不公开，防止外部 new

---

## 3. 单例模式的实现方式

### 3.1 饿汉式（Eager Initialization）

**思想**：在程序加载时就把实例创建好，等着被使用

```c
// singleton_eager.h
#ifndef SINGLETON_EAGER_H
#define SINGLETON_EAGER_H

#include <stdint.h>

typedef struct {
    uint32_t flags;
    void *private_data;
} GlobalConfig;

// 获取全局配置实例（饿汉版）
GlobalConfig* config_get_instance(void);

#endif
```

```c
// singleton_eager.c
#include "singleton_eager.h"
#include <stdio.h>
#include <stdlib.h>

// ========== 饿汉式实现 ==========
// 模块级静态变量，程序加载时就初始化完成
// C 语言中，静态变量默认初始化为 0
static GlobalConfig g_config = {
    .flags = 0xABCD1234,
    .private_data = NULL,
};

// 已初始化标志（静态变量自动初始化为 0）
static int g_initialized = 0;

// 初始化函数（仅执行一次）
static void init_config(void) {
    if (g_initialized) {
        return;
    }
    g_config.flags = 0xABCD1234;
    g_config.private_data = malloc(128);
    if (g_config.private_data == NULL) {
        fprintf(stderr, "[EAGER] Failed to allocate private data\n");
        return;
    }
    g_initialized = 1;
    printf("[EAGER] 全局配置初始化完成\n");
}

// 获取实例（简单直接，无锁开销）
GlobalConfig* config_get_instance(void) {
    if (!g_initialized) {
        init_config();
    }
    return &g_config;
}
```

```c
// main.c
#include "singleton_eager.h"
#include <stdio.h>

int main(void) {
    printf("=== 饿汉式单例测试 ===\n");
    
    GlobalConfig *c1 = config_get_instance();
    GlobalConfig *c2 = config_get_instance();
    
    printf("c1 address: %p\n", (void*)c1);
    printf("c2 address: %p\n", (void*)c2);
    printf("c1 == c2: %s\n", c1 == c2 ? "YES ✓" : "NO ✗");
    
    c1->flags = 0xDEADBEEF;
    printf("修改后 c2->flags = 0x%X\n", c2->flags);  // 同步变化
    
    return 0;
}
```

**特点**：
- 线程安全（静态变量初始化在程序加载时完成）
- 启动时消耗资源（如果初始化很重）
- 没有第一次获取时的延迟

### 3.2 懒汉式（Lazy Initialization）

**思想**：第一次使用时才创建，延迟加载

```c
// singleton_lazy.h
#ifndef SINGLETON_LAZY_H
#define SINGLETON_LAZY_H

#include <stdint.h>

typedef struct {
    uint32_t flags;
    void *private_data;
} LazyConfig;

// 获取全局配置实例（懒汉版）
LazyConfig* lazy_config_get_instance(void);

#endif
```

```c
// singleton_lazy.c
#include "singleton_lazy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== 懒汉式实现（双检锁 DCL）==========
static LazyConfig *g_config = NULL;    // 延迟初始化
static int g_initialized = 0;

static void init_config(void) {
    g_config = malloc(sizeof(LazyConfig));
    if (g_config == NULL) {
        fprintf(stderr, "[LAZY] Failed to allocate config\n");
        return;
    }
    memset(g_config, 0, sizeof(LazyConfig));
    g_config->flags = 0x12345678;
    g_config->private_data = NULL;
    g_initialized = 1;
    printf("[LAZY] 配置延迟初始化完成\n");
}

LazyConfig* lazy_config_get_instance(void) {
    // 第一层检查：快速路径，避免大多数情况下的加锁开销
    if (g_initialized == 0) {
        // 加锁（实际项目中需要 pthread_mutex，这里简化）
        static volatile int lock = 0;
        while (__sync_lock_test_and_set(&lock, 1)) {
            // spin-wait
        }
        
        // 第二层检查：确认真的需要初始化
        if (g_initialized == 0) {
            init_config();
        }
        
        __sync_lock_release(&lock);
    }
    
    return g_config;
}
```

### 3.3 线程安全的懒汉式（pthread_once）

**这是 Linux 内核代码中最常用的方式**

```c
// singleton_pthread_once.h
#ifndef SINGLETON_PTHREAD_ONCE_H
#define SINGLETON_PTHREAD_ONCE_H

#include <stdint.h>

typedef struct {
    uint32_t device_count;
    void *driver_data;
} PCIDeviceManager;

// 获取 PCI 设备管理器实例（线程安全）
PCIDeviceManager* pci_manager_get_instance(void);

#endif
```

```c
// singleton_pthread_once.c
#include "singleton_pthread_once.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// ========== pthread_once 实现（Linux 内核风格）==========
static PCIDeviceManager *g_manager = NULL;
static pthread_once_t g_once_control = PTHREAD_ONCE_INIT;

// 初始化函数，只会被调用一次
static void init_manager(void) {
    g_manager = malloc(sizeof(PCIDeviceManager));
    if (g_manager == NULL) {
        fprintf(stderr, "[PTHREAD_ONCE] Failed to allocate manager\n");
        return;
    }
    memset(g_manager, 0, sizeof(PCIDeviceManager));
    g_manager->device_count = 0;
    g_manager->driver_data = NULL;
    printf("[PTHREAD_ONCE] PCI 设备管理器初始化完成（线程安全）\n");
}

// 获取实例：用 pthread_once 保证初始化函数只执行一次
PCIDeviceManager* pci_manager_get_instance(void) {
    int err = pthread_once(&g_once_control, init_manager);
    if (err != 0) {
        fprintf(stderr, "[PTHREAD_ONCE] pthread_once 失败: %s\n", strerror(err));
        return NULL;
    }
    return g_manager;
}
```

```c
// main_pthread.c
#include "singleton_pthread_once.h"
#include <stdio.h>
#include <pthread.h>

// 线程函数：模拟多个驱动同时获取单例
void* thread_worker(void* arg) {
    int id = *(int*)arg;
    PCIDeviceManager* mgr = pci_manager_get_instance();
    printf("[线程%d] 获取管理器实例: %p, device_count=%u\n", 
           id, (void*)mgr, mgr->device_count);
    return NULL;
}

int main(void) {
    printf("=== pthread_once 单例测试 ===\n");
    
    pthread_t t1, t2, t3;
    int id1 = 1, id2 = 2, id3 = 3;
    
    pthread_create(&t1, NULL, thread_worker, &id1);
    pthread_create(&t2, NULL, thread_worker, &id2);
    pthread_create(&t3, NULL, thread_worker, &id3);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    printf("主线程验证: 三个线程获取的是同一个实例 ✓\n");
    
    return 0;
}
```

---

## 4. 三种实现对比

| 特性 | 饿汉式 | 懒汉式（DCL） | pthread_once |
|:---|:---|:---|:---|
| 线程安全 | ✅ 天然安全 | ⚠️ 需要双检锁 | ✅ 天然安全 |
| 延迟加载 | ❌ 启动时创建 | ✅ 首次使用时 | ✅ 首次使用时 |
| 性能开销 | 启动稍慢 | 首次有锁开销 | 首次有锁开销 |
| 实现复杂度 | 简单 | 复杂（容易出错） | 简单 |
| 适用场景 | 初始化轻量、确定需要 | 初始化重、可能不用 | 高并发多线程环境 |

---

## 5. Linux 内核里的单例模式

### 5.1 全局设备列表（net_device_devices）

Linux 内核中管理网络设备的经典单例：

```c
// 简化自 net/core/dev.c
struct net_device {
    char name[IFNAMSIZ];
    unsigned long state;
    struct net_device *next;  // 内核维护的设备链表
};

// 全局设备列表——实际上是一个单例的设备管理器
static struct net_device *dev_base = NULL;

// 获取网络设备链表头（全局唯一访问点）
struct net_device* dev_get_first(void) {
    return dev_base;
}
```

`dev_base` 是一个模块级静态变量，所有网络设备驱动通过 `register_netdev()` 注册到这个链表。它本质上是一个全局唯一的链表头——就是一个单例。

### 5.2 init_task — 进程 0 的单例

```c
// 简化自 init/init_task.c
struct task_struct init_task = {
    .state = 0,
    .stack = (unsigned long)&init_stack,
    .pid = 0,
    .comm = "swapper",
    // ... 大量初始化
};
```

`init_task` 是 Linux 启动时创建的第一个进程（PID 0），在内核数据段静态分配。它是所有其他进程的祖先，是一个"饿汉式单例"。

### 5.3 pci_bus_devices

```c
// 简化自 drivers/pci/pci.c
static LIST_HEAD(pci_root_buses);  // PCI 根总线列表

// 添加 PCI 总线设备到全局列表
void pci_add_bus(struct pci_bus *bus) {
    list_add_tail(&bus->node, &pci_root_buses);
}

// 获取所有 PCI 设备的入口
struct list_head* pci_get_all_buses(void) {
    return &pci_root_buses;
}
```

---

## 6. 嵌入式固件中的单例

在嵌入式固件中，单例最常见的应用是**外设句柄管理**：

```c
// 串口外设管理器（嵌入式场景）
typedef struct {
    UART_HandleTypeDef *handle;
    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];
} UARTManager;

static UARTManager g_uart1_manager = {0};  // 模块级静态变量 = 单例

UARTManager* uart1_get_manager(void) {
    return &g_uart1_manager;  // 直接返回，天然线程安全（单线程嵌入式）
}
```

**嵌入式注意点**：
- 很多嵌入式系统是单线程的，不需要锁
- 静态变量在 .bss 段，掉电不丢（可用于保存配置）
- 单例的析构函数基本不需要（程序不退出）

---

## 7. 单例模式的适用场景

**应该用：**
- 全局配置管理器（只需要一份配置）
- 日志系统（所有模块共用一个日志输出）
- 设备驱动管理器（PCIe 设备列表、I2C 总线管理等）
- 连接池、线程池（全局唯一管理）

**不应该用：**
- 需要不同配置的多个实例（违反单例原则）
- 需要频繁创建销毁的对象（用对象池更合适）
- 有状态且需要隔离的组件（用依赖注入）

---

## 8. 单例模式的缺点

1. **全局状态**：单例是隐式的全局变量，导致模块间隐式耦合
2. **难以测试**：单例难以 mock，难以替换为测试替身
3. **违反单一职责**：单例既管理自身，又承担业务逻辑
4. **隐式依赖**：使用单例的代码不知道自己依赖了哪个模块

---

## 9. 本章小结

| 要素 | C 语言实现 |
|:---|:---|
| 唯一实例 | 模块级 `static` 变量 + 不暴露构造 |
| 全局访问 | 提供 `get_instance()` 函数作为唯一访问点 |
| 线程安全 | `pthread_once` / 双检锁 / 饿汉式 |
| 初始化控制 | 构造函数私有（或省略），通过初始化函数创建 |

**单例模式的核心**：通过控制实例的数量和访问路径，确保全局唯一性。

---

## 10. 练习

1. 实现一个"日志系统"单例，支持 `log_info()` / `log_error()` / `log_debug()`，所有模块都通过这个单例输出日志

2. 实现一个"内存分配器"单例，内部维护一个内存池，所有模块从池中分配（避免碎片化）

3. 读 Linux 内核 `kernel/sched/core.c`，找 `init_task` 的定义，理解它是如何保证唯一性的

4. （思考题）如果你需要多个不同配置的单例（例如"开发板配置"和"生产板配置"），该怎么改造单例模式？

---

## 代码

`src/singleton_eager.c` — 饿汉式单例实现
`src/singleton_lazy.c` — 懒汉式（DCL）单例实现
`src/singleton_pthread_once.c` — pthread_once 线程安全实现