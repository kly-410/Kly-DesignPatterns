/*
 * 05_Facade - 外观模式
 *
 * 外观模式核心：为一组复杂的子系统提供统一的高层接口，使客户端代码无需关心
 * 各个子系统之间的依赖关系和初始化顺序。
 *
 * 硬件背景（嵌入式场景）：
 *   一个完整的嵌入式系统启动时，各个硬件模块之间存在严格的依赖链：
 *   时钟树（Clock Tree）必须在电源（PMIC）稳定之后才能配置；
 *   DDR 内存控制器必须依赖时钟和电源；
 *   外设（显示、网络）又依赖 DDR 提供的内存空间来进行 DMA 缓冲。
 *   如果应用程序直接调用这 10+ 个初始化函数，不仅容易出错，
 *   而且在硬件 BSP 变更时需要修改大量业务代码。
 *   外观模式将这套启动序列封装为一个 device_boot() 调用，客户端只关心"开机"和"关机"。
 *
 * 本代码演示：
 *   1. 嵌入式固件初始化序列（时钟 -> 电源 -> DDR -> 外设）
 *   2. 子系统间有严格依赖关系（时钟是所有外设的根时钟源，PMIC 为所有rail供电）
 *   3. 外观封装复杂性，提供简单的启动/关机接口
 *
 * 真实硬件对应关系：
 *   - 时钟控制器：SoC 内部的 CCM（Clock Control Module）或 PLL 配置
 *   - PMIC：I2C/SPI 总线上的电源管理芯片（如 Rockchip RK808、NXP PF151）
 *   - DDR：DDR3/DDR4 物理层 + 内存控制器（通常是 Synopsys 或 ARM 提供的 IP）
 *   - 显示：MIPI DSI 接口的 LCD 驱动（嵌入式设备常见）
 *   - 网络：千兆以太网 PHY（通过 RGMII 接口）和 WiFi 模块（通过 SDIO）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ============================================================
 * 子系统A：时钟控制器（Clock Controller）
 * ============================================================
 *
 * 嵌入式系统中，时钟是所有硬件的"心跳"。
 * 在真实硬件中，SoC 内部有一个或多个 PLL（Phase Locked Loop），
 * 通过分频器和多路选择器将原始晶振频率倍频/分频到不同的时钟域：
 *   - ARM CPU 核时钟（如 1GHz）：驱动处理器核
 *   - AXI 总线时钟（如 400MHz）：驱动 SoC 内部高速总线
 *   - APB 总线时钟（如 100MHz）：驱动 UART、I2C、SPI 等低速外设
 *
 * 时钟初始化的依赖关系：
 *   时钟必须在 PMIC 之后初始化，因为 PMIC 提供的 Core Voltage (1.1V)
 *   是 PLL 能够正常工作的前提条件——电压不稳时 PLL 可能失锁。
 *   所有外设在使能之前，都必须先使能其所在时钟域的时钟。
 */
static int clock_initialized = 0;

/*
 * clock_init - 初始化时钟树
 *
 * 为什么这样做：
 *   SoC 上电后，内部时钟默认通常是一个低频的参考时钟（如 24MHz 晶振直接输出）。
 *   要让 CPU 运行在额定频率（如 1GHz），必须配置 PLL 的倍频参数（反馈分频、输出分频等），
 *   这一步也叫"PLL 配置"。如果不配置，系统只能运行在很低的频率，无法发挥性能。
 *
 * 真实硬件操作（以 ARM SoC 为例）：
 *   1. 等待外部晶振（XO）稳定（通常需要几毫秒）
 *   2. 配置 PLL1（ARM PLL）的分频比，使输出为 1GHz
 *   3. 配置 PLL2（DDR PLL）为 1600MT/s（DDR 实际频率）
 *   4. 配置各时钟域的分频器（divider）和多路选择器（mux）
 *   5. 切换各外设的时钟源到对应 PLL
 */
void clock_init(void)
{
    printf("  [CLK] Initializing PLL...\n");
    printf("  [CLK]   ARM:  1GHz\n");
    printf("  [CLK]   AXI:  400MHz\n");
    printf("  [CLK]   APB:  100MHz\n");
    clock_initialized = 1;
}

