/*
 * 03_Composite - 组合模式
 * 
 * 组合模式核心：让单个对象和组合对象使用统一接口
 * 
 * 本代码演示：
 * 1. 嵌入式配置树（文件/目录结构）
 * 2. Linux 设备模型（bus/device/driver 层次）
 * 3. 树形结构的统一遍历
 * 
 * 关键：叶子节点和容器节点对同一操作有不同响应
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// Component：统一接口
// 所有节点（文件或目录）都实现这套接口
// ============================================================
#define MAX_CHILDREN 16

typedef struct _ConfigNode ConfigNode;
struct _ConfigNode {
    const char* name;
    
    // 统一操作
    void (*print)(ConfigNode*, int indent);
    void (*add)(ConfigNode*, ConfigNode*);
    void (*remove)(ConfigNode*, ConfigNode*);
    ConfigNode* (*find)(ConfigNode*, const char* name);
    int (*getValue)(ConfigNode*, char* buf, int size);  // 仅对文件有意义
};

// ============================================================
// Leaf：配置参数文件（叶子节点）
// ============================================================
typedef struct _ConfigFile {
    ConfigNode base;
    char value[64];
} ConfigFile;

static void configfile_print(ConfigNode* node, int indent)
{
    ConfigFile* f = (ConfigFile*)node;
    printf("%*s[FILE] %s = %s\n", indent, "", node->name, f->value);
}

static void configfile_add(ConfigNode* node, ConfigNode* child)
{
    (void)node; (void)child;
    printf("  [ERROR] Cannot add to a file node: %s\n", node->name);
}

static void configfile_remove(ConfigNode* node, ConfigNode* child)
{
    (void)node; (void)child;
    printf("  [ERROR] Cannot remove from a file node: %s\n", node->name);
}

static ConfigNode* configfile_find(ConfigNode* node, const char* name)
{
    if (strcmp(node->name, name) == 0) return node;
    return NULL;
}

static int configfile_getValue(ConfigNode* node, char* buf, int size)
{
    ConfigFile* f = (ConfigFile*)node;
    snprintf(buf, size, "%s", f->value);
    return strlen(f->value);
}

ConfigFile* new_ConfigFile(const char* name, const char* value)
{
    ConfigFile* f = calloc(1, sizeof(ConfigFile));
    f->base.name = name;
    strncpy(f->value, value, 63);
    f->base.print   = configfile_print;
    f->base.add     = configfile_add;
    f->base.remove  = configfile_remove;
    f->base.find    = configfile_find;
    f->base.getValue = configfile_getValue;
    return f;
}

// ============================================================
// Composite：配置目录（容器节点）
// ============================================================
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
        printf("  [DIR] Added '%s' to '%s'\n", child->name, node->name);
    } else {
        printf("  [ERROR] Directory '%s' is full!\n", node->name);
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
            printf("  [DIR] Removed '%s' from '%s'\n", child->name, node->name);
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

static int configdir_getValue(ConfigNode* node, char* buf, int size)
{
    (void)node; (void)buf; (void)size;
    return -1;  // 目录没有值
}

ConfigDir* new_ConfigDir(const char* name)
{
    ConfigDir* d = calloc(1, sizeof(ConfigDir));
    d->base.name = name;
    d->base.print    = configdir_print;
    d->base.add      = configdir_add;
    d->base.remove   = configdir_remove;
    d->base.find     = configdir_find;
    d->base.getValue = configdir_getValue;
    return d;
}

// ============================================================
// 辅助函数：递归统计树信息
// ============================================================
typedef struct _TreeStats {
    int dir_count;
    int file_count;
} TreeStats;

static void count_tree(ConfigNode* node, TreeStats* stats)
{
    if (node->find == configdir_find) {
        stats->dir_count++;
        ConfigDir* dir = (ConfigDir*)node;
        for (int i = 0; i < dir->child_count; i++) {
            count_tree(dir->children[i], stats);
        }
    } else {
        stats->file_count++;
    }
}

// ============================================================
int main()
{
    printf("========== 组合模式演示 ==========\n\n");

    // 构建一个配置树（类似 /etc 或 /sys）
    //
    // root/
    //   ├── system/
    //   │   ├── hostname = " embedded-box "
    //   │   ├── version  = " 1.0.0 "
    //   │   └── network/
    //   │       ├── ip   = " 192.168.1.100 "
    //   │       ├── mask = " 255.255.255.0 "
    //   │       └── gw   = " 192.168.1.1 "
    //   ├── wifi/
    //   │   ├── ssid = " MyAP "
    //   │   └── psk  = " ********* "
    //   └── debug/
    //       ├── level  = " INFO "
    //       └── enabled = " true "

    printf("--- 构建配置树 ---\n");
    
    ConfigDir* root  = new_ConfigDir("root");
    ConfigDir* sys   = new_ConfigDir("system");
    ConfigDir* net   = new_ConfigDir("network");
    ConfigFile* host = new_ConfigFile("hostname", "embedded-box");
    ConfigFile* ver  = new_ConfigFile("version",  "1.0.0");
    ConfigFile* ip   = new_ConfigFile("ip",   "192.168.1.100");
    ConfigFile* mask = new_ConfigFile("mask",  "255.255.255.0");
    ConfigFile* gw   = new_ConfigFile("gw",   "192.168.1.1");
    ConfigDir* wifi  = new_ConfigDir("wifi");
    ConfigFile* ssid = new_ConfigFile("ssid", "MyAP");
    ConfigFile* psk  = new_ConfigFile("psk",  "*********");
    ConfigDir* debug = new_ConfigDir("debug");
    ConfigFile* lvl  = new_ConfigFile("level", "INFO");
    ConfigFile* enb  = new_ConfigFile("enabled", "true");

    // 组装树
    root->base.add(&root->base,  &sys->base);
    root->base.add(&root->base,  &wifi->base);
    root->base.add(&root->base,  &debug->base);
    
    sys->base.add(&sys->base,  host);
    sys->base.add(&sys->base,  ver);
    sys->base.add(&sys->base,  &net->base);
    
    net->base.add(&net->base, ip);
    net->base.add(&net->base, mask);
    net->base.add(&net->base, gw);
    
    wifi->base.add(&wifi->base, ssid);
    wifi->base.add(&wifi->base, psk);
    
    debug->base.add(&debug->base, lvl);
    debug->base.add(&debug->base, enb);
    
    // 统一打印：调用者完全不知道是文件还是目录
    printf("\n--- 打印配置树（统一接口）---\n");
    root->base.print(&root->base, 0);
    
    // 统计树信息
    TreeStats stats = {0};
    count_tree(&root->base, &stats);
    printf("\n--- 树统计 ---\n");
    printf("目录数: %d, 文件数: %d\n", stats.dir_count, stats.file_count);
    
    // 查找节点
    printf("\n--- 查找节点 ---\n");
    ConfigNode* found = root->base.find(&root->base, "ip");
    if (found) {
        printf("找到 '%s' = ", found->name);
        char buf[64];
        if (found->getValue(found, buf, sizeof(buf)) >= 0) {
            printf("%s\n", buf);
        }
    }
    
    found = root->base.find(&root->base, "network");
    if (found) {
        printf("找到目录 '%s'\n", found->name);
    }
    
    found = root->base.find(&root->base, "nonexistent");
    if (!found) {
        printf("'nonexistent' 不存在\n");
    }
    
    // 修改配置值
    printf("\n--- 修改配置 ---\n");
    strcpy(ip->value, "10.0.0.42");
    printf("修改 ip 为 '%s'\n", ip->value);
    
    // 删除节点
    printf("\n--- 删除节点 ---\n");
    root->base.remove(&root->base, &debug->base);
    printf("\n--- 重新打印树 ---\n");
    root->base.print(&root->base, 0);
    
    printf("\n========== 演示结束 ==========\n");
    return 0;
}
