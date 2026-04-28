/**
 * ===========================================================================
 * Command Pattern — 命令模式
 * ===========================================================================
 *
 * 设计模式：行为型模式 · Command
 * 核心思想：把"请求/操作"封装成对象，实现请求者与执行者解耦
 *
 * 【示例1】Smart Home Remote — 智能家居遥控器
 * 【示例2】Linux Input Subsystem — 键盘输入子系统
 * 【示例3】Transaction Undo/Redo — 事务性操作的撤销/重做
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Smart Home Remote（智能家居遥控器）
 * ============================================================================*/

typedef struct _Light {
    char location[32];
    int brightness;
} Light;

static void Light_on(Light* light) {
    light->brightness = 100;
    printf("  [Light:%s] ON (brightness=%d)\n", light->location, light->brightness);
}

static void Light_off(Light* light) {
    light->brightness = 0;
    printf("  [Light:%s] OFF\n", light->location);
}

static void Light_setBrightness(Light* light, int level) {
    light->brightness = level > 0 ? (level > 100 ? 100 : level) : 0;
    printf("  [Light:%s] brightness=%d\n", light->location, light->brightness);
}

static Light* Light_new(const char* location) {
    Light* light = malloc(sizeof(Light));
    strncpy(light->location, location, 31);
    light->brightness = 0;
    return light;
}

typedef struct _CeilingFan {
    char location[32];
    int speed;
} CeilingFan;

static void CeilingFan_high(CeilingFan* fan) { fan->speed = 3; printf("  [CeilingFan:%s] HIGH\n", fan->location); }
static void CeilingFan_medium(CeilingFan* fan) { fan->speed = 2; printf("  [CeilingFan:%s] MEDIUM\n", fan->location); }
static void CeilingFan_low(CeilingFan* fan) { fan->speed = 1; printf("  [CeilingFan:%s] LOW\n", fan->location); }
static void CeilingFan_off(CeilingFan* fan) { fan->speed = 0; printf("  [CeilingFan:%s] OFF\n", fan->location); }

static CeilingFan* CeilingFan_new(const char* location) {
    CeilingFan* fan = malloc(sizeof(CeilingFan));
    strncpy(fan->location, location, 31);
    fan->speed = 0;
    return fan;
}

typedef struct _ICommand {
    void (*execute)(struct _ICommand*);
    void (*undo)(struct _ICommand*);
} ICommand;

typedef struct _LightOnCommand {
    ICommand base;
    Light* light;
    int prev_brightness;
} LightOnCommand;

static void LightOnCommand_execute(ICommand* cmd) {
    LightOnCommand* c = (LightOnCommand*)cmd;
    c->prev_brightness = c->light->brightness;
    Light_on(c->light);
}

static void LightOnCommand_undo(ICommand* cmd) {
    LightOnCommand* c = (LightOnCommand*)cmd;
    Light_setBrightness(c->light, c->prev_brightness);
}

static LightOnCommand* LightOnCommand_new(Light* light) {
    LightOnCommand* c = malloc(sizeof(LightOnCommand));
    c->base.execute = LightOnCommand_execute;
    c->base.undo = LightOnCommand_undo;
    c->light = light;
    c->prev_brightness = 0;
    return c;
}

typedef struct _LightOffCommand {
    ICommand base;
    Light* light;
    int prev_brightness;
} LightOffCommand;

static void LightOffCommand_execute(ICommand* cmd) {
    LightOffCommand* c = (LightOffCommand*)cmd;
    c->prev_brightness = c->light->brightness;
    Light_off(c->light);
}

static void LightOffCommand_undo(ICommand* cmd) {
    LightOffCommand* c = (LightOffCommand*)cmd;
    Light_setBrightness(c->light, c->prev_brightness);
}

static LightOffCommand* LightOffCommand_new(Light* light) {
    LightOffCommand* c = malloc(sizeof(LightOffCommand));
    c->base.execute = LightOffCommand_execute;
    c->base.undo = LightOffCommand_undo;
    c->light = light;
    c->prev_brightness = 0;
    return c;
}

typedef struct _CeilingFanCommand {
    ICommand base;
    CeilingFan* fan;
    int prev_speed;
    int new_speed;
} CeilingFanCommand;

static void CeilingFanCommand_execute(ICommand* cmd) {
    CeilingFanCommand* c = (CeilingFanCommand*)cmd;
    c->prev_speed = c->fan->speed;
    if (c->new_speed == 0) CeilingFan_off(c->fan);
    else if (c->new_speed == 1) CeilingFan_low(c->fan);
    else if (c->new_speed == 2) CeilingFan_medium(c->fan);
    else CeilingFan_high(c->fan);
}

