/*
 * 07_Proxy - 代理模式
 *
 * 代理模式核心：为其他对象提供代理以控制对这个对象的访问。
 * 代理对象与真实对象实现相同的接口（Subject），客户端无需知道
 * 真正操作的是代理还是真实对象。
 *
 * 硬件背景（PCIe 场景）：
 *   PCIe BAR（Base Address Register）是 PCIe 配置空间的一部分，
 *   用于将 PCIe设备的物理地址空间映射到 CPU 的地址空间。
 *   驱动在访问 BAR 之前，需要通过 ioremap() 将物理地址映射到
 *   CPU 可访问的虚拟地址，这个过程有一定开销（尤其是大规模 MMIO）。
 *
 *   代理模式在 PCIe 驱动中的典型应用：
 *   - 虚代理：延迟 ioremap（Lazy ioremap），只在第一次寄存器访问时映射
 *   - 保护代理：ACL 访问控制，防止非特权进程/内核模块误写关键寄存器
 *   - 智能引用：引用计数，追踪有多少使用者持有 BAR 的引用
 *
 * 本代码演示：
 *   1. 虚代理（Lazy Proxy）：PCIe BAR 延迟 ioremap
 *   2. 保护代理（Protection Proxy）：访问控制（ACL）
 *   3. 智能引用（Smart Reference）：引用计数
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
 * ============================================================
 * Subject 接口：代理和真实对象都实现这个接口
 * ============================================================
 *
 * 为什么这样做：
 *   Subject 接口定义了 RealSubject 和 Proxy 的共同行为，
 *   保证客户端代码可以用同一套方式访问真实对象或代理对象。
 *   这就是面向对象中"针对接口编程，而不针对实现编程"的原则。
 *
 *   在 Linux 内核中，这相当于定义一个通用的寄存器操作抽象层，
 *   上层驱动只调用 readl()/writel()，而不关心底层是 PCIe BAR、
 *   还是 Platform 设备的 I/O 内存，或者是 GPIO 寄存器。
 *
 * 接口设计说明：
 *   - write32：将 32 位值写入指定偏移的寄存器
 *   - read32：从指定偏移的寄存器读取 32 位值
 *   - close：关闭/释放访问句柄（释放资源）
 *
 * C 语言实现方式：
 *   用函数指针模拟接口。结构体的第一个成员（base）是一个
 *   函数指针表，类似于 C++ 的纯虚函数或 Java 的 Interface。
 */
typedef struct _RegAccess RegAccess;

/*
 * RegAccess - 寄存器访问接口
 *
 * 为什么这样设计：
 *   C 语言没有原生的 Interface 支持，所以我们用函数指针来模拟。
 *   每个实现接口的结构体（PCIeBAR、LazyBARProxy 等）
 *   都会定义这三个函数指针，分别指向自己的实现。
 *
 *   客户端代码只需要保存 RegAccess* 指针，调用指针指向的函数，
 *   无需关心实际对象是什么类型——这是"鸭子类型"（Duck Typing）
 *   在 C 语言中的实现方式。
 *
 * 接口语义：
 *   - write32(offset, val)：向设备寄存器写入值（寄存器寻址基于 BAR 内的偏移）
 *   - read32(offset)：读取设备寄存器的值
 *   - close()：释放当前对象（真实对象关闭 BAR 映射，代理对象减少引用计数）
 */
struct _RegAccess {
    /*
     * write32 - 向寄存器写入 32 位值
     *
     * 参数说明：
     *   - h：指向当前对象（RegAccess*），用于虚函数表分派
     *   - offset：寄存器在 BAR 空间内的偏移（单位：字节，4 字节对齐）
     *   - val：要写入的 32 位值
     */
    void (*write32)(RegAccess* h, uint32_t offset, uint32_t val);

    /*
     * read32 - 从寄存器读取 32 位值
     *
     * 参数说明：
     *   - h：指向当前对象
     *   - offset：寄存器偏移
     *   返回：读取到的 32 位值（如果未映射，默认返回 0xDEAD）
     */
    uint32_t (*read32)(RegAccess* h, uint32_t offset);

