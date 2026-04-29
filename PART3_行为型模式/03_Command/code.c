/**
 * ===========================================================================
 * Command Pattern — 命令模式
 * ===========================================================================
 *
 * 设计模式：行为型模式 · Command
 * 核心思想：把"请求/操作"封装成对象，实现请求者与执行者解耦
 *
 * 【示例1】Smart Home Remote — 智能家居遥控器
 *         场景：遥控器（Invoker）通过命令对象控制电灯/风扇（Receiver）
 *         嵌入式类比：GPIO 控制器（请求者）通过命令对象驱动外设（执行者）
 *         真实硬件：遥控器按钮 → 命令对象 → Light_on()/CeilingFan_high() 固件函数
 *
 * 【示例2】Linux Input Subsystem — 键盘输入子系统
 *         场景：键盘中断产生 InputEvent（命令对象），驱动层解析事件类型
 *         嵌入式类比：外设 ISR 产生事件，通知上层应用处理
 *         真实硬件：键盘按键 → GPIO 中断 → evdev 子系统 → 用户进程 read()
 *
 * 【示例3】Transaction Undo/Redo — 事务性操作的撤销/重做
 *         场景：对 PCIe 配置寄存器写入带 undo 功能，失败可回滚
 *         嵌入式类比：固件写寄存器前保存旧值，失败时恢复现场
 *         真实硬件：PCIe BAR0/MASK/POWER 寄存器，OTA 升级必须支持回滚
 *
 * ===========================================================================
 *
 * 命令模式三要素：
 *   Command（命令接口）：抽象命令，定义 execute() + undo()
 *   Receiver（执行者）：具体的硬件设备或固件操作函数
 *   Invoker（请求者）：触发命令的对象（遥控器、调度器）
 *
 * 为什么需要命令模式？
 *   1. 请求者与执行者解耦：调用方不知道设备怎么工作，设备不知道被谁调用
 *   2. 命令可排队/延迟执行：嵌入式 ISR 产生的紧急任务放到工作队列执行
 *   3. 支持 undo/redo：事务性操作的核心，关键寄存器写入前保存旧值
 *   4. 日志/审计：命令对象可序列化，记录操作历史（固件 OTA 升级日志）
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Smart Home Remote（智能家居遥控器）
 * ============================================================================
 *
 * 硬件对应关系：
 *   Light（电灯）→ 真实硬件：GPIO 控制的三色灯或 LED，亮度通过 PWM 占空比调节
 *   CeilingFan（吊扇）→ 真实硬件：PWM 控制的直流电机，4档转速对应4个 PWM 配置值
 *
 * 请求者（Invoker）：RemoteControl — 遥控器本身，按下不同按钮触发不同命令
 * 执行者（Receiver）：Light / CeilingFan — 实际的设备固件
 * 命令对象（Command）：LightOnCommand / LightOffCommand / CeilingFanCommand
 *
 * 设计意图：
 *   每次按键只发送"命令对象"给遥控器，遥控器不知道具体是哪种设备
 *   新增一种设备（如智能窗帘）只需新增命令类，无需修改遥控器代码
 *   这正是"开闭原则"（对扩展开放，对修改封闭）的体现
 */

/**
 * Light — 电灯设备（Receiver 执行者之一）
 *
 * 硬件对应：
 *   location → 设备地址或所在房间标识，可对应设备树节点名
 *   brightness → LED 亮度等级（0=关闭, 100=全亮），固件中对应 PWM 占空比寄存器值
 *
 * 实现细节：
 *   这里的 brightness 是纯软件状态，真实硬件中需要通过 PWM 寄存器控制
 *   例如：PWM_DUTY_CYCLE_REG = (brightness * PWM_MAX) / 100
 */
typedef struct _Light {
    char location[32];   /**< 设备位置/名称，如 "Living Room"，固件中可对应设备树节点名 */
    int brightness;      /**< 亮度等级 [0-100]，固件中对应 PWM 占空比寄存器值 */
} Light;

/**
 * Light_on — 打开电灯（Receiver 操作）
 *
 * 为什么亮度设为 100 而不是保存原始状态？
 *   命令模式要求 execute() 幂等或可逆，亮度写 100 是确定性的初始状态
 *   真实硬件：GPIO_set(LED_PIN, HIGH) + PWM_setDuty(100%)
 */
static void Light_on(Light* light) {
    light->brightness = 100;
    printf("  [Light:%s] ON (brightness=%d)\n", light->location, light->brightness);
}

/**
 * Light_off — 关闭电灯（Receiver 操作）
 *
 * 为什么 brightness 设为 0 而不是直接关闭设备？
 *   保存状态让 undo 命令可以恢复真实的前一个亮度值（支持"部分亮度"场景）
 *   真实硬件：GPIO_set(LED_PIN, LOW) 或 PWM_setDuty(0%)
 */
static void Light_off(Light* light) {
    light->brightness = 0;
    printf("  [Light:%s] OFF\n", light->location);
}

/**
 * Light_setBrightness — 设置电灯亮度（支持 undo 的通用写操作）
 *
 * 为什么需要一个单独的 setter 而不是让 undo 直接调用 Light_on/off？
 *   因为 undo 需要恢复到"任意历史亮度值"，不只是 0 或 100 两种状态
 *   例如：当前亮度 80，undo 后应恢复到 50，而不是变成 100
 *   真实硬件：PWM 占空比寄存器写入目标值
 *
 * @param level 目标亮度 [0-100]，超出范围自动截断到 [0, 100]
 */
