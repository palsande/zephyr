/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP over I2C Binding (DMTF DSP0237)
 * For STM32F4 Discovery
 *
 * NOTE: Currently using SIMULATED I2C for testing without real hardware.
 *       Real I2C hardware write code is commented out below.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "runbmc/protocols/mctp_binding.h"

LOG_MODULE_REGISTER(mctp_i2c, LOG_LEVEL_INF);

#if defined(CONFIG_RUNBMC_MCTP_I2C)

/* I2C MCTP Configuration */
#define MCTP_I2C_ADDR         0x1D /* Our I2C slave address (7-bit) */
#define MCTP_I2C_DEST_ADDR    0x20 /* Destination I2C address (example) */
#define MCTP_I2C_COMMAND_CODE 0x0F /* MCTP command code */
#define MCTP_I2C_MTU          68   /* I2C binding MTU */

/* Get I2C device from devicetree */
#if DT_NODE_HAS_STATUS(DT_ALIAS(mctp_i2c), okay)
#define I2C_NODE DT_ALIAS(mctp_i2c)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
/* Fallback: use i2c1 if available */
#define I2C_NODE DT_NODELABEL(i2c1)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
#else
static const struct device *i2c_dev = NULL;
#warning "No I2C device found for MCTP, using simulated I2C"
#endif

/* I2C packet buffer */
static struct {
    uint8_t rx_buf[MCTP_I2C_MTU + 10];
    struct k_sem rx_sem;
    bool has_data;
} mctp_i2c_ctx;

static int mctp_i2c_init(void)
{
    LOG_INF("Initializing MCTP I2C binding (addr=0x%02x)", MCTP_I2C_ADDR);

    k_sem_init(&mctp_i2c_ctx.rx_sem, 0, 1);
    mctp_i2c_ctx.has_data = false;

    if (i2c_dev == NULL) {
        LOG_WRN("No I2C device available - using simulated I2C");
        LOG_INF("MCTP I2C binding initialized (MTU=%u, mode=SIMULATED)", MCTP_I2C_MTU);
        return 0;
    }

    if (!device_is_ready(i2c_dev)) {
        LOG_WRN("I2C device not ready - using simulated I2C");
        LOG_INF("MCTP I2C binding initialized (MTU=%u, mode=SIMULATED)", MCTP_I2C_MTU);
        return 0;
    }

    LOG_INF("I2C device ready at %s", i2c_dev->name);
    LOG_INF("MCTP I2C binding initialized (MTU=%u, mode=SIMULATED)", MCTP_I2C_MTU);

    /* Note: I2C slave mode configuration would go here in production */

    return 0;
}

