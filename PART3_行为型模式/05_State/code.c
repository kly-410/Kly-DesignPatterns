/**
 * ===========================================================================
 * State Pattern — 状态机模式
 * ===========================================================================
 *
 * 设计模式：行为型模式 · State
 * 核心思想：对象行为随内部状态改变，状态切换触发行为变化
 *
 * 【示例1】Gumball Machine — 糖果机状态机
 *         状态：NoQuarter / HasQuarter / Sold / SoldOut
 *         嵌入式类比：简单状态机，如 DMA 控制器状态（IDLE→RUNNING→DONE）
 *
 * 【示例2】TCP Connection State Machine
 *         状态：CLOSED / LISTEN / SYN_SENT / ESTABLISHED / FIN_WAIT_1 ...
 *         嵌入式类比：通信协议状态机，UART/SPI 握手流程
 *
 * 【示例3】PCIe LTSSM (Link Training and Status State Machine)
 *         状态：DETECT / POLLING / CONFIG / RECOVERY / L0 / L1 / DISABLED
 *         嵌入式类比：PCIe 链路训练状态机，固件必须理解每个状态的含义和跳转条件
 *
 * ===========================================================================
 *
 * 状态模式 vs 简单 if-else 状态机：
 *   - if-else 状态机：所有状态处理混在一个函数中，状态多了变成"意大利面条"
 *   - 状态对象模式：每个状态一个对象，状态转换清晰，扩展新状态不影响其他状态
 *
 * 嵌入式视角：
 *   - 状态机无处不在：DMA、UART、SPI、I2C、PCIe、TCP、带 RTOS 的任务
 *   - 理解状态机是固件/驱动工程师的基本功
 *   - PCIe LTSSM 直接决定了链路能否建立，是芯片 bring-up 的核心调试点
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Gumball Machine（糖果机状态机）
 * ============================================================================
 *
 * 这是一个最简单的状态机示例：投币 → 转柄 → 出糖
 *
 * 硬件对应关系：
 *   S_NO_QUARTER — 机器空闲，等待投币（GPIO 输入等待状态）
 *   S_HAS_QUARTER — 已投币，等待转柄（内部标志位置位）
 *   S_SOLD — 正在出货（电机转动中，定时器运行）
 *   S_SOLD_OUT — 糖果售罄（库存计数器为0）
 *
 * 事件驱动模型：
 *   EV_INSERT — 投币（外部中断或用户输入触发）
 *   EV_EJECT — 退币（取消交易）
 *   EV_TURN — 转动手柄（确认购买）
 *   EV_DISPENSE — 出糖完成（定时器/完成中断通知）
 *
 * 为什么用查表法做状态转换？
 *   状态转换表（transitions）比 if-else 更清晰，新增状态只需改表不改代码
 *   嵌入式实时系统中，状态表可以预先验证，防止非法跳转
 */

/**
 * GumballState — 糖果机的四个状态枚举
 *
 * 为什么状态用枚举而不是 #define？
 *   枚举有类型检查，编译器会发现使用错误状态的 bug
 *   #define 只是文本替换，没有类型安全
 *
 * 硬件视角：
 *   每个状态对应外设/子系统的某种工作模式，固件中常用枚举定义状态机状态
 */
typedef enum { S_NO_QUARTER, S_HAS_QUARTER, S_SOLD, S_SOLD_OUT } GumballState;

/**
 * GumballEvent — 糖果机的事件枚举
 *
 * 为什么事件也要用枚举？
 *   保证事件类型的穷举性，新增事件编译器会提示所有 switch 未覆盖
 *   真实固件：外设中断事件、中断屏蔽字位域等都用枚举
 */
typedef enum { EV_INSERT, EV_EJECT, EV_TURN, EV_DISPENSE } GumballEvent;

/**
 * GumballMachine — 糖果机状态机主体
 *
 * 状态机的核心：
 *   state — 当前状态，决定下一次事件如何处理
 *   ball_count — 糖果库存，库存为0时触发 SOLD_OUT 状态
 *
 * 硬件视角：
 *   固件中的状态机通常包含：当前状态变量 + 状态相关的数据
 *   例如：DMA 状态机包含 channel_state 和 transfer_count
 */