static void Light_setBrightness(Light* light, int level) {
    light->brightness = level > 0 ? (level > 100 ? 100 : level) : 0;
    printf("  [Light:%s] brightness=%d\n", light->location, light->brightness);
}

/**
 * Light_new — 构造 Light 对象
 *
 * 为什么用动态分配而不是栈上构造？
 *   命令对象需要持有 Light 指针，且生命周期可能超过创建它的函数作用域
 *   真实嵌入式场景：如果设备对象在堆（动态内存池）中创建，需要 malloc
 *   注意：固件中应优先用静态分配或对象池，此处为演示模式用 malloc
 */
static Light* Light_new(const char* location) {
    Light* light = malloc(sizeof(Light));
    strncpy(light->location, location, 31);
    light->brightness = 0;    /**< 初始状态：熄灯，与硬件上电默认状态一致 */
    return light;
}

/**
 * CeilingFan — 吊扇设备（Receiver 执行者之二）
 *
 * 硬件对应：
 *   speed → 电机转速档位：0=停止, 1=低速, 2=中速, 3=高速
 *   真实硬件：PWM 频率或占空比控制直流电机转速，3档对应3个 PWM 配置值
 */
typedef struct _CeilingFan {
    char location[32];   /**< 设备位置标识 */
    int speed;           /**< 转速档位 [0-3]，对应电机驱动器的 PWM 占空比 */
} CeilingFan;

/**
 * CeilingFan_high — 设置吊扇高速档
 *   真实硬件：PWM 占空比设为 100%（或最高档对应的 PWM 值）
 */
static void CeilingFan_high(CeilingFan* fan) { fan->speed = 3; printf("  [CeilingFan:%s] HIGH\n", fan->location); }

/**
 * CeilingFan_medium — 设置吊扇中速档
 *   真实硬件：PWM 占空比设为中等值（如 60%）
 */
static void CeilingFan_medium(CeilingFan* fan) { fan->speed = 2; printf("  [CeilingFan:%s] MEDIUM\n", fan->location); }

/**
 * CeilingFan_low — 设置吊扇低速档
 *   真实硬件：PWM 占空比设为低值（如 30%），避免启动电流过大
 */
static void CeilingFan_low(CeilingFan* fan) { fan->speed = 1; printf("  [CeilingFan:%s] LOW\n", fan->location); }

/**
 * CeilingFan_off — 关闭吊扇
 *   真实硬件：PWM 占空比设为 0%，电机驱动器进入停止状态
 */
static void CeilingFan_off(CeilingFan* fan) { fan->speed = 0; printf("  [CeilingFan:%s] OFF\n", fan->location); }

/**
 * CeilingFan_new — 构造 CeilingFan 对象
 *
 * 注意：speed 初始化为 0（上电默认停止），与真实硬件上电状态一致
 * 真实固件：电机驱动器默认输出 0 占空比，避免上电时意外转动
 */
static CeilingFan* CeilingFan_new(const char* location) {
    CeilingFan* fan = malloc(sizeof(CeilingFan));
    strncpy(fan->location, location, 31);
    fan->speed = 0;
    return fan;
}

/* ============================================================================
 * 命令接口层（Command 模式的核心抽象）
 * ============================================================================
 *
 * ICommand — 命令对象接口
 *
 * 为什么用函数指针而不是虚函数（C++）？
 *   C 语言没有继承和虚函数，用函数指针模拟多态
 *   每个具体命令结构体的第一个成员是 ICommand base，保证内存布局一致
 *   这叫"结构体嵌入"（struct embedding），相当于 C++ 的公有继承
 *
 * execute() — 执行命令的核心操作
 * undo() — 撤销命令，恢复执行前的状态
 *
 * 嵌入式视角：
 *   execute 可类比为"事务提交"，undo 为"事务回滚"
 *   固件更新关键寄存器时必须支持回滚，否则 OTA 升级失败会导致砖设备
 */

/**
 * ICommand — 命令对象统一接口
 *
 * 所有具体命令（LightOnCommand、CeilingFanCommand 等）都必须实现这两个函数指针
 * 这样 Invoker（RemoteControl）只需调用 cmd->execute(cmd)，无需知道具体命令类型
 * 这就是"依赖倒置"：高层模块（RemoteControl）依赖抽象（ICommand），不依赖具体实现
 */
typedef struct _ICommand {
    void (*execute)(struct _ICommand*);    /**< 执行命令，Receiver 执行实际操作 */
    void (*undo)(struct _ICommand*);       /**< 撤销命令，恢复到 execute 前的状态 */
} ICommand;

/**
 * LightOnCommand — 开灯命令
 *
 * 为什么需要 prev_brightness？
 *   undo 时不能简单地调用 Light_off()（那会把灯关掉），而要恢复到开灯前的亮度
 *   例如：原亮度 30，execute 把亮度改成 100，undo 应恢复到 30
 *
 * 真实硬件场景类比：
 *   修改某个外设寄存器前，先读取旧值保存，再写入新值，失败时恢复旧值
 *   这与固件中保护关键寄存器的做法完全一致
 */
typedef struct _LightOnCommand {
    ICommand base;                          /**< 命令接口，必须作为第一个成员 */
    Light* light;                          /**< 指向目标设备（Receiver） */
    int prev_brightness;                   /**< 保存执行前的亮度，用于 undo 恢复 */
} LightOnCommand;

