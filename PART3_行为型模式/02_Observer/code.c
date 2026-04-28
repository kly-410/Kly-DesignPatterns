/**
 * ===========================================================================
 * Observer Pattern — 观察者模式
 * ===========================================================================
 *
 * 设计模式：行为型模式 · Observer
 * 核心思想：发布-订阅（Publish-Subscribe）机制，一对多依赖通知
 *
 * 本文件展示三种 Observer 模式的 C 语言实现：
 *
 * 【示例1】Weather Station — 气象站（经典观察者模式）
 * 【示例2】Linux Notifier Chain — 内核通知链
 * 【示例3】Interrupt Handler Registry — 中断处理函数注册表
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 第一部分：Weather Station（经典观察者模式）
 * ============================================================================*/

typedef struct _WeatherData WeatherData;

typedef void (*UpdateCallback)(void* self, float temp, float humidity, float pressure);

typedef struct _IObserver {
    UpdateCallback update;
    void* instance;
    char name[32];
} IObserver;

typedef struct _WeatherData {
    IObserver** observers;
    int observer_count;
    int observer_capacity;
    float temperature;
    float humidity;
    float pressure;
    void (*registerObserver)(struct _WeatherData*, IObserver* observer);
    void (*removeObserver)(struct _WeatherData*, IObserver* observer);
    void (*notifyObservers)(struct _WeatherData*);
    void (*setMeasurements)(struct _WeatherData*, float temp, float hum, float pres);
} WeatherData;

static void WeatherData_registerObserver(WeatherData* wd, IObserver* observer) {
    if (wd->observer_count >= wd->observer_capacity) {
        wd->observer_capacity = wd->observer_capacity == 0 ? 4 : wd->observer_capacity * 2;
        wd->observers = realloc(wd->observers, sizeof(IObserver*) * wd->observer_capacity);
    }
    wd->observers[wd->observer_count++] = observer;
    printf("[WeatherData] Observer '%s' registered (total: %d)\n", observer->name, wd->observer_count);
}

static void WeatherData_removeObserver(WeatherData* wd, IObserver* observer) {
    for (int i = 0; i < wd->observer_count; i++) {
        if (wd->observers[i] == observer) {
            for (int j = i; j < wd->observer_count - 1; j++)
                wd->observers[j] = wd->observers[j + 1];
            wd->observer_count--;
            printf("[WeatherData] Observer '%s' removed\n", observer->name);
            return;
        }
    }
}

static void WeatherData_notifyObservers(WeatherData* wd) {
    printf("[WeatherData] Notifying %d observers (temp=%.1f, hum=%.1f, pres=%.1f)\n",
           wd->observer_count, wd->temperature, wd->humidity, wd->pressure);
    for (int i = 0; i < wd->observer_count; i++)
        wd->observers[i]->update(wd->observers[i]->instance, wd->temperature, wd->humidity, wd->pressure);
}

static void WeatherData_setMeasurements(WeatherData* wd, float temp, float hum, float pres) {
    wd->temperature = temp;
    wd->humidity = hum;
    wd->pressure = pres;
    printf("\n[WeatherData] New measurements received\n");
    WeatherData_notifyObservers(wd);
}

static WeatherData* WeatherData_new(void) {
    WeatherData* wd = calloc(1, sizeof(WeatherData));
    wd->registerObserver = WeatherData_registerObserver;
    wd->removeObserver = WeatherData_removeObserver;
    wd->notifyObservers = WeatherData_notifyObservers;
    wd->setMeasurements = WeatherData_setMeasurements;
    return wd;
}

static void WeatherData_delete(WeatherData* wd) {
    free(wd->observers);
    free(wd);
}

typedef struct _CurrentConditionDisplay {
    IObserver base;
    float last_temp;
    float last_hum;
} CurrentConditionDisplay;

static void CurrentConditionDisplay_update(void* self, float temp, float humidity, float pressure) {
    (void)pressure;
    CurrentConditionDisplay* ccd = (CurrentConditionDisplay*)self;
    ccd->last_temp = temp;
    ccd->last_hum = humidity;
    printf("  [CurrentCondition] Display updated: %.1fC, %.1f%%\n", temp, humidity);
}

static CurrentConditionDisplay* CurrentConditionDisplay_new(void) {
    CurrentConditionDisplay* ccd = calloc(1, sizeof(CurrentConditionDisplay));
    ccd->base.update = CurrentConditionDisplay_update;
    ccd->base.instance = ccd;
    strcpy(ccd->base.name, "CurrentCondition");
    return ccd;
}

