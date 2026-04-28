# 07_Proxy — 代理模式

## 问题引入：访问控制与延迟加载

场景 1：**延迟加载**（Lazy Loading）
- 嵌入式设备启动时不需要加载所有驱动
- PCIe 设备插入后才初始化对应驱动
- "按需加载"，加快启动速度

场景 2：**访问控制**
- 用户态程序不能直接访问物理内存
- 通过 `/dev/mem` 或 `mmap` 时需要权限检查
- 内核在中间加一层"代理"，做权限校验

场景 3：**远程代理**
- 在 PC 上调试嵌入式设备时
- 通过 GDB Proxy 代理调试命令
- 真实的调试器在远程嵌入式设备上

---

## 代理模式核心思想

> **不直接访问目标对象，而是通过代理对象间接访问。**

代理和真实对象实现**同一个接口**，客户端无感知。

### 类图

```
┌──────────┐     ┌─────────────────┐     ┌──────────────────┐
│  Client  │────>│     Subject     │     │  RealSubject     │
│ (调用方)  │     │  (抽象接口)      │     │  (真实对象)        │
└──────────┘     │  request()      │     │  request()       │
                 └────────┬────────┘     └──────────────────┘
                          │                        ▲
                          │                        │
                          │         ┌──────────────┘
                          ▼         │
                   ┌────────────┐   │
                   │   Proxy    │───┘
                   │  (代理对象)  │
                   │ - 权限检查   │
                   │ - 延迟加载   │
                   │ - 远程调用   │
                   └────────────┘
```

### 代理类型

| 类型 | 应用场景 | Linux 例子 |
|:---|:---|:---|
| **远程代理** | RMI、RPC | RPC 调用 |
| **虚代理** | 延迟加载 | 页缓存（page cache） |
| **保护代理** | 访问控制 | `ptrace` 权限检查 |
| **智能引用** | 引用计数、审计 | 文件句柄引用计数 |

---

## 模式定义

**Proxy Pattern**：为其他对象提供一种代理以控制对这个对象的访问。

---

## C语言实现：虚拟内存管理（ioremap代理）

### 场景：嵌入式驱动中的 PCIe BAR 代理