/**
 * LightOnCommand_execute — 开灯命令的执行逻辑
 *
 * 为什么先保存亮度再开灯？
 *   状态机的"读-改-写"必须原子化，防止并发场景下 undo 拿到错误的旧值
 *   真实固件：读寄存器（SAVE）→ 修改（MODIFY）→ 写回（WRITE_BACK）
 */
static void LightOnCommand_execute(ICommand* cmd) {
    LightOnCommand* c = (LightOnCommand*)cmd;
    c->prev_brightness = c->light->brightness;  /**< Step 1: 保存旧状态 */
    Light_on(c->light);                          /**< Step 2: 执行新操作 */
}

/**
 * LightOnCommand_undo — 撤销开灯命令
 *
 * 为什么 undo 调用 Light_setBrightness 而不是 Light_off？
 *   undo 的语义是"恢复到 execute 之前的状态"，之前可能是 30 亮度，不一定是关闭
 *   用 Light_setBrightness 可以精确恢复到任意历史值
 */
static void LightOnCommand_undo(ICommand* cmd) {
    LightOnCommand* c = (LightOnCommand*)cmd;
    Light_setBrightness(c->light, c->prev_brightness);
}

/**
 * LightOnCommand_new — 构造开灯命令对象
 *
 * 为什么需要构造器？
 *   C 没有构造函数，初始化逻辑必须显式写出
 *   构造器负责绑定 Receiver（Light）和初始化 prev_brightness
 */
static LightOnCommand* LightOnCommand_new(Light* light) {
    LightOnCommand* c = malloc(sizeof(LightOnCommand));
    c->base.execute = LightOnCommand_execute;     /**< 绑定 execute 函数指针 */
    c->base.undo = LightOnCommand_undo;           /**< 绑定 undo 函数指针 */
    c->light = light;                             /**< 持有 Receiver 指针 */
    c->prev_brightness = 0;                       /**< 初始值，安全默认值 */
    return c;
}

/**
 * LightOffCommand — 关灯命令
 *
 * 与 LightOnCommand 的区别：
 *   LightOnCommand 执行后亮度变为 100，undo 恢复到之前任意值
 *   LightOffCommand 执行后亮度变为 0，undo 恢复之前的亮度
 *   两者 undo 的目的相同（恢复旧值），但 execute 触发的操作不同
 */
typedef struct _LightOffCommand {
    ICommand base;                          /**< 命令接口，与 LightOnCommand 结构一致 */
    Light* light;                          /**< 目标设备 */
    int prev_brightness;                   /**< 执行前的亮度（用于 undo） */
} LightOffCommand;

static void LightOffCommand_execute(ICommand* cmd) {
    LightOffCommand* c = (LightOffCommand*)cmd;
    c->prev_brightness = c->light->brightness;  /**< 保存旧状态，再关灯 */
    Light_off(c->light);
}

static void LightOffCommand_undo(ICommand* cmd) {
    LightOffCommand* c = (LightOffCommand*)cmd;
    Light_setBrightness(c->light, c->prev_brightness);  /**< 精确恢复到旧亮度 */
}

static LightOffCommand* LightOffCommand_new(Light* light) {
    LightOffCommand* c = malloc(sizeof(LightOffCommand));
    c->base.execute = LightOffCommand_execute;
    c->base.undo = LightOffCommand_undo;
    c->light = light;
    c->prev_brightness = 0;
    return c;
}

/**
 * CeilingFanCommand — 吊扇调速命令（支持任意档位）
 *
 * 为什么 CeilingFanCommand 能处理所有档位，而 Light 需要两个命令？
 *   吊扇的 speed 是连续档位（0/1/2/3），需要 new_speed 指定目标档位
 *   电灯只有两种状态（开/关），所以用两个独立命令更直观
 *
 * 嵌入式视角：
 *   这相当于一个"可参数化的命令"，类似固件中的 setParam(type, value) 操作
 *   new_speed 相当于参数，prev_speed 相当于保护现场
 */
typedef struct _CeilingFanCommand {
    ICommand base;              /**< 命令接口 */
    CeilingFan* fan;           /**< 目标吊扇设备 */
    int prev_speed;           /**< 执行前转速（用于 undo） */
    int new_speed;             /**< 目标转速，执行时设为该档位 */
} CeilingFanCommand;

/**
 * CeilingFanCommand_execute — 执行吊扇调速
 *
 * 为什么用 if-else 而不用 switch？
 *   固件风格倾向保守，避免 switch 的 case 穿透风险（虽然这里没有穿透）
 *   超出 [0-3] 范围的值统一当作 high 处理（3），防止异常值导致未知状态
 */
static void CeilingFanCommand_execute(ICommand* cmd) {
    CeilingFanCommand* c = (CeilingFanCommand*)cmd;
    c->prev_speed = c->fan->speed;     /**< 先保存当前转速，再执行新操作 */
    if (c->new_speed == 0) CeilingFan_off(c->fan);
    else if (c->new_speed == 1) CeilingFan_low(c->fan);
    else if (c->new_speed == 2) CeilingFan_medium(c->fan);
    else CeilingFan_high(c->fan);      /**< speed >= 3 时默认高速档 */
}

/**
 * CeilingFanCommand_undo — 撤销吊扇调速
 *
 * 为什么 undo 时要交换 prev_speed 和当前 speed？
 *   undo 语义：恢复到 execute 之前，即恢复到 prev_speed
 *   但 undo 后需要把"当前 speed"存为下一次 undo 的 prev_speed
 *   所以做一次交换：prev_speed = fan.speed（当前）再恢复
 */
