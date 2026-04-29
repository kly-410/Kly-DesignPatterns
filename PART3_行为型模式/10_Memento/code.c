/**
 * ===========================================================================
 * 备忘录模式 — Memento Pattern
 * ===========================================================================
 *
 * 核心思想：保存和恢复对象状态，不破坏封装性
 *
 * 本代码演示（嵌入式视角）：
 * 1. 游戏存档：保存/加载游戏进度
 * 2. 配置寄存器快照：修改前保存，失败后恢复
 * 3. 事务回滚：写入前记录旧值，出错时回滚
 *
 * 备忘录模式三要素：
 *   Originator（发起者）— 状态所有者，负责创建/恢复快照
 *   Memento（备忘录）  — 状态的只读副本
 *   Caretaker（管理者）— 保管多个快照，不操作其内容
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：游戏存档（Memento 最经典例子）
 *
 * 游戏状态 = { level, hp, score }
 * 保存游戏 = 创建 Memento（快照）
 * 读取存档 = 从 Memento 恢复到 Originator
 * ============================================================================*/

/** 前向声明（解决 Game 引用自身的函数指针类型）*/
typedef struct _Game Game;

/** 游戏状态（Memento：备忘录对象）*/
typedef struct {
    int level;   /**< 当前关卡 */
    int hp;      /**< 生命值 */
    int score;   /**< 分数 */
} GameState;

/** 游戏（Originator：发起者）*/
struct _Game {
    GameState state;  /**< 当前状态 */

    /** 创建备忘录：保存当前状态（深拷贝）*/
    GameState* (*save)(Game*);

    /** 恢复状态：从备忘录恢复 */
    void (*restore)(Game*, GameState*);
};

/**
 * 保存游戏状态
 * 关键：用 memcpy 深拷贝一份状态，返回给调用者
 * 调用者（SaveManager）持有这份拷贝，不感知内部结构
 */
static GameState* Game_save(Game* game) {
    GameState* m = malloc(sizeof(GameState));
    memcpy(m, &game->state, sizeof(GameState));  /**< 深拷贝 */
    printf("  [Game] 存档: 关卡=%d, HP=%d, 分数=%d\n",
           game->state.level, game->state.hp, game->state.score);
    return m;
}

/** 从备忘录恢复游戏状态 */
static void Game_restore(Game* game, GameState* m) {
    memcpy(&game->state, m, sizeof(GameState));  /**< 用备忘录内容覆盖当前状态 */
    printf("  [Game] 读档: 关卡=%d, HP=%d, 分数=%d\n",
           game->state.level, game->state.hp, game->state.score);
}

static Game* Game_new(void) {
    Game* g = calloc(1, sizeof(Game));
    g->state.level = 1; g->state.hp = 100; g->state.score = 0;
    g->save = Game_save;
    g->restore = Game_restore;
    return g;
}

/** 存档管理器（Caretaker：管理者）*/
typedef struct {
    GameState* saves[16];   /**< 保存的存档列表 */
    int count;              /**< 已保存的存档数量 */
} SaveManager;

static SaveManager* SaveManager_new(void) {
    return calloc(1, sizeof(SaveManager));
}

/* ============================================================================
 * 第二部分：PCIe 配置寄存器快照
 *
 * 场景：固件修改 PCIe BAR0 地址前，先保存快照
 *       如果修改后链路异常，可以快速恢复到之前的状态
 * ============================================================================*/

/** 前向声明（解决 PCIeDevice 引用自身的函数指针类型）*/
typedef struct _PCIeDevice PCIeDevice;

/** PCIe 设备配置快照（Memento）*/
typedef struct {
    uint32_t bar0;       /**< BAR0 地址 */
    uint32_t bar1;       /**< BAR1 地址 */
    uint8_t irq_line;   /**< IRQ 编号 */
} PCIConfigSnapshot;

/** PCIe 设备（Originator）*/
struct _PCIeDevice {
    uint32_t bar0;
    uint32_t bar1;
    uint8_t irq_line;

    /** 创建配置快照 */
    PCIConfigSnapshot* (*createSnapshot)(PCIeDevice*);

    /** 从快照恢复配置 */
    void (*restoreSnapshot)(PCIeDevice*, PCIConfigSnapshot*);
};

