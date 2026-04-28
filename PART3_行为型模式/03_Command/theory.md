# Command Pattern — 命令模式

## 1. 模式定义

**命令模式（Command Pattern）**：将"请求"封装为对象，从而允许使用不同的请求参数化客户、队列化请求、记录日志、支持撤销（Undo）。

> 核心思想：**命令对象化**。把动作和其参数包装成对象，使得请求者和执行者解耦。

---

## 2. 《Head First 设计模式》核心内容

### 解决方案：命令接口

```cpp
class Command {
public:
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class LightOnCommand : public Command {
    Light* light;
public:
    LightOnCommand(Light* l) : light(l) {}
    void execute() override { light->on(); }
    void undo() override { light->off(); }
};
```

### 类结构

```
RemoteControl（调用者）
  - setCommand()
  - buttonWasPressed()

Command（接口）
  + execute()
  + undo()

LightOnCommand / LightOffCommand（具体命令）
```

---

## 3. Linux 内核实例

### 3.1 Linux 键盘输入子系统

```c
struct input_event {
    struct timeval time;
    __u16 type;   // EV_KEY, EV_REL, EV_ABS
    __u16 code;
    __s32 value;  // 0=release, 1=press, 2=repeat
};
```

按键事件通过 `input_report_key()` → `input_event()` 传递。

### 3.2 Undo/Redo 系统

```c
typedef struct _CommandHistory {
    Command* undo_stack[100];
    Command* redo_stack[100];
    int undo_top;
    void (*execute)(CommandHistory*, Command*);
    void (*undo)(CommandHistory*);
} CommandHistory;
```

---

## 4. C 语言实现思路

```c
typedef struct _ICommand {
    void (*execute)(struct _ICommand*);
    void (*undo)(struct _ICommand*);
} ICommand;

typedef struct _LightOnCommand {
    ICommand base;
    Light* light;
} LightOnCommand;
```

---

## 5. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 将请求封装为对象，支持参数化、队列、撤销 |
| **核心** | 命令对象化 = receiver + action + undo |
| **Linux 场景** | Input Subsystem、workqueue、事务回滚 |