    /*
     * close - 关闭访问句柄
     *
     * 为什么需要 close：
     *   在 PCIe BAR 场景中，ioremap() 分配的虚拟地址空间是有限资源。
     *   驱动卸载时必须调用 iounmap() 释放映射，否则会内存泄漏。
     *   close() 方法统一了释放逻辑，无论是直接关闭 BAR 还是
     *   通过代理关闭，最终都能正确释放资源。
     */
    void (*close)(RegAccess* h);
};

/*
 * ============================================================
 * RealSubject：真实的 PCIe BAR 访问
 * ============================================================
 *
 * PCIe BAR 背景知识：
 *   每个 PCIe 设备最多有 6 个 BAR（BAR0~BAR5），每个 BAR 描述一个地址空间。
 *   BAR 的大小由硬件决定（可以是 1KB、4KB、1MB、8MB 等），
 *   CPU 通过读取配置空间 BAR 寄存器获取大小和当前分配的地址。
 *
 *   典型 PCIe 设备 BAR 布局（以 NVMe SSD 为例）：
 *     BAR0：MSI-X 向量表（通常 4KB）
 *     BAR1：PCIe 寄存器空间（通常 4KB）
 *     BAR3：NVMe Queue Doorbell 等（通常 4KB）
 *     BAR4：PRP/SGL 描述符内存窗口（通常 1MB~8MB）
 *
 *   真实驱动访问流程：
 *     1. 读取 PCIe 配置空间，获取 BAR 的物理地址
 *     2. 调用 ioremap(phys_addr, size) 映射到虚拟地址
 *     3. 通过虚拟地址访问设备寄存器（readl/writel）
 *     4. 驱动卸载时调用 iounmap()
 */

/*
 * PCIeBAR - PCIe BAR 访问对象（RealSubject）
 *
 * 成员说明：
 *   - base：接口函数指针（每个实现都不同）
 *   - phys_addr：BAR 的物理起始地址（从 PCIe 配置空间读取）
 *   - io_remapped：ioremap 后的虚拟地址（NULL 表示未映射）
 *   - mapped：映射状态标志（0=未 ioremap，1=已 ioremap）
 *   - refcount：引用计数（记录有多少个代理/使用者持有此 BAR）
 */
typedef struct _PCIeBAR {
    RegAccess base;
    uint64_t phys_addr;   /* BAR 物理起始地址，从 PCIe 配置空间读取 */
    void* io_remapped;    /* ioremap 后的虚拟地址，访问寄存器时使用 */
    int mapped;           /* 映射状态：0=未映射，1=已映射 */
    int refcount;         /* 引用计数：追踪有多少使用者持有此对象 */
} PCIeBAR;

/*
 * bar_write32 - 向 BAR 空间内的寄存器写入 32 位值
 *
 * 为什么这样做：
 *   写操作直接访问已经 ioremap 的虚拟地址（io_remapped）。
 *   在真实硬件上，writel(val, virt_addr) 会产生一个 Posted Write
 *   或 Non-Posted Write 事务，通过 PCIe 总线发送到设备。
 *
 * 为什么检查 mapped：
 *   如果 BAR 尚未 ioremap，写入的地址是无效的虚拟地址，
 *   会导致 CPU 异常（Page Fault 或更严重的 Undefined Abort）。
 *   这里的检查防止了这种非法访问。
 *
 * 真实硬件对应：
 *   offset = 0x10 的写操作，可能对应某个 BAR 空间内设备的
 *   特定控制寄存器（如 NVMe 的 Admin Queue Doorbell）。
 */
static void bar_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    if (!bar->mapped) {
        printf("  [BAR] ERROR: not ioremap'ed!\n");
        return;
    }
    printf("  [BAR] WRITE @0x%04X <- 0x%08X\n", offset, val);
}

