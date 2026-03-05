/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP Core Protocol Implementation (DMTF DSP0236)
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "runbmc/protocols/mctp.h"
#include "runbmc/protocols/mctp_binding.h"

LOG_MODULE_REGISTER(mctp_core, LOG_LEVEL_INF);

/* MCTP Context */
static struct {
    uint8_t local_eid;                      /* Our Endpoint ID */
    uint8_t binding_type;                   /* Transport binding */
    const struct mctp_binding_ops *binding; /* Binding operations */
    struct mctp_stats stats;                /* Statistics */
    uint8_t next_tag;                       /* Next message tag to use */
    struct k_mutex lock;                    /* Protection for shared data */

    /* Message assembly state */
    struct {
        bool in_progress; /* Assembly in progress */
        uint8_t src_eid;  /* Source EID */
        uint8_t tag;      /* Message tag */
        uint16_t offset;  /* Current offset */
        uint8_t buffer[MCTP_MAX_MESSAGE_SIZE];
    } rx_assembly;

    /* Receive queue */
    struct k_msgq rx_queue;
    char rx_queue_buffer[4 * sizeof(struct mctp_message)];

} mctp_ctx;

/* Helper: Build MCTP header */
static void mctp_build_header(struct mctp_hdr *hdr, uint8_t dest_eid, uint8_t src_eid,
                  uint8_t flags, uint8_t seq, uint8_t tag)
{
    hdr->ver = MCTP_VERSION_1_0;
    hdr->dest_eid = dest_eid;
    hdr->src_eid = src_eid;
    hdr->flags_seq_tag = flags | ((seq & 0x03) << 4) | (tag & 0x07);
}

/* Helper: Parse MCTP header flags */
static inline bool mctp_is_som(uint8_t flags)
{
    return flags & MCTP_HDR_FLAG_SOM;
}
static inline bool mctp_is_eom(uint8_t flags)
{
    return flags & MCTP_HDR_FLAG_EOM;
}
static inline uint8_t mctp_get_seq(uint8_t flags)
{
    return (flags >> 4) & 0x03;
}
static inline uint8_t mctp_get_tag(uint8_t flags)
{
    return flags & 0x07;
}

int mctp_binding_register(const struct mctp_binding_ops *ops)
{
    if (!ops || !ops->init || !ops->send_packet || !ops->recv_packet || !ops->get_mtu) {
        return -EINVAL;
    }

    mctp_ctx.binding = ops;
    return ops->init();
}

int mctp_init(uint8_t binding, uint8_t local_eid)
{
    LOG_INF("Initializing MCTP: binding=%u, EID=0x%02x", binding, local_eid);

    memset(&mctp_ctx, 0, sizeof(mctp_ctx));
    mctp_ctx.local_eid = local_eid;
    mctp_ctx.binding_type = binding;
    mctp_ctx.next_tag = 0;

    k_mutex_init(&mctp_ctx.lock);

    /* Initialize receive queue */
    k_msgq_init(&mctp_ctx.rx_queue, mctp_ctx.rx_queue_buffer, sizeof(struct mctp_message), 4);

    /* Initialize binding - will be registered by platform-specific code */
    if (binding == MCTP_BINDING_I2C) {
#if defined(CONFIG_RUNBMC_MCTP_I2C)
        mctp_binding_register(&mctp_i2c_binding_ops);
#else
        LOG_ERR("I2C binding not configured");
        return -ENOTSUP;
#endif
    } else if (binding == MCTP_BINDING_UART) {
#if defined(CONFIG_RUNBMC_MCTP_UART)
        mctp_binding_register(&mctp_uart_binding_ops);
#else
        LOG_ERR("UART binding not configured");
        return -ENOTSUP;
#endif
    } else {
        LOG_ERR("Unsupported binding: %u", binding);
        return -ENOTSUP;
    }

    LOG_INF("MCTP initialized successfully (MTU=%u bytes)",
        mctp_ctx.binding ? mctp_ctx.binding->get_mtu() : 0);

    return 0;
}