/** 保存当前配置到快照 */
static PCIConfigSnapshot* PCIeDevice_createSnapshot(PCIeDevice* dev) {
    PCIConfigSnapshot* s = malloc(sizeof(PCIConfigSnapshot));
    s->bar0 = dev->bar0;
    s->bar1 = dev->bar1;
    s->irq_line = dev->irq_line;
    printf("  [PCIe] 保存快照: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
    return s;
}

/** 从快照恢复配置 */
static void PCIeDevice_restoreSnapshot(PCIeDevice* dev, PCIConfigSnapshot* s) {
    dev->bar0 = s->bar0;
    dev->bar1 = s->bar1;
    dev->irq_line = s->irq_line;
    printf("  [PCIe] 恢复快照: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
}

static PCIeDevice* PCIeDevice_new(void) {
    PCIeDevice* d = calloc(1, sizeof(PCIeDevice));
    d->bar0 = 0xFEBC0000; d->bar1 = 0xFEDF0000; d->irq_line = 17;
    d->createSnapshot = PCIeDevice_createSnapshot;
    d->restoreSnapshot = PCIeDevice_restoreSnapshot;
    return d;
}

/* ============================================================================
 * 第三部分：事务回滚
 *
 * 场景：固件升级时，多个寄存器的修改必须是原子操作
 *       写寄存器前记录旧值，任意一步失败则全部回滚
 * ============================================================================*/

/** 事务条目接口 */
typedef struct _TransEntry {
    void (*apply)(struct _TransEntry*);    /**< 执行 */
    void (*rollback)(struct _TransEntry*);  /**< 回滚 */
} TransEntry;

/** 寄存器写入事务 */
typedef struct {
    TransEntry base;         /**< 继承事务接口 */
    const char* name;        /**< 寄存器名称 */
    uint32_t old_val;       /**< 旧值（用于回滚）*/
    uint32_t new_val;       /**< 新值（执行时写入）*/
    uint32_t* reg;         /**< 指向实际寄存器的指针 */
} ConfigWrite;

/** 执行写入 */
static void ConfigWrite_apply(TransEntry* base) {
    ConfigWrite* cw = (ConfigWrite*)base;
    printf("  [Write] %s: 0x%08X → 0x%08X\n", cw->name, cw->old_val, cw->new_val);
    *cw->reg = cw->new_val;  /**< 实际写入寄存器 */
}

/** 回滚写入 */
static void ConfigWrite_rollback(TransEntry* base) {
    ConfigWrite* cw = (ConfigWrite*)base;
    printf("  [Rollback] %s: 0x%08X → 0x%08X\n", cw->name, *cw->reg, cw->old_val);
    *cw->reg = cw->old_val;  /**< 恢复旧值 */
}

/** 创建寄存器写入事务 */
static ConfigWrite* ConfigWrite_new(const char* name, uint32_t* reg, uint32_t new_val) {
    ConfigWrite* cw = malloc(sizeof(ConfigWrite));
    cw->base.apply = ConfigWrite_apply;
    cw->base.rollback = ConfigWrite_rollback;
    cw->name = name;
    cw->old_val = *reg;     /**< 记录旧值 */
    cw->new_val = new_val;
    cw->reg = reg;
    return cw;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                 Memento Pattern — 备忘录模式\n");
    printf("=================================================================\n\n");

    /* -------------------- 示例1：游戏存档 -------------------- */
    printf("【示例1】游戏存档 — 读档/存档\n");
    printf("-----------------------------------------------------------------\n");
    Game* game = Game_new();
    SaveManager* saves = SaveManager_new();

    printf("  初始状态: level=%d, hp=%d\n", game->state.level, game->state.hp);

    /**< 保存进度（创建 Memento）*/
    GameState* save1 = game->save(game);
    saves->saves[saves->count++] = save1;

    /**< 继续游戏（状态变化）*/
    game->state.hp -= 30; game->state.score += 500;
    printf("  战斗后: hp=%d, score=%d\n", game->state.hp, game->state.score);

    /**< 读档（从 Memento 恢复）*/
    printf("  读取存档...\n");
    game->restore(game, saves->saves[0]);

    free(game); free(save1); free(saves);

    /* -------------------- 示例2：PCIe 配置快照 -------------------- */
    printf("\n【示例2】PCIe 配置寄存器快照\n");
    printf("-----------------------------------------------------------------\n");
    PCIeDevice* dev = PCIeDevice_new();

    printf("  初始配置: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);

    /**< 修改前保存快照 */
    PCIConfigSnapshot* snap = dev->createSnapshot(dev);

    /**< 修改配置 */
    dev->bar0 = 0xFEBE0000; dev->irq_line = 18;
    printf("  修改后: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);

    /**< 恢复到原始配置 */
    dev->restoreSnapshot(dev, snap);

    free(snap); free(dev);

    /* -------------------- 示例3：事务回滚 -------------------- */
    printf("\n【示例3】事务回滚 — 寄存器写入\n");
    printf("-----------------------------------------------------------------\n");
    uint32_t bar0_reg = 0xFEBC0000;
    printf("  初始: BAR0=0x%X\n", bar0_reg);

    /**< 创建写入事务（自动记录旧值）*/
    ConfigWrite* cw = ConfigWrite_new("BAR0", &bar0_reg, 0xFEBE0000);

    /**< 执行写入 */
    cw->base.apply(&cw->base);
    printf("  写入后: BAR0=0x%X\n", bar0_reg);

    /**< 回滚 */
    cw->base.rollback(&cw->base);
    printf("  回滚后: BAR0=0x%X\n", bar0_reg);

    free(cw);

    printf("\n=================================================================\n");
    printf(" 备忘录核心：Originator 创建/恢复快照，Caretaker 保管\n");
    printf(" 嵌入式场景：固件配置回退、游戏存档、事务性写入\n");
    printf("=================================================================\n");
    return 0;
}