/*
 * bar_read32 - 从 BAR 空间内的寄存器读取 32 位值
 *
 * 为什么这样做：
 *   读操作通过 ioremap 后的虚拟地址访问 PCIe 设备寄存器。
 *   真实硬件上，readl(virt_addr) 会发起一个 PCIe Memory Read 事务（TLP）。
 *
 * 返回值约定：
 *   如果 BAR 未映射，返回 0xDEAD 作为错误标识。
 *   0xDEAD 是嵌入式开发中常用的"魔法数字"（Magic Number），
 *   用于标识已知错误状态，便于调试时快速识别问题。
 *
 * 为什么打印 0xDEADBEEF：
 *   这是一个用于测试的返回值（Mock Value），代表"硬件应该返回的值未知"。
 *   在真实驱动中，如果读到了这个值，说明固件或硬件可能有问题。
 */
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

/*
 * bar_close - 关闭 PCIe BAR
 *
 * 为什么这样做：
 *   close() 的职责是释放 PCIe BAR 占用的资源：
 *     1. 如果已 ioremap，调用 iounmap() 释放虚拟地址映射
 *     2. 打印当前引用计数（用于调试）
 *     3. 释放 PCIeBAR 结构体本身的内存
 *
 * 为什么需要引用计数检查：
 *   PCIe BAR 可能被多个使用者共享（如设备的 MSI-X 表和普通寄存器
 *   映射由不同的驱动模块使用）。只有当 refcount 降到 0 时，
 *   才真正释放 BAR 资源。
 *
 * 真实 Linux 内核对应：
 *   类似于 pci_iounmap() 和 kref_put() 的组合。
 */
static void bar_close(RegAccess* h)
{
    PCIeBAR* bar = (PCIeBAR*)h;
    printf("  [BAR] close (refcount=%d)\n", bar->refcount);
    if (bar->mapped) {
        /* iounmap：Linux 内核函数，释放 ioremap 分配的虚拟地址 */
        bar->mapped = 0;
    }
    free(bar);
}

/*
 * new_PCIeBAR - 构造函数：创建真实的 PCIe BAR 对象
 *
 * 参数 phys_addr：
 *   PCIe BAR 的物理地址，从 PCIe 配置空间的 BAR Register 读取。
 *   真实场景中，驱动通过 pci_read_config_bar() 或类似 API 获取。
 *
 * 为什么用 calloc：
 *   calloc 分配内存并清零，保证所有字段有明确定义的初始值。
 *   避免结构体中未初始化的指针字段导致随机访问错误。
 *
 * refcount 初始化为 1：
 *   表示新创建的对象有且只有一个持有者（调用者）。
 *   如果通过 new_LazyBARProxy 创建代理，refcount 会 +1，
 *   这样最后一个 close 的调用者会触发真正的 free。
 */
PCIeBAR* new_PCIeBAR(uint64_t phys_addr)
{
    PCIeBAR* bar = calloc(1, sizeof(PCIeBAR));
    bar->phys_addr = phys_addr;
    bar->refcount = 1;  /* 新对象初始引用计数为 1 */
    bar->base.write32 = bar_write32;
    bar->base.read32  = bar_read32;
    bar->base.close   = bar_close;
    return bar;
}

/*
 * ioremap_bar - 执行 PCIe BAR 的物理地址到虚拟地址的映射
 *
 * 为什么这样做（延迟 ioremap 的核心）：
 *   ioremap 是将 MMIO 物理地址映射到内核虚拟地址空间的过程。
 *   在真实系统中，这个操作有以下开销：
 *     1. 需要分配一个新的虚拟地址区域（vmalloc 或 ioremap range）
 *     2. 需要建立页表条目（Page Table Entries）
 *     3. 在某些 ARM SoC 上，还需要配置 IOMMU（IO Memory Management Unit）
 *
 *   延迟 ioremap 的优势：
 *     - 如果设备在整个驱动生命周期内没有被访问，就避免了 ioremap 开销
 *     - 在资源受限的嵌入式系统中，延迟映射可以节省虚拟地址空间
 *
 * 为什么先检查 mapped：
 *   防止重复 ioremap。同一物理地址区域只能映射一次，
 *   重复映射会覆盖原有的虚拟地址，导致原有映射失效（use-after-free）。
 *
 * 真实 Linux API：
 *   void __iomem* ioremap(phys_addr_t phys_addr, size_t size)
 *   void iounmap(void __iomem *addr)
 *
 * 模拟实现说明：
 *   这里用 (void*)(0xFFFF0000ULL + bar->phys_addr) 模拟虚拟地址。
 *   真实系统中，ioremap 分配的地址由内核的 vmalloc 区域决定。
 */