static void CeilingFanCommand_undo(ICommand* cmd) {
    CeilingFanCommand* c = (CeilingFanCommand*)cmd;
    int prev = c->prev_speed;
    c->prev_speed = c->fan->speed;
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
    c->new_speed = speed;
    return c;
}

#define REMOTE_SLOTS 7

typedef struct _RemoteControl {
    ICommand* slots[REMOTE_SLOTS];
    ICommand* undo_cmd;
    void (*setCommand)(struct _RemoteControl*, int slot, ICommand* cmd);
    void (*buttonPressed)(struct _RemoteControl*, int slot);
    void (*undoButton)(struct _RemoteControl*);
} RemoteControl;

static void RemoteControl_setCommand(RemoteControl* rc, int slot, ICommand* cmd) {
    if (slot < 0 || slot >= REMOTE_SLOTS) return;
    rc->slots[slot] = cmd;
    printf("[Remote] Slot %d configured\n", slot);
}

static void RemoteControl_buttonPressed(RemoteControl* rc, int slot) {
    if (slot < 0 || slot >= REMOTE_SLOTS || rc->slots[slot] == NULL) return;
    rc->slots[slot]->execute(rc->slots[slot]);
    rc->undo_cmd = rc->slots[slot];
}

static void RemoteControl_undoButton(RemoteControl* rc) {
    if (rc->undo_cmd) {
        printf("[Remote] UNDO pressed\n");
        rc->undo_cmd->undo(rc->undo_cmd);
        rc->undo_cmd = NULL;
    }
}

static RemoteControl* RemoteControl_new(void) {
    RemoteControl* rc = calloc(1, sizeof(RemoteControl));
    rc->setCommand = RemoteControl_setCommand;
    rc->buttonPressed = RemoteControl_buttonPressed;
    rc->undoButton = RemoteControl_undoButton;
    return rc;
}

/* ============================================================================
 * 第二部分：Linux Input Subsystem（键盘输入子系统）
 * ============================================================================*/

#define EV_KEY 0x01
#define KEY_ENTER 28

typedef struct _InputEvent {
    unsigned int type;
    unsigned int code;
    int value;
} InputEvent;

typedef struct _InputDevice {
    char name[32];
    void (*report)(struct _InputDevice*, InputEvent*);
} InputDevice;

static void InputDevice_report(InputDevice* dev, InputEvent* ev) {
    const char* type_name = (ev->type == EV_KEY) ? "EV_KEY" : "UNKNOWN";
    const char* action = (ev->value == 0) ? "RELEASE" : (ev->value == 1 ? "PRESS" : "REPEAT");
    printf("  [Input:%s] type=%s, code=%d, value=%s\n", dev->name, type_name, ev->code, action);
}

static InputDevice* InputDevice_new(const char* name) {
    InputDevice* dev = malloc(sizeof(InputDevice));
    strncpy(dev->name, name, 31);
    dev->report = InputDevice_report;
    return dev;
}

/* ============================================================================
 * 第三部分：Transaction Undo/Redo（事务性操作的撤销/重做）
 * ============================================================================*/

typedef struct _ConfigRegistry {
    uint32_t pcie_bar0;
    uint32_t interrupt_mask;
    uint32_t power_state;
} ConfigRegistry;

typedef struct _ConfigCommand {
    ICommand base;
    ConfigRegistry* reg;
} ConfigCommand;

typedef struct _WriteConfigCommand {
    ConfigCommand base;
    int offset;
    uint32_t new_value;
    uint32_t old_value;
} WriteConfigCommand;

static void WriteConfigCommand_execute(ICommand* cmd) {
    WriteConfigCommand* c = (WriteConfigCommand*)cmd;
    uint32_t* p = (uint32_t*)((char*)c->base.reg + c->offset);
    c->old_value = *p;
    printf("  [Config] Write 0x%04X: 0x%08X -> 0x%08X\n", c->offset, c->old_value, c->new_value);
    *p = c->new_value;
}

static void WriteConfigCommand_undo(ICommand* cmd) {
    WriteConfigCommand* c = (WriteConfigCommand*)cmd;
    uint32_t* p = (uint32_t*)((char*)c->base.reg + c->offset);
    printf("  [Config] UNDO: 0x%04X -> 0x%08X\n", *p, c->old_value);
    *p = c->old_value;
}

static WriteConfigCommand* WriteConfigCommand_new(ConfigRegistry* reg, int offset, uint32_t value) {
    WriteConfigCommand* c = malloc(sizeof(WriteConfigCommand));
    c->base.base.execute = WriteConfigCommand_execute;
    c->base.base.undo = WriteConfigCommand_undo;
    c->base.reg = reg;
    c->offset = offset;
    c->new_value = value;
    c->old_value = 0;
    return c;
}