/*
 * clock_enable - 使能指定外设的时钟
 *
 * 为什么这样做：
 *   SoC 内部的不同外设挂载在不同的时钟域上。在嵌入式 Linux 中，
 *   每个外设驱动在访问外设寄存器之前，都必须调用 clk_enable()。
 *   这是因为默认情况下大多数外设的时钟是关闭的（降低功耗），
 *   只有在真正使用时才打开。
 *
 * 参数 peripheral_id：
 *   代表不同外设的时钟门控编号。在真实内核中对应 struct clk 指针，
 *   这里用整数模拟。例如：在 Rockchip RK3399 中，peripheral_id 10 可能是 DisplayPort，
 *   peripheral_id 20 可能是 GMAC（千兆以太网）。
 *
 * 安全检查：
 *   如果时钟未初始化（clock_initialized == 0），说明系统尚未完成时钟树配置，
 *   此时使能外设时钟是无效的，直接返回错误。
 */
void clock_enable(int peripheral_id)
{
    if (!clock_initialized) {
        printf("  [CLK] ERROR: clock not initialized!\n");
        return;
    }
    printf("  [CLK] Enable peripheral #%d clock\n", peripheral_id);
}

/*
 * clock_disable - 关闭指定外设的时钟
 *
 * 为什么这样做：
 *   功耗优化是嵌入式系统的核心目标之一。在 SoC 中，每个外设都有独立的
 *   时钟门控（Clock Gate），关闭不需要的外设时钟可以显著降低动态功耗。
 *   在关机序列中，必须先关闭所有外设的时钟，再关闭 PLL（因为 PLL 功耗较大），
 *   但这里的设计是最后才将 clock_initialized 置零，PLL 的关闭由硬件在断电时自动完成。
 */
void clock_disable(int peripheral_id)
{
    printf("  [CLK] Disable peripheral #%d clock\n", peripheral_id);
}

/*
 * ============================================================
 * 子系统B：电源管理（PMIC - Power Management Integrated Circuit）
 * ============================================================
 *
 * PMIC 是嵌入式系统中专门负责电源管理的芯片，通常通过 I2C 或 SPI 总线
 * 与 SoC 连接。它管理多个"电源轨"（Power Rail），每个 rail 提供特定电压。
 *
 * 典型 rail 分布：
 *   - Core Rail（如 1.1V）：为 CPU 核和逻辑电路供电，电压最严格（负载瞬态响应要求高）
 *   - IO Rail（如 3.3V）：为 GPIO、SD卡、USB 等 IO 接口供电
 *   - DDR Rail（如 1.35V 或 1.5V）：为 DDR 内存供电，通常有专门的 rail
 *
 * 依赖关系：PMIC 必须在时钟之前（或同时）初始化，因为 PLL 的工作依赖于稳定的电压。
 * 真实 BSP 中，时钟初始化之前通常会先通过 I2C/SPI 配置 PMIC 的默认电压。
 */
static int pmic_initialized = 0;

/*
 * pmic_init - 初始化 PMIC，配置各路电源电压
 *
 * 为什么这样做：
 *   嵌入式系统通常不是直接给整个芯片供电，而是由 PMIC 分配多路电压。
 *   系统启动时，PMIC 需要按顺序上下电（Power Sequence），这个顺序由
 *   硬件设计决定，通常是：Core Voltage 先于 IO Voltage，DDR Voltage 在特定时机。
 *   本函数模拟的是 PMIC 初始化后各路默认电压的设置。
 *
 * 真实硬件：
 *   PMIC 内部有多个 BUCK（降压）和 LDO（低压差线性稳压器）通道，
 *   每路输出的默认电压通过 PMIC 的固件（Device Tree 或 NVM 配置）决定。
 */
void pmic_init(void)
{
    printf("  [PMIC] Initializing power management IC...\n");
    printf("  [PMIC]   Core voltage: 1.1V\n");
    printf("  [PMIC]   IO voltage:   3.3V\n");
    pmic_initialized = 1;
}

/*
 * pmic_set_voltage - 动态调整指定电源轨的电压
 *
 * 为什么这样做：
 *   某些场景下需要动态调压（Dynamic Voltage Scaling，DVS）以优化功耗。
 *   例如：CPU 空闲时降低电压和频率（Idle 时进入低功耗模式）；
 *   或者 DDR 训练（Training）时需要特定的电压条件。
 *
 * 参数：
 *   rail_id：电源轨编号，不同 PMIC 有不同的 rail 编号定义。
 *            例如：rail_id 2 可能对应 DDR VDDQ 轨（3.3V 即 3300mV）。
 *   voltage_mv：目标电压，单位毫伏（mV）。
 *
 * 安全检查：
 *   PMIC 未初始化时（通常是 I2C/SPI 通信未建立），调用 set_voltage 会失败。
 */
