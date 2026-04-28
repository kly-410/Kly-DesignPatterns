# Memento Pattern — 备忘录模式

## 1. 模式定义

**备忘录模式（Memento Pattern）**：在不破坏封装性的前提下，捕获一个对象的内部状态，并在该对象之外保存这个状态。

> 核心思想：**状态快照 + 可回滚**。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class Game {
    int level, hp, mp;
public:
    Memento* save() { return new Memento(level, hp, mp); }
    void restore(Memento* m) { level = m->level; hp = m->hp; mp = m->mp; }
};

class Caretaker {
    vector<Memento*> history;
public:
    void save(Memento* m) { history.push_back(m); }
    Memento* load(int idx) { return history[idx]; }
};
```

---

## 3. Linux 内核实例

PCI 配置空间快照用于热复位恢复。

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 保存对象状态快照，支持回滚 |
| **核心** | Memento 保存快照，Originator 可恢复 |
| **Linux 场景** | PCI config snapshot、CR、Suspend/Resume |
