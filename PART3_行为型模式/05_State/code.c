/**
 * ===========================================================================
 * State Pattern — 状态机模式
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * 第一部分：Gumball Machine（糖果机状态机）
 * ============================================================================*/

typedef enum { S_NO_QUARTER, S_HAS_QUARTER, S_SOLD, S_SOLD_OUT } GumballState;
typedef enum { EV_INSERT, EV_EJECT, EV_TURN, EV_DISPENSE } GumballEvent;

typedef struct _GumballMachine {
    GumballState state;
    int ball_count;
} GumballMachine;

static const char* state_names[] = {"NoQuarter", "HasQuarter", "Sold", "SoldOut"};

static GumballState transitions[4][4] = {
    [S_NO_QUARTER][EV_INSERT] = S_HAS_QUARTER,
    [S_HAS_QUARTER][EV_EJECT] = S_NO_QUARTER,
    [S_HAS_QUARTER][EV_TURN] = S_SOLD,
    [S_SOLD][EV_DISPENSE] = S_NO_QUARTER,
};

static void gumball_handle_event(GumballMachine* m, GumballEvent ev) {
    GumballState next = transitions[m->state][ev];
    printf("  [Event] %d (state=%s)\n", ev, state_names[m->state]);
    if (m->state == S_HAS_QUARTER && ev == EV_TURN) {
        printf("  [Machine] Dispensing gumball...\n");
        m->ball_count--;
    }
    if (next != m->state && next < 4) {
        printf("  [Transition] %s -> %s\n", state_names[m->state], state_names[next]);
        m->state = next;
    }
    if (m->state == S_SOLD) {
        if (m->ball_count > 0) m->state = S_NO_QUARTER;
        else m->state = S_SOLD_OUT;
    }
}

/* ============================================================================
 * 第二部分：TCP Connection State Machine
 * ============================================================================*/

typedef enum {
    TCP_CLOSED, TCP_LISTEN, TCP_SYN_SENT, TCP_SYN_RECV, TCP_ESTABLISHED,
    TCP_FIN_WAIT_1, TCP_FIN_WAIT_2, TCP_CLOSE_WAIT, TCP_LAST_ACK, TCP_TIME_WAIT
} TCPState;

typedef enum { TCP_OPEN, TCP_SYN, TCP_SYN_ACK, TCP_ACK, TCP_FIN, TCP_TIMEOUT } TCPEvent;