static void CeilingFanCommand_undo(ICommand* cmd) {
    CeilingFanCommand* c = (CeilingFanCommand*)cmd;
    int prev = c->prev_speed;                  /**< 暂存 undo 目标值 */
    c->prev_speed = c->fan->speed;             /**< 更新 prev_speed，为下次 undo 准备 */
    if (prev == 0) CeilingFan_off(c->fan);
    else if (prev == 1) CeilingFan_low(c->fan);
    else if (prev == 2) CeilingFan_medium(c->fan);
    else CeilingFan_high(c->fan);
}

static CeilingFanCommand* CeilingFanCommand_new(CeilingFan* fan, int speed) {
    CeilingFanCommand* c = malloc(sizeof(CeilingFanCommand));
    c->base.execute = CeilingFanCommand_execute;
    c->base.undo = CeilingFanCommand_undo;
    c->fan = fan;
    c->prev_speed = 0;
    c->new_speed = speed;              /**< 构造时指定目标转速，执行时使用 */
    return c;
}

/* ============================================================================
 * Invoker 层：RemoteControl（遥控器）
 * ============================================================================
 *
 * RemoteControl — 请求者（Invoker）
 *
 * 为什么需要 RemoteControl？
 *   它持有多个命令槽位（slots），管理命令的注册和触发
 *   这正是"请求者"角色的典型实现：持有命令，但不知道命令的具体内容
 *
 * 嵌入式视角：
 *   slots 数组相当于中断向量表或命令调度表
 *   setCommand 像是在注册中断处理函数
 *   buttonPressed 像是在触发中断
 *
 * undo_cmd — 全局撤销栈（只支持单级撤销）
 *   为什么只有一个撤销命令？
 *   为简化实现，真正生产环境应支持命令历史栈（如 CommandHistory）
 *   单级撤销在固件中对应"看门狗复位前的紧急保存状态"
 */

/**
 * REMOTE_SLOTS — 遥控器按钮数量
 *
 * 为什么是 7？
 *   典型红外遥控器的按钮数量，真实硬件可对应 7 个 GPIO 按键
 *   也可以理解为 7 个中断向量或 7 个命令槽位
 */
#define REMOTE_SLOTS 7

typedef struct _RemoteControl {
    ICommand* slots[REMOTE_SLOTS];     /**< 命令槽位表，索引对应物理按钮编号 */
    ICommand* undo_cmd;                /**< 最近一次执行的命令，用于 undo 操作 */
    void (*setCommand)(struct _RemoteControl*, int slot, ICommand* cmd);   /**< 注册命令 */
    void (*buttonPressed)(struct _RemoteControl*, int slot);                /**< 按下按钮 */
    void (*undoButton)(struct _RemoteControl*);                             /**< 按下撤销键 */
} RemoteControl;

/**
 * RemoteControl_setCommand — 注册命令到指定槽位
 *
 * 为什么需要边界检查？
 *   防止数组越界访问，保护嵌入式系统的内存安全
 *   真实固件：任何外设访问都需要边界/合法性检查，防止异常数据破坏系统
 */
static void RemoteControl_setCommand(RemoteControl* rc, int slot, ICommand* cmd) {
    if (slot < 0 || slot >= REMOTE_SLOTS) return;   /**< 越界保护，忽略无效槽位 */
    rc->slots[slot] = cmd;
    printf("[Remote] Slot %d configured\n", slot);
}

/**
 * RemoteControl_buttonPressed — 按下按钮触发命令
 *
 * 为什么需要 NULL 检查？
 *   未注册的槽位（如空按钮）不执行任何操作，防御性编程
 *   真实固件：中断处理中必须检查外设是否准备好，否则会读到垃圾数据
 *
 * 为什么每次执行后更新 undo_cmd？
 *   撤销按钮应该撤销"最近一次"操作，所以每次 execute 都要更新 undo_cmd
 */
static void RemoteControl_buttonPressed(RemoteControl* rc, int slot) {
    if (slot < 0 || slot >= REMOTE_SLOTS || rc->slots[slot] == NULL) return;  /**< 防御性检查 */
    rc->slots[slot]->execute(rc->slots[slot]);   /**< 调用者只知道 execute 存在，不知道具体命令类型 */
    rc->undo_cmd = rc->slots[slot];             /**< 更新撤销栈顶，指向刚执行完的命令 */
}

/**
 * RemoteControl_undoButton — 按下撤销按钮
 *
 * 为什么 undo 后把 undo_cmd 设为 NULL？
 *   撤销只能执行一次，撤销完成后清除引用，避免重复撤销
 *   真实固件：OTA 回滚操作执行后应清除回滚标记，防止重复触发
 */
static void RemoteControl_undoButton(RemoteControl* rc) {
    if (rc->undo_cmd) {
        printf("[Remote] UNDO pressed\n");
        rc->undo_cmd->undo(rc->undo_cmd);   /**< 调用者只知道 undo 存在，调用者无需知道具体命令 */
        rc->undo_cmd = NULL;                 /**< 撤销完成后清除引用 */
    }
}

/**
 * RemoteControl_new — 构造遥控器
 *
 * 为什么用 calloc 而不是 malloc？
 *   calloc 自动将内存清零，确保 slots[] 数组初始全为 NULL
 *   避免野指针（未初始化的指针）是嵌入式 C 的基本安全要求
 */
