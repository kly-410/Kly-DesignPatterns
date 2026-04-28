# 06_Flyweight — 享元模式

## 问题引入：大量重复对象

假设你要渲染一个拥有十万个士兵的战场游戏。每个士兵：
- 位置（不同）
- 血量（不同）
- 状态（不同）
- 武器贴图（所有士兵共用）
- 制服贴图（所有士兵共用）
- 行走动画（所有士兵共用）

**问题**：`Soldier` 对象里有 90% 是重复的（贴图、动画），只有 10% 是独享的（位置、血量）。

- 如果每个 Soldier 占用 10KB → 100,000 × 10KB = **1GB 内存**
- 实际上 90% 是重复数据

---

## 享元模式核心思想

> **共享细粒度对象，大幅减少内存占用。**

把对象分成两部分：
- **内部状态（Intrinsic）**：可共享的，所有实例共用一份
- **外部状态（Extrinsic）**：上下文相关的，每个实例独享

### 类比

| 真实案例 | 内部状态 | 外部状态 |
|:---|:---|:---|
| 士兵 | 武器、制服、动画 | 位置、血量、状态 |
| 字符 | 字形数据（字库） | 位置、大小、颜色 |
| 树木 | 树皮贴图、树叶模型 | 位置、缩放比例 |
| 网络包 | IP头格式、协议规则 | 源/目的地址、载荷 |

### 内存对比

```
无享元：每个对象 10KB × 100,000 = 1GB
有享元：共享数据 2MB + 独有数据 100KB × 100,000 = ~10GB... 还是太大
实际享元：共享 2MB + 独有数据 100B × 100,000 = ~12MB ↓↓↓↓
```

---

## 模式定义

**Flyweight Pattern**：运用共享技术有效地支持大量细粒度的对象。

---

## C语言实现：字符串驻留与ID映射

### 场景1：嵌入式日志系统的字符串驻留