```c
// ============================================================
// 场景1：PCIe BAR 映射的代理（虚代理：延迟加载）
// 嵌入式设备驱动在访问 BAR 之前，先检查是否已经 ioremap
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// --------- Subject 接口 ---------
typedef struct _PhysicalAddr PhysicalAddr;
struct _PhysicalAddr {
    void (*write32)(PhysicalAddr*, uint32_t offset, uint32_t val);
    uint32_t (*read32)(PhysicalAddr*, uint32_t offset);
    void (*close)(PhysicalAddr*);
};

// --------- RealSubject：真实的 PCIe BAR 访问 ---------
typedef struct _PCIeBAR {
    PhysicalAddr base;
    uint64_t phys_addr;    // 物理地址
    void* io_remapped;     // ioremap 后的虚拟地址
    int mapped;            // 是否已映射
} PCIeBAR;

static void bar_write32(PhysicalAddr* h, uint32_t offset, uint32_t val)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (!bar->mapped) {
        printf("  [BAR] ERROR: not mapped yet! Call ioremap first.\n");
        return;
    }
    uint32_t* reg = (uint32_t*)((char*)bar->io_remapped + offset);
    *reg = val;
    printf("  [BAR] WRITE @0x%04X <- 0x%08X (real @ %p)\n",
           offset, val, reg);
}

static uint32_t bar_read32(PhysicalAddr* h, uint32_t offset)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (!bar->mapped) {
        printf("  [BAR] ERROR: not mapped yet!\n");
        return 0xDEADBEEF;
    }
    uint32_t* reg = (uint32_t*)((char*)bar->io_remapped + offset);
    uint32_t val = *reg;
    printf("  [BAR] READ  @0x%04X -> 0x%08X\n", offset, val);
    return val;
}

static void bar_close(PhysicalAddr* h)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (bar->mapped) {
        printf("  [BAR] iounmap @ %p\n", bar->io_remapped);
        bar->mapped = 0;
    }
    (void)bar;
}

PCIeBAR* new_PCIeBAR(uint64_t phys_addr)
{
    PCIeBAR* bar = calloc(1, sizeof(PCIeBAR));
    bar->phys_addr = phys_addr;
    bar->base.write32 = bar_write32;
    bar->base.read32  = bar_read32;
    bar->base.close   = bar_close;
    // 注意：此时不分配，不 ioremap，真正使用时才映射（虚代理）
    return bar;
}

// 模拟 ioremap
void ioremap_bar(PCIeBAR* bar)
{
    if (bar->mapped) return;
    bar->io_remapped = (void*)(0xFFFF0000ULL + bar->phys_addr);
    bar->mapped = 1;
    printf("  [BAR] ioremap 0x%llX -> %p\n",
           (unsigned long long)bar->phys_addr, bar->io_remapped);
}

// ============================================================
// 代理对象：在访问之前做延迟加载检查
// ============================================================
typedef struct _BARProxy {
    PhysicalAddr base;
    PCIeBAR* real_bar;      // 指向真实对象
} BARProxy;

static void proxy_write32(PhysicalAddr* h, uint32_t offset, uint32_t val)
{
    BARProxy* p = (BARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [Proxy] Lazy ioremap before first access\n");
        ioremap_bar(p->real_bar);
    }
    p->real_bar->base.write32(&p->real_bar->base, offset, val);
}

static uint32_t proxy_read32(PhysicalAddr* h, uint32_t offset)
{
    BARProxy* p = (BARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [Proxy] Lazy ioremap before first access\n");
        ioremap_bar(p->real_bar);
    }
    return p->real_bar->base.read32(&p->real_bar->base, offset);
}

static void proxy_close(PhysicalAddr* h)
{
    BARProxy* p = (BARProxy*)h;
    printf("  [Proxy] Closing\n");
    p->real_bar->base.close(&p->real_bar->base);
    free(p->real_bar);
    free(p);
}

BARProxy* new_BARProxy(PCIeBAR* real_bar)
{
    BARProxy* p = calloc(1, sizeof(BARProxy));
    p->base.write32 = proxy_write32;
    p->base.read32  = proxy_read32;
    p->base.close   = proxy_close;
    p->real_bar = real_bar;
    return p;
}

// ============================================================
// 场景2：访问控制代理（保护代理）
// ============================================================
typedef struct _AccessController AccessController;
struct _AccessController {
    int current_uid;
    int (*check_permission)(AccessController*, int uid, const char* operation);
};

static int check_permission(AccessController* ac, int uid, const char* op)
{
    // root (uid=0) 可以做任何事
    // 普通用户只能读，不能写
    if (uid == 0) return 1;
    if (strcmp(op, "read") == 0) return 1;
    if (strcmp(op, "write") == 0) return 0;
    return 0;
}

AccessController* new_AccessController(void)
{
    AccessController* ac = calloc(1, sizeof(AccessController));
    ac->current_uid = 0;  // 默认以 root 运行
    ac->check_permission = check_permission;
    return ac;
}

typedef struct _ProtectedRegister {
    PhysicalAddr base;
    PCIeBAR* real_bar;
    AccessController* acl;
} ProtectedRegister;

static void prot_write32(PhysicalAddr* h, uint32_t offset, uint32_t val)
{
    ProtectedRegister* pr = (ProtectedRegister*)h;
    if (!pr->acl->check_permission(pr->acl, pr->acl->current_uid, "write")) {
        printf("  [ACL] DENIED: uid=%d cannot write register\n",
               pr->acl->current_uid);
        return;
    }
    pr->real_bar->base.write32(&pr->real_bar->base, offset, val);
}

ProtectedRegister* new_ProtectedRegister(PCIeBAR* real_bar, AccessController* acl)
{
    ProtectedRegister* pr = calloc(1, sizeof(ProtectedRegister));
    pr->base.write32 = prot_write32;
    pr->base.read32  = (void*)proxy_read32;  // 读不需要特殊权限
    pr->base.close   = (void*)proxy_close;
    pr->real_bar = real_bar;
    pr->acl = acl;
    return pr;
}

// ============================================================
int main()
{
    printf("========== 代理模式演示 ==========\n\n");

    // --------- 场景1：虚代理（延迟加载） ---------
    printf("--- 场景1: PCIe BAR 延迟映射 ---\n");
    PCIeBAR* bar1 = new_PCIeBAR(0x40000000ULL);
    BARProxy* proxy1 = new_BARProxy(bar1);
    
    printf("Create BAR @ 0x40000000, no ioremap yet.\n");
    printf("First write access:\n");
    proxy1->base.write32(&proxy1->base, 0x10, 0x12345678);
    printf("\nSecond write access:\n");
    proxy1->base.write32(&proxy1->base, 0x14, 0xABCDEF00);
    proxy1->base.close(&proxy1->base);

    // --------- 场景2：保护代理（访问控制） ---------
    printf("\n--- 场景2: 访问控制代理 ---\n");
    PCIeBAR* bar2 = new_PCIeBAR(0x50000000ULL);
    ioremap_bar(bar2);
    AccessController* acl = new_AccessController();
    
    printf("As uid=0 (root):\n");
    acl->current_uid = 0;
    ProtectedRegister* pr = new_ProtectedRegister(bar2, acl);
    pr->base.write32(&pr->base, 0x20, 0xDEADBEEF);
    
    printf("\nAs uid=1000 (user):\n");
    acl->current_uid = 1000;
    pr->base.write32(&pr->base, 0x20, 0xCAFEBABE);
    printf("(read is allowed for non-root)\n");
    pr->base.read32(&pr->base, 0x20);
    
    free(bar2); free(acl); free(pr);
    return 0;
}
```