static RemoteControl* RemoteControl_new(void) {
    RemoteControl* rc = calloc(1, sizeof(RemoteControl));  /**< calloc = 分配+清零 */
    rc->setCommand = RemoteControl_setCommand;
    rc->buttonPressed = RemoteControl_buttonPressed;
    rc->undoButton = RemoteControl_undoButton;
    return rc;
}

/* ============================================================================
 * 第二部分：Linux Input Subsystem（键盘输入子系统）
 * ============================================================================
 *
 * 场景说明：
 *   键盘按下/释放产生硬件中断，Linux 内核的 evdev 子系统将中断事件
 *   封装为 InputEvent（命令对象），驱动层和用户空间通过统一接口访问
 *
 * 硬件对应关系：
 *   键盘按键 → GPIO 中断 → GPIO Driver（Linux）→ Input Subsystem → 用户进程
 *   EV_KEY (0x01) — 键码类型，表示按键事件
 *   KEY_ENTER (28) — Enter 键的键码（Linux input.h 中定义）
 *   value: 0=RELEASE, 1=PRESS, 2=REPEAT（按键重复）
 *
 * 命令模式在这里的体现：
 *   InputEvent 是"命令对象"，包含"谁按了什么键，值是什么"
 *   InputDevice::report() 是命令的执行方法
 *   实际 Linux 中，input_event 结构体通过 input_report_* 函数层层上报
 */

/**
 * EV_KEY — Linux 输入子系统中按键事件的类型码
 *
 * 在 Linux 内核头文件 <linux/input.h> 中定义：
 *   EV_KEY = 0x01 表示按键事件（PRESS/RELEASE/REPEAT）
 *   其他常用类型：EV_REL (相对位移，如鼠标), EV_ABS (绝对坐标，如触摸屏)
 */
#define EV_KEY 0x01

/**
 * KEY_ENTER — Enter 键的键码
 *
 * 这个值对应 USB HID Usage Table 或 AT Keyboard Scanner Code
 * 固件驱动中，键盘控制器产生 Make Code（按下）/ Break Code（释放）
 * Linux input 子系统负责将硬件码转换为统一的键码体系
 */
#define KEY_ENTER 28

/**
 * InputEvent — 输入事件结构体（命令对象）
 *
 * 为什么 type/code/value 三个字段足够描述所有输入事件？
 *   type — 事件类型（EV_KEY/EV_REL/EV_ABS），决定如何解释 value
 *   code — 具体按键/轴编号（KEY_ENTER/REL_X/ABS_X），与 type 配套
 *   value — 事件值（按下=1/释放=0/重复=2，或坐标轴的原始值）
 *
 * 嵌入式视角：
 *   这个结构体相当于外设驱动的"上报数据结构"
 *   例如：PCIe MSI 中断消息携带事件 ID 和数据
 */
typedef struct _InputEvent {
    unsigned int type;    /**< 事件类型：EV_KEY/EV_REL/EV_ABS */
    unsigned int code;    /**< 事件码：按键编号或轴编号 */
    int value;            /**< 事件值：按下状态或坐标/位移值 */
} InputEvent;

/**
 * InputDevice — 输入设备抽象（Receiver）
 *
 * 实际 Linux 中，每个输入设备（键盘/鼠标/触摸屏）都实现 input_device_ops
 * 这里简化了：只保留 report() 方法，对应内核的 input_event() 函数
 *
 * 为什么 name 是必需的？
 *   固件中每个外设都需要名字，用于 /dev/input/eventX 的标识和日志
 */
typedef struct _InputDevice {
    char name[32];                                        /**< 设备名称，如 "AT Keyboard" */
    void (*report)(struct _InputDevice*, InputEvent*);    /**< 上报输入事件的方法 */
} InputDevice;

/**
 * InputDevice_report — 上报输入事件
 *
 * 为什么把 InputEvent* 作为参数而不是返回值？
 *   事件对象由调用方分配和填充（外设 ISR 中），设备驱动只负责填充字段
 *   这避免了动态分配的开销，对 ISR 上下文非常重要
 *   Linux 内核中 input_event() 也是这样设计的
 *
 * @param dev  目标输入设备
 * @param ev   事件数据（调用方负责分配和填充）
 */
static void InputDevice_report(InputDevice* dev, InputEvent* ev) {
    const char* type_name = (ev->type == EV_KEY) ? "EV_KEY" : "UNKNOWN";   /**< 只处理 EV_KEY 类型 */
    const char* action = (ev->value == 0) ? "RELEASE" : (ev->value == 1 ? "PRESS" : "REPEAT");
    printf("  [Input:%s] type=%s, code=%d, value=%s\n", dev->name, type_name, ev->code, action);
}

/**
 * InputDevice_new — 构造 InputDevice 对象
 *
 * @param name 设备名称，如 "AT Keyboard"
 * @return 新分配的 InputDevice 对象
 */
static InputDevice* InputDevice_new(const char* name) {
    InputDevice* dev = malloc(sizeof(InputDevice));
    strncpy(dev->name, name, 31);
    dev->report = InputDevice_report;
    return dev;
}

