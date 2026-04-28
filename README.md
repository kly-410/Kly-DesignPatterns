# Kly-DesignPatterns

> C 语言设计模式学习仓库 | 配套《Head First 设计模式》+ GoF 原典

## 仓库地址

https://github.com/kly-410/Kly-DesignPatterns

---

## 课程结构

| 部分 | 内容 | 章节数 | 状态 |
|:---|:---|:---|:---|
| [PART0 预备知识](./PART0_预备知识/) | C OOP 写法、函数指针、接口模拟 | 1 | ✅ |
| [PART1 创建型模式](./PART1_创建型模式/) | Singleton / Factory / Abstract Factory / Builder / Prototype | 5 | ✅ |
| [PART2 结构型模式](./PART2_结构型模式/) | Adapter / Bridge / Composite / Decorator / Facade / Flyweight / Proxy | 7 | ✅ |
| [PART3 行为型模式](./PART3_行为型模式/) | Strategy / Observer / Command / Iterator / State / Template / Chain / Visitor / Interpreter / Memento / Mediator | 11 | ✅ |

**23 个设计模式全部交付 ✅**

---

## 每章内容

每个模式目录下包含：
- `theory.md` — 讲义（问题分析 + C语言实现 + Linux内核实例 + 嵌入式实例 + 练习）
- `code.c` — 完整可编译运行的 C 代码
- `Makefile` — 全注释编译脚本

部分模式提供 OOC 风格实现（参考 `QMonkey/OOC-Design-Pattern`）。

---

## 快速开始

```bash
# 编译单个模式
cd PART1_创建型模式/01_Singleton && make run
cd PART3_行为型模式/01_Strategy && make run
cd PART3_行为型模式/02_Observer && make run

# 编译全部（如果支持）
make -C PART0_预备知识
make -C PART1_创建型模式
make -C PART2_结构型模式
make -C PART3_行为型模式
```

---

## 推荐学习路径

```
第一步：读 PART0（预备知识）— 函数指针、结构体嵌入、方法表
        ↓
第二步：PART1 创建型（最容易理解）— Singleton / Factory
        ↓
第三步：PART3 策略模式 — 策略模式是所有模式的基础
        ↓
第四步：PART2 结构型（考验组合能力）— Adapter / Decorator / Proxy
        ↓
第五步：PART3 其余行为型 — Observer / State / Command / Iterator
        ↓
第六步：结合 PCIe 驱动源码，理解 Linux 内核里的模式
```

---

## 环境要求

- GCC 9.0+
- Linux 环境
- Make
-  pthread（部分模式需要）

---

## 参考资料

- [QMonkey/OOC-Design-Pattern](https://github.com/QMonkey/OOC-Design-Pattern) — GoF 23 种模式 C 实现
- [ksvbka/design_pattern_for_embedded_system](https://github.com/ksvbka/design_pattern_for_embedded_system) — 嵌入式 C 设计模式
- 《Head First 设计模式》
- 《GoF 设计模式》

---

*Build by kly | 2026-04*