void pmic_set_voltage(int rail_id, int voltage_mv)
{
    if (!pmic_initialized) {
        printf("  [PMIC] ERROR: PMIC not initialized!\n");
        return;
    }
    printf("  [PMIC]   Rail #%d: %dmV\n", rail_id, voltage_mv);
}

/*
 * pmic_shutdown - 关闭所有电源轨
 *
 * 为什么这样做：
 *   系统关机时，必须按照特定顺序关闭各路电源。错误的断电顺序可能导致硬件损坏。
 *   例如：必须先关闭外设供电，再关闭 DDR（因为 DDR 需要在特定时序内完成自刷新）。
 *   本函数模拟的是 PMIC 发送 Shutdown 命令，所有 BUCK/LDO 输出关闭。
 *
 * 真实硬件安全机制：
 *   大多数 PMIC 支持按序断电（Power Down Sequence），通过配置 PMIC 的
 *   寄存器可以设置每个 rail 的延迟时间（delay），确保顺序正确。
 */
void pmic_shutdown(void)
{
    printf("  [PMIC] Shutdown all power rails\n");
    pmic_initialized = 0;
}

/*
 * ============================================================
 * 子系统C：DDR 内存控制器
 * ============================================================
 *
 * DDR（Double Data Rate）内存是嵌入式系统中最大的 RAM 区域。
 * DDR 控制器的初始化是嵌入式系统 bring-up 过程中最复杂的一步之一，
 * 因为它涉及：
 *   1. 物理层训练（Physical Layer Training）：自动校准 DQ/DQS 采样窗口
 *   2. 带宽和时序配置：根据 DDR 芯片规格（频率、时序参数 CL-tRCD-tRP）配置寄存器
 *   3. 频率切换（如果有 PCDDR 或 LPDDR4 的动态频率切换）
 *
 * 依赖关系：DDR 初始化必须在时钟（PLL 提供 DDR PLL 时钟）和电源（DDR VDDQ）之后。
 * 应用程序只有在 DDR 初始化完成后才能使用 malloc() 分配内存（否则分配的地址无效）。
 */
static int ddr_initialized = 0;
static size_t ddr_capacity = 0;

/*
 * ddr_init - 初始化 DDR 内存控制器
 *
 * 为什么这样做：
 *   DDR 控制器是 SoC 内部的一个 IP 核（如 Synopsys DWC_ddrctl 或 ARM DMC）。
 *   初始化过程包括：
 *     a) DDR PHY 训练：通过对 DDR 颗粒的 DQ 引脚发送已知 Pattern，
 *        自动校准接收端的延迟（Read DQS Gating Training、Write Leveling 等）
 *     b) 配置控制器寄存器：设置时序参数（CL=9, tRCD=9, tRP=9 等）和刷新策略
 *     c) 验证：写入已知数据再读出，确保训练结果正确
 *
 *   物理层训练是 DDR 初始化中最耗时的步骤，在某些 SoC 上可能需要 100ms+。
 *   本函数的 print 输出模拟了这一过程。
 */
void ddr_init(void)
{
    printf("  [DDR] Initializing DDR3 controller...\n");
    printf("  [DDR]   Training memory lanes...\n");
    printf("  [DDR]   Capacity: 512MB @ 1600MT/s\n");
    ddr_capacity = 512 * 1024 * 1024;
    ddr_initialized = 1;
}

/*
 * ddr_alloc - 从 DDR 区域分配内存
 *
 * 为什么这样做：
 *   在嵌入式裸机环境（BSP 驱动开发）中，堆（heap）必须由 DDR 提供。
 *   如果 DDR 未初始化，malloc() 底层没有可用的物理内存映射，
 *   返回的指针指向无效地址，访问会导致 HardFault（硬 fault）。
 *
 * 参数 size：要分配的字节数。
 * 返回值：分配成功返回指针，失败返回 NULL。
 *
 * 真实硬件上下文：
 *   DDR 的物理地址空间由 SoC 的内存映射决定（如 0x40000000 开始），
 *   ioremap 将物理地址映射到虚拟地址空间的 Kernel Space 或 User Space。
 *   这里用标准 malloc 模拟（假设 DDR 已通过 ioremap 映射到用户空间）。
 */
void* ddr_alloc(size_t size)
{
    if (!ddr_initialized) {
        printf("  [DDR] ERROR: DDR not initialized!\n");
        return NULL;
    }
    return malloc(size);
}