static const char* tcp_state_names[] = {
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECV", "ESTABLISHED",
    "FIN_WAIT_1", "FIN_WAIT_2", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

typedef struct _TCPSocket {
    TCPState state;
    void (*set_state)(struct _TCPSocket*, TCPState);
} TCPSocket;

static void TCP_set_state(TCPSocket* sk, TCPState state) {
    printf("  [TCP] %s -> %s\n", tcp_state_names[sk->state], tcp_state_names[state]);
    sk->state = state;
}

static void tcp_dispatch(TCPSocket* sk, TCPEvent ev) {
    printf("  [TCP Event] %d (state=%s)\n", ev, tcp_state_names[sk->state]);
    TCPState next = sk->state;

    if (sk->state == TCP_CLOSED && ev == TCP_OPEN) next = TCP_SYN_SENT;
    else if (sk->state == TCP_CLOSED && ev == TCP_TIMEOUT) next = TCP_LISTEN;
    else if (sk->state == TCP_SYN_SENT && ev == TCP_SYN_ACK) next = TCP_ESTABLISHED;
    else if (sk->state == TCP_ESTABLISHED && ev == TCP_FIN) next = TCP_CLOSE_WAIT;
    else if (sk->state == TCP_CLOSE_WAIT && ev == TCP_CLOSE) next = TCP_LAST_ACK;
    else if (sk->state == TCP_LAST_ACK && ev == TCP_ACK) next = TCP_CLOSED;

    if (next != sk->state) TCP_set_state(sk, next);
}

/* ============================================================================
 * 第三部分：PCIe LTSSM
 * ============================================================================*/

typedef enum {
    LTSSM_DETECT, LTSSM_POLLING, LTSSM_CONFIG, LTSSM_RECOVERY,
    LTSSM_L0, LTSSM_L1, LTSSM_DISABLED
} LTSSMState;

typedef enum { LTSSM_DETECT_ACTIVE, LTSSM_POLLING_ACTIVE, LTSSM_CONFIG_ACTIVE,
              LTSSM_RECOVERY_ACTIVE, LTSSM_PM_REQUEST, LTSSM_ERROR } LTSSMEvent;

static const char* ltssm_names[] = {
    "DETECT", "POLLING", "CONFIG", "RECOVERY", "L0", "L1", "DISABLED"
};

typedef struct _PCIeLink {
    LTSSMState state;
    int gen;
    void (*handle_event)(struct _PCIeLink*, LTSSMEvent);
} PCIeLink;

static void PCIeLink_handle_event(PCIeLink* link, LTSSMEvent ev) {
    printf("  [LTSSM] Event=%d (state=%s)\n", ev, ltssm_names[link->state]);
    LTSSMState next = link->state;

    if (link->state == LTSSM_DETECT && ev == LTSSM_DETECT_ACTIVE) next = LTSSM_POLLING;
    else if (link->state == LTSSM_POLLING && ev == LTSSM_POLLING_ACTIVE) next = LTSSM_CONFIG;
    else if (link->state == LTSSM_CONFIG && ev == LTSSM_CONFIG_ACTIVE) {
        printf("  [LTSSM] Link up! Speed=Gen%d\n", link->gen);
        next = LTSSM_L0;
    }
    else if (link->state == LTSSM_L0 && ev == LTSSM_PM_REQUEST) next = LTSSM_L1;
    else if (link->state == LTSSM_L1 && ev == LTSSM_RECOVERY_ACTIVE) next = LTSSM_L0;

    if (next != link->state) {
        printf("  [LTSSM] %s -> %s\n", ltssm_names[link->state], ltssm_names[next]);
        link->state = next;
    }
}

/* ============================================================================
 * 主函数
 * ============================================================================*/

int main(void) {
    printf("=================================================================\n");
    printf("                     State Pattern — 状态机模式\n");
    printf("=================================================================\n\n");

    /* 示例1：Gumball Machine */
    printf("【示例1】Gumball Machine — 糖果机状态机\n");
    printf("-----------------------------------------------------------------\n");
    GumballMachine m = { .state = S_NO_QUARTER, .ball_count = 3 };
    gumball_handle_event(&m, EV_INSERT);
    gumball_handle_event(&m, EV_TURN);
    gumball_handle_event(&m, EV_INSERT);
    gumball_handle_event(&m, EV_TURN);

    /* 示例2：TCP State Machine */
    printf("\n【示例2】TCP Connection State Machine\n");
    printf("-----------------------------------------------------------------\n");
    TCPSocket sk = { .state = TCP_CLOSED };
    sk.set_state = TCP_set_state;
    printf("  [Scenario] 3-way handshake:\n");
    tcp_dispatch(&sk, TCP_OPEN);
    tcp_dispatch(&sk, TCP_SYN_ACK);
    printf("  [Scenario] Close connection:\n");
    tcp_dispatch(&sk, TCP_FIN);
    tcp_dispatch(&sk, TCP_CLOSE);
    tcp_dispatch(&sk, TCP_ACK);

    /* 示例3：PCIe LTSSM */
    printf("\n【示例3】PCIe Link Training State Machine\n");
    printf("-----------------------------------------------------------------\n");
    PCIeLink link = { .state = LTSSM_DETECT, .gen = 5 };
    link.handle_event = PCIeLink_handle_event;
    link.handle_event(&link, LTSSM_DETECT_ACTIVE);
    link.handle_event(&link, LTSSM_POLLING_ACTIVE);
    link.handle_event(&link, LTSSM_CONFIG_ACTIVE);
    link.handle_event(&link, LTSSM_PM_REQUEST);
    link.handle_event(&link, LTSSM_RECOVERY_ACTIVE);

    printf("\n=================================================================\n");
    printf(" State 模式核心：状态表驱动 or 状态对象模式\n");
    printf(" Linux 内核应用：TCP 状态机、PCIe LTSSM\n");
    printf("=================================================================\n");
    return 0;
}
