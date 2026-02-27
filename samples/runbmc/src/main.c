/*
 * RunBMC - Baseboard Management Controller on Zephyr RTOS
 * Day 1: Multi-threaded BMC Architecture
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>

LOG_MODULE_REGISTER(runbmc, LOG_LEVEL_INF);

/* Thread stack sizes */
#define SENSOR_STACK_SIZE    2048
#define POWER_STACK_SIZE     2048
#define TELEMETRY_STACK_SIZE 2048

/* Thread priorities (lower number = higher priority) */
#define POWER_THREAD_PRIORITY     5 /* Highest priority */
#define SENSOR_THREAD_PRIORITY    7 /* Medium-high priority */
#define TELEMETRY_THREAD_PRIORITY 9 /* Medium priority */

/* BMC System State */
struct bmc_state {
    bool initialized;
    uint32_t uptime_sec;
    uint8_t power_state;        /* 0=Off, 1=On, 2=Transitioning */
    int16_t cpu_temp;           /* CPU temperature in Celsius */
    int16_t gpu_temp[8];        /* 8 GPU temperatures */
    uint16_t power_watts;       /* Total power consumption */
    uint32_t sensor_reads;      /* Total sensor read count */
    uint32_t telemetry_samples; /* Total telemetry samples */
};

/* Global BMC state (protected by mutex) */
static struct bmc_state bmc;
static struct k_mutex bmc_mutex;

/* Thread stacks */
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(power_stack, POWER_STACK_SIZE);
K_THREAD_STACK_DEFINE(telemetry_stack, TELEMETRY_STACK_SIZE);

/* Thread control blocks */
static struct k_thread sensor_thread_data;
static struct k_thread power_thread_data;
static struct k_thread telemetry_thread_data;

/* Thread IDs */
static k_tid_t sensor_tid;
static k_tid_t power_tid;
static k_tid_t telemetry_tid;

/*
 * SENSOR THREAD
 * Continuously monitors all sensors (temperature, voltage, current)
 */
static void sensor_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Sensor thread started");

    while (1) {
        /* Simulate sensor reading (in real BMC, read I2C/SMBus sensors) */
        k_mutex_lock(&bmc_mutex, K_FOREVER);

        /* CPU temperature: 35-85°C */
        bmc.cpu_temp = 35 + (k_uptime_get_32() % 50);

        /* GPU temperatures: each GPU has different temp */
        for (int i = 0; i < 8; i++) {
            bmc.gpu_temp[i] = 40 + (k_uptime_get_32() % 60) + i * 2;
        }

        bmc.sensor_reads++;

        k_mutex_unlock(&bmc_mutex);

        /* Read sensors every 5 seconds (silently) */
        k_sleep(K_SECONDS(5));
    }
}

/*
 * POWER THREAD
 * Manages power sequencing, power states, and power budget
 */
static void power_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Power management thread started");

    /* Simulate power-on sequence */
    k_mutex_lock(&bmc_mutex, K_FOREVER);
    bmc.power_state = 2; /* Transitioning */
    k_mutex_unlock(&bmc_mutex);

    LOG_INF("Power sequence: Enabling voltage rails...");
    k_sleep(K_MSEC(500));

    LOG_INF("Power sequence: Waiting for POWER_GOOD...");
    k_sleep(K_MSEC(300));

    LOG_INF("Power sequence: Releasing CPU reset...");
    k_sleep(K_MSEC(200));

    k_mutex_lock(&bmc_mutex, K_FOREVER);
    bmc.power_state = 1; /* On */
    k_mutex_unlock(&bmc_mutex);

    LOG_INF("Power sequence: System is ON");

    while (1) {
        /* Monitor power consumption */
        k_mutex_lock(&bmc_mutex, K_FOREVER);

        /* Simulate power consumption: 200-800W */
        bmc.power_watts = 200 + (k_uptime_get_32() % 600);

        /* Check if power budget exceeded (silently monitor) */
        if (bmc.power_watts > 750) {
            /* In real system, would trigger power capping */
        }

        k_mutex_unlock(&bmc_mutex);

        /* Monitor power every 3 seconds */
        k_sleep(K_SECONDS(3));
    }
}