/* ============================================================================
 * 第三部分：Transaction Undo/Redo（事务性操作的撤销/重做）
 * ============================================================================
 *
 * 场景说明：
 *   PCIe 配置寄存器写入（BAR0/MASK/POWER）需要事务性保护
 *   每次写入前保存旧值，失败时回滚，保证固件不会因为写入错误而死机
 *
 * 硬件对应关系：
 *   ConfigRegistry — PCIe 配置空间（PCIe Spec Section 6）
 *     BAR0 — Base Address Register 0，映射外设寄存器地址
 *     Interrupt Mask — MSI 中断掩码，控制哪些中断源可以触发 MSI
 *     Power State — PCIe 链路电源状态（D0/D3hot/D3cold）
 *
 * 为什么固件必须支持事务性寄存器写入？
 *   PCIe 外设的寄存器有严格的访问时序要求
 *   写错 BAR0 会导致 CPU 访问非法地址，引发总线错误（SERR）
 *   固件 OTA 升级过程中，一次写失败可能导致整个芯片无法启动
 */

/**
 * ConfigRegistry — PCIe 配置寄存器模拟
 *
 * 为什么用固定偏移的 struct 而不是分散的全局变量？
 *   PCIe 配置空间本身就是一段连续的 MMIO 区域（Memory-Mapped I/O）
 *   用 struct 模拟这段地址空间，便于用 offsetof 计算字段偏移
 *   真实硬件：PCIe BAR 空间被映射到 CPU 地址空间，按偏移访问
 *
 * 字段说明：
 *   pcie_bar0 — 外设寄存器基地址，固件通过它访问外设寄存器
 *   interrupt_mask — MSI 中断屏蔽位，控制哪些中断源可以触发 MSI
 *   power_state — PCIe 链路电源状态，D0=工作，D3=关闭
 */
typedef struct _ConfigRegistry {
    uint32_t pcie_bar0;        /**< PCIe BAR0：外设寄存器物理地址映射 */
    uint32_t interrupt_mask;   /**< MSI 中断掩码寄存器 */
    uint32_t power_state;      /**< PCIe 链路电源状态 */
} ConfigRegistry;

/**
 * ConfigCommand — 配置命令基类
 *
 * 包含所有配置命令共有的字段：指向 ConfigRegistry 的指针
 * 具体命令在此基础上扩展自己的字段（如偏移、值等）
 */
typedef struct _ConfigCommand {
    ICommand base;             /**< 命令接口 */
    ConfigRegistry* reg;       /**< 目标配置寄存器区域 */
} ConfigCommand;

/**
 * WriteConfigCommand — 配置寄存器写入命令
 *
 * 嵌入式场景：
 *   向 PCIe 配置空间写入一个 32-bit 值，写入前保存旧值（用于回滚）
 *   这相当于固件中"读-改-写"序列，外层调用者无需关心具体是哪个寄存器
 *
 * 字段说明：
 *   offset — 相对于 reg 起始地址的字节偏移，计算字段地址用
 *   new_value — 要写入的新值
 *   old_value — 写入前的旧值（由 execute 时读取保存，用于 undo）
 */
typedef struct _WriteConfigCommand {
    ConfigCommand base;     /**< 基类，包含 reg 指针 */
    int offset;             /**< 目标字段在 ConfigRegistry 中的字节偏移 */
    uint32_t new_value;     /**< 写入的新值 */
    uint32_t old_value;     /**< 写入前的旧值，execute 时读取保存 */
} WriteConfigCommand;

/**
 * WriteConfigCommand_execute — 执行寄存器写入
 *
 * 为什么先读后写？
 *   固件的安全规范：修改任何外设寄存器前，必须先备份原值
 *   失败时 undo 能恢复到修改前的精确状态
 *
 * 为什么用指针算术计算地址？
 *   (char*)reg + offset 相当于 MMIO 地址计算
 *   真实硬件：PCIe BAR 空间映射到 CPU 地址空间，按偏移访问
 *   *(uint32_t*)addr 是解引用 MMIO 地址，相当于 readl/writel
 */
static void WriteConfigCommand_execute(ICommand* cmd) {
    WriteConfigCommand* c = (WriteConfigCommand*)cmd;
    uint32_t* p = (uint32_t*)((char*)c->base.reg + c->offset);  /**< 计算目标地址 */
    c->old_value = *p;                                           /**< Step 1: 读取旧值（备份）*/
    printf("  [Config] Write 0x%04X: 0x%08X -> 0x%08X\n", c->offset, c->old_value, c->new_value);
    *p = c->new_value;                                           /**< Step 2: 写入新值 */
}

/**
 * WriteConfigCommand_undo — 撤销寄存器写入
 *
 * 为什么 undo 也要打印日志？
 *   嵌入式固件任何操作都应留有记录（审计日志）
 *   特别是涉及 PCIe 寄存器修改时，日志对排查启动失败至关重要
 */
static void WriteConfigCommand_undo(ICommand* cmd) {
    WriteConfigCommand* c = (WriteConfigCommand*)cmd;
    uint32_t* p = (uint32_t*)((char*)c->base.reg + c->offset);  /**< 重新计算目标地址 */
    printf("  [Config] UNDO: 0x%04X -> 0x%08X\n", *p, c->old_value);
    *p = c->old_value;                                          /**< 恢复旧值 */
}

/**
 * WriteConfigCommand_new — 构造配置写入命令
 *
 * @param reg     目标配置寄存器
 * @param offset  字段在寄存器中的字节偏移
 * @param value   要写入的新值
 * @return 新分配的命令对象
 */
static WriteConfigCommand* WriteConfigCommand_new(ConfigRegistry* reg, int offset, uint32_t value) {
    WriteConfigCommand* c = malloc(sizeof(WriteConfigCommand));
    c->base.base.execute = WriteConfigCommand_execute;
    c->base.base.undo = WriteConfigCommand_undo;
    c->base.reg = reg;
    c->offset = offset;
    c->new_value = value;
    c->old_value = 0;          /**< 旧值在 execute 时由读取得到，初始化为 0 */
    return c;
}