static void ioremap_bar(PCIeBAR* bar)
{
    if (bar->mapped) return;
    bar->io_remapped = (void*)(0xFFFF0000ULL + bar->phys_addr);
    bar->mapped = 1;
    printf("  [BAR] ioremap 0x%llX -> %p\n",
           (unsigned long long)bar->phys_addr, bar->io_remapped);
}

/*
 * ============================================================
 * Proxy 1：虚代理（Lazy Proxy / Virtual Proxy）
 * ============================================================
 *
 * 虚代理的核心思想：延迟加载（Lazy Initialization）。
 * 只有当客户端真正需要使用对象时，才创建真实对象。
 *
 * 为什么需要虚代理（PCIe 场景）：
 *   1. ioremap 的虚拟地址空间是有限的（特别是在 32 位嵌入式系统上）
 *   2. PCIe BAR 数量可能很多（每个设备 1~6 个 BAR），如果全部预映射，
 *      会消耗大量虚拟地址空间
 *   3. 某些 PCIe 设备仅在特定条件下才会被访问（如热插拔设备、
 *      调试模式下的额外 BAR）
 *   4. ioremap 本身有性能开销（TLB shootdown、IOMMU 配置），
 *      延迟映射可以优化启动时间
 *
 * 使用虚代理的代价：
 *   第一次访问时会有额外开销（ioremap + 可能分配数据结构），
 *   但后续访问与直接访问无差异。
 *
 * 真实 Linux 驱动中的虚代理：
 *   在 PCI 驱动中，devm_ioremap_resource() 是常见的预映射方案，
 *   但延迟映射需要驱动自己实现（检查 mapped 标志，按需 ioremap）。
 */

/*
 * LazyBARProxy - 虚代理：延迟 ioremap 的 PCIe BAR 访问代理
 *
 * 成员说明：
 *   - base：接口函数指针（指向代理的实现函数）
 *   - real_bar：指向真实 PCIe BAR 对象的指针（代理持有真实对象的引用）
 *
 * 为什么 real_bar 指针不直接操作：
 *   代理不"拥有"真实对象，而是"持有"对真实对象的引用。
 *   代理将操作委托给 real_bar，但可以在委托前后添加自己的逻辑
 *   （如触发 ioremap、增加引用计数）。
 */
typedef struct _LazyBARProxy {
    RegAccess base;
    PCIeBAR* real_bar;  /* 真实对象，代理将请求委托给它 */
} LazyBARProxy;

/*
 * lazy_write32 - 代理写操作：按需触发 ioremap
 *
 * 为什么这样做：
 *   这是虚代理的核心——"延迟初始化"（Lazy Initialization）。
 *   在第一次写操作（也是第一次任何访问）发生之前，
 *   real_bar 处于未映射状态（mapped == 0）。
 *
 * 为什么不直接初始化：
 *   假设这个 BAR 对应的 PCIe 设备只有在特定条件下才被使用
 *   （如通过 PCIe Capability 的某个字段判断设备能力）。
 *   如果设备根本不会被访问，延迟 ioremap 可以避免浪费虚拟地址空间。
 *
 * 代理逻辑：
 *   1. 检查 real_bar 是否已映射
 *   2. 如果未映射，打印调试信息并触发 ioremap
 *   3. 无论是否触发 ioremap，最终都委托给真实对象的 write32
 *
 * 真实 PCIe 场景：
 *   某些 BAR 可能只在设备的"高级功能"（Advanced Features）使能后才需要访问，
 *   虚代理可以延迟到这个功能实际被使用时再映射。
 */
static void lazy_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [LazyProxy] First access - trigger ioremap\n");
        ioremap_bar(p->real_bar);
    }
    /* 委托给真实对象的写操作 */
    p->real_bar->base.write32(&p->real_bar->base, offset, val);
}

