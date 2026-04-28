# State Pattern — 状态机模式

## 1. 模式定义

**状态机模式（State Pattern）**：允许对象在内部状态改变时改变其行为，看起来就像对象修改了它自己的类。

> 核心思想：**状态对象化**。每个状态封装成独立对象，状态转换由状态对象自己驱动。

---

## 2. 《Head First 设计模式》核心内容

### 状态机方案：每个状态一个类

```cpp
class State {
    void insertQuarter() {}
    void ejectQuarter() {}
    void turnCrank() {}
    void dispense() {}
};

class HasQuarterState : public State {
    GumballMachine* machine;
    void turnCrank() {
        machine->releaseBall();
        if (machine->count > 0)
            machine->setState(machine->soldState);
        else
            machine->setState(machine->soldOutState);
    }
};
```

---

## 3. Linux 内核实例

### TCP 连接状态机

```
CLOSED → SYN_SENT → ESTABLISHED → FIN_WAIT_1 → FIN_WAIT_2 → TIME_WAIT → CLOSED
```

### PCIe 链路训练状态机（LTSSM）

```
Detect → Polling → Configuration → Recovery → L0 → L1/L0S → Recovery → L0
```

---

## 4. C 语言实现

```c
typedef enum { STATE_IDLE, STATE_RUNNING } State;
typedef enum { EV_START, EV_STOP } Event;

static State state_table[NUM_STATES][NUM_EVENTS] = {
    [STATE_IDLE][EV_START] = STATE_RUNNING,
    [STATE_RUNNING][EV_STOP] = STATE_IDLE,
};
```

---

## 5. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 将状态行为封装为对象，通过状态转换驱动行为变化 |
| **核心** | 状态表驱动 |
| **Linux 场景** | TCP 状态机、PCIe LTSSM |
