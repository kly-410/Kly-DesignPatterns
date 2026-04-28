# Visitor Pattern — 访问者模式

## 1. 模式定义

**访问者模式（Visitor Pattern）**：表示一个作用于某对象结构中的各元素的操作。访问者使你可以在不改变各元素的类的前提下定义作用于这些元素的新操作。

> 核心思想：**数据结构和操作分离**，双重分发（Double Dispatch）。

---

## 2. 《Head First 设计模式》核心内容

```cpp
// 双重分发：
element->accept(visitor) → visitor->visitConcreteElement(element)
```

---

## 3. Linux 内核实例

### 设备树（Device Tree）遍历

DTB 由节点组成，访问者模式遍历执行不同操作。

---

## 4. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 数据结构和操作分离，双重分发 |
| **核心** | Element.accept(Visitor) + Visitor.visitElement(Element) |
| **Linux 场景** | DTB 遍历、AST 遍历、sysfs 遍历 |