typedef struct _GumballMachine {
    GumballState state;     /**< 当前状态，驱动整个状态机的核心变量 */
    int ball_count;        /**< 糖果库存数量，>0 才能正常发售 */
} GumballMachine;

/** 状态名称数组，用于日志打印 */
static const char* state_names[] = {"NoQuarter", "HasQuarter", "Sold", "SoldOut"};

/**
 * transitions — 状态转换表（二维数组实现）
 *
 * 为什么用静态二维数组而不是 switch？
 *   表驱动法的优点：状态和事件构成二维索引，直接查表得到下一状态
 *   代码可读性高，新增/修改转换规则只需改表不改代码
 *   嵌入式安全系统常用表驱动代替复杂条件分支，减少 bug
 *
 * 索引方式：transitions[当前状态][事件] = 下一状态
 * 注意：C89 不支持 designated initializer，所以用 C99 复合字面量风格
 */
static GumballState transitions[4][4] = {
    [S_NO_QUARTER][EV_INSERT] = S_HAS_QUARTER,    /**< 空闲时投币 → 等待转柄 */
    [S_HAS_QUARTER][EV_EJECT] = S_NO_QUARTER,    /**< 等待时退币 → 回到空闲 */
    [S_HAS_QUARTER][EV_TURN] = S_SOLD,            /**< 转动手柄 → 开始出货 */
    [S_SOLD][EV_DISPENSE] = S_NO_QUARTER,         /**< 出货完成 → 回到空闲（或售罄） */
};

/**
 * gumball_handle_event — 状态机事件处理函数
 *
 * 处理流程（嵌入式中断处理器的类比）：
 *   1. 根据当前状态和事件查表得到下一状态
 *   2. 执行当前状态的事件处理动作（出货逻辑）
 *   3. 如果状态有变化，输出日志
 *   4. 处理售罄特殊逻辑（库存为0时转到 SOLD_OUT）
 *
 * 为什么状态转换和动作执行分开？
 *   状态转换表只定义"状态拓扑"，不包含具体动作
 *   具体动作（减少库存、打印日志）在事件处理函数中实现
 *   这样状态表和动作逻辑解耦，维护更清晰
 *
 * @param m  糖果机对象
 * @param ev 触发的事件
 */
static void gumball_handle_event(GumballMachine* m, GumballEvent ev) {
    GumballState next = transitions[m->state][ev];   /**< Step 1: 查表得到下一状态 */
    printf("  [Event] %d (state=%s)\n", ev, state_names[m->state]);

    /**
     * Step 2: 执行当前状态的特定动作
     * 注意：这里只处理有副作用的动作（减少库存）
     * 大多数状态转换是纯状态切换，不需要额外动作
     */
    if (m->state == S_HAS_QUARTER && ev == EV_TURN) {
        printf("  [Machine] Dispensing gumball...\n");
        m->ball_count--;     /**< 出货时库存减1，这是不可逆的操作 */
    }

    /**
     * Step 3: 实际状态转换
     * 为什么检查 next != m->state？
     *   查表可能返回当前状态（某些事件在某些状态下无效果）
     *   只有状态真正变化才打印日志和更新状态变量
     */
    if (next != m->state && next < 4) {
        printf("  [Transition] %s -> %s\n", state_names[m->state], state_names[next]);
        m->state = next;
    }

    /**
     * Step 4: 售罄特殊处理
     * 为什么在状态转换后检查？
     *   S_SOLD 状态表示"正在出货"，出货完成后要看库存决定下一状态
     *   库存 > 0 → 回到 S_NO_QUARTER（等待下一位顾客）
     *   库存 = 0 → 转到 S_SOLD_OUT（机器停止服务）
     */
    if (m->state == S_SOLD) {
        if (m->ball_count > 0) m->state = S_NO_QUARTER;
        else m->state = S_SOLD_OUT;
    }
}