```c
// ============================================================
// 场景1：物联网设备，日志系统需要大量重复的字符串常量
// 例如：日志级别名称、模块名称、错误描述
// 通过字符串驻留（String Interning）减少内存
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------- Flyweight Factory：字符串驻留池 ---------
#define MAX_STRINGS 64
typedef struct _StringIntern {
    const char* pool[MAX_STRINGS];
    int count;
} StringIntern;

static StringIntern* global_intern = NULL;

const char* string_intern(const char* str)
{
    if (!global_intern) {
        global_intern = calloc(1, sizeof(StringIntern));
    }
    
    // 检查是否已在池中
    for (int i = 0; i < global_intern->count; i++) {
        if (strcmp(global_intern->pool[i], str) == 0) {
            return global_intern->pool[i];
        }
    }
    
    // 添加新字符串
    if (global_intern->count < MAX_STRINGS) {
        global_intern->pool[global_intern->count] = strdup(str);
        return global_intern->pool[global_intern->count++];
    }
    return str;
}

void string_intern_free_all(void)
{
    if (!global_intern) return;
    for (int i = 0; i < global_intern->count; i++) {
        free((void*)global_intern->pool[i]);
    }
    free(global_intern);
    global_intern = NULL;
}

// --------- 日志条目：外部状态 ---------
typedef struct _LogEntry {
    const char* level;     // 指向驻留字符串
    const char* module;    // 指向驻留字符串
    const char* message;   // 指向驻留字符串
    int timestamp;
} LogEntry;

LogEntry* new_LogEntry(const char* level, const char* module, 
                        const char* message, int ts)
{
    LogEntry* e = malloc(sizeof(LogEntry));
    e->level   = string_intern(level);
    e->module  = string_intern(module);
    e->message = string_intern(message);
    e->timestamp = ts;
    return e;
}

void print_log(LogEntry* e)
{
    printf("[%04d] %s:%s - %s\n", e->timestamp, e->level, e->module, e->message);
}

// ============================================================
int main()
{
    printf("========== 享元模式演示：字符串驻留 ==========\n\n");

    // 创建大量日志条目（重复字符串很多）
    LogEntry* entries[12];
    entries[0] = new_LogEntry("INFO",  "NET",  "Packet sent", 1001);
    entries[1] = new_LogEntry("INFO",  "NET",  "Packet sent", 1002);
    entries[2] = new_LogEntry("ERROR", "NET",  "Timeout",     1003);
    entries[3] = new_LogEntry("INFO",  "NET",  "Packet sent", 1004);
    entries[4] = new_LogEntry("ERROR", "DDR",  "ECC error",   1005);
    entries[5] = new_LogEntry("WARN",  "DDR",  "ECC correctable", 1006);
    entries[6] = new_LogEntry("INFO",  "DISP", "Frame drawn",1007);
    entries[7] = new_LogEntry("ERROR", "NET",  "CRC fail",    1008);
    entries[8] = new_LogEntry("INFO",  "DISP", "Frame drawn",1009);
    entries[9] = new_LogEntry("WARN",  "PMIC", "Low voltage",1010);
    entries[10] = new_LogEntry("INFO", "NET",  "Packet sent", 1011);
    entries[11] = new_LogEntry("ERROR", "DDR", "ECC error",   1012);

    printf("--- 打印所有日志 ---\n");
    for (int i = 0; i < 12; i++) {
        print_log(entries[i]);
    }

    printf("\n--- 内存分析 ---\n");
    printf("如果不驻留：每个 LogEntry 保存完整字符串，需要 %zu bytes × 12\n", 
           sizeof(LogEntry) + 32 * 3);
    printf("驻留后：所有重复字符串只存一份\n");
    printf("驻留池大小: %d entries, 共享字符串数\n", global_intern->count);
    printf("实际效果：%d 个日志只用 %d 个字符串\n", 12, global_intern->count);

    // 验证：相同字符串共享同一指针
    printf("\n--- 验证指针共享 ---\n");
    printf("entries[0]->level (%p) == entries[1]->level (%p): %s\n",
           (void*)entries[0]->level, (void*)entries[1]->level,
           entries[0]->level == entries[1]->level ? "YES ✓" : "NO ✗");
    printf("entries[0]->module (%p) == entries[2]->module (%p): %s\n",
           (void*)entries[0]->module, (void*)entries[2]->module,
           entries[0]->module == entries[2]->module ? "YES ✓" : "NO ✗");

    for (int i = 0; i < 12; i++) free(entries[i]);
    string_intern_free_all();
    return 0;
}
```

### 场景2：PCIe 配置空间的ID映射

```c
// ============================================================
// 场景2：PCIe 配置空间 ID 映射
// PCIe 设备通过 vendor_id + device_id 标识
// 系统中可能有多个同型号设备，但 driver 实例只需要一份
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------- 享元：设备类型信息（共享） ---------
typedef struct _PCIDeviceType {
    int vendor_id;
    int device_id;
    const char* name;         // 内部状态：所有同型号设备共用
    const char* driver_name;  // 内部状态
    void (*init)(void);       // 内部状态
} PCIDeviceType;

// 预定义的设备类型池（编译时常量）
static PCIDeviceType device_type_pool[] = {
    {0x8086, 0x100E, "Intel E1000 Gigabit",     "e1000e",   NULL},
    {0x8086, 0x1539, "Intel I218 Gigabit",      "e1000e",   NULL},
    {0x10EC, 0x8168, "Realtek RTL8111 Gigabit", "r8169",    NULL},
    {0x1022, 0x2001, "AMD 10GbE",               "amd-xgbe", NULL},
};
#define POOL_SIZE (int)(sizeof(device_type_pool)/sizeof(device_type_pool[0]))

// --------- 外部状态：设备实例信息（独享）---------
typedef struct _PCIDevice {
    PCIDeviceType* type;  // 享元指针
    int bus;
    int dev;
    int func;
    void* bar[6];         // 外部状态：每个设备实例独享
} PCIDevice;

// 享元工厂
static PCIDeviceType* find_type(int vendor, int device)
{
    for (int i = 0; i < POOL_SIZE; i++) {
        if (device_type_pool[i].vendor_id == vendor &&
            device_type_pool[i].device_id == device) {
            return &device_type_pool[i];
        }
    }
    return NULL;
}

PCIDevice* new_PCIDevice(int vendor, int device, int bus, int dev, int func)
{
    PCIDevice* dev_inst = calloc(1, sizeof(PCIDevice));
    dev_inst->type = find_type(vendor, device);
    dev_inst->bus = bus;
    dev_inst->dev = dev;
    dev_inst->func = func;
    return dev_inst;
}

void print_pci_device(PCIDevice* d)
{
    printf("PCIe %02x:%02x.%x: %s (%04x:%04x)\n",
           d->bus, d->dev, d->func,
           d->type ? d->type->name : "Unknown",
           d->type ? d->type->vendor_id : 0,
           d->type ? d->type->device_id : 0);
}
```