/*
 * lazy_read32 - 代理读操作：同样支持延迟 ioremap
 *
 * 为什么这样做：
 *   读操作同样可能触发 ioremap，逻辑与写操作相同。
 *   在真实 PCIe 设备中，如果尝试读一个未映射地址的寄存器，
 *   硬件会返回一个全 0 或全 F 的值，但不会报错——
 *   这可能导致驱动逻辑错误（如误判设备不存在）。
 *   因此延迟 ioremap 对读操作同样重要。
 */
static uint32_t lazy_read32(RegAccess* h, uint32_t offset)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    if (!p->real_bar->mapped) {
        printf("  [LazyProxy] First access - trigger ioremap\n");
        ioremap_bar(p->real_bar);
    }
    return p->real_bar->base.read32(&p->real_bar->base, offset);
}

/*
 * lazy_close - 代理关闭：引用计数释放
 *
 * 为什么这样做：
 *   当代理被关闭时，代理减少真实对象的引用计数。
 *   只有当引用计数降到 0（或更低）时，才真正关闭真实对象。
 *   这防止了"代理还在使用，但真实对象已被释放"的情况。
 *
 * 引用计数的重要性：
 *   在真实系统中，BAR 可能同时被多个使用者持有。
 *   例如：eth0（以太网驱动）和 eth0-wol（网络唤醒驱动）
 *   共享同一个 PCIe BAR。关闭 eth0-wol 不应该释放 BAR，
 *   因为 eth0 仍在使用。
 *
 * 为什么要 free(p)：
 *   代理对象（LazyBARProxy）本身也需要释放，
 *   因为它是 calloc 分配的结构体，不是真实 BAR 对象。
 */
static void lazy_close(RegAccess* h)
{
    LazyBARProxy* p = (LazyBARProxy*)h;
    printf("  [LazyProxy] close\n");
    p->real_bar->refcount--;
    if (p->real_bar->refcount <= 0) {
        /* 最后一个使用者关闭，释放真实 BAR */
        p->real_bar->base.close(&p->real_bar->base);
    }
    free(p);  /* 释放代理对象本身 */
}

/*
 * new_LazyBARProxy - 构造函数：创建虚代理
 *
 * 参数 real_bar：
 *   真实 PCIe BAR 对象的指针。代理持有这个指针，但不拥有它。
 *
 * 为什么传入已有的 real_bar 而不是创建新的：
 *   虚代理的目的是包装已有的真实对象，控制对它的访问，
 *   而不是创建一个新的副本。这是代理模式区别于装饰器模式的关键：
 *   - 代理：控制对已有对象的访问（一个真实对象）
 *   - 装饰器：给对象添加功能（可以叠加多个装饰器）
 *
 * 为什么 refcount++：
 *   代理持有真实对象，所以引用计数加 1。
 *   当所有代理都被关闭时（refcount 回到 1），真实对象才会被释放。
 */
LazyBARProxy* new_LazyBARProxy(PCIeBAR* real_bar)
{
    LazyBARProxy* p = calloc(1, sizeof(LazyBARProxy));
    p->base.write32 = lazy_write32;
    p->base.read32  = lazy_read32;
    p->base.close   = lazy_close;
    p->real_bar = real_bar;
    real_bar->refcount++;  /* 代理持有引用，计数 +1 */
    return p;
}

/*
 * ============================================================
 * Proxy 2：保护代理（Protection Proxy / Access Control Proxy）
 * ============================================================
 *
 * 保护代理的核心思想：访问控制。
 * 在执行实际操作之前，检查调用者是否有足够的权限。
 *
 * 为什么需要保护代理（硬件/PCIe 场景）：
 *   1. PCIe 设备的某些寄存器是"关键寄存器"（Critical Registers），
 *      如设备的 Control 寄存器（控制设备复位、电源管理等），
 *      误写可能导致系统崩溃或硬件损坏。
 *   2. 在虚拟机或容器环境中，需要防止客户机（Guest VM）
 *      直接访问宿主机的硬件资源。
 *   3. 在驱动开发阶段，防止测试代码误写关键寄存器。
 *
 *   例如：PCIe NVMe 设备的 Admin 寄存器区域应该是特权操作，
 *   而普通用户进程的读写请求应该被拒绝。
 *
 * 真实硬件保护机制：
 *   - MMU/MPU：区分用户态和内核态权限
 *   - SMMU（System Memory Management Unit）：IOMMU 的 ARM 版本，
 *     可以对 PCIe 设备的 DMA 访问做权限校验
 *   - PCIe ACS（Access Control Services）：在 PCIe 协议层面做访问控制
 */

