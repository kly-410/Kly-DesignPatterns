/**
 * ===========================================================================
 * Mediator Pattern — 中介者模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Smart Home Mediator
 * ============================================================================*/

typedef struct _IMediator IMediator;

typedef struct _IColleague {
    IMediator* mediator;
    void (*send)(struct _IColleague*, const char* event);
    void (*receive)(struct _IColleague*, const char* event);
    const char* name;
} IColleague;

typedef struct _IMediator {
    void (*notify)(IMediator*, void* sender, const char* event);
} IMediator;

typedef struct _SmartHomeMediator {
    IMediator base;
    IColleague* light;
    IColleague* ac;
} SmartHomeMediator;

static void SmartHomeMediator_notify(IMediator* base, void* sender, const char* event) {
    SmartHomeMediator* m = (SmartHomeMediator*)base;
    if (sender == m->light && strcmp(event, "LIGHT_ON") == 0) {
        printf("  [Mediator] LIGHT_ON -> set AC to 26C\n");
        m->ac->receive(m->ac, "SET_TEMP_26");
    }
}

static SmartHomeMediator* SmartHomeMediator_new(void) {
    SmartHomeMediator* m = calloc(1, sizeof(SmartHomeMediator));
    m->base.notify = SmartHomeMediator_notify;
    return m;
}

typedef struct _Light {
    IColleague base;
    int brightness;
} Light;

static void Light_send(IColleague* base, const char* event) {
    Light* l = (Light*)base;
    base->mediator->notify(base->mediator, l, event);
}

static void Light_receive(IColleague* base, const char* event) {
    Light* l = (Light*)base;
    if (strcmp(event, "ON") == 0) { l->brightness = 100; printf("  [Light] ON\n"); }
    else if (strcmp(event, "OFF") == 0) { l->brightness = 0; printf("  [Light] OFF\n"); }
}

static Light* Light_new(IMediator* m) {
    Light* l = calloc(1, sizeof(Light));
    l->base.mediator = m; l->base.send = Light_send; l->base.receive = Light_receive;
    l->base.name = "Light"; l->brightness = 0;
    return l;
}

typedef struct _AC {
    IColleague base;
    int temperature;
} AC;

static void AC_receive(IColleague* base, const char* event) {
    AC* ac = (AC*)base;
    if (strcmp(event, "SET_TEMP_26") == 0) {
        ac->temperature = 26; printf("  [AC] Set to 26C\n");
    }
}

static AC* AC_new(IMediator* m) {
    AC* ac = calloc(1, sizeof(AC));
    ac->base.mediator = m; ac->base.receive = AC_receive;
    ac->base.name = "AC"; ac->temperature = 24;
    return ac;
}

/* ============================================================================
 * 第二部分：PCIe Bus Mediator
 * ============================================================================*/

typedef enum { DEVICE_RC, DEVICE_EP } PCIeDeviceType;

typedef struct _PCIeDeviceNode {
    PCIeDeviceType type;
    char name[32];
    struct _PCIeBusMediator* bus;
    void (*send)(struct _PCIeDeviceNode*, const char* msg);
    void (*onReceive)(struct _PCIeDeviceNode*, const char* msg);
} PCIeDeviceNode;

typedef struct _PCIeBusMediator {
    PCIeDeviceNode* devices[8];
    int device_count;
    void (*notifyDevice)(struct _PCIeBusMediator*, PCIeDeviceNode*, const char* msg);
} PCIeBusMediator;

static void PCIeBusMediator_notifyDevice(PCIeBusMediator* m, PCIeDeviceNode* sender, const char* msg) {
    printf("  [PCIeBus] %s -> %s\n", sender->name, msg);
    for (int i = 0; i < m->device_count; i++) {
        if (m->devices[i] != sender) {
            m->devices[i]->onReceive(m->devices[i], msg);
        }
    }
}

static PCIeBusMediator* PCIeBusMediator_new(void) {
    PCIeBusMediator* m = calloc(1, sizeof(PCIeBusMediator));
    m->notifyDevice = PCIeBusMediator_notifyDevice;
    return m;
}

