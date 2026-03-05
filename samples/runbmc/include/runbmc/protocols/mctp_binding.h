/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP Transport Binding Interface
 */

#ifndef RUNBMC_PROTOCOLS_MCTP_BINDING_H_
#define RUNBMC_PROTOCOLS_MCTP_BINDING_H_

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * MCTP Transport Binding Operations
 * Each physical binding (I2C, UART, etc.) implements these
 */
struct mctp_binding_ops {
    /**
     * Initialize the binding
     * @return 0 on success, negative errno on failure
     */
    int (*init)(void);

    /**
     * Send raw MCTP packet
     * @param pkt Packet data (including MCTP header)
     * @param len Packet length
     * @return 0 on success, negative errno on failure
     */
    int (*send_packet)(const uint8_t *pkt, uint16_t len);

    /**
     * Receive raw MCTP packet
     * @param pkt Buffer to store packet
     * @param max_len Maximum buffer size
     * @param timeout Timeout
     * @return Number of bytes received, or negative errno
     */
    int (*recv_packet)(uint8_t *pkt, uint16_t max_len, k_timeout_t timeout);

    /**
     * Get binding MTU (Maximum Transmission Unit)
     * @return MTU in bytes
     */
    uint16_t (*get_mtu)(void);
};

/**
 * Register MCTP transport binding
 *
 * @param ops Binding operations
 * @return 0 on success, negative errno on failure
 */
int mctp_binding_register(const struct mctp_binding_ops *ops);

/* I2C Binding */
#if defined(CONFIG_RUNBMC_MCTP_I2C)
extern const struct mctp_binding_ops mctp_i2c_binding_ops;
#endif

/* UART Binding */
#if defined(CONFIG_RUNBMC_MCTP_UART)
extern const struct mctp_binding_ops mctp_uart_binding_ops;
#endif

#endif /* RUNBMC_PROTOCOLS_MCTP_BINDING_H_ */
