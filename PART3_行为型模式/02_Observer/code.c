/**
 * ===========================================================================
 * 观察者模式（Observer Pattern）— C语言实现
 * ===========================================================================
 *
 * 【模式名称】观察者模式（Observer Pattern）
 * 【类型】行为型模式（Behavioral Pattern）
 * 【别称】发布-订阅模式（Publish-Subscribe Pattern）
 * 【解决什么问题】
 *   当一个对象（Subject）的状态变化需要通知多个其他对象时，
 *   如果直接在 Subject 内部维护观察者列表、用 if-else 处理通知：
 *   - Subject 和观察者紧耦合（改观察者类型要改 Subject）
 *   - 难以在运行时动态添加/删除观察者
 *   - 违反单一职责原则（Subject 核心职责被通知逻辑污染）
 *
 * 【观察者模式的核心思想】
 *   将"发布-订阅"机制抽象成标准接口：
 *   - Subject（发布者）：维护观察者列表，状态变化时遍历通知
 *   - Observer（订阅者）：定义更新回调接口，具体行为由子类实现
 *   - 推送方式：Subject 主动推送数据（PUSH）；或拉取方式（PULL）
 *
 * 【三个示例的实战场景】
 *   - 示例1 WeatherStation：经典观察者模式，最接近教材的演示
 *   - 示例2 Linux Notifier Chain：内核网络子系统的事件通知
 *   - 示例3 Interrupt Handler Registry：PCIe 设备中断处理函数注册表
 *
 * 【Linux 内核真实应用】
 *   - notifier_call_chain()  — 网络设备、热插拔、电源管理事件
 *   - request_irq() / free_irq() — 中断线处理函数注册/注销
 *   - blocking_notifier_call_chain() — 阻塞式通知链
 *   - kobject_uevent() — 用户空间热插拔事件（/sbin/mdev）
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 第一部分：Weather Station（经典观察者模式）
 *
 * 【场景描述】
 *   气象站（WeatherData）定期获取温度/湿度/气压数据，
 *   多个显示设备（CurrentConditionDisplay、StatisticsDisplay）需要实时更新。
 *
 * 【设计要点】
 *   - Subject 和 Observer 通过接口（函数指针）解耦
 *   - 支持动态注册/注销观察者（运行时动态订阅/取消订阅）
 *   - Subject 状态变化时主动遍历通知所有 Observer（广播）
 * ============================================================================*/

// -----------------------------------------------------------------------------
// 前向声明：打破循环依赖
//   Observer 需要知道 WeatherData 存在，
//   WeatherData 需要知道 Observer 接口存在
// -----------------------------------------------------------------------------
typedef struct _WeatherData WeatherData;

// -----------------------------------------------------------------------------
// 观察者更新回调接口
// 【参数】
//   self   : 观察者实例指针（通常指向派生类结构体）
//   temp   : 温度（摄氏度）
//   humidity: 相对湿度（%）
//   pressure: 气压（hPa）
// 【返回值】void
// 【调用时机】WeatherData.notifyObservers() 被调用时
// -----------------------------------------------------------------------------
typedef void (*UpdateCallback)(void* self, float temp, float humidity, float pressure);

// -----------------------------------------------------------------------------
// 观察者基类接口
// 【设计意图】
//   C 语言没有类继承，我们用结构体模拟：
//   - UpdateCallback 模拟虚函数（多态）
//   - instance 指向派生类实例（在 update 中做 downcast）
//   - name 用于日志追踪（哪个观察者被注册了）
// -----------------------------------------------------------------------------
typedef struct _IObserver {
    UpdateCallback update;  // 虚函数指针：派生类实现具体更新逻辑
    void* instance;        // 指向派生类实例，用于在回调中访问子类数据
    char name[32];          // 观察者名称（调试用）
} IObserver;