/*
 * ACL - 访问控制列表（Access Control List）
 *
 * 为什么这样设计：
 *   ACL 是一个轻量级的权限描述符，包含调用者的身份标识（UID）。
 *   在真实系统中，UID 通常是进程 ID（用户空间）或 CPU 特权级别（内核空间）。
 *
 * 成员：
 *   - uid：用户标识符。0 通常代表 root（超级管理员），
 *          其他值代表普通用户或受限进程。
 *
 * 扩展方向：
 *   真实 ACL 可以包含多个字段：UID、GID（用户组 ID）、
 *   权限掩码（READ/WRITE/EXECUTE），以及时间窗口限制等。
 */
typedef struct _ACL {
    int uid;  /* 用户标识符：0=root/特权用户，其他=普通用户 */
} ACL;

/*
 * check_write_permission - 检查写操作权限
 *
 * 为什么这样做：
 *   写 PCIe BAR 寄存器是一个"有副作用"（Side Effect）的操作，
 *   可能改变设备的运行状态，因此需要更严格的权限控制。
 *   读操作通常是安全的（只读，不改变设备状态），所以通常对读开放。
 *
 * 权限模型：
 *   - uid == 0（root/内核代码）：允许写入
 *   - uid != 0（普通用户进程）：拒绝写入
 *
 * 真实 Linux 权限模型：
 *   在 Linux 中，设备驱动通过 inode/cdev 权限位
 *   （如 mknod 创建的设备文件权限 660）控制用户访问。
 *   但 PCIe BAR 寄存器是内核直接映射的 MMIO，不经过文件系统，
 *   所以这里用 ACL 模拟内存保护（类似 ARM TrustZone 的 TZPC）。
 */
static int check_write_permission(ACL* acl)
{
    if (acl->uid == 0) return 1;  /* root：完全信任，允许写入 */
    return 0;                      /* 非 root：拒绝写入，保护硬件 */
}

/*
 * ProtectedBARProxy - 保护代理：基于 ACL 的访问控制
 *
 * 成员说明：
 *   - base：接口函数指针（指向代理的实现函数）
 *   - real_bar：真实 BAR 对象（代理将合法请求委托给它）
 *   - acl：访问控制列表（描述当前调用者的身份和权限）
 *
 * 为什么同时持有 real_bar 和 acl：
 *   保护代理需要两个输入：真实对象（target）和权限上下文（context）。
 *   每次操作时，代理用 acl 判断是否允许操作，如果允许则委托给 real_bar。
 */
typedef struct _ProtectedBARProxy {
    RegAccess base;
    PCIeBAR* real_bar;
    ACL* acl;  /* 权限上下文，由调用者传入，代表当前请求者的身份 */
} ProtectedBARProxy;

/*
 * prot_write32 - 保护代理写操作：权限检查
 *
 * 为什么这样做：
 *   保护代理在将写请求转发给真实对象之前，先进行权限检查。
 *   这就像一个"门卫"（Doorkeeper）：没有钥匙（权限）的人无法进门。
 *
 * 为什么对写操作做权限检查，对读操作不做：
 *   这是根据本系统的安全策略设计的。
 *   写操作会改变硬件状态（可能导致硬件故障），所以需要验证权限。
 *   读操作是只读的，通常被认为是安全的。
 *
 * 真实 PCIe 场景：
 *   某些 PCIe 设备的 BAR 空间包含只读配置寄存器（如设备 ID、Vendor ID）和
 *   可写控制寄存器。保护代理可以精确到寄存器级别的权限控制，
 *   而不只是一个 BAR 级别的大颗粒保护。
 *
 * 如果权限检查失败：
 *   直接返回，不调用真实对象的 write32。
 *   在真实系统中，这里可能还会记录一条审计日志。
 */
