/**
 * 策略模式 — OOC风格实现（参考QMonkey/OOC-Design-Pattern）
 * 
 * 本文件演示：
 * 1. 策略模式的OOP实现方式（使用联合体嵌入接口）
 * 2. lib/base.h 提供的 new/delete 宏
 * 3. 运行时切换策略
 * 
 * 目录结构（与OOC-Design-Pattern一致）：
 *   include/           — 头文件（接口声明）
 *   src/               — 源文件（实现）
 *   lib/include/base.h — 公共OOP框架
 * 
 * 编译方式：
 *   make -f Makefile_ooc
 */

#include <stdio.h>
#include "base.h"
#include "itravel_strategy.h"
#include "person.h"
#include "train_strategy.h"
#include "airplane_strategy.h"

int main(void)
{
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   策略模式 — OOC风格实现（参考OOC-Design-Pattern）        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    // ============================================================
    // 第一步：创建具体的策略对象
    // 
    // new(TrainStrategy) 做了什么？
    //   1. malloc(sizeof(TrainStrategy))  — 分配内存
    //   2. TrainStrategy_construct(addr) — 调用构造器
    // 这是 lib/base.h 提供的便利宏，模拟C++的 new
    // ============================================================
    TrainStrategy *trainStrategy = new(TrainStrategy);
    AirplaneStrategy *airplaneStrategy = new(AirplaneStrategy);

    // ============================================================
    // 第二步：获取策略接口指针
    // 
    // TrainStrategy 内部是一个联合体：
    //   union {
    //     ITravelStrategy;        // 匿名成员，等价于"继承"了接口
    //     ITravelStrategy itravelStrategy;  // 给它起个名字方便取地址
    //   };
    // 我们通过命名成员取到接口指针
    // ============================================================
    ITravelStrategy *itravelStrategy = &trainStrategy->itravelStrategy;

    // ============================================================
    // 第三步：创建上下文对象，注入策略
    // 
    // Person（上下文）持有 ITravelStrategy*，
    // 不知道也不关心具体是哪种出行方式
    // ============================================================
    Person *person = new(Person, itravelStrategy);

    // 使用当前的出行策略
    printf("使用当前策略：\n");
    person->travel(person);

    // ============================================================
    // 第四步：运行时切换策略
    // 
    // 关键：person本身的代码没变，
    // 只是把持有的策略指针换成了另一个实现
    // 这是策略模式的核心：策略可替换，上下文不变
    // ============================================================
    itravelStrategy = &airplaneStrategy->itravelStrategy;
    person->setTravelStrategy(person, itravelStrategy);

    printf("\n切换策略后：\n");
    person->travel(person);

    // ============================================================
    // 第五步：清理资源
    // 
    // delete(Person, person) 做了什么？
    //   1. Person_destruct(person) — 调用析构器
    //   2. free(person)           — 释放内存
    // ============================================================
    delete(Person, person);
    delete(AirplaneStrategy, airplaneStrategy);
    delete(TrainStrategy, trainStrategy);

    printf("\n=== 策略模式核心：把会变的行为封装成策略，上下文保持不变 ===\n");
    return 0;
}