// -----------------------------------------------------------------------------
// Subject（发布者）：气象数据
// 【核心职责】
//   - 维护观察者列表（动态数组，支持扩容）
//   - 提供注册/注销接口
//   - 状态变化时触发通知
// 【字段解析】
//   - observer_capacity : 观察者数组容量（动态扩容用）
//   - observer_count   : 当前观察者数量
//   - temperature/humidity/pressure : 气象数据（Subject 的状态）
//   - registerObserver  : 注册观察者（依赖注入）
//   - removeObserver   : 注销观察者（运行时退订）
//   - notifyObservers  : 广播通知（核心：遍历所有观察者并调用其 update）
//   - setMeasurements  : 更新气象数据（触发通知的入口）
// -----------------------------------------------------------------------------
typedef struct _WeatherData {
    IObserver** observers;       // 观察者指针数组（动态扩容）
    int observer_count;          // 当前已注册观察者数量
    int observer_capacity;       // 数组容量（0 表示未初始化）
    float temperature;           // 温度（Subject 状态）
    float humidity;              // 湿度
    float pressure;              // 气压
    void (*registerObserver)(struct _WeatherData*, IObserver* observer);
    void (*removeObserver)(struct _WeatherData*, IObserver* observer);
    void (*notifyObservers)(struct _WeatherData*);
    void (*setMeasurements)(struct _WeatherData*, float temp, float hum, float pres);
} WeatherData;

// -----------------------------------------------------------------------------
// 注册观察者
// 【参数】
//   wd      : WeatherData 指针（Subject）
//   observer: 待注册的观察者指针
// 【行为】
//   - 检查容量，不够则扩容（2倍策略，避免频繁 realloc）
//   - 将观察者添加到数组末尾
// 【容量扩容逻辑】
//   - 初始容量 0：首次扩容设为 4
//   - 之后每次扩容翻倍（4→8→16→...）
//   - 使用 realloc 而非 free+malloc，保留已有数据
// -----------------------------------------------------------------------------
static void WeatherData_registerObserver(WeatherData* wd, IObserver* observer) {
    if (wd->observer_count >= wd->observer_capacity) {
        wd->observer_capacity = wd->observer_capacity == 0 ? 4 : wd->observer_capacity * 2;
        wd->observers = realloc(wd->observers, sizeof(IObserver*) * wd->observer_capacity);
    }
    wd->observers[wd->observer_count++] = observer;
    printf("[WeatherData] Observer '%s' registered (total: %d)\n", observer->name, wd->observer_count);
}

