/**
 * PART0 — C语言面向对象写法：函数指针、接口模拟、结构体嵌入
 * 
 * 本代码演示：
 * 1. 函数指针实现策略模式（qsort风格的比较器）
 * 2. 结构体嵌入模拟继承（Animal → Dog/Bird）
 * 3. 方法表实现多态（类似Linux内核的vtable）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*====================================================================
 * 示例1：函数指针实现策略模式
 * 场景：整数数组排序，支持升序/降序策略，运行时切换
 *====================================================================*/

// 定义比较器类型：接收两个整数指针，返回比较结果
// 这是策略模式的核心：把"算法"当成指针传递
typedef int (*CompareFunc)(int a, int b);

// 升序策略
int compare_asc(int a, int b) {
    return a - b;
}

// 降序策略
int compare_desc(int a, int b) {
    return b - a;
}

// 绝对值升序策略
int compare_abs(int a, int b) {
    int aa = a >= 0 ? a : -a;
    int bb = b >= 0 ? b : -b;
    return aa - bb;
}

// 排序函数（上下文角色）：本身不包含排序逻辑，只负责比较和交换
// 具体排序策略由调用方通过函数指针传入
void sort_array(int *arr, int n, CompareFunc cmp) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (cmp(arr[i], arr[j]) > 0) {
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

void demo_strategy_with_function_pointer(void) {
    printf("=== 示例1：函数指针实现策略模式 ===\n");
    
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    int n = 8;

    // 使用升序策略
    sort_array(arr, n, compare_asc);
    printf("升序: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    // 使用降序策略：sort_array本身一行没改，只是换了传入的函数指针
    sort_array(arr, n, compare_desc);
    printf("降序: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    // 使用绝对值策略：负数按绝对值排序
    int arr2[] = {-3, 1, -4, 1, -5};
    sort_array(arr2, 5, compare_abs);
    printf("绝对值排序: ");
    for (int i = 0; i < 5; i++) printf("%d ", arr2[i]);
    printf("\n\n");
}


/*====================================================================
 * 示例2：结构体嵌入模拟"继承"
 * 场景：动物基类 + 狗/鸟子类
 *====================================================================*/

// ===== 抽象基类：动物 =====
typedef struct {
    const char *name;   // 所有动物共有的属性
    int age;
} AnimalBase;

// ===== 子类：狗 =====
typedef struct {
    AnimalBase animal;          // 第一个成员：匿名嵌入，等价于"继承"了AnimalBase的所有字段
    const char *breed;      // 狗特有的属性
} Dog;

// 狗的构造函数：子类实例的创建方式
Dog* dog_new(const char *name, int age, const char *breed) {
    Dog *d = malloc(sizeof(Dog));
    d->animal.name = name;
    d->animal.age = age;
    d->breed = breed;
    return d;
}

void dog_introduce(Dog *d) {
    printf("我叫%s，是一只%d岁的%s\n", 
           d->animal.name, d->animal.age, d->breed);
}

// ===== 子类：鸟 =====
typedef struct {
    AnimalBase animal;          // 同样嵌入AnimalBase
    double wingspan;       // 鸟特有的属性：翼展
} Bird;

Bird* bird_new(const char *name, int age, double wingspan) {
    Bird *b = malloc(sizeof(Bird));
    b->animal.name = name;
    b->animal.age = age;
    b->wingspan = wingspan;
    return b;
}

void bird_introduce(Bird *b) {
    printf("我叫%s，是一只%d岁的鸟，翼展%.1f米\n",
           b->animal.name, b->animal.age, b->wingspan);
}

void demo_struct_embedding(void) {
    printf("=== 示例2：结构体嵌入模拟继承 ===\n");
    
    Dog *d = dog_new("旺财", 3, "金毛");
    Bird *b = bird_new("小鹦", 2, 0.3);

    dog_introduce(d);
    bird_introduce(b);

    // 重要：C语言不检查类型安全，需要程序员自己保证类型正确
    // 下面这行是危险操作：把Dog*当AnimalBase*传入
    // 合法：内存布局前两个字段就是AnimalBase
    AnimalBase *a = (AnimalBase*)d;
    printf("%s 重%d岁\n", a->name, a->age);

    free(d);
    free(b);
    printf("\n");
}


/*====================================================================
 * 示例3：方法表（Virtual Table）实现多态
 * 这是Linux内核所有驱动的核心机制
 *====================================================================*/

// ===== 抽象接口：动物 =====
// 方法表结构体：定义一组函数指针，代表"动物"能做的事情
// 注意：方法签名里第一个参数是void*，指向具体实例
typedef struct {
    void (*speak)(void *);      // 叫
    void (*move)(void *);       // 移动
    const char* (*get_name)(void *);  // 获取名字
} AnimalVTable;

// 动物基类结构体：包含名字 + 方法表指针
// 关键：通过vtable指针实现"多态"
typedef struct {
    const char *name;
    const AnimalVTable *vtable;  // 指向方法表，类似于C++的vptr
} Animal;

// 统一调用接口：通过方法表调用具体实现
// 这样main()里可以用同一行代码调用不同动物的行为
void animal_speak(Animal *a) {
    a->vtable->speak(a);
}

void animal_move(Animal *a) {
    a->vtable->move(a);
}

const char* animal_get_name(Animal *a) {
    return a->vtable->get_name(a);
}

// ===== 具体类：狗 =====
// 狗的实现：叫
static void dog_speak(void *self) {
    Animal *a = (Animal*)self;
    printf("%s说：汪汪汪！\n", a->name);
}
// 狗的实现：移动
static void dog_move(void *self) {
    Animal *a = (Animal*)self;
    printf("%s在地上跑\n", a->name);
}
// 狗的实现：获取名字
static const char* dog_get_name(void *self) {
    return ((Animal*)self)->name;
}

// 狗的方法表（静态分配，在代码段里）
// 所有狗实例共用同一个方法表
static const AnimalVTable DogVTable = {
    .speak = dog_speak,
    .move = dog_move,
    .get_name = dog_get_name,
};

// 狗的构造函数
Animal* animal_new_dog(const char *name) {
    Animal *a = malloc(sizeof(Animal));
    a->name = name;
    a->vtable = &DogVTable;    // 指向狗的方法表
    return a;
}

// ===== 具体类：鸟 =====
// 鸟的实现：叫
static void bird_speak(void *self) {
    Animal *a = (Animal*)self;
    printf("%s说：唧唧喳喳！\n", a->name);
}
// 鸟的实现：移动
static void bird_move(void *self) {
    Animal *a = (Animal*)self;
    printf("%s在天上飞\n", a->name);
}
// 鸟的实现：获取名字
static const char* bird_get_name(void *self) {
    return ((Animal*)self)->name;
}

static const AnimalVTable BirdVTable = {
    .speak = bird_speak,
    .move = bird_move,
    .get_name = bird_get_name,
};

Animal* animal_new_bird(const char *name) {
    Animal *a = malloc(sizeof(Animal));
    a->name = name;
    a->vtable = &BirdVTable;
    return a;
}

// ===== 具体类：鱼 =====
static void fish_speak(void *self) {
    Animal *a = (Animal*)self;
    printf("%s说：咕噜咕噜（吐泡泡）\n", a->name);
}
static void fish_move(void *self) {
    Animal *a = (Animal*)self;
    printf("%s在水里游\n", a->name);
}
static const char* fish_get_name(void *self) {
    return ((Animal*)self)->name;
}

static const AnimalVTable FishVTable = {
    .speak = fish_speak,
    .move = fish_move,
    .get_name = fish_get_name,
};

Animal* animal_new_fish(const char *name) {
    Animal *a = malloc(sizeof(Animal));
    a->name = name;
    a->vtable = &FishVTable;
    return a;
}

void demo_virtual_table(void) {
    printf("=== 示例3：方法表实现多态（类似Linux内核驱动） ===\n");

    // 创建不同动物，用统一类型Animal*保存
    Animal *animals[3];
    animals[0] = animal_new_dog("大黄");
    animals[1] = animal_new_bird("小鹦");
    animals[2] = animal_new_fish("金鱼");

    // 统一调用：同一行代码，不同对象表现不同行为
    // 这就是"多态"：Animal*指向的具体对象不同，行为就不同
    printf("\n所有动物都会叫：\n");
    for (int i = 0; i < 3; i++) {
        printf("  -> ");
        animal_speak(animals[i]);
    }

    printf("\n所有动物都会移动：\n");
    for (int i = 0; i < 3; i++) {
        printf("  -> ");
        animal_move(animals[i]);
    }

    printf("\n获取动物名字：\n");
    for (int i = 0; i < 3; i++) {
        printf("  -> %s\n", animal_get_name(animals[i]));
    }

    printf("\n重要：这个模式就是Linux内核驱动模型的原理！\n");
    printf("pci_driver.probe = xxx  <--->  DogVTable.speak = dog_speak\n");
    printf("PCIe核心调用pci_driver->probe()，这就是多态。\n\n");

    for (int i = 0; i < 3; i++) {
        free(animals[i]);
    }
}


/*====================================================================
 * 主函数：运行所有演示
 *====================================================================*/

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   PART0 — C语言的面向对象写法：函数指针与接口模拟       ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    demo_strategy_with_function_pointer();
    demo_struct_embedding();
    demo_virtual_table();

    printf("=== 所有示例运行完毕 ===\n");
    return 0;
}
