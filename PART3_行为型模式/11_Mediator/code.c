/**
 * ===========================================================================
 * 中介者模式 — Mediator Pattern
 * ===========================================================================
 *
 * 核心思想：用一个中介对象封装一组对象的交互，降低对象间耦合
 *
 * 本代码演示（嵌入式视角）：
 * 1. 智能家居协调：灯打开 → 空调自动调节（对象不直接通信）
 * 2. PCIe 总线仲裁：RC/EP/Switch 之间通过总线协调器通信
 *
 * 中介者模式 vs 观察者模式：
 *   观察者：Subject 主动通知所有 Observer（一对多）
 *   中介者：Colleague 之间不直接通信，通过 Mediator 协调（星形）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：前向声明（解决头文件中类型循环依赖）
 * ============================================================================*/

typedef struct _IMediator IMediator;   /**< 中介者接口（前向声明）*/
typedef struct _IColleague IColleague;  /**< 同事接口（前向声明）*/

/* ============================================================================
 * 第二部分：智能家居中介者
 * ============================================================================*/

/** 中介者接口：只定义一个 notify 方法 */
typedef struct _IMediator {
    /**
     * 通知方法：sender 发送 event，mediator 负责路由
     * @param sender  发送消息的同事
     * @param event  事件/消息内容
     */
    void (*notify)(IMediator*, void* sender, const char* event);
} IMediator;

/** 同事接口：每个设备都有发送和接收能力 */
typedef struct _IColleague {
    IMediator* mediator;   /**< 关联的中介者（通过中介者通信）*/
    void (*send)(struct _IColleague*, const char* event);    /**< 发送消息 */
    void (*receive)(struct _IColleague*, const char* event); /**< 接收消息 */
    const char* name;      /**< 设备名称（调试用）*/
} IColleague;

/** 具体中介者：管理灯和空调 */
typedef struct _SmartHomeMediator {
    IMediator base;       /**< 继承中介者接口 */
    IColleague* light;   /**< 关联的灯 */
    IColleague* ac;       /**< 关联的空调 */
} SmartHomeMediator;

/**
 * 中介者处理消息
 * 规则：灯开了 → 空调自动调到 26 度
 */
static void SmartHomeMediator_notify(IMediator* base, void* sender, const char* event) {
    SmartHomeMediator* m = (SmartHomeMediator*)base;
    if (sender == m->light && strcmp(event, "LIGHT_ON") == 0) {
        printf("  [Mediator] 灯开了 → 通知空调调节温度\n");
        m->ac->receive(m->ac, "SET_TEMP_26");
    }
}

static SmartHomeMediator* SmartHomeMediator_new(void) {
    SmartHomeMediator* m = calloc(1, sizeof(SmartHomeMediator));
    m->base.notify = SmartHomeMediator_notify;
    return m;
}

/** 灯对象 */
typedef struct _Light {
    IColleague base;    /**< 继承同事接口 */
    int brightness;    /**< 灯的亮度 */
} Light;

/** 灯发送消息（发给中介者，不直接找空调）*/
static void Light_send(IColleague* base, const char* event) {
    Light* l = (Light*)base;
    base->mediator->notify(base->mediator, l, event);
}

/** 灯接收消息 */
static void Light_receive(IColleague* base, const char* event) {
    Light* l = (Light*)base;
    if (strcmp(event, "ON") == 0) {
        l->brightness = 100;
        printf("  [Light] 灯亮了\n");
    } else if (strcmp(event, "OFF") == 0) {
        l->brightness = 0;
        printf("  [Light] 灯灭了\n");
    }
}

static Light* Light_new(IMediator* m) {
    Light* l = calloc(1, sizeof(Light));
    l->base.mediator = m;
    l->base.send = Light_send;
    l->base.receive = Light_receive;
    l->base.name = "Light";
    l->brightness = 0;
    return l;
}

/** 空调对象 */
typedef struct _AC {
    IColleague base;   /**< 继承同事接口 */
    int temperature;  /**< 当前温度 */
} AC;

/** 空调接收消息（不主动发，只响应）*/
static void AC_receive(IColleague* base, const char* event) {
    AC* ac = (AC*)base;
    if (strcmp(event, "SET_TEMP_26") == 0) {
        ac->temperature = 26;
        printf("  [AC] 温度设置为 26°C\n");
    }
}

static AC* AC_new(IMediator* m) {
    AC* ac = calloc(1, sizeof(AC));
    ac->base.mediator = m;
    ac->base.receive = AC_receive;
    ac->base.name = "AC";
    ac->temperature = 24;
    return ac;
}