typedef struct _StatisticsDisplay {
    IObserver base;
    int count;
    float sum_temp;
    float max_temp;
    float min_temp;
} StatisticsDisplay;

static void StatisticsDisplay_update(void* self, float temp, float humidity, float pressure) {
    (void)pressure;
    (void)humidity;
    StatisticsDisplay* sd = (StatisticsDisplay*)self;
    sd->count++;
    sd->sum_temp += temp;
    if (sd->count == 1 || temp > sd->max_temp) sd->max_temp = temp;
    if (sd->count == 1 || temp < sd->min_temp) sd->min_temp = temp;
    printf("  [Statistics] Avg=%.1fC, Max=%.1fC, Min=%.1fC, Count=%d\n",
           sd->sum_temp / sd->count, sd->max_temp, sd->min_temp, sd->count);
}

static StatisticsDisplay* StatisticsDisplay_new(void) {
    StatisticsDisplay* sd = calloc(1, sizeof(StatisticsDisplay));
    sd->base.update = StatisticsDisplay_update;
    sd->base.instance = sd;
    strcpy(sd->base.name, "Statistics");
    return sd;
}

/* ============================================================================
 * 第二部分：Linux Notifier Chain（内核通知链）
 * ============================================================================*/

#define NETDEV_UP     0x01
#define NETDEV_DOWN   0x02
#define NETDEV_CHANGE 0x03

typedef int (*NotifierCallback)(unsigned long event, void* data);

typedef struct _NotifierBlock {
    NotifierCallback callback;
    char name[32];
    struct _NotifierBlock* next;
} NotifierBlock;

typedef struct _NotifierChain {
    NotifierBlock* head;
    void (*registerNotifier)(struct _NotifierChain*, NotifierCallback, const char* name);
    void (*unregisterNotifier)(struct _NotifierChain*, NotifierCallback);
    int (*callChain)(struct _NotifierChain*, unsigned long event, void* data);
} NotifierChain;

static void NotifierChain_registerNotifier(NotifierChain* nc, NotifierCallback cb, const char* name) {
    NotifierBlock* nb = malloc(sizeof(NotifierBlock));
    nb->callback = cb;
    nb->next = nc->head;
    strncpy(nb->name, name, 31);
    nc->head = nb;
    printf("[NotifierChain] Registered: %s\n", name);
}

static void NotifierChain_unregisterNotifier(NotifierChain* nc, NotifierCallback cb) {
    NotifierBlock** pp = &nc->head;
    while (*pp) {
        if ((*pp)->callback == cb) {
            NotifierBlock* to_free = *pp;
            printf("[NotifierChain] Unregistered: %s\n", to_free->name);
            *pp = (*pp)->next;
            free(to_free);
            return;
        }
        pp = &(*pp)->next;
    }
}

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

static NotifierChain* NotifierChain_new(void) {
    NotifierChain* nc = calloc(1, sizeof(NotifierChain));
    nc->registerNotifier = NotifierChain_registerNotifier;
    nc->unregisterNotifier = NotifierChain_unregisterNotifier;
    nc->callChain = NotifierChain_callChain;
    return nc;
}

static void NotifierChain_delete(NotifierChain* nc) {
    NotifierBlock* nb = nc->head;
    while (nb) { NotifierBlock* next = nb->next; free(nb); nb = next; }
    free(nc);
}

static int netmanager_notifier(unsigned long event, void* data) {
    const char* dev_name = (const char*)data;
    if (event == NETDEV_UP) printf("  [NetManager] Device %s is UP\n", dev_name);
    else if (event == NETDEV_DOWN) printf("  [NetManager] Device %s is DOWN\n", dev_name);
    else if (event == NETDEV_CHANGE) printf("  [NetManager] Device %s changed\n", dev_name);
    return 0;
}

static int powermanager_notifier(unsigned long event, void* data) {
    const char* dev_name = (const char*)data;
    if (event == NETDEV_DOWN) printf("  [PowerManager] Device %s down - can suspend\n", dev_name);
    return 0;
}

/* ============================================================================
 * 第三部分：Interrupt Handler Registry（中断处理函数注册表）
 * ============================================================================*/

#define IRQ_NONE    0
#define IRQ_HANDLED 1

typedef int (*IRQHandler)(int irq, void* dev_id);

typedef struct _IRQDesc {
    int irq;
    IRQHandler* handlers;
    void** dev_ids;
    int handler_count;
    int capacity;
} IRQDesc;

