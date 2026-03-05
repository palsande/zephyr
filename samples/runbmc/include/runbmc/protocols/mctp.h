/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * MCTP (Management Component Transport Protocol) - DMTF DSP0236
 */

#ifndef RUNBMC_PROTOCOLS_MCTP_H_
#define RUNBMC_PROTOCOLS_MCTP_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

/* MCTP Version */
#define MCTP_VERSION_1_0 0x01

/* MCTP Endpoint ID (EID) */
#define MCTP_EID_NULL        0x00
#define MCTP_EID_BROADCAST   0xFF
#define MCTP_EID_BMC_DEFAULT 0x08 /* Our BMC's default EID */

/* MCTP Message Types (DSP0239) */
#define MCTP_MSG_TYPE_CONTROL  0x00 /* MCTP Control Protocol */
#define MCTP_MSG_TYPE_PLDM     0x01 /* Platform Level Data Model */
#define MCTP_MSG_TYPE_NCSI     0x02 /* NC-SI over MCTP */
#define MCTP_MSG_TYPE_ETHERNET 0x03 /* Ethernet over MCTP */
#define MCTP_MSG_TYPE_NVME     0x04 /* NVMe MI over MCTP */
#define MCTP_MSG_TYPE_VENDOR   0x7E /* Vendor Defined */

/* MCTP Header Flags */
#define MCTP_HDR_FLAG_SOM 0x80 /* Start of Message */
#define MCTP_HDR_FLAG_EOM 0x40 /* End of Message */
#define MCTP_HDR_FLAG_TO  0x08 /* Tag Owner */

/* MCTP Control Commands */
#define MCTP_CTRL_SET_EID          0x01
#define MCTP_CTRL_GET_EID          0x02
#define MCTP_CTRL_GET_MCTP_VERSION 0x04
#define MCTP_CTRL_GET_MESSAGE_TYPE 0x05

/* MCTP Transport Bindings */
#define MCTP_BINDING_I2C      0x01
#define MCTP_BINDING_UART     0x02
#define MCTP_BINDING_PCIE_VDM 0x03

/* MCTP Limits */
#define MCTP_BASELINE_MTU     64   /* Baseline MTU (bytes) */
#define MCTP_I2C_MTU          68   /* I2C binding MTU */
#define MCTP_MAX_MESSAGE_SIZE 4096 /* Max assembled message */
#define MCTP_MAX_TAG          7    /* 3-bit tag field */

/**
 * MCTP Transport Packet Header
 * This appears in every MCTP packet
 */
struct mctp_hdr {
    uint8_t ver;           /* MCTP version (4 bits) + reserved (4 bits) */
    uint8_t dest_eid;      /* Destination Endpoint ID */
    uint8_t src_eid;       /* Source Endpoint ID */
    uint8_t flags_seq_tag; /* SOM, EOM, Pkt_Seq, TO, Msg_Tag */
} __packed;

/**
 * MCTP Message - assembled from one or more packets
 */
struct mctp_message {
    uint8_t src_eid;                     /* Source EID */
    uint8_t dest_eid;                    /* Destination EID */
    uint8_t msg_type;                    /* Message type */
    uint8_t tag;                         /* Message tag */
    uint16_t len;                        /* Message length */
    uint8_t data[MCTP_MAX_MESSAGE_SIZE]; /* Message payload */
};

/**
 * MCTP Statistics
 */
struct mctp_stats {
    uint32_t tx_packets;      /* Transmitted packets */
    uint32_t rx_packets;      /* Received packets */
    uint32_t tx_messages;     /* Transmitted messages */
    uint32_t rx_messages;     /* Received messages */
    uint32_t tx_errors;       /* Transmission errors */
    uint32_t rx_errors;       /* Reception errors */
    uint32_t assembly_errors; /* Message assembly errors */
};

/**
 * MCTP Control Response Codes
 */
enum mctp_ctrl_completion_code {
    MCTP_CTRL_CC_SUCCESS = 0x00,
    MCTP_CTRL_CC_ERROR = 0x01,
    MCTP_CTRL_CC_INVALID_DATA = 0x02,
    MCTP_CTRL_CC_INVALID_LENGTH = 0x03,
    MCTP_CTRL_CC_NOT_READY = 0x04,
    MCTP_CTRL_CC_UNSUPPORTED_CMD = 0x05,
};

/**
 * Initialize MCTP subsystem
 *
 * @param binding Transport binding to use (MCTP_BINDING_*)
 * @param local_eid Our Endpoint ID
 * @return 0 on success, negative errno on failure
 */
int mctp_init(uint8_t binding, uint8_t local_eid);

/**
 * Send MCTP message
 *
 * @param dest_eid Destination Endpoint ID
 * @param msg_type Message type
 * @param data Message payload
 * @param len Payload length
 * @return 0 on success, negative errno on failure
 */
int mctp_send_message(uint8_t dest_eid, uint8_t msg_type, const uint8_t *data, uint16_t len);

/**
 * Receive MCTP message (blocking with timeout)
 *
 * @param msg Buffer to store received message
 * @param timeout Timeout in milliseconds (K_FOREVER, K_NO_WAIT, or ms)
 * @return 0 on success, negative errno on failure
 */
int mctp_recv_message(struct mctp_message *msg, k_timeout_t timeout);

/**
 * Get MCTP statistics
 *
 * @param stats Pointer to statistics structure to fill
 */
void mctp_get_stats(struct mctp_stats *stats);

/**
 * Get local Endpoint ID
 *
 * @return Local EID
 */
uint8_t mctp_get_local_eid(void);

/**
 * Get MCTP binding type
 *
 * @return Binding type (MCTP_BINDING_*)
 */
uint8_t mctp_get_binding(void);

/**
 * Reset MCTP statistics
 */
void mctp_reset_stats(void);

#endif /* RUNBMC_PROTOCOLS_MCTP_H_ */
