/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP over UART Binding (DMTF DSP0253)
 * For QEMU RISC-V
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "runbmc/protocols/mctp_binding.h"

LOG_MODULE_REGISTER(mctp_uart, LOG_LEVEL_INF);

#if defined(CONFIG_RUNBMC_MCTP_UART)

/* UART MCTP Configuration */
#define MCTP_UART_MTU  64   /* UART binding MTU */
#define MCTP_UART_SYNC 0x7E /* Frame sync byte */
#define MCTP_UART_ESC  0x7D /* Escape byte */

/* Get UART device from devicetree */
#define UART_NODE DT_CHOSEN(zephyr_console)
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);

/* UART RX buffer */
#define RX_BUF_SIZE 256
static struct {
    uint8_t rx_buf[RX_BUF_SIZE];
    uint16_t rx_len;
    bool rx_in_progress;
    struct k_msgq rx_queue;
    char rx_queue_buf[4 * (MCTP_UART_MTU + 10)];
    struct k_sem tx_sem;
} mctp_uart_ctx;

/* UART interrupt handler */
static void mctp_uart_isr(const struct device *dev, void *user_data)
{
    uint8_t byte;

    if (!uart_irq_update(dev)) {
        return;
    }

    while (uart_irq_rx_ready(dev)) {
        if (uart_fifo_read(dev, &byte, 1) != 1) {
            break;
        }

        /* Process received byte */
        if (byte == MCTP_UART_SYNC) {
            if (mctp_uart_ctx.rx_in_progress && mctp_uart_ctx.rx_len > 0) {
                /* End of frame - queue it */
                if (k_msgq_put(&mctp_uart_ctx.rx_queue, mctp_uart_ctx.rx_buf,
                           K_NO_WAIT) != 0) {
                    LOG_WRN("UART RX queue full");
                }
            }
            /* Start new frame */
            mctp_uart_ctx.rx_in_progress = true;
            mctp_uart_ctx.rx_len = 0;
        } else if (mctp_uart_ctx.rx_in_progress) {
            if (mctp_uart_ctx.rx_len < RX_BUF_SIZE) {
                mctp_uart_ctx.rx_buf[mctp_uart_ctx.rx_len++] = byte;
            }
        }
    }
}

static int mctp_uart_init(void)
{
    LOG_INF("Initializing MCTP UART binding");

    k_msgq_init(&mctp_uart_ctx.rx_queue, mctp_uart_ctx.rx_queue_buf, MCTP_UART_MTU + 10, 4);
    k_sem_init(&mctp_uart_ctx.tx_sem, 1, 1);
    mctp_uart_ctx.rx_in_progress = false;
    mctp_uart_ctx.rx_len = 0;

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    /* Configure UART interrupt */
    uart_irq_callback_set(uart_dev, mctp_uart_isr);
    uart_irq_rx_enable(uart_dev);

    LOG_INF("MCTP UART binding initialized (MTU=%u)", MCTP_UART_MTU);
    return 0;
}

static int mctp_uart_send_packet(const uint8_t *pkt, uint16_t len)
{
    if (len > MCTP_UART_MTU) {
        return -EMSGSIZE;
    }

    k_sem_take(&mctp_uart_ctx.tx_sem, K_FOREVER);

    LOG_DBG("UART TX: %u bytes", len);

    /* Send frame: SYNC + packet + SYNC */
    uint8_t sync = MCTP_UART_SYNC;
    uart_fifo_fill(uart_dev, &sync, 1);

    /* Send packet bytes (with byte stuffing if needed) */
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = pkt[i];
        if (byte == MCTP_UART_SYNC || byte == MCTP_UART_ESC) {
            /* Escape special bytes */
            uint8_t esc = MCTP_UART_ESC;
            uart_fifo_fill(uart_dev, &esc, 1);
            byte ^= 0x20;
        }
        uart_fifo_fill(uart_dev, &byte, 1);
    }

    uart_fifo_fill(uart_dev, &sync, 1);

    k_sem_give(&mctp_uart_ctx.tx_sem);
    return 0;
}

static int mctp_uart_recv_packet(uint8_t *pkt, uint16_t max_len, k_timeout_t timeout)
{
    uint8_t frame[MCTP_UART_MTU + 10];
    int ret = k_msgq_get(&mctp_uart_ctx.rx_queue, frame, timeout);

    if (ret != 0) {
        return -EAGAIN;
    }

    /* Decode frame (handle byte unstuffing) */
    uint16_t len = 0;
    bool escaped = false;

    for (uint16_t i = 0; i < mctp_uart_ctx.rx_len && len < max_len; i++) {
        uint8_t byte = mctp_uart_ctx.rx_buf[i];

        if (escaped) {
            pkt[len++] = byte ^ 0x20;
            escaped = false;
        } else if (byte == MCTP_UART_ESC) {
            escaped = true;
        } else {
            pkt[len++] = byte;
        }
    }

    LOG_DBG("UART RX: %u bytes", len);
    return len;
}

static uint16_t mctp_uart_get_mtu(void)
{
    return MCTP_UART_MTU;
}

const struct mctp_binding_ops mctp_uart_binding_ops = {
    .init = mctp_uart_init,
    .send_packet = mctp_uart_send_packet,
    .recv_packet = mctp_uart_recv_packet,
    .get_mtu = mctp_uart_get_mtu,
};

#endif /* CONFIG_RUNBMC_MCTP_UART */
