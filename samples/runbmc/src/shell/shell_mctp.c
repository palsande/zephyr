/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP Shell Commands
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>

#include "runbmc/protocols/mctp.h"

LOG_MODULE_REGISTER(shell_mctp, LOG_LEVEL_INF);

/* mctp info - Show MCTP configuration */
static int cmd_mctp_info(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t eid = mctp_get_local_eid();
    uint8_t binding = mctp_get_binding();
    const char *binding_str;

    switch (binding) {
    case MCTP_BINDING_I2C:
        binding_str = "I2C";
        break;
    case MCTP_BINDING_UART:
        binding_str = "UART";
        break;
    case MCTP_BINDING_PCIE_VDM:
        binding_str = "PCIe VDM";
        break;
    default:
        binding_str = "Unknown";
        break;
    }

    shell_print(sh, "MCTP Configuration:");
    shell_print(sh, "  Local EID:        0x%02x", eid);
    shell_print(sh, "  Binding:          %s (0x%02x)", binding_str, binding);
    shell_print(sh, "  Version:          1.0 (DSP0236)");
    shell_print(sh, "  Max Message Size: %u bytes", MCTP_MAX_MESSAGE_SIZE);

    return 0;
}

/* mctp send - Send test message */
static int cmd_mctp_send(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: mctp send <dest_eid> [message]");
        return -EINVAL;
    }

    /* Parse destination EID */
    uint8_t dest_eid = strtoul(argv[1], NULL, 16);

    /* Build test message */
    const char *msg_str = (argc >= 3) ? argv[2] : "Hello from RunBMC!";
    uint8_t msg_data[256];
    uint16_t msg_len = strlen(msg_str);
    memcpy(msg_data, msg_str, msg_len);

    shell_print(sh, "Sending MCTP message to EID 0x%02x (%u bytes)...", dest_eid, msg_len);

    int ret = mctp_send_message(dest_eid, MCTP_MSG_TYPE_VENDOR, msg_data, msg_len);

    if (ret == 0) {
        shell_print(sh, "Message sent successfully");
    } else {
        shell_error(sh, "Failed to send message: %d", ret);
    }

    return ret;
}

/* mctp recv - Receive message (with timeout) */
static int cmd_mctp_recv(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t timeout_ms = 5000; /* Default 5 seconds */

    if (argc >= 2) {
        timeout_ms = strtoul(argv[1], NULL, 10);
    }

    shell_print(sh, "Waiting for MCTP message (timeout=%u ms)...", timeout_ms);

    struct mctp_message msg;
    int ret = mctp_recv_message(&msg, K_MSEC(timeout_ms));

    if (ret == 0) {
        shell_print(sh, "Message received:");
        shell_print(sh, "  Source EID:   0x%02x", msg.src_eid);
        shell_print(sh, "  Dest EID:     0x%02x", msg.dest_eid);
        shell_print(sh, "  Message Type: 0x%02x", msg.msg_type);
        shell_print(sh, "  Tag:          %u", msg.tag);
        shell_print(sh, "  Length:       %u bytes", msg.len);

        /* Print message data (first 64 bytes as hex) */
        shell_print(sh, "  Data:");
        for (uint16_t i = 0; i < MIN(msg.len, 64); i += 16) {
            char hex_str[64] = {0};
            char *ptr = hex_str;
            for (uint16_t j = 0; j < 16 && (i + j) < msg.len; j++) {
                ptr += sprintf(ptr, "%02x ", msg.data[i + j]);
            }
            shell_print(sh, "    %04x: %s", i, hex_str);
        }

        if (msg.len > 64) {
            shell_print(sh, "    ... (%u more bytes)", msg.len - 64);
        }
    } else if (ret == -EAGAIN) {
        shell_print(sh, "No message received (timeout)");
    } else {
        shell_error(sh, "Failed to receive message: %d", ret);
    }

    return 0;
}

/* mctp stats - Show MCTP statistics */
static int cmd_mctp_stats(const struct shell *sh, size_t argc, char **argv)
{
    struct mctp_stats stats;
    mctp_get_stats(&stats);

    shell_print(sh, "MCTP Statistics:");
    shell_print(sh, "  TX Packets:       %u", stats.tx_packets);
    shell_print(sh, "  RX Packets:       %u", stats.rx_packets);
    shell_print(sh, "  TX Messages:      %u", stats.tx_messages);
    shell_print(sh, "  RX Messages:      %u", stats.rx_messages);
    shell_print(sh, "  TX Errors:        %u", stats.tx_errors);
    shell_print(sh, "  RX Errors:        %u", stats.rx_errors);
    shell_print(sh, "  Assembly Errors:  %u", stats.assembly_errors);

    return 0;
}

/* mctp reset - Reset statistics */
static int cmd_mctp_reset(const struct shell *sh, size_t argc, char **argv)
{
    mctp_reset_stats();
    shell_print(sh, "MCTP statistics reset");
    return 0;
}

/* Register MCTP shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(
    mctp_cmds, SHELL_CMD(info, NULL, "Show MCTP configuration", cmd_mctp_info),
    SHELL_CMD(send, NULL, "Send MCTP message: send <eid> [message]", cmd_mctp_send),
    SHELL_CMD(recv, NULL, "Receive MCTP message: recv [timeout_ms]", cmd_mctp_recv),
    SHELL_CMD(stats, NULL, "Show MCTP statistics", cmd_mctp_stats),
    SHELL_CMD(reset, NULL, "Reset MCTP statistics", cmd_mctp_reset), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(mctp, &mctp_cmds, "MCTP commands", NULL);