/*
 * ddr_free - 释放 DDR 内存
 *
 * 为什么这样做：
 *   与 ddr_alloc 配套使用。真实硬件中，如果分配的内存是用于 DMA 缓冲，
 *   释放前需要先确保 DMA 传输完成（避免 use-after-free）。
 */
void ddr_free(void* ptr)
{
    free(ptr);
}

/*
 * ddr_shutdown - 关闭 DDR 控制器
 *
 * 为什么这样做：
 *   DDR 有关机序列（Power Down Sequence）：
 *     1. 进入自刷新（Self-Refresh）模式（数据不丢失，但停止访问）
 *     2. 等待所有 pending 事务完成（All Banks Precharge）
 *     3. 关闭 DDR PLL
 *   关机时必须严格按序执行，否则可能导致 DDR 颗粒永久损坏（概率低但存在）。
 */
void ddr_shutdown(void)
{
    printf("  [DDR] Release DDR controller\n");
    ddr_initialized = 0;
}

/*
 * ============================================================
 * 子系统D：显示驱动（Display Driver）
 * ============================================================
 *
 * 嵌入式显示系统通常包括：
 *   - SoC 内部的 Display Controller（如 MIPI DSI Host Controller）
 *   - 外部显示面板（LCD、OLED，通过 MIPI DSI 或 RGB 接口连接）
 *   - 背光驱动（Backlight，通常由 PWM 控制 LED 电流）
 *
 * 依赖关系：
 *   显示控制器需要时钟（通常来自 Display PLL）、电源（PMIC 的背光轨），
 *   以及 DDR（因为显示帧缓冲 FrameBuffer 存储在 DDR 中，每次刷屏 DMA 从 DDR 读取）
 *
 * 真实启动流程：
 *   1. 配置 Display PLL（通常在时钟子系统内）
 *   2. 分配 FrameBuffer（DDR 内存，约 1920×1080×4×2 ≈ 16MB，双缓冲）
 *   3. 初始化 DSI Host Controller（配置 Video Mode、时序参数）
 *   4. 通过 DSI 发送初始化命令序列（DCS Commands）到面板
 *   5. 配置 PWM 背光亮度
 *   6. 使能显示控制器，开始扫描输出
 */
static int display_initialized = 0;

/*
 * display_init - 初始化显示子系统
 *
 * 为什么这样做：
 *   初始化过程中，面板需要接收初始化命令序列（通常是一串 I2C/DSI Mipi 命令），
 *   这些命令配置面板的像素格式、分辨率、VC（Virtual Channel）等。
 *   命令序列由面板厂商提供（通常是一串 unsigned char 数组）。
 */
void display_init(void)
{
    printf("  [DISP] Initializing MIPI DSI display...\n");
    printf("  [DISP]   Resolution: 1920x1080 @ 60Hz\n");
    display_initialized = 1;
}

/*
 * display_set_brightness - 设置背光亮度
 *
 * 为什么这样做：
 *   嵌入式显示的背光通常由 PWM（Pulse Width Modulation）控制。
 *   PWM 占空比决定 LED 的平均电流，从而控制亮度。
 *   level 范围 0~100 表示亮度百分比。
 *
 * 真实硬件：
 *   PWM 模块由 SoC 定时器提供，频率通常在 200Hz~10kHz 之间。
 *   频率太低会看到闪烁，频率太高则开关损耗增加。
 */
void display_set_brightness(int level)
{
    if (!display_initialized) {
        printf("  [DISP] ERROR: Display not initialized!\n");
        return;
    }
    printf("  [DISP]   Backlight: %d%%\n", level);
}

/*
 * display_shutdown - 关闭显示
 *
 * 为什么这样做：
 *   关机时需要先关闭显示控制器（停止扫描），再关闭面板电源，
 *   最后关闭背光。顺序错误可能导致 LCD 面板上出现闪烁或残影。
 */
void display_shutdown(void)
{
    printf("  [DISP] Power off display\n");
    display_initialized = 0;
}