/* ============================================================================
 * 第二部分：TCP Connection State Machine
 * ============================================================================
 *
 * TCP 状态机是网络协议中最经典的状态机之一，RFC 793 定义了 11 种状态
 *
 * 硬件/嵌入式视角：
 *   TCP 状态机与 PCIe LTSSM、UART 通信协议、I2C 总线状态机等本质相同
 *   都是"请求-应答"驱动的状态跳转，理解 TCP 状态机就能理解大多数通信协议
 *
 * 真实硬件场景：
 *   PCIe 链路训练（LTSSM）与 TCP 握手/关闭流程高度相似
 *   DMA 传输与 TCP 状态机也有共通之处：设置 → 运行 → 完成/错误
 *
 * TCP 状态与嵌入式状态的对应：
 *   TCP_CLOSED        — 外设断电/未初始化
 *   TCP_LISTEN        — 等待连接请求（UART 等待 RX 中断）
 *   TCP_SYN_SENT      — 已发送请求，等待对方应答
 *   TCP_SYN_RECV      — 收到对方请求，已回复，等待最终确认
 *   TCP_ESTABLISHED   — 连接建立，数据传输中（SPI/I2C 正常通信状态）
 *   TCP_FIN_WAIT_*    — 正在关闭连接（发送完数据，等待对方确认）
 *   TCP_CLOSE_WAIT    — 收到对方关闭请求，等待本地应用处理
 *   TCP_LAST_ACK       — 本地已发送关闭请求，等待最后确认
 *   TCP_TIME_WAIT     — 等待足够时间确保对方收到关闭确认
 */

/**
 * TCPState — TCP 连接状态枚举
 *
 * 这 10 种状态覆盖了一个 TCP 连接从建立到关闭的完整生命周期
 * 固件调试网络问题时，理解当前处于哪个状态是关键
 */
typedef enum {
    TCP_CLOSED,           /**< 关闭状态，连接不存在 */
    TCP_LISTEN,           /**< 监听状态，被动打开，等待对方发起连接 */
    TCP_SYN_SENT,         /**< 已发送 SYN，请求建立连接 */
    TCP_SYN_RECV,         /**< 已收到 SYN 并回复，等待对方 ACK（三次握手第二步）*/
    TCP_ESTABLISHED,      /**< 连接建立完成，数据传输状态 */
    TCP_FIN_WAIT_1,       /**< 已发送 FIN，等待对方 ACK 和 FIN */
    TCP_FIN_WAIT_2,       /**< 收到对方 ACK，等待对方 FIN（四次挥手等待）*/
    TCP_CLOSE_WAIT,       /**< 收到对方 FIN，本地等待应用关闭连接 */
    TCP_LAST_ACK,         /**< 本地发送 FIN 后等待最后确认 */
    TCP_TIME_WAIT         /**< 关闭后等待 2MSL，确保对方收到所有数据 */
} TCPState;

/**
 * TCPEvent — TCP 事件枚举
 *
 * 事件来源：
 *   本地应用调用 close()/connect() — 主动触发
 *   对方发来 SYN/ACK/FIN — 网络事件（硬件中断）
 *   超时定时器到期 — 时间驱动
 *
 * 嵌入式视角：
 *   每个 TCP 事件对应一个硬件事件（数据包到达/定时器触发）
 *   网络驱动中断服务程序（ISR）负责产生这些事件
 */
typedef enum {
    TCP_OPEN,     /**< 主动打开连接（本端调用 connect()）*/
    TCP_SYN,      /**< 收到 SYN 包（对方请求连接）*/
    TCP_SYN_ACK,  /**< 收到 SYN+ACK 包（三次握手中间状态）*/
    TCP_ACK,      /**< 收到 ACK 包（确认数据）*/
    TCP_FIN,      /**< 收到/发送 FIN 包（关闭连接请求）*/
    TCP_CLOSE,    /**< 本地主动调用 close() */
    TCP_TIMEOUT   /**< 超时事件（重传定时器/RTT 定时器）*/
} TCPEvent;