/* ============================================================================
 * 第三部分：PCIe 总线仲裁器（中介者）
 * ============================================================================*/

/** PCIe 设备类型 */
typedef enum { DEVICE_RC, DEVICE_EP } PCIeDeviceType;

/** PCIe 总线中介者 */
typedef struct _PCIeBusMediator {
    IMediator base;       /**< 继承中介者接口 */
    IColleague* devices[8];  /**< 总线上挂载的所有设备 */
    int device_count;
} PCIeBusMediator;

/** PCIe 总线通知实现：广播给其他设备 */
static void PCIeBusMediator_notify(IMediator* base, void* sender, const char* event) {
    PCIeBusMediator* m = (PCIeBusMediator*)base;
    printf("  [PCIeBus] %s 发送: %s\n", ((IColleague*)sender)->name, event);
    /**< 广播给其他所有设备（除了发送者）*/
    for (int i = 0; i < m->device_count; i++) {
        IColleague* dev = m->devices[i];
        if (dev != sender) {
            dev->receive(dev, event);
        }
    }
}

static PCIeBusMediator* PCIeBusMediator_new(void) {
    PCIeBusMediator* m = calloc(1, sizeof(PCIeBusMediator));
    m->base.notify = PCIeBusMediator_notify;
    return m;
}

/** PCIe 设备节点 */
typedef struct _PCIeDeviceNode {
    IColleague base;   /**< 继承同事接口 */
    PCIeDeviceType type;   /**< 设备类型：RC 或 EP */
} PCIeDeviceNode;

/** 设备发送消息（通过总线中介者）*/
static void PCIeDevice_send(IColleague* base, const char* msg) {
    PCIeDeviceNode* d = (PCIeDeviceNode*)base;
    base->mediator->notify(base->mediator, d, msg);  /**< 通过中介者广播 */
}

/** 设备接收消息（这里简化，直接打印）*/
static void PCIeDevice_receive(IColleague* base, const char* msg) {
    (void)base;
    printf("  [Device] 收到: %s\n", msg);
}

static PCIeDeviceNode* PCIeDeviceNode_new(PCIeDeviceType type, const char* name, IMediator* m) {
    PCIeDeviceNode* d = calloc(1, sizeof(PCIeDeviceNode));
    d->base.mediator = m;
    d->base.send = PCIeDevice_send;
    d->base.receive = PCIeDevice_receive;
    d->base.name = name;
    d->type = type;
    return d;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                  Mediator Pattern — 中介者模式\n");
    printf("=================================================================\n\n");

    /* -------------------- 示例1：智能家居 -------------------- */
    printf("【示例1】智能家居 — 灯与空调通过中介者协调\n");
    printf("-----------------------------------------------------------------\n");

    SmartHomeMediator* mediator = SmartHomeMediator_new();
    Light* light = Light_new(&mediator->base);
    AC* ac = AC_new(&mediator->base);

    /**< 中介者持有灯和空调的引用，用于协调 */
    mediator->light = &light->base;
    mediator->ac = &ac->base;

    printf("  操作：打开灯\n");
    light->base.send(&light->base, "LIGHT_ON");  /**< 灯发给中介者，不直接找空调 */
    printf("  结果：中介者协调，空调自动调节\n");

    free(light); free(ac); free(mediator);

    /* -------------------- 示例2：PCIe 总线 -------------------- */
    printf("\n【示例2】PCIe 总线仲裁 — RC 和 EP 通过总线协调\n");
    printf("-----------------------------------------------------------------\n");

    PCIeBusMediator* pcie_bus = PCIeBusMediator_new();
    PCIeDeviceNode* rc = PCIeDeviceNode_new(DEVICE_RC, "RC", &pcie_bus->base);
    PCIeDeviceNode* ep1 = PCIeDeviceNode_new(DEVICE_EP, "X710_EP", &pcie_bus->base);

    pcie_bus->devices[pcie_bus->device_count++] = &rc->base;
    pcie_bus->devices[pcie_bus->device_count++] = &ep1->base;

    printf("  X710_EP 发起内存请求:\n");
    ep1->base.send(&ep1->base, "MEMORY_REQUEST");

    free(rc); free(ep1); free(pcie_bus);

    printf("\n=================================================================\n");
    printf(" 中介者核心：星形拓扑，所有通信经过中介者\n");
    printf(" 嵌入式场景：PCIe总线仲裁、I2C总线、外设中断协调\n");
    printf("=================================================================\n");
    return 0;
}