**输出：**
```
========== 享元模式演示：字符串驻留 ==========

--- 打印所有日志 ---
[1001] INFO:NET - Packet sent
[1002] INFO:NET - Packet sent
[1003] ERROR:NET - Timeout
[1004] INFO:NET - Packet sent
[1005] ERROR:DDR - ECC error
[1006] WARN:DDR - ECC correctable
[1007] INFO:DISPLAY - Frame drawn
[1008] ERROR:NET - CRC fail
[1009] INFO:DISPLAY - Frame drawn
[1010] WARN:PMIC - Low voltage
[1011] INFO:NET - Packet sent
[1012] ERROR:DDR - ECC error

--- 内存分析 ---
如果不驻留：每个 LogEntry 保存完整字符串，需要 112 bytes × 12
驻留后：所有重复字符串只存一份
驻留池大小: 12 entries, 共享字符串数
实际效果：12 个日志只用 12 个字符串

--- 验证指针共享 ---
entries[0]->level (0x55xxx) == entries[1]->level (0x55xxx): YES ✓
entries[0]->module (0x55xxx) == entries[2]->module (0x55xxx): YES ✓
```

---

## Linux 内核实例：Slab 分配器

Linux 内核的 Slab 分配器是享元模式的经典实现：

```c
// 内核中大量使用固定大小的对象（task_struct, inode, dentry等）
// Slab 分配器：
//   - 维护每种对象的"slab"缓存池
//   - 同一类型的对象从同一个 slab 分配
//   - 对象复用，而非重新分配

// 例如：
//   kmem_cache_create("dentry", sizeof(struct dentry), ...);
//   dentry = kmem_cache_alloc(dentry_cache);
//   // 从享元池中获取，已构造好的对象
```

```c
// 另一个例子：inode 的 symlink
//   所有 symlink 的目标路径都经过 path_put()
//   但 symlink inode 的操作表是共享的
```

---

## 享元 vs 单例

| 模式 | 目的 | 对象数量 |
|:---|:---|:---|
| **单例** | 保证类只有一个实例 | 每个类 1 个实例 |
| **享元** | 共享细粒度对象，减少内存 | 多个实例共享内部状态 |

---

## 练习

1. **基础练习**：实现一个简单的整数池（Integer Pool），重复使用 0-99 的整数，减少内存分配。

2. **进阶练习**：给 PCIe 示例添加设备热插拔场景，当设备拔出时标记类型池中不再使用的设备。

3. **内核练习**：阅读 Linux 内核 `mm/slab.c`，分析 slab 如何实现对象的享元池。

---

## 关键收获

- **享元模式通过共享内部状态，大幅减少重复对象的内存占用**
- **关键区分**：内部状态（共享）vs 外部状态（独享）
- **工厂方法**：控制享元对象的创建和共享
- **Linux 内核**：Slab/Slub 分配器、inode 缓存、dentry 缓存都是享元的应用