/*
 * TELEMETRY THREAD
 * Collects and aggregates system metrics (silently)
 */
static void telemetry_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Telemetry thread started");

    while (1) {
        k_mutex_lock(&bmc_mutex, K_FOREVER);

        bmc.telemetry_samples++;
        bmc.uptime_sec = k_uptime_get_32() / 1000;

        k_mutex_unlock(&bmc_mutex);

        /* Collect telemetry every 5 seconds (silently) */
        k_sleep(K_SECONDS(5));
    }
}

/*
 * SHELL COMMANDS
 * Custom BMC commands for debugging and management
 */

/* bmc info - Display BMC information */
static int cmd_bmc_info(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "=================================");
    shell_print(sh, "RunBMC System Information");
    shell_print(sh, "=================================");
    shell_print(sh, "Version:      0.1.0-dev");
    shell_print(sh, "Platform:     QEMU RISC-V32");
    shell_print(sh, "Build Date:   %s %s", __DATE__, __TIME__);
    shell_print(sh, "Zephyr:       v%d.%d.%d", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR,
            KERNEL_PATCHLEVEL);
    shell_print(sh, "Features:     Power, Sensors, Telemetry");

    return 0;
}

/* bmc status - Display current system status */
static int cmd_bmc_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    k_mutex_lock(&bmc_mutex, K_FOREVER);

    shell_print(sh, "=================================");
    shell_print(sh, "BMC System Status");
    shell_print(sh, "=================================");
    shell_print(sh, "Uptime:           %u seconds (%u min)", bmc.uptime_sec,
            bmc.uptime_sec / 60);
    shell_print(sh, "Power State:      %s",
            bmc.power_state == 0   ? "OFF"
            : bmc.power_state == 1 ? "ON"
                       : "TRANSITIONING");
    shell_print(sh, "Power Draw:       %u W", bmc.power_watts);
    shell_print(sh, "Sensor Reads:     %u", bmc.sensor_reads);
    shell_print(sh, "Telemetry Samples: %u", bmc.telemetry_samples);

    k_mutex_unlock(&bmc_mutex);

    return 0;
}

/* bmc sensors - Display all sensor readings */
static int cmd_bmc_sensors(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    k_mutex_lock(&bmc_mutex, K_FOREVER);

    shell_print(sh, "=================================");
    shell_print(sh, "Sensor Readings");
    shell_print(sh, "=================================");
    shell_print(sh, "CPU Temperature:  %d°C", bmc.cpu_temp);
    shell_print(sh, "GPU Temperatures:");
    for (int i = 0; i < 8; i++) {
        shell_print(sh, "  GPU%d:           %d°C", i, bmc.gpu_temp[i]);
    }

    k_mutex_unlock(&bmc_mutex);

    return 0;
}

/* bmc power - Display power information */
static int cmd_bmc_power(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    k_mutex_lock(&bmc_mutex, K_FOREVER);

    shell_print(sh, "=================================");
    shell_print(sh, "Power Management");
    shell_print(sh, "=================================");
    shell_print(sh, "Current State:    %s",
            bmc.power_state == 0   ? "OFF"
            : bmc.power_state == 1 ? "ON"
                       : "TRANSITIONING");
    shell_print(sh, "Power Draw:       %u W", bmc.power_watts);
    shell_print(sh, "Power Budget:     800 W");
    shell_print(sh, "Power Headroom:   %d W", 800 - bmc.power_watts);

    k_mutex_unlock(&bmc_mutex);

    return 0;
}

/* bmc threads - Display thread information */
static int cmd_bmc_threads(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "=================================");
    shell_print(sh, "BMC Threads");
    shell_print(sh, "=================================");
    shell_print(sh, "Thread         Priority  State");
    shell_print(sh, "---------------------------------");
    shell_print(sh, "Power          %d         Running", POWER_THREAD_PRIORITY);
    shell_print(sh, "Sensor         %d         Running", SENSOR_THREAD_PRIORITY);
    shell_print(sh, "Telemetry      %d         Running", TELEMETRY_THREAD_PRIORITY);

    return 0;
}

