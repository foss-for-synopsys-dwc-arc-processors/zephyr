/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_AUDIT_LOGGING_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_AUDIT_LOGGING_H_

#include <arch/arc/v2/secureshield/arc_secure.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#define SS_AUDIT_OP_GET_INFO          0
#define SS_AUDIT_OP_GET_RECORD_INFO   1
#define SS_AUDIT_OP_RETRIEVE_RECORD   2
#define SS_AUDIT_OP_ADD_RECORD        3
#define SS_AUDIT_OP_DELETE_RECORD     4

/*!
 * \struct log_entry
 *
 * \brief Structure of a single log entry
 *        in the log
 * \details This can't be represented as a
 *          structure because the payload
 *          is of variable size, i.e.
 * |Offset |Name        |
 * |-------|------------|
 * | 0     |TIMESTAMP   |
 * | 8     |IV_COUNTER  |
 * |12     |PARTITION ID|
 * |16     |SIZE        |
 * |20     |RECORD ID   |
 * |24     |PAYLOAD     |
 * |20+SIZE|MAC         |
 *
 * SIZE: at least LOG_MIN_SIZE bytes, known only at runtime. It's the size of
 *       the (RECORD_ID, PAYLOAD) fields
 *
 * MAC_SIZE: known at build time (currently, 4 bytes)
 *
 * At runtime, when adding a record, the value of SIZE has to be checked and
 * must be less than LOG_SIZE - MAC_SIZE - 12 and equal or greater than
 * LOG_MIN_SIZE
 *
 */

/*!
 * \def LOG_MIN_SIZE
 *
 * \brief Minimum size of the encrypted part
 */
#define LOG_MIN_SIZE (4)

/*!
 * \enum audit_tlv_type
 *
 * \brief Possible types for a TLV entry
 *        in payload
 */
// enum audit_tlv_type {
//     TLV_TYPE_ID = 0,
//     TLV_TYPE_AUTH = 1,
//
//     /* This is used to force the maximum size */
//     TLV_TYPE_MAX = INT_MAX
// };

/*!
 * \struct audit_tlv_entry
 *
 * \brief TLV entry structure with a flexible array member
 */
// struct audit_tlv_entry {
//     enum audit_tlv_type type;
//     uint32_t length;
//     uint8_t value[];
// };

/*!
 * \def LOG_MAC_SIZE
 *
 * \brief Size in bytes of the MAC for each entry
 */
#define LOG_MAC_SIZE (4)

/*!
 * \struct audit_record
 *
 * \brief This structure contains the record that is added to the audit log
 *        by the requesting secure service
 */
struct audit_record {
    uint32_t size;      /*!< Size in bytes of the id and payload fields */
    uint32_t id;        /*!< ID of the record */
    uint8_t  payload[]; /*!< Flexible array member for payload */
};

/*!
 * \struct log_hdr
 *
 * \brief Fixed size header for a log record
 */
struct log_hdr {
    uint64_t timestamp;
    uint32_t iv_counter;
    uint32_t thread_id;
    uint32_t size;
    uint32_t id;
};

/*!
 * \struct log_tlr
 *
 * \brief tailing with Message authentication code (MAC)
 */
struct log_tlr {
    uint8_t mac[LOG_MAC_SIZE];
};

struct audit_token {
	uint32_t token_size;
	uint8_t *token;
};

/*!
 * \def LOG_HDR_SIZE
 *
 * \brief Size in bytes of the (fixed) header for each entry
 */
#define LOG_HDR_SIZE (sizeof(struct log_hdr))

/*!
 * \def LOG_TLR_SIZE
 *
 * \brief Size in bytes of the (fixed) trailer for each entry
 */
#define LOG_TLR_SIZE (sizeof(struct log_tlr))
/*!
 * \struct log_tlr
 *
 * \brief Fixed size logging entry trailer
 */

uint32_t ss_audit_get_info(uint32_t *num_records, uint32_t *size);

uint32_t ss_audit_get_record_info(const uint32_t record_index, uint32_t *size);

uint32_t ss_audit_retrieve_record(const uint32_t record_index,
							   const struct audit_token *token,
							   const uint32_t buffer_size, uint8_t *buffer);

uint32_t ss_audit_add_record(const struct audit_record *record);

uint32_t ss_audit_delete_record(const uint32_t record_index,
							 const struct audit_token *token);


#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /*ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_AUDIT_LOGGING_H_ */