int mctp_send_message(uint8_t dest_eid, uint8_t msg_type, const uint8_t *data, uint16_t len)
{
    if (!mctp_ctx.binding) {
        return -ENODEV;
    }

    if (len > MCTP_MAX_MESSAGE_SIZE) {
        return -EMSGSIZE;
    }

    k_mutex_lock(&mctp_ctx.lock, K_FOREVER);

    uint16_t mtu = mctp_ctx.binding->get_mtu();
    uint16_t payload_per_pkt = mtu - sizeof(struct mctp_hdr) - 1; /* -1 for msg_type */
    uint8_t packet[mtu];
    uint16_t offset = 0;
    uint8_t seq = 0;
    uint8_t tag = mctp_ctx.next_tag;
    int ret = 0;

    mctp_ctx.next_tag = (mctp_ctx.next_tag + 1) & MCTP_MAX_TAG;

    LOG_DBG("Sending message: dest=0x%02x type=0x%02x len=%u tag=%u", dest_eid, msg_type, len,
        tag);

    while (offset < len) {
        uint16_t chunk_len = MIN(len - offset, payload_per_pkt);
        uint8_t flags = MCTP_HDR_FLAG_TO; /* We own the tag */

        if (offset == 0) {
            flags |= MCTP_HDR_FLAG_SOM;
        }
        if (offset + chunk_len >= len) {
            flags |= MCTP_HDR_FLAG_EOM;
        }

        /* Build packet */
        struct mctp_hdr *hdr = (struct mctp_hdr *)packet;
        mctp_build_header(hdr, dest_eid, mctp_ctx.local_eid, flags, seq, tag);

        /* Add message type (only in first packet with SOM) */
        uint8_t *payload = packet + sizeof(struct mctp_hdr);
        uint16_t pkt_len = sizeof(struct mctp_hdr);

        if (flags & MCTP_HDR_FLAG_SOM) {
            *payload++ = msg_type;
            pkt_len++;
        }

        memcpy(payload, data + offset, chunk_len);
        pkt_len += chunk_len;

        /* Send packet */
        ret = mctp_ctx.binding->send_packet(packet, pkt_len);
        if (ret < 0) {
            LOG_ERR("Failed to send packet: %d", ret);
            mctp_ctx.stats.tx_errors++;
            goto out;
        }

        mctp_ctx.stats.tx_packets++;
        offset += chunk_len;
        seq = (seq + 1) & 0x03;
    }

    mctp_ctx.stats.tx_messages++;
    LOG_DBG("Message sent successfully (%u packets)", seq);

out:
    k_mutex_unlock(&mctp_ctx.lock);
    return ret;
}

