# Iterator Pattern — 迭代器模式

## 1. 模式定义

**迭代器模式（Iterator Pattern）**：提供一种方法顺序访问一个聚合对象中的各个元素，而又不需暴露该对象的内部表示。

> 核心思想：**遍历逻辑抽象化**。

---

## 2. 《Head First 设计模式》核心内容

```cpp
class Iterator {
    boolean hasNext();
    Object next();
}

Iterator iterator = menu.createIterator();
while (iterator.hasNext()) {
    MenuItem item = (MenuItem) iterator.next();
}
```

> **单一职责原则（SRP）**：迭代器将遍历职责从聚合对象中分离出来。

---

## 3. Linux 内核实例

### list_head 遍历

```c
struct list_head {
    struct list_head *next, *prev;
};

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

遍历驱动列表：
```c
list_for_each(pos, &driver_list) {
    Driver* drv = container_of(pos, Driver, node);
}
```

---

## 4. C 语言实现

```c
typedef struct _IIterator {
    int (*hasNext)(struct _IIterator*);
    void* (*next)(struct _IIterator*);
} IIterator;

typedef struct _ArrayIterator {
    IIterator base;
    int* array;
    int size;
    int index;
} ArrayIterator;
```

---

## 5. 总结

| 要点 | 内容 |
|:---|:---|
| **意图** | 提供统一遍历接口，隐藏底层存储结构 |
| **核心** | hasNext() + next() 函数指针接口 |
| **Linux 场景** | list_head 遍历、hlist、radix-tree |
