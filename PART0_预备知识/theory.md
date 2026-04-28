# PART0 — C 语言的面向对象写法：函数指针、接口模拟、结构体嵌入

## 本章目标

学完本章后，你将掌握：
1. 理解 C 语言如何模拟"接口"和"类"
2. 掌握函数指针的核心用法
3. 掌握结构体嵌入（嵌入继承）的写法
4. 理解"上下文 + 策略"的基本结构

---

## 1. 从 Duck 例子理解为什么需要这些技巧

Head First 设计模式里用一个鸭子模拟器引入策略模式：

```java
interface FlyBehavior {
    void fly();
}

class Duck {
    FlyBehavior flyBehavior;  // 组合
    void performFly() {
        flyBehavior.fly();  // 委托
    }
}
```

**C 没有接口，C 怎么做到同样的效果？**——用函数指针模拟。

---

## 2. 函数指针：最核心的语法

函数指针的本质：**把函数当成变量传递**。

```c
#include <stdio.h>

// 定义一种"能比较两个整数"的函数类型
typedef int (*CompareFunc)(int a, int b);

// 具体策略实现：从小到大
int compare_asc(int a, int b) {
    return a - b;
}

// 具体策略实现：从大到小
int compare_desc(int a, int b) {
    return b - a;
}

// 上下文：使用策略的函数
void sort_array(int *arr, int n, CompareFunc cmp) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (cmp(arr[i], arr[j]) > 0) {  // 用函数指针调用策略
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

int main(void) {
    int arr[] = {3, 1, 4, 1, 5};

    sort_array(arr, 5, compare_asc);   // 用升序策略
    // arr = {1, 1, 3, 4, 5}

    sort_array(arr, 5, compare_desc);  // 换降序策略，sort_array 本身一行没改
    // arr = {5, 4, 3, 1, 1}
}
```

**关键点**：
- `CompareFunc` 是一个**类型**，定义了一种函数签名
- `sort_array` 不知道也不关心具体用的是哪种排序策略
- 加新策略：写一个新函数，指针传进去就行了，原函数不用改

---

## 3. 结构体嵌入：模拟"类"和"继承"

### 3.1 基本结构体

```c
typedef struct {
    int x;
    int y;
} Point;
```

### 3.2 结构体嵌入（模拟继承）

```c
// 基类：动物
typedef struct {
    const char *name;
    int age;
} Animal;

// 子类：狗（嵌入 Animal）
typedef struct {
    Animal animal;        // 第一个成员默认匿名嵌入，等价于"继承"
    const char *breed;   // 狗特有的字段
} Dog;

// 构造
Dog* dog_new(const char *name, int age, const char *breed) {
    Dog *d = malloc(sizeof(Dog));
    d->animal.name = name;
    d->animal.age = age;
    d->breed = breed;
    return d;
}
```

**为什么第一个匿名成员能"模拟继承"？**
- `Dog` 的内存布局：`{ name + age + breed }`，前两个字段就是 `Animal`
- 把 `Dog*` 传给期望 `Animal*` 的函数——**类型系统不看内存布局，靠程序员自觉**

### 3.3 有类型安全的嵌入写法

```c
// 如果想让编译器也帮忙检查，用有名字的嵌入
typedef struct {
    Animal animal;
    const char *breed;
} Dog;
```

---

## 4. 方法表：模拟"类的方法"

这是 Linux 内核和所有高质量 C 代码库的核心技巧：

```c
#include <stdio.h>
#include <stdlib.h>

// ===== 抽象接口：动物 =====
typedef struct AnimalVTable {
    void (*speak)(void *);      // 抽象方法：叫
    void (*move)(void *);       // 抽象方法：移动
} AnimalVTable;

typedef struct {
    const char *name;
    const AnimalVTable *vtable;  // 方法表指针
} Animal;

// 统一调用接口：通过方法表调用
void animal_speak(Animal *a) {
    a->vtable->speak(a);
}

void animal_move(Animal *a) {
    a->vtable->move(a);
}

// ===== 具体类：狗 =====
// 狗的具体实现：叫
static void dog_speak(void *self) {
    printf("Woof! Woof!\n");
}
// 狗的具体实现：移动
static void dog_move(void *self) {
    printf("Dog runs on 4 legs\n");
}

// 狗的方法表（静态分配，代码段里）
static const AnimalVTable DogVTable = {
    .speak = dog_speak,
    .move = dog_move,
};

// 狗的构造函数
Animal* dog_new(const char *name) {
    Animal *a = malloc(sizeof(Animal));
    a->name = name;
    a->vtable = &DogVTable;
    return a;
}

// ===== 具体类：鸟 =====
static void bird_speak(void *self) {
    printf("Chirp chirp!\n");
}
static void bird_move(void *self) {
    printf("Bird flies\n");
}

static const AnimalVTable BirdVTable = {
    .speak = bird_speak,
    .move = bird_move,
};

Animal* bird_new(const char *name) {
    Animal *a = malloc(sizeof(Animal));
    a->name = name;
    a->vtable = &BirdVTable;
    return a;
}

// ===== 测试：统一调用 =====
int main(void) {
    Animal *animals[] = { dog_new("Buddy"), bird_new("Tweety") };

    for (int i = 0; i < 2; i++) {
        printf("%s: ", animals[i]->name);
        animal_speak(animals[i]);   // 多态：同一行代码，不同对象不同行为
        animal_move(animals[i]);
    }
}
```

**编译运行**：
```bash
gcc -o test code.c && ./test
```
```
Buddy: Woof! Woof!
Dog runs on 4 legs
Tweety: Chirp chirp!
Bird flies
```

**这个模式就是 Linux 内核 `struct file_operations` 和 `struct pci_driver` 的原理。**

---

## 5. Linux 内核里的实际例子

### 5.1 `pci_driver` — 策略模式的内核实现

```c
struct pci_driver {
    const char *name;
    const struct pci_device_id *id_table;     // 支持的设备列表
    int (*probe)(struct pci_dev *,            // 插入时的策略
                 const struct pci_device_id *);
    void (*remove)(struct pci_dev *);          // 拔出的策略
    int (*suspend)(struct pci_dev *, pm_message_t);
    int (*resume)(struct pci_dev *);
    // ...
};
```

每个驱动实现自己的 probe/remove，PCIe 核心框架调用它们——**策略模式**。

### 5.2 `file_operations` — 设备抽象

```c
struct file_operations {
    loff_t (*llseek)(struct file *, loff_t, int);
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
    int (*open)(struct inode *, struct file *);
    int (*release)(struct inode *, struct file *);
    // ...
};
```

---

## 6. 本章小结

| 概念 | C 语言的实现 | 对应 OOP 概念 |
|:---|:---|:---|
| 接口 | `typedef` + 函数指针 | interface |
| 类 | `struct` + 方法表 | class |
| 继承 | 结构体第一个成员匿名嵌入 | inheritance |
| 多态 | 通过方法表指针调用 | polymorphism |
| 组合 | 结构体持有另一个结构体指针 | composition |

**C 的面向对象不是"语法支持"，是"程序员手动实现"**，但效果完全一样。

---

## 7. 练习

1. 用函数指针实现一个计算器，支持 + - * / 四种运算策略，运行时切换
2. 用结构体嵌入实现一个"猫"类，继承自"动物"，实现 speak 和 move 方法
3. 读 Linux 内核 `include/linux/pci.h` 的 `pci_driver` 结构体，找出它的方法表

---

## 代码

`code.c` — 本章所有代码的完整可编译版本
`Makefile` — 全注释编译脚本
