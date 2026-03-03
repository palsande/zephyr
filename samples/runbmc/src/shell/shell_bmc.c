/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/version.h>
#include <runbmc/core/bmc.h>
#include <runbmc/core/platform.h>

/* bmc info */
static int cmd_bmc_info(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "===================================");
    shell_print(sh, "RunBMC System Information");
    shell_print(sh, "===================================");
    shell_print(sh, "RunBMC:       v%d.%d.%d", RUNBMC_VERSION_MAJOR, RUNBMC_VERSION_MINOR,
            RUNBMC_VERSION_PATCH);
    shell_print(sh, "Zephyr:       v%d.%d.%d", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR,
            KERNEL_PATCHLEVEL);
    shell_print(sh, "Platform:     %s", platform_get_name());
    shell_print(sh, "Build:        %s %s", __DATE__, __TIME__);
    shell_print(sh, "Max GPUs:     %d", CONFIG_RUNBMC_MAX_GPUS);

    return 0;
}

/* bmc status */
static int cmd_bmc_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    struct bmc_telemetry_sample sample;
    int ret = bmc_get_status(&sample);

    if (ret < 0) {
        shell_error(sh, "Failed to get status: %d", ret);
        return ret;
    }

    shell_print(sh, "===================================");
    shell_print(sh, "BMC System Status");
    shell_print(sh, "===================================");
    shell_print(sh, "Uptime:           %u seconds (%u min)", sample.uptime_sec,
            sample.uptime_sec / 60);
    shell_print(sh, "Power State:      %s",
            sample.power_state == BMC_POWER_STATE_S5           ? "OFF"
            : sample.power_state == BMC_POWER_STATE_S0         ? "ON"
            : sample.power_state == BMC_POWER_STATE_TRANSITION ? "TRANSITIONING"
                                       : "UNKNOWN");
    shell_print(sh, "Power Draw:       %u W", sample.power_watts);
    shell_print(sh, "Sensor Reads:     %u", sample.sensor_reads);
    shell_print(sh, "Telemetry Samples: %u", sample.sample_count);

    return 0;
}

/* bmc sensors */
static int cmd_bmc_sensors(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    struct bmc_telemetry_sample sample;
    int ret = bmc_get_status(&sample);

    if (ret < 0) {
        shell_error(sh, "Failed to get sensors: %d", ret);
        return ret;
    }

    shell_print(sh, "===================================");
    shell_print(sh, "Sensor Readings");
    shell_print(sh, "===================================");
    shell_print(sh, "CPU Temperature:  %d°C", sample.cpu_temp);
    shell_print(sh, "GPU Temperatures:");
    for (int i = 0; i < CONFIG_RUNBMC_MAX_GPUS; i++) {
        shell_print(sh, "  GPU%d:           %d°C", i, sample.gpu_temp[i]);
    }

    return 0;
}

/* bmc telemetry */
static int cmd_bmc_telemetry(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    struct bmc_telemetry_sample sample;
    int ret = bmc_get_status(&sample);

    if (ret < 0) {
        shell_error(sh, "Failed to get telemetry: %d", ret);
        return ret;
    }

    shell_print(sh, "===================================");
    shell_print(sh, "Telemetry Snapshot #%u", sample.sample_count);
    shell_print(sh, "===================================");
    shell_print(sh, "Uptime:           %u seconds (%u min)", sample.uptime_sec,
            sample.uptime_sec / 60);
    shell_print(sh, "Power State:      %s",
            sample.power_state == BMC_POWER_STATE_S5           ? "OFF"
            : sample.power_state == BMC_POWER_STATE_S0         ? "ON"
            : sample.power_state == BMC_POWER_STATE_TRANSITION ? "TRANSITIONING"
                                       : "UNKNOWN");
    shell_print(sh, "");
    shell_print(sh, "Temperatures:");
    shell_print(sh, "  CPU:            %d°C", sample.cpu_temp);
    for (int i = 0; i < CONFIG_RUNBMC_MAX_GPUS; i++) {
        shell_print(sh, "  GPU%d:           %d°C", i, sample.gpu_temp[i]);
    }
    shell_print(sh, "");
    shell_print(sh, "Power:");
    shell_print(sh, "  Current Draw:   %u W", sample.power_watts);
    shell_print(sh, "  Budget:         800 W");
    shell_print(sh, "  Headroom:       %d W", 800 - sample.power_watts);
    shell_print(sh, "");
    shell_print(sh, "Statistics:");
    shell_print(sh, "  Sensor Reads:   %u", sample.sensor_reads);
    shell_print(sh, "  Samples:        %u", sample.sample_count);

    return 0;
}

/* bmc features */
static int cmd_bmc_features(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "===================================");
    shell_print(sh, "Compiled Features");
    shell_print(sh, "===================================");
#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
    shell_print(sh, "[x] Power Management");
#else
    shell_print(sh, "[ ] Power Management");
#endif
#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
    shell_print(sh, "[x] Sensor Framework");
#else
    shell_print(sh, "[ ] Sensor Framework");
#endif
#ifdef CONFIG_RUNBMC_TELEMETRY
    shell_print(sh, "[x] Telemetry Collection");
#else
    shell_print(sh, "[ ] Telemetry Collection");
#endif
#ifdef CONFIG_RUNBMC_THERMAL_MANAGEMENT
    shell_print(sh, "[x] Thermal Management");
#else
    shell_print(sh, "[ ] Thermal Management");
#endif
#ifdef CONFIG_RUNBMC_PLDM
    shell_print(sh, "[x] PLDM Protocol");
#else
    shell_print(sh, "[ ] PLDM Protocol");
#endif
#ifdef CONFIG_RUNBMC_MCTP
    shell_print(sh, "[x] MCTP Transport");
#else
    shell_print(sh, "[ ] MCTP Transport");
#endif
#ifdef CONFIG_RUNBMC_FRU
    shell_print(sh, "[x] FRU Management");
#else
    shell_print(sh, "[ ] FRU Management");
#endif

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_bmc, SHELL_CMD(info, NULL, "Display BMC information", cmd_bmc_info),
    SHELL_CMD(status, NULL, "Display system status", cmd_bmc_status),
    SHELL_CMD(sensors, NULL, "Display sensor readings", cmd_bmc_sensors),
    SHELL_CMD(telemetry, NULL, "Display telemetry snapshot", cmd_bmc_telemetry),
    SHELL_CMD(features, NULL, "Display compiled features", cmd_bmc_features),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bmc, &sub_bmc, "RunBMC commands", NULL);
