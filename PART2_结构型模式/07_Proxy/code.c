/*
 * 07_Proxy - 代理模式
 * 
 * 代理模式核心：为其他对象提供代理以控制对这个对象的访问
 * 
 * 本代码演示：
 * 1. 虚代理：PCIe BAR 延迟 ioremap
 * 2. 保护代理：访问控制（ACL）
 * 3. 智能引用：引用计数
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================
// Subject 接口：代理和真实对象都实现这个接口
// ============================================================
typedef struct _RegAccess RegAccess;
struct _RegAccess {
    void (*write32)(RegAccess*, uint32_t offset, uint32_t val);
    uint32_t (*read32)(RegAccess*, uint32_t offset);
    void (*close)(RegAccess*);
};

// ============================================================
// RealSubject：真实的 PCIe BAR 访问
// ============================================================
typedef struct _PCIeBAR {
    RegAccess base;
    uint64_t phys_addr;
    void* io_remapped;
    int mapped;
    int refcount;
} PCIeBAR;

static void bar_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (!bar->mapped) {
        printf("  [BAR] ERROR: not ioremap'ed!\n");
        return;
    }
    printf("  [BAR] WRITE @0x%04X <- 0x%08X\n", offset, val);
}

static uint32_t bar_read32(RegAccess* h, uint32_t offset)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (!bar->mapped) {
        printf("  [BAR] ERROR: not ioremap'ed!\n");
        return 0xDEAD;
    }
    printf("  [BAR] READ  @0x%04X -> 0xDEADBEEF\n", offset);
    return 0xDEADBEEF;
}

static void bar_close(RegAccess* h)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    printf("  [BAR] close (refcount=%d)\n", bar->refcount);
    if (bar->mapped) {
        bar->mapped = 0;
    }
    free(bar);
}

PCIeBAR* new_PCIeBAR(uint64_t phys_addr)
{
    PCIeBAR* bar = calloc(1, sizeof(PCIeBAR));
    bar->phys_addr = phys_addr;
    bar->refcount = 1;
    bar->base.write32 = bar_write32;
    bar->base.read32  = bar_read32;
    bar->base.close   = bar_close;
    return bar;
}

static void ioremap_bar(PCIeBAR* bar)
{
    if (bar->mapped) return;
    bar->io_remapped = (void*)(0xFFFF0000ULL + bar->phys_addr);
    bar->mapped = 1;
    printf("  [BAR] ioremap 0x%llX -> %p\n",
           (unsigned long long)bar->phys_addr, bar->io_remapped);
}

// ============================================================
// Proxy 1：虚代理（延迟加载）
// 第一次访问时才 ioremap
// ============================================================
typedef struct _LazyBARProxy {
    RegAccess base;
    PCIeBAR* real_bar;
} LazyBARProxy;

static void lazy_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [LazyProxy] First access - trigger ioremap\n");
        ioremap_bar(p->real_bar);
    }
    p->real_bar->base.write32(&p->real_bar->base, offset, val);
}

static uint32_t lazy_read32(RegAccess* h, uint32_t offset)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [LazyProxy] First access - trigger ioremap\n");
        ioremap_bar(p->real_bar);
    }
    return p->real_bar->base.read32(&p->real_bar->base, offset);
}

static void lazy_close(RegAccess* h)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    printf("  [LazyProxy] close\n");
    p->real_bar->refcount--;
    if (p->real_bar->refcount <= 0) {
        p->real_bar->base.close(&p->real_bar->base);
    }
    free(p);
}

LazyBARProxy* new_LazyBARProxy(PCIeBAR* real_bar)
{
    LazyBARProxy* p = calloc(1, sizeof(LazyBARProxy));
    p->base.write32 = lazy_write32;
    p->base.read32  = lazy_read32;
    p->base.close   = lazy_close;
    p->real_bar = real_bar;
    real_bar->refcount++;
    return p;
}

// ============================================================
// Proxy 2：保护代理（访问控制）
// ============================================================
typedef struct _ACL {
    int uid;
} ACL;

static int check_write_permission(ACL* acl)
{
    if (acl->uid == 0) return 1;  // root
    return 0;  // non-root cannot write
}

typedef struct _ProtectedBARProxy {
    RegAccess base;
    PCIeBAR* real_bar;
    ACL* acl;
} ProtectedBARProxy;

static void prot_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    if (!check_write_permission(p->acl)) {
        printf("  [ACL] DENIED: uid=%d cannot write PCIe BAR\n", p->acl->uid);
        return;
    }
    p->real_bar->base.write32(&p->real_bar->base, offset, val);
}

static uint32_t prot_read32(RegAccess* h, uint32_t offset)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    // 读操作对所有用户开放
    if (!p->real_bar->mapped) {
        printf("  [Proxy] ioremap (implicit for read)\n");
        ioremap_bar(p->real_bar);
    }
    return p->real_bar->base.read32(&p->real_bar->base, offset);
}

static void prot_close(RegAccess* h)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    printf("  [ProtectedProxy] close\n");
    free(p);
}

ProtectedBARProxy* new_ProtectedBARProxy(PCIeBAR* real_bar, ACL* acl)
{
    ProtectedBARProxy* p = calloc(1, sizeof(ProtectedBARProxy));
    p->base.write32 = prot_write32;
    p->base.read32  = prot_read32;
    p->base.close   = prot_close;
    p->real_bar = real_bar;
    p->acl = acl;
    return p;
}

// ============================================================
// 演示
// ============================================================
void demo_lazy_proxy(void)
{
    printf("--- 虚代理：延迟 ioremap ---\n");
    
    PCIeBAR* bar = new_PCIeBAR(0x40000000ULL);
    printf("创建 PCIe BAR @ 0x40000000（未 ioremap）\n");
    
    LazyBARProxy* proxy = new_LazyBARProxy(bar);
    
    printf("\n第一次写操作（触发延迟 ioremap）:\n");
    proxy->base.write32(&proxy->base, 0x10, 0x12345678);
    
    printf("\n第二次写操作（已映射，直接访问）:\n");
    proxy->base.write32(&proxy->base, 0x14, 0xABCDEF00);
    
    proxy->base.close(&proxy->base);
}

void demo_protected_proxy(void)
{
    printf("\n--- 保护代理：访问控制 ---\n");
    
    PCIeBAR* bar = new_PCIeBAR(0x50000000ULL);
    ACL root_acl = {.uid = 0};
    ACL user_acl = {.uid = 1000};
    
    printf("创建受保护的 PCIe BAR\n");
    
    printf("\n以 root (uid=0) 写入:\n");
    ProtectedBARProxy* root_proxy = new_ProtectedBARProxy(bar, &root_acl);
    root_proxy->base.write32(&root_proxy->base, 0x20, 0xDEADBEEF);
    
    printf("\n以普通用户 (uid=1000) 写入:\n");
    ProtectedBARProxy* user_proxy = new_ProtectedBARProxy(bar, &user_acl);
    user_proxy->base.write32(&user_proxy->base, 0x20, 0xCAFEBABE);
    
    printf("\n读取操作（对所有用户开放）:\n");
    user_proxy->base.read32(&user_proxy->base, 0x20);
    
    root_proxy->base.close(&root_proxy->base);
    user_proxy->base.close(&user_proxy->base);
}

int main()
{
    printf("========== 代理模式演示 ==========\n\n");
    
    demo_lazy_proxy();
    demo_protected_proxy();
    
    printf("\n========== 代理模式类型总结 ==========\n");
    printf("虚代理（Lazy Proxy）：\n");
    printf("  延迟加载，第一次访问时才创建真实对象\n");
    printf("  场景：ioremap、驱动加载、数据库连接池\n\n");
    printf("保护代理（Protection Proxy）：\n");
    printf("  在访问前检查权限，控制对对象的访问\n");
    printf("  场景：文件系统权限、硬件寄存器保护\n\n");
    printf("智能引用（Smart Reference）：\n");
    printf("  访问对象时执行额外的引用计数、缓存清理等\n");
    printf("  场景：C++ 智能指针、内核 slab 分配器\n");
    
    return 0;
}