/** TCP 状态名称表，用于调试日志 */
static const char* tcp_state_names[] = {
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECV", "ESTABLISHED",
    "FIN_WAIT_1", "FIN_WAIT_2", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

/**
 * TCPSocket — TCP 连接 socket 对象
 *
 * 为什么要把 socket 建模为对象？
 *   socket 的当前状态 + 状态迁移逻辑天然适合状态机模式
 *   一个 socket 对象 = 状态变量 + 状态迁移方法
 *   多个 socket 并存时，每个对象维护自己的状态，互不干扰
 *
 * 嵌入式视角：
 *   一个 UART 通道 = uart_port 结构（状态变量）+ uart_irq_handler（状态迁移）
 *   与 TCPSocket 的设计完全一致
 */
typedef struct _TCPSocket {
    TCPState state;                                /**< 当前 TCP 连接状态 */
    void (*set_state)(struct _TCPSocket*, TCPState);  /**< 状态迁移方法 */
} TCPSocket;

/**
 * TCP_set_state — 状态迁移方法
 *
 * 为什么要有专门的方法来设置状态？
 *   统一的状态迁移入口，所有状态变化都会打印日志
 *   调试 TCP 连接问题时，日志是唯一的可见性来源
 *   固件中类似的设计：所有外设状态变化都经过统一的状态机框架
 */
static void TCP_set_state(TCPSocket* sk, TCPState state) {
    printf("  [TCP] %s -> %s\n", tcp_state_names[sk->state], tcp_state_names[state]);
    sk->state = state;
}

/**
 * tcp_dispatch — TCP 事件分发处理
 *
 * 为什么用 if-else 而不用查表法？
 *   TCP 状态机有 10 个状态 × 7 个事件 = 70 种组合
 *   很多组合是非法组合（不会发生），if-else 只处理实际可能发生的转换
 *   表驱动法适合状态少、转换规则简单的情况；TCP 这种复杂协议适合 if-else
 *
 * 为什么每个分支都要调用 set_state 而不是直接赋值？
 *   保证所有状态变化都经过 set_state() 统一入口
 *   便于统一添加日志、通知、回调等附加逻辑
 *
 * @param sk  TCP socket 对象
 * @param ev  触发的事件
 */
static void tcp_dispatch(TCPSocket* sk, TCPEvent ev) {
    printf("  [TCP Event] %d (state=%s)\n", ev, tcp_state_names[sk->state]);
    TCPState next = sk->state;     /**< 初始假设状态不变 */

    /** 三次握手（主动打开）: CLOSED → SYN_SENT → ESTABLISHED */
    if (sk->state == TCP_CLOSED && ev == TCP_OPEN) next = TCP_SYN_SENT;
    else if (sk->state == TCP_CLOSED && ev == TCP_TIMEOUT) next = TCP_LISTEN;  /**< 被动打开 */
    else if (sk->state == TCP_SYN_SENT && ev == TCP_SYN_ACK) next = TCP_ESTABLISHED;
    /** 四次挥手: ESTABLISHED → CLOSE_WAIT → LAST_ACK → CLOSED */
    else if (sk->state == TCP_ESTABLISHED && ev == TCP_FIN) next = TCP_CLOSE_WAIT;
    else if (sk->state == TCP_CLOSE_WAIT && ev == TCP_CLOSE) next = TCP_LAST_ACK;
    else if (sk->state == TCP_LAST_ACK && ev == TCP_ACK) next = TCP_CLOSED;

    if (next != sk->state) TCP_set_state(sk, next);   /**< 状态有变化才迁移 */
}

/* ============================================================================
 * 第三部分：PCIe LTSSM (Link Training and Status State Machine)
 * ============================================================================
 *
 * PCIe LTSSM 是 PCIe 链路训练状态机，PCIe Spec Section 4.2 定义了 11 个主状态
 *
 * 硬件对应关系：
 *   LTSSM_DETECT — 检测阶段，检测对方是否存在（RX 检测电阻）
 *   LTSSM_POLLING — 轮询阶段，检测链路速率（Gen1/Gen2/Gen3/Gen4/Gen5）
 *   LTSSM_CONFIG — 配置阶段，协商 lane 宽度、极性、lane 编号
 *   LTSSM_RECOVERY — 恢复阶段，链路重新训练（如速率切换、热插拔）
 *   LTSSM_L0 — 正常工作状态，链路已就绪，可以传输 TLP/DLLP
 *   LTSSM_L1 — 低功耗状态，停止传输，功耗显著降低
 *   LTSSM_DISABLED — 链路被禁用（如外设拔除或软件禁用）
 *
 * 为什么固件工程师必须理解 LTSSM？
 *   芯片 bring-up 时，PCIe 链路训练是第一个要调试的外设协议
 *   如果 LTSSM 卡在 DETECT/POLLING/CONFIG，说明硬件或固件配置有问题
 *   Gen5 链路（32GT/s）对信号完整性要求极高，RECOVERY 状态出现频繁
 *
 * 状态机与真实示波器/逻辑分析仪的对应：
 *   调试时观察 LTSSM 状态寄存器（Link Status Register）
 *   示波器抓取 TS1/TS2 训练序列确认链路训练阶段
 */

/**
 * LTSSMState — PCIe 链路训练状态枚举
 *
 * 注意：实际 PCIe Spec 定义了更多子状态，这里只列出主要状态
 * 真实固件中：PCIe 控制器 IP 会输出完整的状态信息和子状态
 */
typedef enum {
    LTSSM_DETECT,      /**< Detect：检测对方是否存在，RX 端检测器电阻 */
    LTSSM_POLLING,     /**< Polling：交换 TS1/TS2，确定双方速率和宽度 */
    LTSSM_CONFIG,      /**< Configuration：协商 Lane 编号、极性、宽度 */
    LTSSM_RECOVERY,    /**< Recovery：速率切换或链路重新训练 */
    LTSSM_L0,          /**< L0：正常工作状态，数据传输就绪 */
    LTSSM_L1,          /**< L1：低功耗待机状态，停止 TLP/DLLP 传输 */
    LTSSM_DISABLED     /**< Disabled：链路被禁用 */
} LTSSMState;

/**
 * LTSSMEvent — PCIe LTSSM 事件枚举
 *
 * 事件来源：
 *   LTSSM_DETECT_ACTIVE — 检测完成，RX 检测到对方
 *   LTSSM_POLLING_ACTIVE — 轮询完成，双方达成速率共识
 *   LTSSM_CONFIG_ACTIVE — 配置完成，链路协商成功
 *   LTSSM_RECOVERY_ACTIVE — 恢复完成，链路重新就绪
 *   LTSSM_PM_REQUEST — 电源管理请求（软件或硬件触发进入低功耗）
 *   LTSSM_ERROR — 链路训练错误（示波器可观察到 TS1/TS2 超时）
 */
typedef enum {
    LTSSM_DETECT_ACTIVE,      /**< 检测到对方，RX 端电阻检测通过 */
    LTSSM_POLLING_ACTIVE,    /**< 轮询完成，双方确认链路速率 */
    LTSSM_CONFIG_ACTIVE,     /**< 配置完成，Lane 协商成功 */
    LTSSM_RECOVERY_ACTIVE,   /**< 恢复完成，链路重新进入 L0 */
    LTSSM_PM_REQUEST,         /**< 电源管理请求，进入 L1/L2 */
    LTSSM_ERROR               /**< 链路错误，训练失败 */
} LTSSMEvent;

/** LTSSM 状态名称表 */
static const char* ltssm_names[] = {
    "DETECT", "POLLING", "CONFIG", "RECOVERY", "L0", "L1", "DISABLED"
};

/**
 * PCIeLink — PCIe 链路对象
 *
 * 为什么需要 PCIeLink 对象？
 *   固件中，每个 PCIe 控制器对应一个 PCIeLink 对象
 *   控制器 IP 核输出 LTSSM 状态寄存器，固件读取并更新对象状态
 *   gen 字段表示当前链路速率（Gen1=2.5GT/s, Gen2=5GT/s, Gen3=8GT/s...）
 *
 * 真实硬件：
 *   PCIe 控制器 IP（如 Synopsys PCIE PHY）通过 APB 或 AXI 接口暴露状态寄存器
 *   固件轮询或中断读取链路状态，决定后续操作
 */
typedef struct _PCIeLink {
    LTSSMState state;                                /**< 当前 LTSSM 状态 */
    int gen;                                         /**< 当前链路速率代数（1-5）*/
    void (*handle_event)(struct _PCIeLink*, LTSSMEvent);  /**< 事件处理函数 */
} PCIeLink;

/**
 * PCIeLink_handle_event — PCIe LTSSM 事件处理
 *
 * 为什么 L0 → L1 需要 PM_REQUEST？
 *   PCIe L1 是低功耗状态，需要 PCIe Spec 定义的低功耗退出序列
 *   固件需要通过 power management 协议（PM Substate Capability）请求进入 L1
 *   这与 TCP 的 FIN_WAIT 类比：主动请求进入低功耗状态（类比主动关闭）
 *
 * 为什么 L1 → L0 需要经过 RECOVERY？
 *   L1 退出时链路需要重新训练（类似从深度睡眠唤醒）
 *   RECOVERY 状态重新协商链路参数，然后回到 L0
 *   这与 TCP 从 TIME_WAIT 回到 CLOSED 再重新建立连接类似
 *
 * @param link  PCIe 链路对象
 * @param ev    LTSSM 事件
 */
static void PCIeLink_handle_event(PCIeLink* link, LTSSMEvent ev) {
    printf("  [LTSSM] Event=%d (state=%s)\n", ev, ltssm_names[link->state]);
    LTSSMState next = link->state;   /**< 默认：状态不变 */

    if (link->state == LTSSM_DETECT && ev == LTSSM_DETECT_ACTIVE)
        next = LTSSM_POLLING;   /**< 检测通过，开始轮询速率 */
    else if (link->state == LTSSM_POLLING && ev == LTSSM_POLLING_ACTIVE)
        next = LTSSM_CONFIG;    /**< 速率协商完成，开始链路配置 */
    else if (link->state == LTSSM_CONFIG && ev == LTSSM_CONFIG_ACTIVE) {
        /** 配置完成，打印链路速率信息（Gen5 = 32GT/s，每 lane 双向 64GB/s）*/
        printf("  [LTSSM] Link up! Speed=Gen%d\n", link->gen);
        next = LTSSM_L0;        /**< 链路就绪，进入正常工作状态 */
    }
    else if (link->state == LTSSM_L0 && ev == LTSSM_PM_REQUEST)
        next = LTSSM_L1;        /**< 软件请求进入低功耗 */
    else if (link->state == LTSSM_L1 && ev == LTSSM_RECOVERY_ACTIVE)
        next = LTSSM_L0;        /**< 从 L1 唤醒，重新训练后回到 L0 */

    if (next != link->state) {
        printf("  [LTSSM] %s -> %s\n", ltssm_names[link->state], ltssm_names[next]);
        link->state = next;
    }
}

/* ============================================================================
 * 主函数 — 演示三个状态机示例
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                     State Pattern — 状态机模式\n");
    printf("=================================================================\n\n");

    /**
     * =========================================================================
     * 示例1：Gumball Machine — 糖果机状态机
     * =========================================================================
     *
     * 演示场景：连续两位顾客购买糖果，第二位顾客遇到售罄
     * 状态流转：S_NO_QUARTER → S_HAS_QUARTER → S_SOLD → S_NO_QUARTER → S_HAS_QUARTER → S_SOLD → S_SOLD_OUT
     */
    printf("【示例1】Gumball Machine — 糖果机状态机\n");
    printf("-----------------------------------------------------------------\n");
    GumballMachine m = { .state = S_NO_QUARTER, .ball_count = 3 };  /**< 初始：空闲，库存3 */
    gumball_handle_event(&m, EV_INSERT);  /**< 顾客1投币：NO_QUARTER → HAS_QUARTER */
    gumball_handle_event(&m, EV_TURN);    /**< 顾客1转柄：HAS_QUARTER → SOLD → NO_QUARTER */
    gumball_handle_event(&m, EV_INSERT);  /**< 顾客2投币：NO_QUARTER → HAS_QUARTER */
    gumball_handle_event(&m, EV_TURN);    /**< 顾客2转柄：HAS_QUARTER → SOLD → SOLD_OUT（库存耗尽）*/

    /**
     * =========================================================================
     * 示例2：TCP State Machine — TCP 连接状态机
     * =========================================================================
     *
     * 演示场景：三次握手建立连接，然后四次挥手关闭
     * 建立流程：CLOSED → SYN_SENT → ESTABLISHED
     * 关闭流程：ESTABLISHED → CLOSE_WAIT → LAST_ACK → CLOSED
     */
    printf("\n【示例2】TCP Connection State Machine\n");
    printf("-----------------------------------------------------------------\n");
    TCPSocket sk = { .state = TCP_CLOSED };   /**< 初始：连接关闭 */
    sk.set_state = TCP_set_state;              /**< 绑定状态迁移方法 */

    printf("  [Scenario] 3-way handshake:\n");
    tcp_dispatch(&sk, TCP_OPEN);    /**< 主动打开：CLOSED → SYN_SENT */
    tcp_dispatch(&sk, TCP_SYN_ACK); /**< 收到 SYN+ACK：SYN_SENT → ESTABLISHED */

    printf("  [Scenario] Close connection:\n");
    tcp_dispatch(&sk, TCP_FIN);     /**< 收到对方 FIN：ESTABLISHED → CLOSE_WAIT */
    tcp_dispatch(&sk, TCP_CLOSE);   /**< 本地关闭：CLOSE_WAIT → LAST_ACK */
    tcp_dispatch(&sk, TCP_ACK);    /**< 收到最后 ACK：LAST_ACK → CLOSED */

    /**
     * =========================================================================
     * 示例3：PCIe LTSSM — PCIe 链路训练状态机
     * =========================================================================
     *
     * 演示场景：完整的链路训练流程 + 电源管理切换
     * 训练流程：DETECT → POLLING → CONFIG → L0
     * 电源管理：L0 → L1（PM_Request）→ L0（Recovery）
     *
     * 为什么 L1 后能直接回到 L0？
     *   这是简化的演示，实际 PCIe 从 L1 唤醒需要经过 RECOVERY
     *   真实固件流程：L1 → RECOVERY → L0
     */
    printf("\n【示例3】PCIe Link Training State Machine\n");
    printf("-----------------------------------------------------------------\n");
    PCIeLink link = { .state = LTSSM_DETECT, .gen = 5 };  /**< 初始：Gen5 链路检测 */
    link.handle_event = PCIeLink_handle_event;

    /** 链路训练三步曲：检测 → 轮询 → 配置 → L0 */
    link.handle_event(&link, LTSSM_DETECT_ACTIVE);  /**< DETECT → POLLING */
    link.handle_event(&link, LTSSM_POLLING_ACTIVE); /**< POLLING → CONFIG */
    link.handle_event(&link, LTSSM_CONFIG_ACTIVE);  /**< CONFIG → L0 */

    /** 电源管理演示：进入低功耗 L1，然后唤醒回到 L0 */
    link.handle_event(&link, LTSSM_PM_REQUEST);         /**< L0 → L1（电源管理请求）*/
    link.handle_event(&link, LTSSM_RECOVERY_ACTIVE);    /**< L1 → L0（唤醒恢复）*/

    printf("\n=================================================================\n");
    printf(" State 模式核心：状态表驱动 or 状态对象模式\n");
    printf(" Linux 内核应用：TCP 状态机、PCIe LTSSM、DMA 控制器\n");
    printf("=================================================================\n");
    return 0;
}