typedef struct _CommandHistory {
    ICommand* undo_stack[64];
    int undo_top;
    void (*execute)(struct _CommandHistory*, ICommand*);
    void (*undo)(struct _CommandHistory*);
} CommandHistory;

static void CommandHistory_execute(CommandHistory* h, ICommand* cmd) {
    cmd->execute(cmd);
    h->undo_stack[h->undo_top++] = cmd;
}

static void CommandHistory_undo(CommandHistory* h) {
    if (h->undo_top == 0) { printf("  [History] Nothing to undo\n"); return; }
    ICommand* cmd = h->undo_stack[--h->undo_top];
    cmd->undo(cmd);
}

static CommandHistory* CommandHistory_new(void) {
    CommandHistory* h = calloc(1, sizeof(CommandHistory));
    h->execute = CommandHistory_execute;
    h->undo = CommandHistory_undo;
    return h;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                     Command Pattern — 命令模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Smart Home Remote */
    printf("【示例1】Smart Home Remote — 智能家居遥控器\n");
    printf("-----------------------------------------------------------------\n");
    Light* living_light = Light_new("Living Room");
    CeilingFan* living_fan = CeilingFan_new("Living Room");
    RemoteControl* remote = RemoteControl_new();

    ICommand* light_on = &LightOnCommand_new(living_light)->base;
    ICommand* light_off = &LightOffCommand_new(living_light)->base;
    ICommand* fan_high = &CeilingFanCommand_new(living_fan, 3)->base;
    ICommand* fan_off = &CeilingFanCommand_new(living_fan, 0)->base;

    remote->setCommand(remote, 0, light_on);
    remote->setCommand(remote, 1, light_off);
    remote->setCommand(remote, 2, fan_high);
    remote->setCommand(remote, 3, fan_off);

    printf("\n  Press slot 0 (light on):\n");
    remote->buttonPressed(remote, 0);
    remote->undoButton(remote);

    printf("\n  Press slot 2 (fan high):\n");
    remote->buttonPressed(remote, 2);
    remote->undoButton(remote);

    free(living_light); free(living_fan); free(remote);

    /* 示例2：Linux Input Subsystem */
    printf("\n【示例2】Linux Input Subsystem — 键盘输入子系统\n");
    printf("-----------------------------------------------------------------\n");
    InputDevice* keyboard = InputDevice_new("AT Keyboard");
    InputEvent ev_enter_press = {EV_KEY, KEY_ENTER, 1};
    InputEvent ev_enter_release = {EV_KEY, KEY_ENTER, 0};
    printf("  Keyboard interrupt fires:\n");
    keyboard->report(keyboard, &ev_enter_press);
    keyboard->report(keyboard, &ev_enter_release);
    free(keyboard);

    /* 示例3：Transaction Undo/Redo */
    printf("\n【示例3】Transaction Undo/Redo — 事务性操作的撤销/重做\n");
    printf("-----------------------------------------------------------------\n");
    ConfigRegistry reg = { .pcie_bar0 = 0xFEBC0000, .interrupt_mask = 0x00000001, .power_state = 0x00000003 };
    CommandHistory* history = CommandHistory_new();

    int off_bar0 = (int)&((ConfigRegistry*)0)->pcie_bar0;
    int off_mask = (int)&((ConfigRegistry*)0)->interrupt_mask;
    int off_power = (int)&((ConfigRegistry*)0)->power_state;

    printf("  Initial: BAR0=0x%08X, MASK=0x%08X\n", reg.pcie_bar0, reg.interrupt_mask);

    ICommand* cmd1 = &WriteConfigCommand_new(&reg, off_bar0, 0xFEBE0000)->base;
    ICommand* cmd2 = &WriteConfigCommand_new(&reg, off_mask, 0x00000003)->base;
    ICommand* cmd3 = &WriteConfigCommand_new(&reg, off_power, 0x00000000)->base;

    printf("\n  Executing 3 config changes:\n");
    history->execute(history, cmd1);
    history->execute(history, cmd2);
    history->execute(history, cmd3);
    printf("  After: BAR0=0x%08X, MASK=0x%08X\n", reg.pcie_bar0, reg.interrupt_mask);

    printf("\n  Undo 2 times:\n");
    history->undo(history);
    history->undo(history);
    printf("  After 2x undo: BAR0=0x%08X\n", reg.pcie_bar0);

    free(history);

    printf("\n=================================================================\n");
    printf(" Command 模式核心：命令封装 receiver+action+undo\n");
    printf(" Linux 内核应用：Input Subsystem、workqueue、事务回滚\n");
    printf("=================================================================\n");
    return 0;
}