typedef struct _IRQController {
    IRQDesc* irqs[16];
    void (*request_irq)(struct _IRQController*, int irq, IRQHandler handler, void* dev_id);
    void (*free_irq)(struct _IRQController*, int irq, IRQHandler handler);
    int (*handle_irq)(struct _IRQController*, int irq);
} IRQController;

static void IRQController_request_irq(IRQController* ic, int irq, IRQHandler handler, void* dev_id) {
    if (irq < 0 || irq >= 16) return;
    if (ic->irqs[irq] == NULL) {
        ic->irqs[irq] = calloc(1, sizeof(IRQDesc));
        ic->irqs[irq]->irq = irq;
        ic->irqs[irq]->capacity = 4;
        ic->irqs[irq]->handlers = malloc(sizeof(IRQHandler) * 4);
        ic->irqs[irq]->dev_ids = malloc(sizeof(void*) * 4);
    }
    IRQDesc* desc = ic->irqs[irq];
    if (desc->handler_count >= desc->capacity) {
        desc->capacity *= 2;
        desc->handlers = realloc(desc->handlers, sizeof(IRQHandler) * desc->capacity);
        desc->dev_ids = realloc(desc->dev_ids, sizeof(void*) * desc->capacity);
    }
    desc->handlers[desc->handler_count] = handler;
    desc->dev_ids[desc->handler_count++] = dev_id;
    printf("[IRQController] IRQ%d: registered handler (total: %d)\n", irq, desc->handler_count);
}

static void IRQController_free_irq(IRQController* ic, int irq, IRQHandler handler) {
    if (irq < 0 || irq >= 16 || ic->irqs[irq] == NULL) return;
    IRQDesc* desc = ic->irqs[irq];
    for (int i = 0; i < desc->handler_count; i++) {
        if (desc->handlers[i] == handler) {
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

static IRQController* IRQController_new(void) {
    IRQController* ic = calloc(1, sizeof(IRQController));
    ic->request_irq = IRQController_request_irq;
    ic->free_irq = IRQController_free_irq;
    ic->handle_irq = IRQController_handle_irq;
    return ic;
}

static void IRQController_delete(IRQController* ic) {
    for (int i = 0; i < 16; i++) {
        if (ic->irqs[i]) { free(ic->irqs[i]->handlers); free(ic->irqs[i]->dev_ids); free(ic->irqs[i]); }
    }
    free(ic);
}

typedef struct _PCIEDevice {
    char name[32];
    unsigned int bar0;
} PCIEDevice;

static int pcie_irq_handler(int irq, void* dev_id) {
    PCIEDevice* dev = (PCIEDevice*)dev_id;
    printf("  [PCIe:%s] IRQ%d handler executed (BAR0=0x%x)\n", dev->name, irq, dev->bar0);
    return IRQ_HANDLED;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                   Observer Pattern — 观察者模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Weather Station */
    printf("【示例1】Weather Station — 气象站\n");
    printf("-----------------------------------------------------------------\n");
    WeatherData* wd = WeatherData_new();
    CurrentConditionDisplay* ccd = CurrentConditionDisplay_new();
    StatisticsDisplay* sd = StatisticsDisplay_new();

    wd->registerObserver(wd, &ccd->base);
    wd->registerObserver(wd, &sd->base);
    wd->setMeasurements(wd, 25.5f, 60.0f, 1013.25f);
    wd->setMeasurements(wd, 26.0f, 58.0f, 1012.50f);
    wd->removeObserver(wd, &ccd->base);
    wd->setMeasurements(wd, 27.0f, 55.0f, 1015.00f);

    free(ccd); free(sd); WeatherData_delete(wd);

    /* 示例2：Linux Notifier Chain */
    printf("\n【示例2】Linux Notifier Chain — 内核通知链\n");
    printf("-----------------------------------------------------------------\n");
    NotifierChain* nc = NotifierChain_new();
    char dev[] = "eth0";
    nc->registerNotifier(nc, netmanager_notifier, "NetManager");
    nc->registerNotifier(nc, powermanager_notifier, "PowerManager");
    nc->callChain(nc, NETDEV_UP, dev);
    nc->callChain(nc, NETDEV_DOWN, dev);
    NotifierChain_delete(nc);

    /* 示例3：Interrupt Handler Registry */
    printf("\n【示例3】Interrupt Handler Registry — 中断处理函数注册表\n");
    printf("-----------------------------------------------------------------\n");
    IRQController* ic = IRQController_new();
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