static void prot_write32(RegAccess* h, uint32_t offset, uint32_t val)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    if (!check_write_permission(p->acl)) {
        printf("  [ACL] DENIED: uid=%d cannot write PCIe BAR\n", p->acl->uid);
        return;  /* 权限不足，操作被拒绝 */
    }
    /* 权限检查通过，委托给真实对象执行写操作 */
    p->real_bar->base.write32(&p->real_bar->base, offset, val);
}

/*
 * prot_read32 - 保护代理读操作
 *
 * 为什么这样做：
 *   读操作通常对所有用户开放（Linux 的 /dev/mem 在开启时也允许读），
 *   但为了统一接口，仍然经过代理转发。
 *
 * 隐式 ioremap：
 *   这里对未映射的 BAR 也会触发 ioremap，
 *   因为读操作需要有效的映射才能获取正确的值（全 0 或全 F 是无效的）。
 *   真实系统中，ioremap 是特权操作（需要内核权限），但代理本身在内核空间，
 *   所以可以执行。
 */
static uint32_t prot_read32(RegAccess* h, uint32_t offset)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    /* 读操作对所有用户开放 */
    if (!p->real_bar->mapped) {
        printf("  [Proxy] ioremap (implicit for read)\n");
        ioremap_bar(p->real_bar);
    }
    return p->real_bar->base.read32(&p->real_bar->base, offset);
}

/*
 * prot_close - 保护代理关闭
 *
 * 为什么这样做：
 *   保护代理不管理真实对象的引用计数（不同于虚代理）。
 *   这是因为保护代理通常是一对一的关系——每个调用者创建一个代理实例。
 *   如果需要共享真实对象，应该使用引用计数的虚代理。
 *
 * 这里只释放代理对象本身，不影响 real_bar。
 */
static void prot_close(RegAccess* h)
{
    ProtectedBARProxy* p = (ProtectedBARProxy*)h;
    printf("  [ProtectedProxy] close\n");
    free(p);  /* 仅释放代理对象，real_bar 由其他管理机制处理 */
}

/*
 * new_ProtectedBARProxy - 构造函数：创建保护代理
 *
 * 参数说明：
 *   - real_bar：真实 PCIe BAR 对象（被保护的资源）
 *   - acl：访问控制列表（描述请求者的权限）
 *
 * 为什么同时传入 real_bar 和 acl：
 *   保护代理需要知道"保护的是什么"（real_bar）和
 *   "谁在请求"（acl）。两个参数缺一不可。
 *
 * 与虚代理的区别：
 *   - 虚代理：控制对象的创建时机（延迟加载）
 *   - 保护代理：控制对象的访问权限（权限检查）
 *   两者可以组合使用（先检查权限，再延迟加载）。
 */
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

/*
 * ============================================================
 * 演示函数
 * ============================================================
 */

/*
 * demo_lazy_proxy - 演示虚代理（延迟 ioremap）
 *
 * 演示目标：
 *   展示虚代理如何延迟 ioremap，只有在第一次访问时才真正映射。
 *
 * 真实 PCIe 场景：
 *   某些 PCIe BAR（如用于调试接口的 BAR）只有在驱动加载后
 *   才会被访问。虚代理可以在启动阶段跳过这个 BAR 的映射，
 *   等到实际需要调试时才做映射，从而节省启动时间和地址空间。
 */
void demo_lazy_proxy(void)
{
    printf("--- 虚代理：延迟 ioremap ---\n");

    /* 创建 PCIe BAR 对象，物理地址 0x40000000（典型的 PCIe 设备 BAR 地址） */
    PCIeBAR* bar = new_PCIeBAR(0x40000000ULL);
    printf("创建 PCIe BAR @ 0x40000000（未 ioremap）\n");

    /* 创建虚代理包装 BAR
     * 此时 BAR 尚未 ioremap，real_bar->mapped == 0
     */
    LazyBARProxy* proxy = new_LazyBARProxy(bar);

    /* 第一次写操作：触发 ioremap
     * 在调用之前，BAR 仍然是未映射状态
     * 代理的 write32 检测到这一点，调用 ioremap_bar() 建立映射
     */
    printf("\n第一次写操作（触发延迟 ioremap）:\n");
    proxy->base.write32(&proxy->base, 0x10, 0x12345678);

    /* 第二次写操作：直接访问
     * 映射已经建立，ioremap_bar() 的 mapped 检查会跳过映射步骤
     */
    printf("\n第二次写操作（已映射，直接访问）:\n");
    proxy->base.write32(&proxy->base, 0x14, 0xABCDEF00);

    /* 关闭代理
     * 代理关闭后，refcount 减 1，但由于只有这一个代理持有引用，
     * 所以 bar_close() 被调用，真实 BAR 也被释放
     */
    proxy->base.close(&proxy->base);
}

