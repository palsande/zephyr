#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(power_seq_fsm, LOG_LEVEL_INF);

typedef struct {
    uint32_t enable_delay_ms;
    uint32_t pg_timeout_ms;
    bool enabled;
    bool power_good;
} rail_config_t;

static rail_config_t rails[CONFIG_NUM_RAILS];

typedef enum {
    FSM_IDLE,
    FSM_ENABLE_RAIL,
    FSM_WAIT_FOR_PG,
    FSM_TIMEOUT_CHECK,
    FSM_NEXT_RAIL,
    FSM_DONE,
    FSM_ERROR
} fsm_state_t;

static fsm_state_t state = FSM_IDLE;
static int current_rail = 0;
static uint32_t elapsed_ms = 0;

void load_rail_config(int index) {
    switch (index) {
    case 0:
        rails[index].enable_delay_ms = 0;
        rails[index].pg_timeout_ms = 10;
        break;
    case 1:
        rails[index].enable_delay_ms = 10;
        rails[index].pg_timeout_ms = 10;
        break;
    case 2:
        rails[index].enable_delay_ms = 10;
        rails[index].pg_timeout_ms = 10;
        break;
    default:
        break;
    }
}

static void init_rails(void) {
    for (int i = 0; i < CONFIG_NUM_RAILS; i++) {
        load_rail_config(i);
        rails[i].enabled = false;
        rails[i].power_good = false;
    }
}

static bool simulate_pg_signal(int rail_index) {
    k_msleep(rails[rail_index].pg_timeout_ms);
    rails[rail_index].power_good = true;
    return true;
}

int main(void) {
    LOG_INF("Starting FSM-based power sequencer simulation...");
    init_rails();

    state = FSM_IDLE;

    while (state != FSM_DONE && state != FSM_ERROR) {
        switch (state) {
        case FSM_IDLE:
            LOG_INF("FSM: IDLE -> ENABLE_RAIL");
            state = FSM_ENABLE_RAIL;
            break;

        case FSM_ENABLE_RAIL:
            k_msleep(rails[current_rail].enable_delay_ms);
            rails[current_rail].enabled = true;
            LOG_INF("Rail %d enabled", current_rail);
            elapsed_ms = 0;
            state = FSM_WAIT_FOR_PG;
            break;

        case FSM_WAIT_FOR_PG:
            if (simulate_pg_signal(current_rail)) {
                LOG_INF("PG received for rail %d", current_rail);
                state = FSM_NEXT_RAIL;
            } else {
                state = FSM_TIMEOUT_CHECK;
            }
            break;

        case FSM_TIMEOUT_CHECK:
            if (elapsed_ms >= rails[current_rail].pg_timeout_ms) {
                LOG_ERR("Timeout waiting for PG on rail %d", current_rail);
                state = FSM_ERROR;
            } else {
                k_msleep(1);
                elapsed_ms++;
            }
            break;

        case FSM_NEXT_RAIL:
            current_rail++;
            if (current_rail >= CONFIG_NUM_RAILS) {
                state = FSM_DONE;
            } else {
                state = FSM_ENABLE_RAIL;
            }
            break;

        case FSM_DONE:
            LOG_INF("FSM: All rails enabled and PG received.");
            break;

        case FSM_ERROR:
            LOG_ERR("FSM: Error occurred during sequencing.");
            break;
        }
    }

    return 0;
}