/* ============================================================================
 * CommandHistory — 命令历史管理器（支持多次撤销）
 * ============================================================================
 *
 * 为什么需要命令历史栈？
 *   RemoteControl 只支持单级撤销（一个 undo_cmd），无法撤销多步操作
 *   CommandHistory 用栈结构支持多次 undo，直到栈空为止
 *
 * 嵌入式视角：
 *   这相当于"操作日志"，在固件升级或危险操作前，记录所有可回滚步骤
 *   深嵌入式系统（如航天控制器）要求每次写操作都必须可追溯、可撤销
 */
#define HISTORY_SIZE 64   /**< 命令历史栈深度，64层足够应对大多数场景 */

typedef struct _CommandHistory {
    ICommand* undo_stack[HISTORY_SIZE];   /**< 撤销栈，存储所有已执行的命令 */
    int undo_top;                         /**< 栈顶指针，-1 时表示空栈（但这里用 0 表示空栈）*/
    void (*execute)(struct _CommandHistory*, ICommand*);  /**< 执行并入栈 */
    void (*undo)(struct _CommandHistory*);                /**< 出栈并撤销 */
} CommandHistory;

/**
 * CommandHistory_execute — 执行命令并入栈
 *
 * 为什么 execute 要手动调用 cmd->execute 而不是让调用者直接调？
 *   封装为一层，保证所有命令执行都经过历史管理器
 *   直接调 cmd->execute(cmd) 会绕过历史栈，导致 undo 无法追踪
 */
static void CommandHistory_execute(CommandHistory* h, ICommand* cmd) {
    cmd->execute(cmd);                     /**< 执行命令本身 */
    h->undo_stack[h->undo_top++] = cmd;   /**< 执行后入栈（关键：保存执行顺序）*/
}

/**
 * CommandHistory_undo — 弹出栈顶命令并撤销
 *
 * 为什么用 --undo_top 而不是 undo_top--？
 *   --undo_top 先减再取值，保证栈顶指针指向"最后入栈的命令"
 *   pop 逻辑：先移动指针，再取命令，保证 undo 顺序正确（后进先出）
 */
static void CommandHistory_undo(CommandHistory* h) {
    if (h->undo_top == 0) { printf("  [History] Nothing to undo\n"); return; }  /**< 空栈检查 */
    ICommand* cmd = h->undo_stack[--h->undo_top];   /**< 弹出最近命令 */
    cmd->undo(cmd);                                /**< 执行撤销 */
}

/**
 * CommandHistory_new — 构造命令历史管理器
 *
 * 为什么用 calloc？
 *   undo_top 必须初始化为 0（空栈），防止野内存导致不可预测的行为
 *   防御性初始化是嵌入式 C 的基本安全规范
 */
static CommandHistory* CommandHistory_new(void) {
    CommandHistory* h = calloc(1, sizeof(CommandHistory));  /**< calloc 分配+清零，undo_top 初始化为 0 */
    h->execute = CommandHistory_execute;
    h->undo = CommandHistory_undo;
    return h;
}