/*
 * ============================================================
 * 子系统E：网络子系统（Network Subsystem）
 * ============================================================
 *
 * 嵌入式网关设备的网络通常包含两个部分：
 *   - 有线网络：千兆以太网 PHY，通过 RGMII（Reduced Gigabit Media Independent Interface）
 *              与 SoC 的 MAC（Ethernet Controller）连接
 *   - 无线网络：WiFi 模块，通过 SDIO 或 USB 接口连接
 *
 * 以太网 PHY 的初始化依赖：
 *   - 时钟：RGMII 接口需要参考时钟（通常 125MHz，由 SoC 提供）
 *   - 电源：某些 PHY 需要多路电压（如 1.1V 核心 + 3.3V IO）
 *   - DDR：网络驱动需要分配 RX/TX Buffer（存储在 DDR 中）
 *
 * RGMII vs 其他接口：
 *   RGMII 是千兆以太网最常用的 MAC-PHY 接口，使用 4-bit 数据总线，
 *   上升沿和下降沿同时采样（DDR 方式），因此 125MHz 时钟即可实现 1Gbps 带宽。
 */
static int net_initialized = 0;

/*
 * net_init - 初始化网络子系统
 *
 * 为什么这样做：
 *   有线网部分需要配置 MAC 控制器（设置 MII/RGMII 接口、协商速率/双工）。
 *   无线网部分需要加载 WiFi 固件（通常是 .bin 文件，通过 SDIO 协议下载到 WiFi 芯片）。
 *   WiFi 固件加载是 WiFi 驱动初始化中最耗时的步骤，可能需要 200ms+。
 *
 * 真实 Linux 网络驱动初始化流程：
 *   1. 探测 PHY（通过 MDIO 总线读取 PHY ID）
 *   2. 配置 MAC 寄存器（双工模式、速率）
 *   3. 等待 PHY 自动协商（Auto-Negotiation）完成
 *   4. 创建网络设备（netdev），注册到 Linux 内核
 */
void net_init(void)
{
    printf("  [NET] Initializing network subsystem...\n");
    printf("  [NET]   PHY: Gigabit Ethernet (RGMII)\n");
    printf("  [NET]   WiFi: 802.11ac (SDIO)\n");
    net_initialized = 1;
}

/*
 * net_send - 通过网络发送数据
 *
 * 为什么这样做：
 *   在嵌入式网关中，数据通常通过 DMA（Direct Memory Access）从 DDR 传输到
 *   网络控制器的 TX FIFO，无需 CPU 干预复制数据。
 *   因此在调用 net_send 之前，DDR 必须已初始化——TX Buffer 必须在 DDR 中。
 *
 * 参数：
 *   data：指向要发送数据的指针（必须在 DDR 区域）
 *   len：数据字节数
 */
void net_send(const char* data, size_t len)
{
    if (!net_initialized) {
        printf("  [NET] ERROR: Network not initialized!\n");
        return;
    }
    printf("  [NET]   TX: %zu bytes\n", len);
}

/*
 * net_shutdown - 关闭网络子系统
 *
 * 为什么这样做：
 *   关闭网络前，需要停止 TX/RX 队列（确保没有 pending 的 DMA 传输），
 *   然后关闭 MAC 和 PHY 的电源（或进入低功耗模式）。
 */
void net_shutdown(void)
{
    printf("  [NET] Stop network subsystem\n");
    net_initialized = 0;
}

/*
 * ============================================================
 * Facade：设备初始化管理器（Device Manager）
 * ============================================================
 *
 * DeviceManager 是外观模式的核心。
 *
 * 为什么需要它：
 *   想象一个应用程序在启动时需要做这些事：
 *     1. 调用 clock_init()
 *     2. 调用 pmic_init()
 *     3. 调用 ddr_init()
 *     4. 调用 display_init()，但必须先 clock_enable(10)
 *     5. 调用 net_init()，但必须先 clock_enable(20) 和 pmic_set_voltage(2, 3300)
 *   ——这是 10+ 个函数调用，应用程序员必须记住初始化顺序和参数。
 *
 *   有了 DeviceManager，应用程序只需要：
 *     DeviceManager* dev = new_DeviceManager();
 *     dev->boot(dev);  // 一行代码完成所有初始化
 *
 * 外观模式的本质：
 *   它并不替代子系统，而是封装了子系统的使用顺序和复杂度，
 *   使客户端代码与子系统解耦。当某个子系统的初始化流程变更时
 *   （如新增了一个外设，或初始化顺序调整），只需要修改 Facade，
 *   所有调用 Facade 的客户端代码无需任何修改。
 *
 * 结构设计：
 *   DeviceManager 是一个"结构体函数指针"模式（类似 C 语言的多态）的封装，
 *   boot 和 shutdown 是两个核心方法，符合面向接口的设计原则。
 */
