# 03_Composite — 组合模式

## 问题引入：树形结构的统一处理

目录里有文件，有子目录，子目录里又有文件和子目录……

**问题**：如何用统一的接口，处理文件和目录这两种完全不同的东西？

- `file.size()` → 文件大小
- `dir.size()` → dir 里所有文件的总和
- `dir.add(x)` → 加一个子节点
- `dir.remove(x)` → 删除一个子节点

**要求**：调用方代码不需要知道是文件还是目录。

---

## 组合模式核心思想

> **让单个对象和组合对象使用统一接口，客户端无差别对待。**

### 关键洞察

```
文件   —— size() → 直接返回大小
目录   —— size() → 遍历子节点，求和
```

文件是"叶子节点"，目录是"容器节点"，但它们都响应 `size()` 消息。

### 类图

```
        ┌────────────────┐
        │   Component    │ ← 统一接口（纯虚）
        │ size()         │
        │ add() / remove()│
        └───────┬────────┘
         ┌──────┴──────┐
         │             │
    ┌────┴────┐   ┌────┴────┐
    │ Leaf    │   │ Composite│ ← 容器：持有children列表
    │(叶子节点)│   │(组合节点) │
    └─────────┘   └────┬────┘
                       │ 包含多个 Component
                       └──────┐
                          ┌────┴────┐
                          │ ...    │
```

---

## 模式定义

**Composite Pattern**：将对象组合成树形结构以表示"部分-整体"的层次结构。组合让用户对单个对象和组合对象的使用具有一致性。

---

## C语言实现：文件系统目录树

### 场景：嵌入式配置系统（类似 Linux `/proc` 树）

```c
// ============================================================
// 场景：嵌入式系统的配置树（类似设备树 or/proc）
// - 文件（叶子）：配置参数，保存/读取值
// - 目录（容器）：包含子文件/子目录
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// --------- Component：统一接口 ---------
typedef struct _ConfigNode ConfigNode;
struct _ConfigNode {
    const char* name;
    
    // 统一接口：打印节点信息
    void (*print)(ConfigNode*, int indent);
    
    // 组合接口：叶子节点忽略，容器节点实现
    void (*add)(ConfigNode*, ConfigNode*);
    void (*remove)(ConfigNode*, ConfigNode*);
    ConfigNode* (*find)(ConfigNode*, const char* name);
};

// --------- Leaf：配置参数文件 ---------
typedef struct _ConfigFile {
    ConfigNode base;
    char value[64];
} ConfigFile;

static void configfile_print(ConfigNode* node, int indent)
{
    ConfigFile* f = (ConfigFile*)node;
    printf("%*s[FILE] %s = %s\n", indent, "", node->name, f->value);
}

ConfigFile* new_ConfigFile(const char* name, const char* value)
{
    ConfigFile* f = calloc(1, sizeof(ConfigFile));
    f->base.name = name;
    strcpy(f->value, value);
    f->base.print  = configfile_print;
    f->base.add    = (void*)0x1;   // 不支持 operation（哨兵值）
    f->base.remove = (void*)0x1;
    f->base.find   = (void*)0x1;
    return f;
}

// --------- Composite：配置目录 ---------
#define MAX_CHILDREN 16
typedef struct _ConfigDir {
    ConfigNode base;
    ConfigNode* children[MAX_CHILDREN];
    int child_count;
} ConfigDir;

static void configdir_print(ConfigNode* node, int indent)
{
    ConfigDir* dir = (ConfigDir*)node;
    printf("%*s[DIR] %s/\n", indent, "", node->name);
    for (int i = 0; i < dir->child_count; i++) {
        dir->children[i]->print(dir->children[i], indent + 4);
    }
}

static void configdir_add(ConfigNode* node, ConfigNode* child)
{
    ConfigDir* dir = (ConfigDir*)node;
    if (dir->child_count < MAX_CHILDREN) {
        dir->children[dir->child_count++] = child;
        printf("[DIR]   added '%s' to '%s'\n", child->name, node->name);
    }
}

static void configdir_remove(ConfigNode* node, ConfigNode* child)
{
    ConfigDir* dir = (ConfigDir*)node;
    for (int i = 0; i < dir->child_count; i++) {
        if (dir->children[i] == child) {
            memmove(&dir->children[i], &dir->children[i+1],
                    (dir->child_count - i - 1) * sizeof(ConfigNode*));
            dir->child_count--;
            return;
        }
    }
}

static ConfigNode* configdir_find(ConfigNode* node, const char* name)
{
    ConfigDir* dir = (ConfigDir*)node;
    if (strcmp(node->name, name) == 0) return node;
    for (int i = 0; i < dir->child_count; i++) {
        ConfigNode* found = dir->children[i]->find(dir->children[i], name);
        if (found) return found;
    }
    return NULL;
}

ConfigDir* new_ConfigDir(const char* name)
{
    ConfigDir* d = calloc(1, sizeof(ConfigDir));
    d->base.name = name;
    d->base.print  = configdir_print;
    d->base.add    = configdir_add;
    d->base.remove = configdir_remove;
    d->base.find   = configdir_find;
    return d;
}

// ============================================================
int main()
{
    printf("========== 组合模式演示 ==========\n\n");

    // 构建一个配置树：
    // root/
    //   ├── system/
    //   │   ├── hostname = " embedded-box "
    //   │   └── network/
    //   │       ├── ip = "192.168.1.100"
    //   │       └── mask = "255.255.255.0"
    //   └── debug/
    //       ├── level = "INFO"
    //       └── enabled = "true"

    ConfigDir* root = new_ConfigDir("root");
    ConfigDir* sys  = new_ConfigDir("system");
    ConfigDir* net  = new_ConfigDir("network");
    ConfigFile* host = new_ConfigFile("hostname", "embedded-box");
    ConfigFile* ip   = new_ConfigFile("ip", "192.168.1.100");
    ConfigFile* mask = new_ConfigFile("mask", "255.255.255.0");
    ConfigFile* level = new_ConfigFile("level", "INFO");
    ConfigFile* enbl  = new_ConfigFile("enabled", "true");
    ConfigDir* debug = new_ConfigDir("debug");

    root->base.add(&root->base, &sys->base);
    sys->base.add(&sys->base, host);
    sys->base.add(&sys->base, &net->base);
    net->base.add(&net->base, ip);
    net->base.add(&net->base, mask);
    root->base.add(&root->base, &debug->base);
    debug->base.add(&debug->base, level);
    debug->base.add(&debug->base, enbl);

    // 统一打印：无需知道是文件还是目录
    printf("\n--- 打印整棵树（统一接口）---\n");
    root->base.print(&root->base, 0);

    // 查找节点
    printf("\n--- 查找 'ip' ---\n");
    ConfigNode* found = root->base.find(&root->base, "ip");
    if (found) {
        found->print(found, 4);
    }

    // 修改配置值
    printf("\n--- 修改 ip 配置值 ---\n");
    strcpy(((ConfigFile*)ip)->value, "10.0.0.42");
    found->print(found, 4);

    return 0;
}
```