**输出：**
```
========== 代理模式演示 ==========

--- 场景1: PCIe BAR 延迟映射 ---
Create BAR @ 0x40000000, no ioremap yet.
First write access:
  [Proxy] Lazy ioremap before first access
  [BAR] ioremap 0x40000000 -> 0xFFFF000040000000
  [BAR] WRITE @0x10 <- 0x12345678 (real @ 0xFFFF000040000010)

Second write access:
  [BAR] WRITE @0x14 <- 0xABCDEF00 (real @ 0xFFFF000040000014)

  [Proxy] Closing
  [BAR] iounmap @ 0xFFFF000040000000

--- 场景2: 访问控制代理 ---
  [BAR] ioremap 0x50000000 -> 0xFFFF000050000000
As uid=0 (root):
  [BAR] WRITE @0x20 <- 0xDEADBEEF (real @ 0xFFFF000050000020)

As uid=1000 (user):
  [ACL] DENIED: uid=1000 cannot write register
(read is allowed for non-root)
  [BAR] READ  @0x20 -> 0xDEADBEEF
```

---

## Linux 内核实例：IOMMU 映射代理

Linux 的 IOMMU 子系统是代理模式的经典应用：

```c
// 应用程序调用 mmap() 访问设备内存
// 内核不会直接映射物理地址，而是经过 IOMMU 代理：
//   user_mmap()
//     → dma_mmap_page_pool()    ← 代理层
//       → check ACL/permission
//       → IOMMU page table walk  ← IOMMU 虚拟化
//         → actual hardware
```

```c
// 另一个例子：userfaultfd
//   应用程序访问未映射的内存页时
//   内核通过 userfaultfd 代理拦截页面fault
//   由应用程序决定如何填充页面
```

---

## 代理 vs 适配器 vs 装饰器

| 模式 | 关系 | 目的 |
|:---|:---|:---|
| **代理** | 实现同一接口，控制访问 | **控制**对对象的访问 |
| **适配器** | 实现同一接口，转换接口 | **转换**接口 |
| **装饰器** | 实现同一接口，添加功能 | **增强**对象功能 |

---

## 练习

1. **基础练习**：实现一个日志代理，给所有 `write/read` 操作添加审计日志。

2. **进阶练习**：实现一个缓存代理（Cache Proxy），把频繁读取的数据缓存起来，减少真实设备访问次数。

3. **内核练习**：阅读 Linux 内核 `drivers/iommu/iommu.c`，分析 IOMMU 如何作为 DMA 访问的代理。

---

## 关键收获

- **代理模式在不改变接口的情况下，增加访问控制层**
- **虚代理**：延迟加载（第一次访问时才创建/加载真实对象）
- **保护代理**：权限检查（访问前验证权限）
- **智能引用**：引用计数、审计等增强功能
- **Linux 内核**：IOMMU、userfaultfd、ptrace 都是代理的应用