typedef struct _DeviceManager DeviceManager;

/*
 * DeviceManager 结构体
 *
 * 为什么不把所有成员公开？
 *   封装原则：客户端只需要调用 boot()/shutdown()，不需要知道内部的 mask。
 *   将 subsystem_mask 设为私有（无公开 setter），防止外部代码直接修改状态。
 *
 * 成员说明：
 *   - initialized：设备就绪标志，0=未就绪，1=已就绪
 *   - subsystem_mask：位掩码，记录各子系统初始化状态
 *   - boot：函数指针，指向实际启动逻辑（device_boot）
 *   - shutdown：函数指针，指向实际关机逻辑（device_shutdown）
 *
 * subsystem_mask 的位定义：
 *   每一位代表一个子系统，用掩码方便地用位运算判断状态。
 *   例如：(mask & MASK_DDR) != 0 表示 DDR 已初始化。
 *   这种设计在固件中很常见，Linux 内核的 CONFIG_* 宏也大量使用位掩码。
 */
struct _DeviceManager {
    int initialized;
    int subsystem_mask;
    int (*boot)(DeviceManager*);
    int (*shutdown)(DeviceManager*);
};

/*
 * 子系统位掩码定义
 *
 * 为什么用位掩码而不是布尔数组？
 *   1. 位掩码是单整数，比较操作比数组遍历更快（硬件层面支持位指令）
 *   2. 可以一次性通过一个整数的不同位表示多个状态，减少内存占用
 *   3. 内核中广泛使用（如 ioctl 的请求码、设备树的中断掩码等）
 *
 * 掩码与子系统的对应关系：
 *   MASK_CLK (0x01)：时钟子系统
 *   MASK_PMIC (0x02)：电源管理子系统
 *   MASK_DDR (0x04)：DDR 内存子系统
 *   MASK_DISP (0x08)：显示子系统
 *   MASK_NET (0x10)：网络子系统
 */
#define MASK_CLK   0x01
#define MASK_PMIC  0x02
#define MASK_DDR   0x04
#define MASK_DISP  0x08
#define MASK_NET   0x10

/*
 * device_boot - 设备启动序列（核心启动函数）
 *
 * 为什么这样做（启动顺序的依赖链）：
 *
 *   [第1步] clock_init()
 *     所有硬件的根时钟源必须第一个初始化。PLL 输出稳定后，才能为
 *     各子系统提供参考时钟。没有时钟，任何外设都无法工作。
 *
 *   [第2步] pmic_init()
 *     PMIC 提供电源，但这里把它放在时钟之后是因为：
 *     在某些 SoC 中，PMIC 的 I2C 控制器的时钟来自 APB 总线，
 *     而 APB 总线时钟需要先通过 clock_init 配置。
 *     即便 I2C 不依赖 PLL（使用独立的 24MHz RCOSC），
 *     PMIC 输出稳定也需要一个上电延迟（通常 10~50ms）。
 *
 *   [第3步] ddr_init()
 *     DDR 必须在显示和网络之前初始化，因为：
 *     - 显示的帧缓冲（FrameBuffer）需要 DDR 存储
 *     - 网络的 TX/RX Buffer 需要 DDR 存储
 *     没有 DDR，显示和网络都无法分配内存。
 *
 *   [第4步] display_init() + clock_enable(10)
 *     显示需要：
 *     - DDR（帧缓冲）   ← 已初始化
 *     - 时钟（Display PLL）← 通过 clock_enable(10) 使能
 *     - PMIC 电源轨    ← 已初始化
 *     在 BSP 中，显示时钟使能编号（10）是 BSP 工程师分配的，
 *     具体编号由 SoC 的 Clock ID 决定。
 *
 *   [第5步] net_init() + clock_enable(20) + pmic_set_voltage(2, 3300)
 *     网络（千兆以太网）需要：
 *     - DDR（TX/RX Buffer）  ← 已初始化
 *     - 时钟（RGMII RefClk） ← 通过 clock_enable(20) 使能
 *     - PMIC 电压轨 rail 2 调压到 3.3V（RGMII IO 通常需要 3.3V）
 *     如果 IO 电压不对（如 1.8V），RGMII 信号可能损坏 PHY 或通信不稳定。
 *
 * 这个启动顺序是嵌入式 BSP 工程师多年经验的沉淀，遵循芯片手册
 *（Chip TRM - Technical Reference Manual）中的 Power Sequence 规范。
 */