/*
 * demo_protected_proxy - 演示保护代理（ACL 访问控制）
 *
 * 演示目标：
 *   展示保护代理如何基于 UID 进行权限控制，
 *   拒绝非特权用户的写操作，同时允许读操作。
 *
 * 真实 PCIe 安全场景：
 *   - root/内核驱动：可以读写所有寄存器
 *   - 普通用户进程：只能读寄存器，不能写关键控制寄存器
 *   - 调试工具：可能有受限的写权限（如只能写某些调试寄存器）
 *
 * 典型的安全事件：
 *   如果某个用户态程序试图写 PCIe 网卡的 MAC 地址寄存器，
 *   保护代理会拦截这个请求并返回错误（而非让非法写操作发送到硬件）。
 */
void demo_protected_proxy(void)
{
    printf("\n--- 保护代理：访问控制 ---\n");

    PCIeBAR* bar = new_PCIeBAR(0x50000000ULL);

    /* 定义两个 ACL：root（uid=0）和普通用户（uid=1000） */
    ACL root_acl = {.uid = 0};
    ACL user_acl = {.uid = 1000};

    printf("创建受保护的 PCIe BAR\n");

    /* 以 root 身份写入：应该成功
     * check_write_permission 返回 1，代理将请求转发给真实 BAR
     */
    printf("\n以 root (uid=0) 写入:\n");
    ProtectedBARProxy* root_proxy = new_ProtectedBARProxy(bar, &root_acl);
    root_proxy->base.write32(&root_proxy->base, 0x20, 0xDEADBEEF);

    /* 以普通用户身份写入：应该被拒绝
     * check_write_permission 返回 0，代理拒绝请求，打印 DENIED 消息
     */
    printf("\n以普通用户 (uid=1000) 写入:\n");
    ProtectedBARProxy* user_proxy = new_ProtectedBARProxy(bar, &user_acl);
    user_proxy->base.write32(&user_proxy->base, 0x20, 0xCAFEBABE);

    /* 读取操作：应该对所有用户开放
     * 无论 root 还是普通用户，都能成功读取寄存器
     */
    printf("\n读取操作（对所有用户开放）:\n");
    user_proxy->base.read32(&user_proxy->base, 0x20);

    /* 关闭代理 */
    root_proxy->base.close(&root_proxy->base);
    user_proxy->base.close(&user_proxy->base);
}

/*
 * ============================================================
 * 主函数：代理模式演示入口
 * ============================================================
 *
 * 本演示展示了两种最常用的代理模式：
 *   1. 虚代理（Lazy Proxy）：控制"何时"创建/初始化真实对象
 *   2. 保护代理（Protection Proxy）：控制"谁能"访问真实对象
 *
 * 代理模式的通用价值：
 *   - 客户端代码无需知道自己在操作代理还是真实对象（透明性）
 *   - 在不修改真实对象的情况下添加额外的逻辑（访问控制、延迟加载等）
 *   - 真实对象可以在需要时才加载，降低内存占用和初始化时间
 *
 * 真实嵌入式系统中的代理模式：
 *   - Linux VFS（Virtual File System）：文件的 read/write 通过
 *     struct file_operations 指针分发，实际可能是磁盘文件、
 *     网络文件系统、内存文件系统——这就是代理模式的思想
 *   - U-Boot 驱动模型：dm_* API 通过设备代理访问真实硬件
 *   - DMA 子系统：通过 DMA Channel Proxy 控制对 DMA Engine 的访问
 */
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