/* bmc telemetry - Display telemetry snapshot */
static int cmd_bmc_telemetry(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    k_mutex_lock(&bmc_mutex, K_FOREVER);

    shell_print(sh, "=================================");
    shell_print(sh, "Telemetry Snapshot #%u", bmc.telemetry_samples);
    shell_print(sh, "=================================");
    shell_print(sh, "Uptime:           %u seconds (%u min)", bmc.uptime_sec,
            bmc.uptime_sec / 60);
    shell_print(sh, "Power State:      %s",
            bmc.power_state == 0   ? "OFF"
            : bmc.power_state == 1 ? "ON"
                       : "TRANSITIONING");
    shell_print(sh, "");
    shell_print(sh, "Temperature Sensors:");
    shell_print(sh, "  CPU:            %d°C", bmc.cpu_temp);
    for (int i = 0; i < 8; i++) {
        shell_print(sh, "  GPU%d:           %d°C", i, bmc.gpu_temp[i]);
    }
    shell_print(sh, "");
    shell_print(sh, "Power:");
    shell_print(sh, "  Current Draw:   %u W", bmc.power_watts);
    shell_print(sh, "  Budget:         800 W");
    shell_print(sh, "  Headroom:       %d W", 800 - bmc.power_watts);
    shell_print(sh, "");
    shell_print(sh, "Statistics:");
    shell_print(sh, "  Sensor Reads:   %u", bmc.sensor_reads);
    shell_print(sh, "  Telemetry Samples: %u", bmc.telemetry_samples);

    k_mutex_unlock(&bmc_mutex);

    return 0;
}

/* Register shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_bmc, SHELL_CMD(info, NULL, "Display BMC information", cmd_bmc_info),
    SHELL_CMD(status, NULL, "Display system status", cmd_bmc_status),
    SHELL_CMD(sensors, NULL, "Display sensor readings", cmd_bmc_sensors),
    SHELL_CMD(power, NULL, "Display power information", cmd_bmc_power),
    SHELL_CMD(threads, NULL, "Display thread information", cmd_bmc_threads),
    SHELL_CMD(telemetry, NULL, "Display telemetry snapshot", cmd_bmc_telemetry),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bmc, &sub_bmc, "RunBMC commands", NULL);

/*
 * MAIN FUNCTION
 * Initialize BMC and start all threads
 */
int main(void)
{
    printk("\n");
    printk("================================================\n");
    printk("       RunBMC - Baseboard Management Controller\n");
    printk("              on Zephyr RTOS\n");
    printk("================================================\n");
    printk("Day 1: Multi-threaded BMC Architecture\n");
    printk("Platform: %s\n", CONFIG_BOARD);
    printk("================================================\n\n");

    /* Initialize mutex */
    k_mutex_init(&bmc_mutex);

    /* Initialize BMC state */
    memset(&bmc, 0, sizeof(bmc));
    bmc.initialized = true;

    LOG_INF("Initializing RunBMC...");

    /* Create power management thread (highest priority) */
    power_tid = k_thread_create(&power_thread_data, power_stack,
                    K_THREAD_STACK_SIZEOF(power_stack), power_thread, NULL, NULL,
                    NULL, POWER_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(power_tid, "power_mgmt");

    /* Create sensor monitoring thread */
    sensor_tid = k_thread_create(&sensor_thread_data, sensor_stack,
                     K_THREAD_STACK_SIZEOF(sensor_stack), sensor_thread, NULL, NULL,
                     NULL, SENSOR_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(sensor_tid, "sensor_mon");

    /* Create telemetry collection thread */
    telemetry_tid = k_thread_create(&telemetry_thread_data, telemetry_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_stack), telemetry_thread,
                    NULL, NULL, NULL, TELEMETRY_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(telemetry_tid, "telemetry");

    LOG_INF("All BMC threads started successfully");
    LOG_INF("System ready. Type 'bmc help' for commands.");

    /* Main thread becomes idle - all work done in other threads */
    return 0;
}