static int device_boot(DeviceManager* dm)
{
    printf("\n========================================\n");
    printf("          设备启动序列\n");
    printf("========================================\n");

    /*
     * 启动顺序：时钟优先，然后电源，然后 DDR，然后外设
     * 顺序错误可能导致硬件工作异常甚至损坏
     */

    /* 第1步：初始化时钟树（所有外设的根时钟源） */
    clock_init();
    dm->subsystem_mask |= MASK_CLK;

    /* 第2步：初始化 PMIC（配置各路电源电压） */
    pmic_init();
    dm->subsystem_mask |= MASK_PMIC;

    /* 第3步：初始化 DDR 控制器（必须在外设之前，因为外设需要 DMA Buffer） */
    ddr_init();
    dm->subsystem_mask |= MASK_DDR;

    /* 第4步：使能显示时钟，然后初始化显示子系统
     * 显示的帧缓冲需要 DDR，所以 DDR 必须在显示之前初始化
     */
    clock_enable(10);   /* 10 = Display PLL 时钟门控 ID */
    display_init();
    dm->subsystem_mask |= MASK_DISP;

    /* 第5步：使能网络时钟，调整 IO 电压，然后初始化网络
     * 某些千兆 PHY 需要 IO 电压为 3.3V，而不是默认的 1.8V
     * 网络 TX/RX Buffer 存储在 DDR 中，依赖 DDR 已就绪
     */
    clock_enable(20);   /* 20 = Ethernet (RGMII) RefClk 时钟门控 ID */
    pmic_set_voltage(2, 3300);  /* rail 2 = 网口 PHY IO 电压轨，调至 3.3V */
    net_init();
    dm->subsystem_mask |= MASK_NET;

    dm->initialized = 1;

    printf("\n========================================\n");
    printf("          启动完成！\n");
    printf("========================================\n\n");
    return 0;
}

/*
 * device_shutdown - 设备关机序列（核心关机函数）
 *
 * 为什么这样做（关机顺序必须严格反向）：
 *
 *   关机顺序与启动顺序严格相反——最后启动的子系统最先关闭。
 *   这是嵌入式系统的通用原则，因为后面的子系统可能还在依赖前面的子系统。
 *
 *   例如：display_shutdown() 时需要访问 DDR 刷新帧缓冲数据，
 *         所以 DDR 必须在 display 之后才能关闭。
 *
 *   [反向依赖链]
 *     网络正在使用 DDR 存放 TX Buffer → 关闭网络后才能关闭 DDR
 *     显示正在使用 DDR 存放 FrameBuffer → 关闭显示后才能关闭 DDR
 *     DDR 需要时钟才能工作   → 关闭 DDR 后才能关闭时钟树
 *     PMIC 关闭后时钟树失锁   → PMIC 是最后关闭的
 *
 *   如果顺序错误：
 *     - 先关闭 DDR 再关闭网络：网络 DMA 访问已释放的 DDR → HardFault
 *     - 先关闭 PMIC 再关闭 DDR：DDR PLL 失锁，DDR 数据丢失
 *
 * 安全检查：
 *   如果 dm->initialized == 0（设备未启动），直接返回，避免重复关机。
 */
static int device_shutdown(DeviceManager* dm)
{
    if (!dm->initialized) return 0;

    printf("\n========================================\n");
    printf("          设备关机序列\n");
    printf("========================================\n");

    /* 第1步：关闭网络（依赖 DDR 的 TX/RX Buffer） */
    net_shutdown();
    clock_disable(20);  /* 关闭网络时钟（RGMII RefClk） */

    /* 第2步：关闭显示（依赖 DDR 的 FrameBuffer） */
    display_shutdown();
    clock_disable(10);  /* 关闭显示时钟（Display PLL） */

    /* 第3步：关闭 DDR（此时已无外设依赖它） */
    ddr_shutdown();

    /* 第4步：关闭 PMIC（电压轨关闭，时钟树自然失锁）
     * 时钟树依赖 PLL，PMIC 断电后 PLL 停止振荡
     */
    pmic_shutdown();
    clock_initialized = 0;  /* 重置时钟状态，与实际硬件行为同步 */

    dm->initialized = 0;
    dm->subsystem_mask = 0;  /* 重置所有子系统状态位 */

    printf("\n========================================\n");
    printf("          关机完成\n");
    printf("========================================\n");
    return 0;
}

