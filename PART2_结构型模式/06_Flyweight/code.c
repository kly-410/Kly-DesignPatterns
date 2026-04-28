/*
 * 06_Flyweight - 享元模式
 * 
 * 享元模式核心：共享细粒度对象，减少内存
 * 
 * 本代码演示：
 * 1. 字符串驻留（String Interning）
 * 2. PCIe 设备类型池
 * 3. 内部状态 vs 外部状态
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 享元工厂：字符串驻留池
// ============================================================
#define MAX_INTERNED 128

typedef struct _StringIntern {
    const char* pool[MAX_INTERNED];
    int count;
} StringIntern;

static StringIntern* global_intern = NULL;

const char* string_intern(const char* str)
{
    if (!global_intern) {
        global_intern = calloc(1, sizeof(StringIntern));
    }
    
    // 查找是否已在池中
    for (int i = 0; i < global_intern->count; i++) {
        if (strcmp(global_intern->pool[i], str) == 0) {
            return global_intern->pool[i];
        }
    }
    
    // 新增字符串
    if (global_intern->count < MAX_INTERNED) {
        global_intern->pool[global_intern->count] = strdup(str);
        return global_intern->pool[global_intern->count++];
    }
    return str;  // 池满了，直接返回
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

int string_intern_count(void)
{
    return global_intern ? global_intern->count : 0;
}

// ============================================================
// LogEntry：日志条目
// - level, module, message 是外部状态（可共享的字符串）
// - timestamp 是外部状态（独享的）
// ============================================================
typedef struct _LogEntry {
    const char* level;
    const char* module;
    const char* message;
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
// 演示1：大量日志的字符串驻留
// ============================================================
void demo_string_interning(void)
{
    printf("========== 演示1: 字符串驻留 ==========\n\n");
    
    LogEntry* entries[16];
    entries[0] = new_LogEntry("INFO",  "NET",  "Packet sent",      1001);
    entries[1] = new_LogEntry("INFO",  "NET",  "Packet sent",      1002);
    entries[2] = new_LogEntry("ERROR", "NET",  "Timeout",          1003);
    entries[3] = new_LogEntry("INFO",  "NET",  "Packet sent",      1004);
    entries[4] = new_LogEntry("ERROR", "DDR",  "ECC error",         1005);
    entries[5] = new_LogEntry("WARN",  "DDR",  "ECC correctable",  1006);
    entries[6] = new_LogEntry("INFO",  "DISP", "Frame drawn",      1007);
    entries[7] = new_LogEntry("ERROR", "NET",  "CRC fail",          1008);
    entries[8] = new_LogEntry("INFO",  "DISP", "Frame drawn",      1009);
    entries[9] = new_LogEntry("WARN",  "PMIC", "Low voltage",      1010);
    entries[10] = new_LogEntry("INFO", "NET",  "Packet sent",      1011);
    entries[11] = new_LogEntry("ERROR", "DDR", "ECC error",        1012);
    entries[12] = new_LogEntry("INFO", "NET",  "Packet sent",      1013);
    entries[13] = new_LogEntry("INFO", "DISP", "Frame drawn",     1014);
    entries[14] = new_LogEntry("ERROR", "NET", "Timeout",         1015);
    entries[15] = new_LogEntry("INFO", "DDR", "Memory initialized",1016);

    printf("--- 打印所有日志 ---\n");
    for (int i = 0; i < 16; i++) {
        print_log(entries[i]);
    }
    
    printf("\n--- 字符串共享验证 ---\n");
    printf("  entries[0]->level (%p) == entries[2]->level (%p): %s\n",
           (void*)entries[0]->level, (void*)entries[2]->level,
           entries[0]->level == entries[2]->level ? "SHARED ✓" : "different");
    printf("  entries[0]->module (%p) == entries[4]->module (%p): %s\n",
           (void*)entries[0]->module, (void*)entries[4]->module,
           entries[0]->module == entries[4]->module ? "SHARED ✓" : "different");
    
    printf("\n--- 内存分析 ---\n");
    int shared_count = string_intern_count();
    printf("  共创建 %d 条日志\n", 16);
    printf("  字符串池中只有 %d 个不同字符串\n", shared_count);
    printf("  节省：%d × (平均字符串长度) × 2 指针空间\n", 16 - shared_count);
    printf("  加上 timestamp 等字段，LogEntry 本身还是独享的\n");
    printf("  但字符串本身被大量复用，内存大幅下降\n");
    
    for (int i = 0; i < 16; i++) free(entries[i]);
}

// ============================================================
// 享元：PCIe 设备类型（内部状态：共享）
// ============================================================
typedef struct _PCIDeviceType {
    int vendor_id;
    int device_id;
    const char* name;
    const char* driver_name;
} PCIDeviceType;

static PCIDeviceType device_type_pool[] = {
    {0x8086, 0x100E, "Intel E1000 (Gigabit)",      "e1000e"},
    {0x8086, 0x1539, "Intel I218 (Gigabit)",        "e1000e"},
    {0x10EC, 0x8168, "Realtek RTL8111 (Gigabit)",   "r8169"},
    {0x1022, 0x2001, "AMD 10GbE Ethernet",          "amd-xgbe"},
    {0x10B4, 0x9230, "SysKonnect SK-9843",          "sk98lin"},
};
#define TYPE_POOL_SIZE (int)(sizeof(device_type_pool)/sizeof(device_type_pool[0]))

// ============================================================
// 外部状态：PCIe 设备实例（独享）
// ============================================================
typedef struct _PCIDevice {
    PCIDeviceType* type;  // 享元指针
    int bus, dev, func;
    void* bar[6];
} PCIDevice;

static PCIDeviceType* find_type(int vendor, int device)
{
    for (int i = 0; i < TYPE_POOL_SIZE; i++) {
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
    printf("  PCIe %02x:%02x.%x: %s [%s]\n",
           d->bus, d->dev, d->func,
           d->type ? d->type->name : "Unknown",
           d->type ? d->type->driver_name : "none");
}

// ============================================================
// 演示2：PCIe 设备类型池
// ============================================================
void demo_pci_flyweight(void)
{
    printf("\n========== 演示2: PCIe 设备类型池 ==========\n\n");
    
    // 系统中可能有多个同型号设备
    PCIDevice* devices[6];
    devices[0] = new_PCIDevice(0x8086, 0x100E, 1, 0, 0);  // Intel E1000
    devices[1] = new_PCIDevice(0x8086, 0x100E, 2, 0, 0);  // 另一个 E1000
    devices[2] = new_PCIDevice(0x10EC, 0x8168, 3, 0, 0);  // Realtek
    devices[3] = new_PCIDevice(0x10EC, 0x8168, 4, 0, 0);  // 另一个 Realtek
    devices[4] = new_PCIDevice(0x8086, 0x1539, 5, 0, 0);  // I218
    devices[5] = new_PCIDevice(0xFFFF, 0xFFFF, 0, 0, 0);  // 未知设备

    printf("--- 所有 PCIe 设备 ---\n");
    for (int i = 0; i < 6; i++) {
        print_pci_device(devices[i]);
    }
    
    printf("\n--- 类型池验证 ---\n");
    printf("  devices[0]->type == devices[1]->type: %s\n",
           devices[0]->type == devices[1]->type ? "SHARED ✓" : "different");
    printf("  devices[2]->type == devices[3]->type: %s\n",
           devices[2]->type == devices[3]->type ? "SHARED ✓" : "different");
    
    printf("\n--- 内存分析 ---\n");
    printf("  如果不共享：6 × sizeof(PCIDeviceType) = 6 × %zu bytes\n", 
           sizeof(PCIDeviceType));
    printf("  共享后：只存一份类型数据，设备实例只存指针\n");
    printf("  类型数据大小：%zu bytes\n", sizeof(PCIDeviceType) * TYPE_POOL_SIZE);
    
    for (int i = 0; i < 6; i++) free(devices[i]);
}

// ============================================================
int main()
{
    printf("========== 享元模式演示 ==========\n\n");
    
    demo_string_interning();
    demo_pci_flyweight();
    
    string_intern_free_all();
    
    printf("\n========== 总结 ==========\n");
    printf("享元模式把对象分成两部分：\n");
    printf("  - 内部状态（Intrinsic）：可共享，多个对象共用一份\n");
    printf("  - 外部状态（Extrinsic）：上下文相关，每个对象独享\n");
    printf("  - 字符串驻留：level/module/message 是内部状态\n");
    printf("  - PCIe 类型池：name/driver 是内部状态\n");
    
    return 0;
}