static int mctp_i2c_send_packet(const uint8_t *pkt, uint16_t len)
{
    if (len > MCTP_I2C_MTU) {
        return -EMSGSIZE;
    }

    LOG_DBG("I2C TX: %u bytes", len);

    /* ========================================================================
     * SIMULATED I2C SEND (for testing without real MCTP hardware)
     * ========================================================================
     * This simulates successful I2C transmission by logging packet details.
     * No actual I2C hardware write is performed.
     */

    LOG_INF("I2C TX (simulated): %u bytes to EID 0x%02x", len, pkt[1]);

    /* Decode and display MCTP header for debugging */
    if (len >= 4) {
        LOG_DBG("  MCTP Header:");
        LOG_DBG("    Version:     0x%02x", pkt[0] >> 4);
        LOG_DBG("    Dest EID:    0x%02x", pkt[1]);
        LOG_DBG("    Src EID:     0x%02x", pkt[2]);
        LOG_DBG("    Flags:       0x%02x (SOM=%d EOM=%d Seq=%d Tag=%d)", pkt[3],
            !!(pkt[3] & 0x80),    /* SOM */
            !!(pkt[3] & 0x40),    /* EOM */
            (pkt[3] >> 4) & 0x03, /* Packet Seq */
            pkt[3] & 0x07);       /* Tag */
    }

    /* Show first 16 bytes of packet as hex dump for verification */
    LOG_HEXDUMP_DBG(pkt, MIN(len, 16), "Packet");

    /* Simulate successful transmission */
    LOG_DBG("I2C TX simulated successfully");

    /* ========================================================================
     * REAL I2C HARDWARE WRITE (commented out - enable in production)
     * ========================================================================
     * Uncomment this section when you have real MCTP I2C devices connected.
     *
     * I2C MCTP Packet Format (DSP0237):
     * Byte 0:   Destination I2C slave address (7-bit)
     * Byte 1:   Command code (0x0F for MCTP)
     * Byte 2:   Byte count (MCTP packet length)
     * Byte 3-N: MCTP packet (header + payload)
     */

#if 0 /* Set to 1 to enable real I2C hardware write */
    if (i2c_dev != NULL && device_is_ready(i2c_dev)) {
        uint8_t i2c_pkt[MCTP_I2C_MTU + 3];

        /* Build I2C MCTP packet per DSP0237 */
        i2c_pkt[0] = MCTP_I2C_DEST_ADDR;      /* Destination I2C address */
        i2c_pkt[1] = MCTP_I2C_COMMAND_CODE;   /* MCTP command code (0x0F) */
        i2c_pkt[2] = len;                      /* Byte count */
        memcpy(&i2c_pkt[3], pkt, len);        /* MCTP packet */

        /* Write to I2C bus (skip destination address byte) */
        int ret = i2c_write(i2c_dev, &i2c_pkt[1], len + 2, i2c_pkt[0]);
        if (ret < 0) {
            LOG_ERR("I2C write failed: %d", ret);
            return ret;
        }

        LOG_DBG("I2C write successful to device 0x%02x", MCTP_I2C_DEST_ADDR);
        return 0;
    }
#endif

    /* Simulated success */
    return 0;
}

static int mctp_i2c_recv_packet(uint8_t *pkt, uint16_t max_len, k_timeout_t timeout)
{
    /* ========================================================================
     * SIMULATED I2C RECEIVE (for testing without real MCTP hardware)
     * ========================================================================
     * Currently returns -EAGAIN (no data) since we have no real I2C devices.
     */

    LOG_DBG("I2C RX: waiting for packet (simulated, will timeout)");

    /* For simulation, just wait for timeout then return no data */
    if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
        k_sleep(K_MSEC(100));
    } else {
        k_sleep(timeout);
    }

    /* No data available in simulation */
    return -EAGAIN;

    /* ========================================================================
     * REAL I2C HARDWARE READ (commented out - enable in production)
     * ========================================================================
     * In production with I2C slave mode:
     * 1. Configure I2C in slave mode at MCTP_I2C_ADDR (0x1D)
     * 2. Set up interrupt handler for I2C slave events
     * 3. When data arrives, parse I2C MCTP packet format:
     *    - Byte 0: Command code (should be 0x0F)
     *    - Byte 1: Byte count
     *    - Byte 2-N: MCTP packet
     * 4. Copy MCTP packet to buffer and signal semaphore
     * 5. This function waits on semaphore and returns packet
     *
     * Example pseudo-code:
     *
     * if (k_sem_take(&mctp_i2c_ctx.rx_sem, timeout) == 0) {
     *     uint16_t pkt_len = mctp_i2c_ctx.rx_buf[1];  // Byte count
     *     if (pkt_len <= max_len) {
     *         memcpy(pkt, &mctp_i2c_ctx.rx_buf[2], pkt_len);
     *         return pkt_len;
     *     }
     * }
     * return -EAGAIN;
     */
}

static uint16_t mctp_i2c_get_mtu(void)
{
    return MCTP_I2C_MTU;
}

const struct mctp_binding_ops mctp_i2c_binding_ops = {
    .init = mctp_i2c_init,
    .send_packet = mctp_i2c_send_packet,
    .recv_packet = mctp_i2c_recv_packet,
    .get_mtu = mctp_i2c_get_mtu,
};

#endif /* CONFIG_RUNBMC_MCTP_I2C */
