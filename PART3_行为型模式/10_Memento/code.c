/**
 * ===========================================================================
 * Memento Pattern — 备忘录模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Game Save/Load（游戏存档）
 * ============================================================================*/

typedef struct _GameState {
    int level;
    int hp;
    int score;
} GameState;

typedef struct _Game {
    GameState state;
    GameState* (*save)(struct _Game*);
    void (*restore)(struct _Game*, GameState*);
} Game;

static GameState* Game_save(Game* game) {
    GameState* m = malloc(sizeof(GameState));
    memcpy(m, &game->state, sizeof(GameState));
    printf("  [Game] Saved: level=%d, hp=%d, score=%d\n", game->state.level, game->state.hp, game->state.score);
    return m;
}

static void Game_restore(Game* game, GameState* m) {
    memcpy(&game->state, m, sizeof(GameState));
    printf("  [Game] Restored: level=%d, hp=%d, score=%d\n", game->state.level, game->state.hp, game->state.score);
}

static Game* Game_new(void) {
    Game* g = calloc(1, sizeof(Game));
    g->state.level = 1; g->state.hp = 100; g->state.score = 0;
    g->save = Game_save;
    g->restore = Game_restore;
    return g;
}

typedef struct _SaveManager {
    GameState* saves[16];
    int count;
} SaveManager;

static void SaveManager_save(SaveManager* mgr, GameState* state) {
    mgr->saves[mgr->count++] = state;
    printf("  [SaveManager] Saved to slot %d\n", mgr->count - 1);
}

static GameState* SaveManager_load(SaveManager* mgr, int slot) {
    if (slot < 0 || slot >= mgr->count) return NULL;
    return mgr->saves[slot];
}

static SaveManager* SaveManager_new(void) {
    SaveManager* m = calloc(1, sizeof(SaveManager));
    return m;
}

/* ============================================================================
 * 第二部分：Config Registry Snapshot
 * ============================================================================*/

typedef struct {
    uint32_t bar0;
    uint32_t bar1;
    uint8_t irq_line;
} PCIConfigSnapshot;

typedef struct _PCIeDevice {
    uint32_t bar0;
    uint32_t bar1;
    uint8_t irq_line;
    PCIConfigSnapshot* (*createSnapshot)(struct _PCIeDevice*);
    void (*restoreSnapshot)(struct _PCIeDevice*, PCIConfigSnapshot*);
} PCIeDevice;

static PCIConfigSnapshot* PCIeDevice_createSnapshot(PCIeDevice* dev) {
    PCIConfigSnapshot* s = malloc(sizeof(PCIConfigSnapshot));
    s->bar0 = dev->bar0; s->bar1 = dev->bar1; s->irq_line = dev->irq_line;
    printf("  [PCIe] Snapshot: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
    return s;
}

static void PCIeDevice_restoreSnapshot(PCIeDevice* dev, PCIConfigSnapshot* s) {
    dev->bar0 = s->bar0; dev->bar1 = s->bar1; dev->irq_line = s->irq_line;
    printf("  [PCIe] Restored: BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
}

static PCIeDevice* PCIeDevice_new(void) {
    PCIeDevice* d = calloc(1, sizeof(PCIeDevice));
    d->bar0 = 0xFEBC0000; d->bar1 = 0xFEDF0000; d->irq_line = 17;
    d->createSnapshot = PCIeDevice_createSnapshot;
    d->restoreSnapshot = PCIeDevice_restoreSnapshot;
    return d;
}

/* ============================================================================
 * 第三部分：Transaction Rollback
 * ============================================================================*/

typedef struct _TransEntry {
    void (*apply)(struct _TransEntry*);
    void (*rollback)(struct _TransEntry*);
} TransEntry;

typedef struct _ConfigWrite {
    TransEntry base;
    const char* name;
    uint32_t old_val;
    uint32_t new_val;
    uint32_t* reg;
} ConfigWrite;

static void ConfigWrite_apply(TransEntry* base) {
    ConfigWrite* cw = (ConfigWrite*)base;
    printf("  [ConfigWrite] %s: 0x%X -> 0x%X\n", cw->name, cw->old_val, cw->new_val);
    *cw->reg = cw->new_val;
}

static void ConfigWrite_rollback(TransEntry* base) {
    ConfigWrite* cw = (ConfigWrite*)base;
    printf("  [ConfigWrite] Rollback %s: 0x%X -> 0x%X\n", cw->name, *cw->reg, cw->old_val);
    *cw->reg = cw->old_val;
}

static ConfigWrite* ConfigWrite_new(const char* name, uint32_t* reg, uint32_t new_val) {
    ConfigWrite* cw = malloc(sizeof(ConfigWrite));
    cw->base.apply = ConfigWrite_apply;
    cw->base.rollback = ConfigWrite_rollback;
    cw->name = name;
    cw->old_val = *reg;
    cw->new_val = new_val;
    cw->reg = reg;
    return cw;
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                    Memento Pattern — 备忘录模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Game Save/Load */
    printf("【示例1】Game Save/Load — 游戏存档\n");
    printf("-----------------------------------------------------------------\n");
    Game* game = Game_new();
    SaveManager* saves = SaveManager_new();
    printf("  Initial state: level=%d, hp=%d\n", game->state.level, game->state.hp);
    GameState* save1 = game->save(game);
    saves->save(saves, save1);
    game->state.hp -= 30; game->state.score += 500;
    printf("  After fighting: hp=%d, score=%d\n", game->state.hp, game->state.score);
    GameState* load1 = saves->load(saves, 0);
    if (load1) game->restore(game, load1);
    free(game); free(saves);

    /* 示例2：Config Snapshot */
    printf("\n【示例2】Config Registry Snapshot — 配置寄存器快照\n");
    printf("-----------------------------------------------------------------\n");
    PCIeDevice* dev = PCIeDevice_new();
    printf("  Initial config:\n");
    printf("    BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
    PCIConfigSnapshot* snap = dev->createSnapshot(dev);
    dev->bar0 = 0xFEBE0000; dev->irq_line = 18;
    printf("  Modified:\n    BAR0=0x%X, IRQ=%d\n", dev->bar0, dev->irq_line);
    dev->restoreSnapshot(dev, snap);
    free(snap); free(dev);

    /* 示例3：Transaction Rollback */
    printf("\n【示例3】Transaction Rollback — 事务回滚\n");
    printf("-----------------------------------------------------------------\n");
    uint32_t reg_val = 0xFEBC0000;
    printf("  Initial: 0x%X\n", reg_val);
    ConfigWrite* cw = ConfigWrite_new("BAR0", &reg_val, 0xFEBE0000);
    cw->base.apply(&cw->base);
    printf("  After apply: 0x%X\n", reg_val);
    cw->base.rollback(&cw->base);
    printf("  After rollback: 0x%X\n", reg_val);
    free(cw);

    printf("\n=================================================================\n");
    printf(" Memento 模式核心：Memento 保存快照，Originator 可恢复\n");
    printf("=================================================================\n");
    return 0;
}