/* ============================================================================
 * 主函数 — 演示三个命令模式示例
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                     Command Pattern — 命令模式\n");
    printf("=================================================================\n\n");

    /**
     * =========================================================================
     * 示例1：Smart Home Remote — 智能家居遥控器
     * =========================================================================
     *
     * 演示场景：遥控器注册4个命令，按下按钮触发执行，然后撤销
     * 为什么撤销后再撤销？
     *   remote->undoButton(remote) 撤销最近一次按下的命令（slot 0 的 light_on）
     *   连续两次 buttonPressed 和两次 undoButton 展示完整的前进-撤销循环
     */
    printf("【示例1】Smart Home Remote — 智能家居遥控器\n");
    printf("-----------------------------------------------------------------\n");
    Light* living_light = Light_new("Living Room");
    CeilingFan* living_fan = CeilingFan_new("Living Room");
    RemoteControl* remote = RemoteControl_new();

    /**
     * 为什么用 &LightOnCommand_new(...)->base 而不是先存变量再取地址？
     *   C 语言的优先级：() > -> > &，所以 &c->base 是 &c->base
     *   实际：&(LightOnCommand_new(...)->base)，先创建对象，再取其 base 成员地址
     *   这行相当于：LightOnCommand* tmp = LightOnCommand_new(...); ICommand* cmd = &tmp->base;
     */
    ICommand* light_on = &LightOnCommand_new(living_light)->base;
    ICommand* light_off = &LightOffCommand_new(living_light)->base;
    ICommand* fan_high = &CeilingFanCommand_new(living_fan, 3)->base;
    ICommand* fan_off = &CeilingFanCommand_new(living_fan, 0)->base;

    remote->setCommand(remote, 0, light_on);
    remote->setCommand(remote, 1, light_off);
    remote->setCommand(remote, 2, fan_high);
    remote->setCommand(remote, 3, fan_off);

    printf("\n  Press slot 0 (light on):\n");
    remote->buttonPressed(remote, 0);    /**< 执行开灯命令 */
    remote->undoButton(remote);         /**< 撤销开灯，恢复到亮度 0 */

    printf("\n  Press slot 2 (fan high):\n");
    remote->buttonPressed(remote, 2);    /**< 执行吊扇高速命令 */
    remote->undoButton(remote);         /**< 撤销吊扇命令，恢复到转速 0 */

    free(living_light); free(living_fan); free(remote);

    /**
     * =========================================================================
     * 示例2：Linux Input Subsystem — 键盘输入子系统
     * =========================================================================
     *
     * 演示场景：键盘 Enter 键按下和释放，InputDevice.report() 模拟驱动上报事件
     * 为什么 ev_enter_press 和 ev_enter_release 是栈上变量而不是 malloc？
     *   ISR 上下文中不能使用 malloc（可能导致阻塞），栈变量更安全
     *   真实 Linux 驱动：input_event 在栈上分配，通过 input_event() 传递给子系统
     */
    printf("\n【示例2】Linux Input Subsystem — 键盘输入子系统\n");
    printf("-----------------------------------------------------------------\n");
    InputDevice* keyboard = InputDevice_new("AT Keyboard");
    InputEvent ev_enter_press = {EV_KEY, KEY_ENTER, 1};    /**< value=1: 按下事件 */
    InputEvent ev_enter_release = {EV_KEY, KEY_ENTER, 0}; /**< value=0: 释放事件 */
    printf("  Keyboard interrupt fires:\n");
    keyboard->report(keyboard, &ev_enter_press);      /**< 上报按下事件 */
    keyboard->report(keyboard, &ev_enter_release);   /**< 上报释放事件 */
    free(keyboard);

    /**
     * =========================================================================
     * 示例3：Transaction Undo/Redo — PCIe 寄存器事务操作
     * =========================================================================
     *
     * 演示场景：连续写入3个 PCIe 配置寄存器，然后撤销2次
     *
     * 为什么用 offsetof 风格计算偏移？
     *   &((ConfigRegistry*)0)->pcie_bar0 计算 pcie_bar0 字段相对于结构体起始的字节偏移
     *   这是固件中访问外设寄存器数组的常用技巧：通过基址+偏移定位寄存器
     *   真实硬件：PCIe 配置空间的 BAR0/INTERRUPT_MASK/POWER_STATE 都在配置空间的不同偏移处
     *
     * 为什么初始值是 0xFEBC0000、0x00000001、0x00000003？
     *   0xFEBC0000 是典型的 PCIe BAR0 地址（A类PCIe设备常用的映射地址）
     *   0x00000001 表示 MSI 中断使能位（bit 0 = MSI enable）
     *   0x00000003 表示 D0 电源状态（PCIe 电源管理中 D0 = 全功能工作状态）
     */
    printf("\n【示例3】Transaction Undo/Redo — 事务性操作的撤销/重做\n");
    printf("-----------------------------------------------------------------\n");
    ConfigRegistry reg = { .pcie_bar0 = 0xFEBC0000, .interrupt_mask = 0x00000001, .power_state = 0x00000003 };
    CommandHistory* history = CommandHistory_new();

    /**
     * 计算三个字段在 ConfigRegistry 中的偏移量
     * offsetof(ConfigRegistry, pcie_bar0) 的手工实现
     * 嵌入式场景：外设寄存器地址 = 基址 + 偏移，这一步计算外设寄存器的偏移地址
     */
    int off_bar0 = (int)&((ConfigRegistry*)0)->pcie_bar0;
    int off_mask = (int)&((ConfigRegistry*)0)->interrupt_mask;
    int off_power = (int)&((ConfigRegistry*)0)->power_state;

    printf("  Initial: BAR0=0x%08X, MASK=0x%08X\n", reg.pcie_bar0, reg.interrupt_mask);

    /**
     * 创建三个配置写入命令
     * cmd1: 将 BAR0 从 0xFEBC0000 改为 0xFEBE0000（外设地址重映射）
     * cmd2: 将 INTERRUPT_MASK 从 0x00000001 改为 0x00000003（同时使能 MSI 和另一个中断源）
     * cmd3: 将 POWER_STATE 从 D0(0x00000003) 改为 OFF(0x00000000)（关闭 PCIe 链路）
     */
    ICommand* cmd1 = (ICommand*)&WriteConfigCommand_new(&reg, off_bar0, 0xFEBE0000)->base;
    ICommand* cmd2 = (ICommand*)&WriteConfigCommand_new(&reg, off_mask, 0x00000003)->base;
    ICommand* cmd3 = (ICommand*)&WriteConfigCommand_new(&reg, off_power, 0x00000000)->base;

    printf("\n  Executing 3 config changes:\n");
    history->execute(history, cmd1);  /**< 执行 BAR0 重映射 */
    history->execute(history, cmd2);  /**< 执行中断掩码修改 */
    history->execute(history, cmd3);  /**< 执行电源状态关闭 */
    printf("  After: BAR0=0x%08X, MASK=0x%08X\n", reg.pcie_bar0, reg.interrupt_mask);

    printf("\n  Undo 2 times:\n");
    history->undo(history);   /**< 撤销第3个命令（恢复 power_state）*/
    history->undo(history);   /**< 撤销第2个命令（恢复 interrupt_mask）*/
    printf("  After 2x undo: BAR0=0x%08X\n", reg.pcie_bar0);

    free(history);

    printf("\n=================================================================\n");
    printf(" Command 模式核心：命令封装 receiver+action+undo\n");
    printf(" Linux 内核应用：Input Subsystem、workqueue、事务回滚\n");
    printf(" PCIe 固件应用：配置寄存器写入保护、OTA 升级回滚\n");
    printf("=================================================================\n");
    return 0;
}