static void PCIeDeviceNode_send(PCIeDeviceNode* dev, const char* msg) {
    printf("  [%s] Sending: %s\n", dev->name, msg);
    dev->bus->notifyDevice(dev->bus, dev, msg);
}

static PCIeDeviceNode* PCIeDeviceNode_new(PCIeDeviceType type, const char* name) {
    PCIeDeviceNode* d = calloc(1, sizeof(PCIeDeviceNode));
    d->type = type; strncpy(d->name, name, 31);
    d->send = PCIeDeviceNode_send;
    d->onReceive = (void(*)(PCIeDeviceNode*, const char*))printf;
    return d;
}

/* ============================================================================
 * 第三部分：Chat Room Mediator
 * ============================================================================*/

typedef struct _User {
    char name[32];
    void (*send)(struct _User*, const char* msg);
    void (*receive)(struct _User*, const char* from, const char* msg);
} User;

typedef struct _ChatRoom {
    User* users[16];
    int user_count;
    void (*broadcast)(struct _ChatRoom*, User* sender, const char* msg);
} ChatRoom;

static void ChatRoom_broadcast(ChatRoom* room, User* sender, const char* msg) {
    printf("  [ChatRoom] %s broadcasts: \"%s\"\n", sender->name, msg);
    for (int i = 0; i < room->user_count; i++) {
        if (room->users[i] != sender) {
            room->users[i]->receive(room->users[i], sender->name, msg);
        }
    }
}

static ChatRoom* ChatRoom_new(void) {
    ChatRoom* room = calloc(1, sizeof(ChatRoom));
    room->broadcast = ChatRoom_broadcast;
    return room;
}

static User* User_new(const char* name) {
    User* u = calloc(1, sizeof(User));
    strncpy(u->name, name, 31);
    u->send = (void(*)(User*, const char*))printf;
    u->receive = (void(*)(User*, const char*, const char*))printf;
    return u;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                   Mediator Pattern — 中介者模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Smart Home Mediator */
    printf("【示例1】Smart Home Mediator — 智能家居中介者\n");
    printf("-----------------------------------------------------------------\n");
    SmartHomeMediator* mediator = SmartHomeMediator_new();
    Light* light = Light_new(&mediator->base);
    AC* ac = AC_new(&mediator->base);
    mediator->light = &light->base;
    mediator->ac = &ac->base;
    printf("  Light turns on:\n");
    light->base.send(&light->base, "LIGHT_ON");
    free(light); free(ac); free(mediator);

    /* 示例2：PCIe Bus Mediator */
    printf("\n【示例2】PCIe Bus Mediator — PCIe 总线协调器\n");
    printf("-----------------------------------------------------------------\n");
    PCIeBusMediator* pcie_bus = PCIeBusMediator_new();
    PCIeDeviceNode* rc = PCIeDeviceNode_new(DEVICE_RC, "RC");
    PCIeDeviceNode* ep1 = PCIeDeviceNode_new(DEVICE_EP, "X710_EP");
    rc->bus = pcie_bus; ep1->bus = pcie_bus;
    pcie_bus->devices[pcie_bus->device_count++] = rc;
    pcie_bus->devices[pcie_bus->device_count++] = ep1;
    printf("  X710_EP requests memory:\n");
    ep1->send(ep1, "MEMORY_REQUEST");
    free(rc); free(ep1); free(pcie_bus);

    /* 示例3：Chat Room Mediator */
    printf("\n【示例3】Chat Room Mediator — 聊天室\n");
    printf("-----------------------------------------------------------------\n");
    ChatRoom* chat = ChatRoom_new();
    User* alice = User_new("Alice");
    User* bob = User_new("Bob");
    chat->users[chat->user_count++] = alice;
    chat->users[chat->user_count++] = bob;
    printf("  Alice broadcasts:\n");
    chat->broadcast(chat, alice, "Hello everyone!");
    free(alice); free(bob); free(chat);

    printf("\n=================================================================\n");
    printf(" Mediator 模式核心：对象间通信集中化，星形拓扑\n");
    printf(" Linux 内核应用：PCIe Core、I2C 总线、TCP 协议栈\n");
    printf("=================================================================\n");
    return 0;
}