/*
 * new_DeviceManager - 构造函数
 *
 * 为什么这样做：
 *   使用 calloc 分配并清零结构体内存，确保所有字段有明确定义的初始值。
 *   初始化列表包含：
 *     - 初始就绪状态为 0（未初始化）
 *     - 子系统掩码为 0（无任何子系统启动）
 *     - 函数指针绑定到各自的实现函数
 *
 * 为什么不直接在结构体声明处初始化？
 *   结构体声明只是类型定义，不分配内存。
 *   结构体内部的 static 变量或全局变量可以在定义时初始化，
 *   但 new_DeviceManager() 是动态分配（用于运行时多态），必须在运行时赋值。
 */
DeviceManager* new_DeviceManager(void)
{
    DeviceManager* dm = calloc(1, sizeof(DeviceManager));
    dm->initialized = 0;
    dm->subsystem_mask = 0;
    dm->boot = device_boot;
    dm->shutdown = device_shutdown;
    return dm;
}

/*
 * device_is_ready - 查询设备就绪状态
 *
 * 为什么这样做：
 *   应用程序在调用 boot() 之后，可以通过此函数确认设备是否真正就绪。
 *   在真实嵌入式系统中，boot() 可能分多个阶段（Stage 1/Stage 2），
 *   此函数对应最后一个阶段的完成标志。
 */
int device_is_ready(DeviceManager* dm)
{
    return dm->initialized;
}

/*
 * ============================================================
 * 主函数：演示外观模式的使用
 * ============================================================
 *
 * 演示目标：
 *   对比两种使用方式的代码量：
 *   - 无外观：app 直接调用 10+ 个初始化函数，还需记住顺序
 *   - 有外观：app 只需调用 device_boot() / device_shutdown()
 *
 * 真实项目结构（以 Linux 内核为例）：
 *   board-init.c（具体板级的初始化代码）
 *     ├── arch/arm/mach-xxx/board-xxx.c
 *     │     └── 定义 platform_device、clock、pinctrl 等资源
 *   subsystems/（各子系统的驱动）
 *     ├── drivers/clk/xxx-clk.c
 *     ├── drivers/mfd/xxx-pmic.c
 *     ├── drivers/memory/xxx-ddr.c
 *     └── drivers/video/xxx-dsi.c
 *   Facade 的角色由 board file（板级初始化文件）承担，
 *   它负责按照正确的顺序调用各子系统的初始化函数。
 */
int main()
{
    printf("========== 外观模式演示 ==========\n\n");
    printf("场景：物联网网关设备启动\n");
    printf("10个复杂的初始化步骤 → 1个 device_boot() 调用\n\n");

    /* 创建 DeviceManager 实例（外观对象）
     * 这里 new_DeviceManager() 相当于工厂方法，
     * 封装了结构体分配 + 函数指针绑定的细节
     */
    DeviceManager* dev = new_DeviceManager();

    /* 一行代码启动所有子系统——这就是外观模式的价值 */
    dev->boot(dev);

    /* 查询就绪状态 */
    printf("设备就绪: %s\n", device_is_ready(dev) ? "YES" : "NO");

    printf("\n[应用程序] 设备已就绪，开始工作...\n\n");

    /* 应用程序代码：直接使用子系统，无需关心初始化顺序
     * 这是外观模式对客户端代码的核心价值——解耦
     */

    /* 网络发送：演示网络子系统工作正常
     * TX Buffer 的地址在 DDR 区域，DMA 控制器将负责传输
     */
    char test_data[] = "Hello IoT Gateway!";
    net_send(test_data, strlen(test_data));

    /* 背光控制：演示显示子系统工作正常
     * 实际是对 PWM 占空比的配置
     */
    display_set_brightness(80);

    /* DDR 内存分配：演示 DDR 内存可用
     * 在真实嵌入式系统中，这里可能分配 DMA 一致性缓存内存
     * （使用 dma_alloc_coherent() 而非 malloc）
     */
    void* buf = ddr_alloc(1024);
    if (buf) {
        printf("\n[应用程序] DDR allocation: OK (1024 bytes)\n");
        ddr_free(buf);
    }

    /* 一行代码关闭所有子系统——反向启动顺序自动保证 */
    dev->shutdown(dev);

    /* 释放 DeviceManager 本身占用的内存 */
    free(dev);

    printf("\n========== 对比：外观 vs 无外观 ==========\n");
    printf("无外观：app 需要调用 10 个初始化函数，还需记住顺序\n");
    printf("有外观：app 只需调用 device_boot() / device_shutdown()\n");

    return 0;
}
