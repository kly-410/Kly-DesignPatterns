# Kly-DesignPatterns

> C 语言设计模式学习仓库 | 配套 Head First 设计模式 + GoF 原典

## 背景

本仓库是 kly（PCIe 芯片验证工程师）的设计模式学习项目，专为 **C 语言背景的工程师** 设计。

参考资料：
- 《Head First 设计模式》
- 《GoF 设计模式》
- Linux Kernel 源码
- OOC-Design-Pattern（GitHub）

## 课程结构

| 部分 | 内容 | 章节数 |
|:---|:---|:---|
| [PART0 预备知识](./PART0_预备知识/) | C 语言的 OOP 写法、函数指针、接口模拟 | 1 |
| [PART1 创建型模式](./PART1_创建型模式/) | Singleton / Factory / Abstract Factory / Builder / Prototype | 5 |
| [PART2 结构型模式](./PART2_结构型模式/) | Adapter / Bridge / Composite / Decorator / Facade / Flyweight / Proxy | 7 |
| [PART3 行为型模式](./PART3_行为型模式/) | Strategy / Observer / Command / Iterator / Mediator / State / Template / Chain / Visitor / Interpreter / Memento | 11 |

## 每章内容

每个模式目录下包含：
- `theory.md` — 讲义（问题分析 + C语言实现 + 内核实例 + 练习）
- `code.c` — 完整可编译运行的 C 代码
- `test.c` — 测试代码
- `Makefile` — 编译脚本（全注释）

## 推荐学习路径

```
第一步：读 PART0（预备知识）
        ↓
第二步：PART1 创建型（最容易理解）
        ↓
第三步：PART2 结构型（考验组合能力）
        ↓
第四步：PART3 行为型（最抽象，最大量）
        ↓
第五步：结合 PCIe 驱动源码，理解 Linux 内核里的模式
```

## 环境要求

- GCC 9.0+
- Linux 环境（部分示例需要 Linux API）
- Make

## 编译

```bash
# 编译某个模式
cd PART3_行为型模式/13_Strategy/code && make

# 运行测试
./test
```

## 致谢

- [QMonkey/OOC-Design-Pattern](https://github.com/QMonkey/OOC-Design-Pattern) — GoF 23 种模式 C 实现
- [ksvbka/design_pattern_for_embedded_system](https://github.com/ksvbka/design_pattern_for_embedded_system) — 嵌入式 C 设计模式
- 《Head First 设计模式》
- 《GoF 设计模式》

---

*Build by kly | 2026-04*