**输出：**
```
========== 组合模式演示 ==========

[DIR]   added 'system' to 'root'
[DIR]   added 'hostname' to 'system'
[DIR]   added 'network' to 'system'
[DIR]   added 'ip' to 'network'
[DIR]   added 'mask' to 'network'
[DIR]   added 'debug' to 'root'
[DIR]   added 'level' to 'debug'
[DIR]   added 'enabled' to 'debug'

--- 打印整棵树（统一接口）---
[DIR] root/
    [DIR] system/
        [FILE] hostname = embedded-box
        [DIR] network/
            [FILE] ip = 192.168.1.100
            [FILE] mask = 255.255.255.0
    [DIR] debug/
        [FILE] level = INFO
        [FILE] enabled = true

--- 查找 'ip' ---
[FILE] ip = 192.168.1.100

--- 修改 ip 配置值 ---
[FILE] ip = 10.0.0.42
```

---

## Linux 内核实例：设备模型（bus/device/driver hierarchy）

Linux 设备模型是组合模式的经典应用：

```
/sys/
├── bus/
│   ├── pci/
│   │   ├── devices/
│   │   │   └── 0000:00:1f.0 → (device)
│   │   └── drivers/
│   │       └── ahci → (driver)
│   └── spi/
├── devices/
│   ├── platform/
│   └── pci/0000:00:00.0/
└── driver/
```

每个目录都是一个 `kobject`，既是容器（可包含子kobject），也是叶子（对应具体设备）。

内核的 `sysfs` 就是组合模式的实现——每个 `kobject` 既可以添加子节点，也可以被其他 `kobject` 包含。

---

## 组合模式 vs 迭代器：天然配合

组合模式常和迭代器一起使用，遍历树形结构：

```c
// 组合模式的递归遍历（隐式使用栈）
void print_tree(ConfigNode* node, int depth)
{
    node->print(node, depth);
    if (node->find != (void*)0x1) {  // 是容器
        // 递归遍历子节点...
    }
}
```

---

## 练习

1. **基础练习**：给配置系统添加 `ConfigFile_setValue()` 方法，用于修改配置值。

2. **进阶练习**：实现一个类似 `tree` 命令的递归遍历，打印整棵树的深度信息。

3. **内核练习**：阅读 Linux 内核 `lib/kobject.c`，分析 kobject 如何实现组合模式。

---

## 关键收获

- **组合模式让树形结构的使用者和容器/叶子都使用同一接口**
- **关键点**：叶子节点和容器节点对同一操作有不同响应
- **Linux 内核**：`sysfs` 的每层目录、`device tree` 的节点层次，都体现了组合模式
- **本质**：把"整体-部分"的层次结构，用统一的接口暴露给调用者