/* Process received MCTP packet */
static int mctp_process_rx_packet(const uint8_t *pkt, uint16_t len)
{
    if (len < sizeof(struct mctp_hdr) + 1) {
        LOG_ERR("Packet too short: %u bytes", len);
        mctp_ctx.stats.rx_errors++;
        return -EINVAL;
    }

    const struct mctp_hdr *hdr = (const struct mctp_hdr *)pkt;
    uint8_t flags = hdr->flags_seq_tag;
    uint8_t tag = mctp_get_tag(flags);
    uint8_t src_eid = hdr->src_eid;

    const uint8_t *payload = pkt + sizeof(struct mctp_hdr);
    uint16_t payload_len = len - sizeof(struct mctp_hdr);

    LOG_DBG("RX packet: src=0x%02x dst=0x%02x tag=%u SOM=%d EOM=%d len=%u", src_eid,
        hdr->dest_eid, tag, mctp_is_som(flags), mctp_is_eom(flags), payload_len);

    /* Check if packet is for us */
    if (hdr->dest_eid != mctp_ctx.local_eid && hdr->dest_eid != MCTP_EID_BROADCAST) {
        LOG_WRN("Packet not for us (dest=0x%02x)", hdr->dest_eid);
        return -EADDRNOTAVAIL;
    }

    /* Handle single-packet message */
    if (mctp_is_som(flags) && mctp_is_eom(flags)) {
        struct mctp_message msg;
        msg.src_eid = src_eid;
        msg.dest_eid = hdr->dest_eid;
        msg.tag = tag;
        msg.msg_type = *payload++;
        payload_len--;
        msg.len = payload_len;
        memcpy(msg.data, payload, payload_len);

        if (k_msgq_put(&mctp_ctx.rx_queue, &msg, K_NO_WAIT) != 0) {
            LOG_ERR("RX queue full, dropping message");
            mctp_ctx.stats.rx_errors++;
            return -ENOMEM;
        }

        mctp_ctx.stats.rx_messages++;
        LOG_DBG("Single-packet message received: type=0x%02x len=%u", msg.msg_type,
            msg.len);
        return 0;
    }

    /* Handle multi-packet message assembly */
    if (mctp_is_som(flags)) {
        /* Start new message */
        mctp_ctx.rx_assembly.in_progress = true;
        mctp_ctx.rx_assembly.src_eid = src_eid;
        mctp_ctx.rx_assembly.tag = tag;
        mctp_ctx.rx_assembly.offset = 0;

        /* First byte is message type */
        uint8_t msg_type = *payload++;
        payload_len--;

        mctp_ctx.rx_assembly.buffer[mctp_ctx.rx_assembly.offset++] = msg_type;
        memcpy(&mctp_ctx.rx_assembly.buffer[mctp_ctx.rx_assembly.offset], payload,
               payload_len);
        mctp_ctx.rx_assembly.offset += payload_len;

        LOG_DBG("Message assembly started: type=0x%02x", msg_type);
    } else {
        /* Continue existing message */
        if (!mctp_ctx.rx_assembly.in_progress || mctp_ctx.rx_assembly.src_eid != src_eid ||
            mctp_ctx.rx_assembly.tag != tag) {
            LOG_ERR("Unexpected packet in assembly");
            mctp_ctx.stats.assembly_errors++;
            mctp_ctx.rx_assembly.in_progress = false;
            return -EINVAL;
        }

        if (mctp_ctx.rx_assembly.offset + payload_len > MCTP_MAX_MESSAGE_SIZE) {
            LOG_ERR("Message too large");
            mctp_ctx.stats.assembly_errors++;
            mctp_ctx.rx_assembly.in_progress = false;
            return -EMSGSIZE;
        }

        memcpy(&mctp_ctx.rx_assembly.buffer[mctp_ctx.rx_assembly.offset], payload,
               payload_len);
        mctp_ctx.rx_assembly.offset += payload_len;
    }

    /* Check if message complete */
    if (mctp_is_eom(flags)) {
        struct mctp_message msg;
        msg.src_eid = src_eid;
        msg.dest_eid = hdr->dest_eid;
        msg.tag = tag;
        msg.msg_type = mctp_ctx.rx_assembly.buffer[0];
        msg.len = mctp_ctx.rx_assembly.offset - 1;
        memcpy(msg.data, &mctp_ctx.rx_assembly.buffer[1], msg.len);

        mctp_ctx.rx_assembly.in_progress = false;

        if (k_msgq_put(&mctp_ctx.rx_queue, &msg, K_NO_WAIT) != 0) {
            LOG_ERR("RX queue full, dropping assembled message");
            mctp_ctx.stats.rx_errors++;
            return -ENOMEM;
        }

        mctp_ctx.stats.rx_messages++;
        LOG_INF("Multi-packet message assembled: type=0x%02x len=%u", msg.msg_type,
            msg.len);
    }

    return 0;
}

int mctp_recv_message(struct mctp_message *msg, k_timeout_t timeout)
{
    if (!mctp_ctx.binding) {
        return -ENODEV;
    }

    /* First check if we have a message in the queue */
    if (k_msgq_get(&mctp_ctx.rx_queue, msg, K_NO_WAIT) == 0) {
        return 0;
    }

    /* Otherwise, receive and process packets until we get a message */
    uint8_t pkt_buf[256];
    k_timeout_t start_time = K_TIMEOUT_ABS_MS(k_uptime_get() + timeout.ticks);

    while (true) {
        int ret = mctp_ctx.binding->recv_packet(pkt_buf, sizeof(pkt_buf), K_MSEC(100));

        if (ret > 0) {
            mctp_ctx.stats.rx_packets++;
            mctp_process_rx_packet(pkt_buf, ret);

            /* Check if we got a message */
            if (k_msgq_get(&mctp_ctx.rx_queue, msg, K_NO_WAIT) == 0) {
                return 0;
            }
        }

        /* Check timeout */
        if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
            if (k_uptime_get() >= start_time.ticks) {
                return -EAGAIN;
            }
        }
    }

    return -EAGAIN;
}

void mctp_get_stats(struct mctp_stats *stats)
{
    k_mutex_lock(&mctp_ctx.lock, K_FOREVER);
    memcpy(stats, &mctp_ctx.stats, sizeof(struct mctp_stats));
    k_mutex_unlock(&mctp_ctx.lock);
}

uint8_t mctp_get_local_eid(void)
{
    return mctp_ctx.local_eid;
}

uint8_t mctp_get_binding(void)
{
    return mctp_ctx.binding_type;
}

void mctp_reset_stats(void)
{
    k_mutex_lock(&mctp_ctx.lock, K_FOREVER);
    memset(&mctp_ctx.stats, 0, sizeof(mctp_ctx.stats));
    k_mutex_unlock(&mctp_ctx.lock);
}