// -----------------------------------------------------------------------------
// 注销观察者
// 【参数】wd: WeatherData 指针，observer: 待注销的观察者指针
// 【行为】
//   - 在观察者数组中查找目标（地址比较）
//   - 找到后将其后面的所有元素前移（O(n) 移动）
//   - 数组长度减 1
// 【注意】本实现不支持"快速注销"（将末尾元素覆盖目标），
//         因为观察者顺序在某些场景下可能有意义
// -----------------------------------------------------------------------------
static void WeatherData_removeObserver(WeatherData* wd, IObserver* observer) {
    for (int i = 0; i < wd->observer_count; i++) {
        if (wd->observers[i] == observer) {
            // 找到目标，将 i+1 及之后的元素前移
            for (int j = i; j < wd->observer_count - 1; j++)
                wd->observers[j] = wd->observers[j + 1];
            wd->observer_count--;
            printf("[WeatherData] Observer '%s' removed\n", observer->name);
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// 通知所有观察者（广播）
// 【调用时机】
//   WeatherData_setMeasurements() 内部调用
//   用户主动触发一次数据更新后，广播给所有订阅者
// 【行为】
//   - 遍历 observers[] 数组
//   - 对每个观察者调用其 update 虚函数
//   - 传入当前的气象数据（推送给观察者）
// 【PUSH vs PULL 两种模式】
//   - PUSH（本实现）：Subject 主动推送完整数据（temp/humidity/pressure）
//   - PULL：Subject 只通知"状态变了"，Observer 按需拉取数据
//   - 本实现用 PUSH，因为 WeatherData 数据量小，Push 更简单
// -----------------------------------------------------------------------------
static void WeatherData_notifyObservers(WeatherData* wd) {
    printf("[WeatherData] Notifying %d observers (temp=%.1f, hum=%.1f, pres=%.1f)\n",
           wd->observer_count, wd->temperature, wd->humidity, wd->pressure);
    for (int i = 0; i < wd->observer_count; i++)
        wd->observers[i]->update(wd->observers[i]->instance,
                                  wd->temperature, wd->humidity, wd->pressure);
}

// -----------------------------------------------------------------------------
// 更新气象数据（触发通知的入口）
// 【参数】temp: 温度，hum: 湿度，pres: 气压
// 【调用时机】
//   气象站硬件读取到新数据后（定时轮询或硬件中断触发）
// 【行为】
//   1. 更新内部状态
//   2. 打印日志（方便调试）
//   3. 触发 notifyObservers 广播
// -----------------------------------------------------------------------------
static void WeatherData_setMeasurements(WeatherData* wd, float temp, float hum, float pres) {
    wd->temperature = temp;
    wd->humidity = hum;
    wd->pressure = pres;
    printf("\n[WeatherData] New measurements received\n");
    WeatherData_notifyObservers(wd);
}

// -----------------------------------------------------------------------------
// WeatherData 构造函数
// 【返回】分配好的 WeatherData 实例，所有函数指针已初始化
// 【内存管理】调用方负责 free（使用 WeatherData_delete()）
// -----------------------------------------------------------------------------
static WeatherData* WeatherData_new(void) {
    WeatherData* wd = calloc(1, sizeof(WeatherData));
    wd->registerObserver = WeatherData_registerObserver;
    wd->removeObserver = WeatherData_removeObserver;
    wd->notifyObservers = WeatherData_notifyObservers;
    wd->setMeasurements = WeatherData_setMeasurements;
    return wd;
}

// -----------------------------------------------------------------------------
// WeatherData 析构函数
// 【行为】
//   - 释放观察者数组内存（观察者实例本身由调用方管理）
//   - 释放 WeatherData 自身
// 【注意】不负责释放观察者实例，因为观察者是独立分配的对象
// -----------------------------------------------------------------------------
static void WeatherData_delete(WeatherData* wd) {
    free(wd->observers);
    free(wd);
}

// -----------------------------------------------------------------------------
// 具体观察者：当前天气显示
// 【职责】显示当前的温度和湿度
// 【字段】
//   base  : 基类部分（包含虚函数和实例指针）
//   last_temp/last_hum : 上一次更新的数据（用于缓存或对比）
// -----------------------------------------------------------------------------
typedef struct _CurrentConditionDisplay {
    IObserver base;       // 基类（必须是第一个字段，支持指针转换）
    float last_temp;      // 上次温度（可用于变化趋势计算）
    float last_hum;       // 上次湿度
} CurrentConditionDisplay;

// -----------------------------------------------------------------------------
// CurrentConditionDisplay 的 update 实现
// 【参数】self 实际指向 CurrentConditionDisplay 实例
// 【行为】缓存当前数据并打印显示
// -----------------------------------------------------------------------------
static void CurrentConditionDisplay_update(void* self, float temp, float humidity, float pressure) {
    (void)pressure;  // 本显示不需要气压数据，忽略
    CurrentConditionDisplay* ccd = (CurrentConditionDisplay*)self;
    ccd->last_temp = temp;
    ccd->last_hum = humidity;
    printf("  [CurrentCondition] Display updated: %.1fC, %.1f%%\n", temp, humidity);
}

// -----------------------------------------------------------------------------
// CurrentConditionDisplay 构造函数
// -----------------------------------------------------------------------------
static CurrentConditionDisplay* CurrentConditionDisplay_new(void) {
    CurrentConditionDisplay* ccd = calloc(1, sizeof(CurrentConditionDisplay));
    ccd->base.update = CurrentConditionDisplay_update;
    ccd->base.instance = ccd;  // 基类 instance 指向派生类自身
    strcpy(ccd->base.name, "CurrentCondition");
    return ccd;
}

// -----------------------------------------------------------------------------
// 具体观察者：统计数据显示
// 【职责】持续统计温度的均值/最大值/最小值
// 【字段】
//   count    : 已接收数据批次
//   sum_temp : 温度累计和（用于计算均值）
//   max_temp/min_temp : 最大/最小温度
// -----------------------------------------------------------------------------
typedef struct _StatisticsDisplay {
    IObserver base;
    int count;             // 统计样本数量
    float sum_temp;         // 累计温度（计算均值用）
    float max_temp;         // 历史最高温度
    float min_temp;         // 历史最低温度
} StatisticsDisplay;

// -----------------------------------------------------------------------------
// StatisticsDisplay 的 update 实现
// 【行为】
//   - 累加计数
//   - 累加温度（用于均值）
//   - 更新最大值/最小值
//   - 打印统计信息
// -----------------------------------------------------------------------------
static void StatisticsDisplay_update(void* self, float temp, float humidity, float pressure) {
    (void)pressure; (void)humidity;  // 本显示不需要气压和湿度
    StatisticsDisplay* sd = (StatisticsDisplay*)self;
    sd->count++;
    sd->sum_temp += temp;
    if (sd->count == 1 || temp > sd->max_temp) sd->max_temp = temp;
    if (sd->count == 1 || temp < sd->min_temp) sd->min_temp = temp;
    printf("  [Statistics] Avg=%.1fC, Max=%.1fC, Min=%.1fC, Count=%d\n",
           sd->sum_temp / sd->count, sd->max_temp, sd->min_temp, sd->count);
}

// -----------------------------------------------------------------------------
// StatisticsDisplay 构造函数
// -----------------------------------------------------------------------------
static StatisticsDisplay* StatisticsDisplay_new(void) {
    StatisticsDisplay* sd = calloc(1, sizeof(StatisticsDisplay));
    sd->base.update = StatisticsDisplay_update;
    sd->base.instance = sd;
    strcpy(sd->base.name, "Statistics");
    return sd;
}


/* ============================================================================
 * 第二部分：Linux Notifier Chain（内核通知链）
 *
 * 【场景描述】
 *   Linux 内核中，多个子系统需要监听同一硬件事件。
 *   例如：网卡设备 UP/DOWN 事件需要网络管理器 + 电源管理器同时响应。
 *
 * 【设计要点】
 *   - 单向链表（不支持双向，因为不需要反向遍历）
 *   - 头插法注册（新注册的在链表头部，通知时先收到）
 *   - 所有回调串行执行，无并发
 *   - 回调返回 int，允许某个观察者阻止后续通知（本实现未用）
 *
 * 【Linux 内核真实实现】
 *   - include/linux/notifier.h 中的 struct notifier_block
 *   - register_netdevice_notifier() / unregister_netdevice_notifier()
 *   - call_netdevice_notifiers() — 触发通知
 * ============================================================================*/

// -----------------------------------------------------------------------------
// 网络设备事件类型常量
// 【嵌入式映射】
//   - NETDEV_UP   : PCIe 设备使能（BAR 已映射，中断已注册）
//   - NETDEV_DOWN : PCIe 设备禁用（释放资源）
//   - NETDEV_CHANGE: PCIe 链路速率/宽度变化（LTSSM 状态切换）
// -----------------------------------------------------------------------------
#define NETDEV_UP     0x01
#define NETDEV_DOWN   0x02
#define NETDEV_CHANGE 0x03

// -----------------------------------------------------------------------------
// 通知回调函数类型
// 【参数】
//   event : 事件类型（NETDEV_UP/DOWN/CHANGE 等）
//   data  : 事件相关数据（通常是设备结构体指针）
// 【返回值】int — 0 表示成功，负值表示阻止后续通知（本实现忽略）
// -----------------------------------------------------------------------------
typedef int (*NotifierCallback)(unsigned long event, void* data);

// -----------------------------------------------------------------------------
// 通知链节点（观察者实例）
// 【字段】
//   callback : 该观察者的回调函数
//   name     : 观察者名称（调试用）
//   next     : 链表下一节点（单向链表，头插法）
// -----------------------------------------------------------------------------
typedef struct _NotifierBlock {
    NotifierCallback callback;   // 观察者的回调函数
    char name[32];               // 名称（调试用）
    struct _NotifierBlock* next; // 链表指针
} NotifierBlock;

// -----------------------------------------------------------------------------
// 通知链（Subject）
// 【字段】
//   head : 链表头指针（NULL 表示空链表）
//   registerNotifier  : 注册观察者
//   unregisterNotifier: 注销观察者
//   callChain         : 触发通知
// -----------------------------------------------------------------------------
typedef struct _NotifierChain {
    NotifierBlock* head;  // 单向链表头（NULL=空链表）
    void (*registerNotifier)(struct _NotifierChain*, NotifierCallback, const char* name);
    void (*unregisterNotifier)(struct _NotifierChain*, NotifierCallback);
    int (*callChain)(struct _NotifierChain*, unsigned long event, void* data);
} NotifierChain;

// -----------------------------------------------------------------------------
// 注册观察者（头插法）
// 【参数】nc: 通知链，cb: 回调函数，name: 观察者名称
// 【头插法说明】
//   新节点插入到 head 之前，新节点成为新的 head
//   优点：O(1) 插入；缺点：后注册的先收到通知
// 【内存分配】每次注册都 malloc 新节点（注销时 free）
// -----------------------------------------------------------------------------
static void NotifierChain_registerNotifier(NotifierChain* nc, NotifierCallback cb, const char* name) {
    NotifierBlock* nb = malloc(sizeof(NotifierBlock));
    nb->callback = cb;
    nb->next = nc->head;   // 新节点指向原 head
    strncpy(nb->name, name, 31);
    nc->head = nb;         // 更新 head 指向新节点
    printf("[NotifierChain] Registered: %s\n", name);
}

// -----------------------------------------------------------------------------
// 注销观察者（单向链表查找并删除）
// 【参数】nc: 通知链，cb: 回调函数指针
// 【实现】维护前驱指针 pp，遍历找到后修改链表
// 【内存】释放被删除节点的内存
// -----------------------------------------------------------------------------
static void NotifierChain_unregisterNotifier(NotifierChain* nc, NotifierCallback cb) {
    NotifierBlock** pp = &nc->head;  // 指向 head 指针本身的地址
    while (*pp) {
        if ((*pp)->callback == cb) {
            NotifierBlock* to_free = *pp;
            printf("[NotifierChain] Unregistered: %s\n", to_free->name);
            *pp = (*pp)->next;  // 修改前驱指针的指向，跳过目标节点
            free(to_free);
            return;
        }
        pp = &(*pp)->next;
    }
}

// -----------------------------------------------------------------------------
// 触发通知链
// 【参数】nc: 通知链，event: 事件类型，data: 事件数据
// 【返回值】int（本实现始终返回 0）
// 【遍历顺序】从头节点开始，依次调用每个观察者的回调
// 【注意】如果某个观察者返回非零值，内核会停止后续通知（本实现未实现此逻辑）
// -----------------------------------------------------------------------------
static int NotifierChain_callChain(NotifierChain* nc, unsigned long event, void* data) {
    NotifierBlock* nb = nc->head;
    printf("[NotifierChain] Broadcasting event=%lu\n", event);
    while (nb) {
        int ret = nb->callback(event, data);
        printf("[NotifierChain]   %s handled (ret=%d)\n", nb->name, ret);
        nb = nb->next;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// 构造函数
// -----------------------------------------------------------------------------
static NotifierChain* NotifierChain_new(void) {
    NotifierChain* nc = calloc(1, sizeof(NotifierChain));
    nc->registerNotifier = NotifierChain_registerNotifier;
    nc->unregisterNotifier = NotifierChain_unregisterNotifier;
    nc->callChain = NotifierChain_callChain;
    return nc;
}

// -----------------------------------------------------------------------------
// 析构函数：释放整条链表
// -----------------------------------------------------------------------------
static void NotifierChain_delete(NotifierChain* nc) {
    NotifierBlock* nb = nc->head;
    while (nb) { NotifierBlock* next = nb->next; free(nb); nb = next; }
    free(nc);
}

// -----------------------------------------------------------------------------
// 网络管理器通知回调
// 【职责】设备 UP 时记录日志，DOWN 时记录日志，CHANGE 时记录日志
// -----------------------------------------------------------------------------
static int netmanager_notifier(unsigned long event, void* data) {
    const char* dev_name = (const char*)data;
    if (event == NETDEV_UP) printf("  [NetManager] Device %s is UP\n", dev_name);
    else if (event == NETDEV_DOWN) printf("  [NetManager] Device %s is DOWN\n", dev_name);
    else if (event == NETDEV_CHANGE) printf("  [NetManager] Device %s changed\n", dev_name);
    return 0;
}

// -----------------------------------------------------------------------------
// 电源管理器通知回调
// 【职责】设备 DOWN 时允许系统进入低功耗（suspend）
// 【特点】只关心 NETDEV_DOWN 事件，对其他事件不处理
// -----------------------------------------------------------------------------
static int powermanager_notifier(unsigned long event, void* data) {
    const char* dev_name = (const char*)data;
    if (event == NETDEV_DOWN) printf("  [PowerManager] Device %s down - can suspend\n", dev_name);
    return 0;
}


/* ============================================================================
 * 第三部分：Interrupt Handler Registry（中断处理函数注册表）
 *
 * 【场景描述】
 *   PCIe 设备共享中断线时，多个设备的中断处理函数注册到同一条 IRQ。
 *   当硬件中断触发时，需要依次调用所有已注册的处理函数。
 *
 * 【Linux 内核真实实现】
 *   - kernel/irq/manage.c 中的 request_threaded_irq()
 *   - 每个 IRQ 号对应一个 irq_desc 结构体
 *   - irq_desc.handlers[] 是一个链表/数组，存储所有处理函数
 *   - handle_irq() 遍历所有处理函数，直到某个返回 IRQ_HANDLED
 *
 * 【与 Notifier Chain 的区别】
 *   - Notifier Chain：所有观察者都会被调用（串行）
 *   - IRQ Registry ：遍历直到某个返回 IRQ_HANDLED（短路）
 *   - 本实现采用遍历所有（类似 NMI 或软中断的共享 IRQ）
 * ============================================================================*/

#define IRQ_NONE    0  // 无设备声称该中断
#define IRQ_HANDLED 1  // 有设备声称该中断，已处理

// -----------------------------------------------------------------------------
// 中断处理函数类型
// 【参数】
//   irq   : 中断号
//   dev_id: 设备标识（注册时传入，用于识别是哪个设备）
// 【返回值】IRQ_NONE 或 IRQ_HANDLED
// -----------------------------------------------------------------------------
typedef int (*IRQHandler)(int irq, void* dev_id);

// -----------------------------------------------------------------------------
// IRQ 描述符：管理一条 IRQ 线上的所有处理函数
// 【字段】
//   irq           : 中断号
//   handlers      : 处理函数指针数组
//   dev_ids       : 每个处理函数对应的设备标识
//   handler_count : 当前已注册数量
//   capacity      : 数组容量（动态扩容）
// -----------------------------------------------------------------------------
typedef struct _IRQDesc {
    int irq;
    IRQHandler* handlers;   // 处理函数数组
    void** dev_ids;        // 设备标识数组（与 handlers 一一对应）
    int handler_count;      // 当前已注册数量
    int capacity;           // 数组容量（扩容阈值）
} IRQDesc;

// -----------------------------------------------------------------------------
// IRQ 控制器（管理所有 IRQ 描述符）
// 【字段】
//   irqs[16] : 最多支持 16 条 IRQ 线（简单模拟）
//   request_irq : 注册中断处理函数
//   free_irq    : 注销中断处理函数
//   handle_irq  : 触发中断处理（中断上半部调用）
// -----------------------------------------------------------------------------
typedef struct _IRQController {
    IRQDesc* irqs[16];  // 固定大小数组，每条 IRQ 线一个描述符
    void (*request_irq)(struct _IRQController*, int irq, IRQHandler handler, void* dev_id);
    void (*free_irq)(struct _IRQController*, int irq, IRQHandler handler);
    int (*handle_irq)(struct _IRQController*, int irq);
} IRQController;

// -----------------------------------------------------------------------------
// 注册中断处理函数
// 【参数】
//   ic      : IRQ 控制器
//   irq     : 中断号（0-15）
//   handler : 处理函数
//   dev_id  : 设备标识（传递给 handler，用于识别设备）
// 【行为】
//   - 首次注册该 IRQ 号时：分配 IRQDesc 并初始化
//   - 之后：追加到 handlers[] 数组末尾
//   - 数组满时扩容（2倍）
// -----------------------------------------------------------------------------
static void IRQController_request_irq(IRQController* ic, int irq,
                                       IRQHandler handler, void* dev_id) {
    if (irq < 0 || irq >= 16) return;
    // 首次注册该 IRQ：分配 IRQDesc
    if (ic->irqs[irq] == NULL) {
        ic->irqs[irq] = calloc(1, sizeof(IRQDesc));
        ic->irqs[irq]->irq = irq;
        ic->irqs[irq]->capacity = 4;      // 初始容量 4
        ic->irqs[irq]->handlers = malloc(sizeof(IRQHandler) * 4);
        ic->irqs[irq]->dev_ids = malloc(sizeof(void*) * 4);
    }
    IRQDesc* desc = ic->irqs[irq];
    // 容量不够时扩容
    if (desc->handler_count >= desc->capacity) {
        desc->capacity *= 2;
        desc->handlers = realloc(desc->handlers, sizeof(IRQHandler) * desc->capacity);
        desc->dev_ids = realloc(desc->dev_ids, sizeof(void*) * desc->capacity);
    }
    // 追加到数组末尾
    desc->handlers[desc->handler_count] = handler;
    desc->dev_ids[desc->handler_count++] = dev_id;
    printf("[IRQController] IRQ%d: registered handler (total: %d)\n", irq, desc->handler_count);
}

// -----------------------------------------------------------------------------
// 注销中断处理函数
// 【参数】ic: IRQ 控制器，irq: 中断号，handler: 处理函数指针
// 【行为】在 handlers[] 中查找并删除该处理函数
// 【注意】只删除数组元素，不释放 IRQDesc（除非数组为空）
// -----------------------------------------------------------------------------
static void IRQController_free_irq(IRQController* ic, int irq, IRQHandler handler) {
    if (irq < 0 || irq >= 16 || ic->irqs[irq] == NULL) return;
    IRQDesc* desc = ic->irqs[irq];
    for (int i = 0; i < desc->handler_count; i++) {
        if (desc->handlers[i] == handler) {
            // 找到目标，后面的元素前移
            for (int j = i; j < desc->handler_count - 1; j++) {
                desc->handlers[j] = desc->handlers[j + 1];
                desc->dev_ids[j] = desc->dev_ids[j + 1];
            }
            desc->handler_count--;
            printf("[IRQController] IRQ%d: freed handler\n", irq);
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// 处理中断
// 【参数】ic: IRQ 控制器，irq: 中断号
// 【返回值】IRQ_HANDLED 或 IRQ_NONE
// 【调用时机】硬件中断触发时（中断上半部）
// 【行为】
//   遍历该 IRQ 线上的所有处理函数，依次调用
//   任何一个返回 IRQ_HANDLED 则认为已处理
// -----------------------------------------------------------------------------
static int IRQController_handle_irq(IRQController* ic, int irq) {
    if (irq < 0 || irq >= 16 || ic->irqs[irq] == NULL) return IRQ_NONE;
    IRQDesc* desc = ic->irqs[irq];
    printf("[IRQController] Handling IRQ%d (%d handlers)\n", irq, desc->handler_count);
    int ret = IRQ_NONE;
    for (int i = 0; i < desc->handler_count; i++) {
        int r = desc->handlers[i](irq, desc->dev_ids[i]);
        if (r == IRQ_HANDLED) ret = IRQ_HANDLED;
    }
    return ret;
}

// -----------------------------------------------------------------------------
// IRQController 构造函数
// -----------------------------------------------------------------------------
static IRQController* IRQController_new(void) {
    IRQController* ic = calloc(1, sizeof(IRQController));
    ic->request_irq = IRQController_request_irq;
    ic->free_irq = IRQController_free_irq;
    ic->handle_irq = IRQController_handle_irq;
    return ic;
}

// -----------------------------------------------------------------------------
// IRQController 析构函数
// 【行为】
//   遍历所有 IRQ 描述符，释放每个描述符的 handlers/dev_ids 数组
//   最后释放 IRQController 自身
// -----------------------------------------------------------------------------
static void IRQController_delete(IRQController* ic) {
    for (int i = 0; i < 16; i++) {
        if (ic->irqs[i]) {
            free(ic->irqs[i]->handlers);
            free(ic->irqs[i]->dev_ids);
            free(ic->irqs[i]);
        }
    }
    free(ic);
}

// -----------------------------------------------------------------------------
// PCIe 设备结构体（模拟）
// 【字段】
//   name : 设备名称/型号
//   bar0 : PCIe BAR0 地址（用于 MMIO 访问）
// -----------------------------------------------------------------------------
typedef struct _PCIEDevice {
    char name[32];
    unsigned int bar0;
} PCIEDevice;

// -----------------------------------------------------------------------------
// PCIe 设备的中断处理函数
// 【参数】
//   irq   : 中断号
//   dev_id: PCIEDevice 指针（由 request_irq 传入）
// 【返回值】IRQ_HANDLED（声称该中断并处理）
// 【实际行为】
//   - 读取 PCIe 状态寄存器（判断中断来源）
//   - 清除中断标志位（写 1 清 0）
//   - 处理业务数据
// -----------------------------------------------------------------------------
static int pcie_irq_handler(int irq, void* dev_id) {
    PCIEDevice* dev = (PCIEDevice*)dev_id;
    printf("  [PCIe:%s] IRQ%d handler executed (BAR0=0x%x)\n",
           dev->name, irq, dev->bar0);
    return IRQ_HANDLED;
}


/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                   Observer Pattern — 观察者模式\n");
    printf("=================================================================\n\n");

    /*------------------------------
     * 示例1：Weather Station
     * 演示经典观察者模式：
     *   1. 创建 Subject 和两个 Observer
     *   2. Observer 注册到 Subject
     *   3. Subject 推送数据更新
     *   4. Observer 动态注销
     *   5. Subject 再次推送数据（剩余 Observer 收到）
     *------------------------------*/
    printf("【示例1】Weather Station — 气象站\n");
    printf("-----------------------------------------------------------------\n");
    WeatherData* wd = WeatherData_new();
    CurrentConditionDisplay* ccd = CurrentConditionDisplay_new();
    StatisticsDisplay* sd = StatisticsDisplay_new();

    // 两个 Observer 注册到 Subject
    wd->registerObserver(wd, &ccd->base);
    wd->registerObserver(wd, &sd->base);

    // Subject 推送数据：Observer 收到更新
    wd->setMeasurements(wd, 25.5f, 60.0f, 1013.25f);
    wd->setMeasurements(wd, 26.0f, 58.0f, 1012.50f);

    // CurrentCondition 退订（动态删除）
    wd->removeObserver(wd, &ccd->base);

    // 再次推送：只有 Statistics 收到
    wd->setMeasurements(wd, 27.0f, 55.0f, 1015.00f);

    // 释放内存
    free(ccd); free(sd); WeatherData_delete(wd);

    /*------------------------------
     * 示例2：Linux Notifier Chain
     * 演示内核式通知链：
     *   1. 创建通知链
     *   2. 注册多个子系统观察者
     *   3. 触发网络事件，观察者收到通知
     *------------------------------*/
    printf("\n【示例2】Linux Notifier Chain — 内核通知链\n");
    printf("-----------------------------------------------------------------\n");
    NotifierChain* nc = NotifierChain_new();
    char dev[] = "eth0";

    // 两个子系统注册到通知链
    nc->registerNotifier(nc, netmanager_notifier, "NetManager");
    nc->registerNotifier(nc, powermanager_notifier, "PowerManager");

    // 触发两次事件
    nc->callChain(nc, NETDEV_UP, dev);
    nc->callChain(nc, NETDEV_DOWN, dev);

    NotifierChain_delete(nc);

    /*------------------------------
     * 示例3：Interrupt Handler Registry
     * 演示共享中断线注册：
     *   1. 创建 IRQ 控制器
     *   2. 两个 PCIe 设备注册到同一 IRQ 17
     *   3. 模拟 IRQ 17 中断触发
     *------------------------------*/
    printf("\n【示例3】Interrupt Handler Registry — 中断处理函数注册表\n");
    printf("-----------------------------------------------------------------\n");
    IRQController* ic = IRQController_new();

    // 两个 PCIe 设备共享 IRQ 17
    PCIEDevice dev1 = {.name = "RTL8111", .bar0 = 0xFEBC0000};
    PCIEDevice dev2 = {.name = "X710", .bar0 = 0xFEDF0000};
    ic->request_irq(ic, 17, pcie_irq_handler, &dev1);
    ic->request_irq(ic, 17, pcie_irq_handler, &dev2);

    printf("\n[Simulation] IRQ17 interrupt triggered:\n");
    ic->handle_irq(ic, 17);

    IRQController_delete(ic);

    printf("\n=================================================================\n");
    printf(" Observer 模式核心：函数指针列表 + 动态注册/注销 + 遍历通知\n");
    printf(" Linux 内核应用：Notifier Chain、IRQ Handler、uevent\n");
    printf("=================================================================\n");
    return 0;
}
