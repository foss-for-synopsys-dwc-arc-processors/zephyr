/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <init.h>
#include <kernel.h>

#include "arch/arc/v2/secureshield/arc_ss_audit_logging.h"
/*!
 * \def MEMBER_SIZE
 *
 * \brief Standard macro to get size of elements in a struct
 */
#define MEMBER_SIZE(type,member) sizeof(((type *)0)->member)

/*!
 * \def LOG_FIXED_FIELD_SIZE
 *
 * \brief Size of the mandatory header fields that are before the info received
 *        from the client partition, i.e.
 *        [TIMESTAMP][IV_COUNTER][PARTITION_ID][SIZE]
 */
#define LOG_FIXED_FIELD_SIZE (MEMBER_SIZE(struct log_hdr, timestamp) + \
							  MEMBER_SIZE(struct log_hdr, iv_counter) + \
							  MEMBER_SIZE(struct log_hdr, thread_id) + \
							  MEMBER_SIZE(struct log_hdr, size))
/*!
 * \def LOG_SIZE
 *
 * \brief Size of the allocated space for the log, in bytes
 *
 * \note Must be a multiple of 8 bytes.
 */
#define LOG_SIZE (1024)

/*!
 * \struct log_vars
 *
 * \brief Contains the state variables associated to the current state of the
 *        audit log
 */
struct log_vars {
	uint32_t first_el_idx; /*!< Index in the log of the first element
							    in chronological order */
	uint32_t last_el_idx;  /*!< Index in the log of the last element
							    in chronological order */
	uint32_t num_records;  /*!< Indicates the number of records
							    currently stored in the log. It has to be
							    zero after a reset, i.e. log is empty */
	uint32_t stored_size;  /*!< Indicates the total size of the items
							    currently stored in the log */
};

#if defined(CONFIG_ARC_SECURE_FIRMWARE)

/*!
 * \var log_buffer
 *
 * \brief The private buffer containing the the log in memory
 *
 * \note Aligned to 4 bytes to keep the wrapping on a 4-byte aligned boundary
 */
__attribute__ ((aligned(4)))
static uint8_t log_buffer[LOG_SIZE] = {0};

/*!
 * \var scratch_buffer
 *
 * \brief Scratch buffers needed to hold plain text (and encrypted, if
 *        available) log items to be added
 */
static uint64_t scratch_buffer[(LOG_SIZE)/8] = {0};

/*!
 * \var log_state
 *
 * \brief Current state variables for the log
 */
static struct log_vars log_state = {0};

/*!
 * \brief Static inline function to get the log buffer ptr from index
 *
 * \param[in] idx Byte index to be converted to pointer in the log buffer
 *
 * \return Pointer at the beginning of the log item in the log buffer
 */
static inline struct log_hdr *GET_LOG_POINTER(const uint32_t idx)
{
	return (struct log_hdr *)( &log_buffer[idx] );
}

/*!
 * \brief Static inline function to get the pointer to the SIZE field
 *
 * \param[in] idx Byte index to which retrieve the corresponding size pointer
 *
 * \return Pointer to the size field in the log item header
 */
static inline uint32_t *GET_SIZE_FIELD_POINTER(const uint32_t idx)
{
	return (uint32_t *) GET_LOG_POINTER( (idx +
					    ((uint32_t) &((struct log_hdr *) 0)->size)) % LOG_SIZE );
}

/*!
 * \brief Static inline function to compute the full log entry size starting
 *        from the value of the size field
 *
 * \param[in] size Size of the log line from which derive the size of the whole
 *                 log item
 *
 * \return Full log item size
 */
static inline uint32_t COMPUTE_LOG_ENTRY_SIZE(const uint32_t size)
{
	return (LOG_FIXED_FIELD_SIZE + size + LOG_MAC_SIZE);
}

/*!
 * \brief Static inline function to get the index to the base of the log buffer
 *        for the next item with respect to the current item
 *
 * \param[in] idx Byte index of the current item in the log
 *
 * \return Index of the next item in the log
 */
static inline uint32_t GET_NEXT_LOG_INDEX(const uint32_t idx)
{
	return (uint32_t) ( (idx + COMPUTE_LOG_ENTRY_SIZE(
							    *GET_SIZE_FIELD_POINTER(idx)) ) % LOG_SIZE );
}

/*!
 * \brief Static function to update the state variables of the log after the
 *        addition of a new log record of a given size
 *
 * \param[in] first_el_idx First element index
 * \param[in] last_el_idx  Last element index
 * \param[in] stored_size  New value of the stored size
 * \param[in] num_records  Number of elements stored
 *
 */
static void _audit_update_state(const uint32_t first_el_idx,
							    const uint32_t last_el_idx,
							    const uint32_t stored_size,
							    const uint32_t num_records)
{
	/* Update the indexes */
	log_state.first_el_idx = first_el_idx;
	log_state.last_el_idx = last_el_idx;

	/* Update the number of records stored */
	log_state.num_records = num_records;

	/* Update the size of the stored records */
	log_state.stored_size = stored_size;
}

/*!
 * \brief Static function to identify the begin and end position for a new write
 *        into the log. It will replace items based on "older entries first"
 *        policy in case not enough space is available in the log
 *
 * \param[in]  size  Size of the record we need to fit
 * \param[out] begin Pointer to the index to begin
 * \param[out] end   Pointer to the index to end
 *
 */
static void _audit_replace_record(const uint32_t size, uint32_t *begin, uint32_t *end)
{
	uint32_t first_el_idx = 0, last_el_idx = 0;
	uint32_t num_items = 0, stored_size = 0;
	uint32_t start_pos = 0, stop_pos = 0;

	/* Retrieve the current state variables of the log */
	first_el_idx = log_state.first_el_idx;
	last_el_idx = log_state.last_el_idx;
	num_items = log_state.num_records;
	stored_size = log_state.stored_size;

	/* If there is not enough size, remove older entries */
	while (size > (LOG_SIZE - stored_size)) {

		/* In case we did a full loop without finding space, reset */
		if (num_items == 0) {
			first_el_idx = 0;
			last_el_idx = 0;
			num_items = 0;
			stored_size = 0;
			break;
		}

		/* Remove the oldest */
		stored_size -= COMPUTE_LOG_ENTRY_SIZE(
							    *GET_SIZE_FIELD_POINTER(first_el_idx) );
		num_items--;
		first_el_idx = GET_NEXT_LOG_INDEX(first_el_idx);
	}

	/* Get the start and stop positions */
	if (num_items == 0) {
		start_pos = first_el_idx;
	} else {
		start_pos = GET_NEXT_LOG_INDEX(last_el_idx);
	}
	stop_pos = ((start_pos + COMPUTE_LOG_ENTRY_SIZE(size)) % LOG_SIZE);

	/* Return begin and end positions */
	*begin = start_pos;
	*end = stop_pos;

	/* Update the state with the new values of variables */
	_audit_update_state(first_el_idx, last_el_idx, stored_size, num_items);
}

/*!
 * \brief Static function to perform memory copying into the log buffer. It
 *        takes into account circular wrapping on the log buffer size.
 *
 * \param[in]  src  Pointer to the source buffer
 * \param[in]  size Size in bytes to be copied
 * \param[out] dest Pointer to the destination buffer
 *
 */
static uint32_t _audit_buffer_copy(const uint8_t *src, const uint32_t size, uint8_t *dest)
{
	uint32_t idx = 0;
	uint32_t dest_idx = (uint32_t)dest - (uint32_t)&log_buffer[0];

	if ((dest_idx >= LOG_SIZE) || (size > LOG_SIZE)) {
		return -ENOSR;
	}

	/* TODO: This can be an optimized copy using uint32_t
	 *       and enforcing the condition that wrapping
	 *       happens only on 4-byte boundaries
	 */

	for (idx = 0; idx < size; idx++) {
		log_buffer[(dest_idx + idx) % LOG_SIZE] = src[idx];
	}

	return 0;
}

/*!
 * \brief Static function to emulate memcpy
 *
 * \param[in]  src  Pointer to the source buffer
 * \param[in]  size Size in bytes to be copied
 * \param[out] dest Pointer to the destination buffer
 *
 */
static uint32_t _audit_memcpy(const uint8_t *src, const uint32_t size, uint8_t *dest)
{
	uint32_t idx = 0;

	for (idx = 0; idx < size; idx++) {
		dest[idx] = src[idx];
	}

	return 0;
}

/*!
 * \brief Static function to format a log entry before the addition to the log
 *
 * \param[in]  record       Pointer to the record to be added
 * \param[in]  thread_id    Value of the partition ID for the partition which
 *                          originated the audit logging request
 * \param[out] buffer       Pointer to the buffer to format
 *
 */
static uint32_t _audit_format_buffer(const struct audit_record *record,
								  const uint32_t thread_id, uint64_t *buffer)
{
	static uint64_t last_timestamp = 0;
	struct log_hdr *hdr = NULL;
	struct log_tlr *tlr = NULL;
	uint32_t size;
	uint8_t idx;
	uint32_t status;

	/* Get the size from the record */
	size = record->size;

	/* Format the scratch buffer with the complete log item */
	hdr = (struct log_hdr *) buffer;

	/* iv_counter is concatenated to timestamp to get a
	 * 12 byte unique IV to be used by the encryption module, and needs
	 * to be increased every time the timestamp didn't change between
	 * consecutive invocations.
	 */
	hdr->timestamp = z_tsc_read();
	if(last_timestamp != hdr->timestamp)
	{
		last_timestamp = hdr->timestamp;
		hdr->iv_counter = 0;
	}
	else
	{
		hdr->iv_counter++;
	}
	hdr->thread_id = thread_id;

	/* Copy the record into the scratch buffer */
	status = _audit_memcpy((const uint8_t *) record,
						  size+4, (uint8_t *) &(hdr->size));
	if (status != 0) {
		return status;
	}

	/* FIXME: The MAC here is just a dummy value for prototyping. It will be
	 *        filled by a call to the crypto interface directly when available.
	 */
	tlr = (struct log_tlr *) ((uint8_t *)hdr + LOG_FIXED_FIELD_SIZE + size);
	for (idx=0; idx<LOG_MAC_SIZE; idx++) {
	    tlr->mac[idx] = idx;
	}

	return 0;
}

/* secure service: audit logging */
/* initialization function
 * @brief audit logging service initialization
 *
 * This function provides the default initialization mechanism for
 * Audit logging secure service.
 */
static int audit_logging_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Clear the log state variables */
	_audit_update_state(0, 0, 0, 0);

	return 0;
}

SYS_INIT(audit_logging_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

uint32_t ss_audit_get_info(uint32_t *num_records, uint32_t *size)
{
	/* Return the number of records that are currently stored */
	*num_records = log_state.num_records;

	/* Return the size of the records currently stored */
	*size = log_state.stored_size;

	return 0;
}

uint32_t ss_audit_get_record_info(const uint32_t record_index,
										   uint32_t *size)
{
	uint32_t start_idx, idx;
	if (record_index >= log_state.num_records) {
		return -EINVAL;
	}

	/* First element to read from the log */
	start_idx = log_state.first_el_idx;

	/* Move the start_idx index to the desired element */
	for (idx = 0; idx < record_index; idx++) {
		start_idx = GET_NEXT_LOG_INDEX(start_idx);
	}

	/* Get the size of the requested record */
	*size = COMPUTE_LOG_ENTRY_SIZE(*GET_SIZE_FIELD_POINTER(start_idx));
	return 0;
}

uint32_t ss_audit_retrieve_record(const uint32_t record_index,
												const struct audit_token *token,
											    const uint32_t buffer_size,
											    uint8_t *buffer)
{
	uint32_t idx, start_idx, record_size;
	uint32_t status;

	/* FixMe: Currently token and token_size parameters are not evaluated
	 *        to check if the removal of the desired record_index is
	 *        authorised
	 */
	if (token != NULL) {
		return -ENOTSUP;
	}

	if (buffer == NULL) {
		return -EINVAL;
	}
	/* Get the size of the record we want to retrieve */
	status = ss_audit_get_record_info(record_index, &record_size);
	if (status != 0) {
		return status;
	}
	/* buffer_size must be enough to hold the requested record */
	if (buffer_size < record_size) {
		return -EINVAL;
	}

	/* First element to read from the log */
	start_idx = log_state.first_el_idx;

	/* Move the start_idx index to the desired element */
	for (idx = 0; idx < record_index; idx++) {
		start_idx = GET_NEXT_LOG_INDEX(start_idx);
	}

	/* Do the copy */
	for (idx = 0; idx < record_size; idx++) {
		buffer[idx] = log_buffer[(start_idx + idx) % LOG_SIZE];
	}

	/* return with the retrieved size */
	return record_size;
}

uint32_t ss_audit_add_record(const struct audit_record *record)
{
	uint32_t start_pos = 0, stop_pos = 0;
	uint32_t first_el_idx = 0, last_el_idx = 0, size = 0;
	uint32_t num_items = 0, stored_size = 0;
	uint32_t thread_id;
	uint32_t status;

	/* parameter check */
	// TODO
	/* access policy check */
	// TODO
	thread_id = (uint32_t) k_current_get();
	/* read the size from the input record */
	size = record->size;
	/* Check that size is a 4-byte multiple as expected */
	if (size % 4) {
		return -ENOTSUP;
	}
	/* Check that the entry to be added is not greater than the
	 * maximum space available
	 */
	if (size > (LOG_SIZE - (LOG_FIXED_FIELD_SIZE+LOG_MAC_SIZE))) {
		return -ENOSR;
	}
	/* Get the size in bytes and num of elements present in the log */
	status = ss_audit_get_info(&num_items, &stored_size);
	if (status !=  0) {
		return status;
	}

	if (num_items == 0) {
		start_pos = 0;
	} else {

		/* The log is not empty, need to decide the candidate position
		 * and invalidate older entries in case there is not enough space
		 */
		_audit_replace_record(COMPUTE_LOG_ENTRY_SIZE(size),
						    &start_pos, &stop_pos);
	}

	/* Format the scratch buffer with the complete log item */
	status = _audit_format_buffer(record, thread_id, &scratch_buffer[0]);
	if (status != 0) {
		return status;
	}

	/* TODO: At this point, encryption should be called if supported */

	/* Do the copy of the log item to be added in the log */
	status = _audit_buffer_copy((const uint8_t *) &scratch_buffer[0],
							   COMPUTE_LOG_ENTRY_SIZE(size),
							   (uint8_t *) &log_buffer[start_pos]);
	if (status != 0) {
		return status;
	}

	/* Retrieve current log state */
	first_el_idx = log_state.first_el_idx;
	num_items = log_state.num_records;
	stored_size = log_state.stored_size;

	/* The last element is the one we just added */
	last_el_idx = start_pos;

	/* Update the number of items and stored size */
	num_items++;
	stored_size += COMPUTE_LOG_ENTRY_SIZE(size);

	/* Update the log state */
	_audit_update_state(first_el_idx, last_el_idx, stored_size, num_items);

	/* TODO: At this point, we would need to update the stored copy in
	 *       persistent storage. Need to define a strategy for this
	 */

	/* Stream to a secure UART if available for the platform and built */
	// audit_uart_redirection(last_el_idx);

	return 0;
}

uint32_t ss_audit_delete_record(const uint32_t record_index,
										 const struct audit_token *token)
{
	uint32_t first_el_idx, size_removed;
	/* FixMe: Currently only the removal of the oldest entry, i.e.
	 *        record_index 0, is supported. This has to be extended
	 *        to support removal of random records
	 */
	if (record_index > 0) {
		return -ENOTSUP;
	}

	/* FixMe: Currently token parameters are not evaluated
	 *        to check if the removal of the desired record_index is
	 *        authorised
	 */
	if (token != NULL) {
		return -ENOTSUP;
	}

	/* Check that the record index to be removed is contained in the log */
	if (record_index >= log_state.num_records) {
		return -EINVAL;
	}

	/* If the log contains just one element, reset the state and return */
	if (log_state.num_records == 1) {

		/* Clear the log state variables */
		_audit_update_state(0, 0, 0, 0);

		return 0;
	}

	/* Get the index to the element to be removed */
	first_el_idx = log_state.first_el_idx;

	/* Get the size of the element that is being removed */
	size_removed = COMPUTE_LOG_ENTRY_SIZE(
								    *GET_SIZE_FIELD_POINTER(first_el_idx));

	/* Remove the oldest entry, it means moving the first element to the
	 * next log index */
	first_el_idx = GET_NEXT_LOG_INDEX(first_el_idx);

	/* Update the state with the new head and decrease the number of records
	 * currently stored and the new size of the stored records */
	log_state.first_el_idx = first_el_idx;
	log_state.num_records--;
	log_state.stored_size -= size_removed;

	return 0;
}

/*
 * @brief secure mpu service
 *
 * offer audit logging service. Normal world can read record but only secure
 * world can write.
 * this service will check the input args and make sure the operations only
 * can be applied to normal address
 */
uint32_t arc_s_service_audit_logging(uint32_t arg1, uint32_t arg2, uint32_t arg3,
			     uint32_t arg4, uint32_t ops)
{
	uint32_t ret = 0;

	switch (ops) {
	case SS_AUDIT_OP_GET_INFO:
		ret = ss_audit_get_info((uint32_t *)arg1, (uint32_t *)arg2);
		break;
	case SS_AUDIT_OP_GET_RECORD_INFO:
		ret = ss_audit_get_record_info(arg1, (uint32_t *)arg2);
		break;
	case SS_AUDIT_OP_RETRIEVE_RECORD:
		ret = ss_audit_retrieve_record(arg1, (struct audit_token *)arg2, arg3, (uint8_t *)arg4);
		break;
	case SS_AUDIT_OP_ADD_RECORD:
		ret = ss_audit_add_record((struct audit_record *)arg1);
		break;
	case SS_AUDIT_OP_DELETE_RECORD:
		ret = ss_audit_delete_record(arg1, (struct audit_token *)arg2);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

#else /* CONFIG_ARC_NORMAL_FIRMWARE */

uint32_t ss_audit_get_info(uint32_t *num_records, uint32_t *size)
{
	return z_arc_s_call_invoke6((uint32_t)num_records, (uint32_t)size, 0, 0,
				    SS_AUDIT_OP_GET_INFO, 0, ARC_S_CALL_AUDIT_LOGGING);
}

uint32_t ss_audit_get_record_info(const uint32_t record_index,
										   uint32_t *size)
{
	return z_arc_s_call_invoke6(record_index, (uint32_t)size, 0, 0,
				    SS_AUDIT_OP_GET_RECORD_INFO, 0, ARC_S_CALL_AUDIT_LOGGING);
}

uint32_t ss_audit_retrieve_record(const uint32_t record_index,
												const struct audit_token *token,
											    const uint32_t buffer_size,
											    uint8_t *buffer)
{
	return z_arc_s_call_invoke6(record_index, (uint32_t)token, buffer_size, (uint32_t)buffer,
				    SS_AUDIT_OP_RETRIEVE_RECORD, 0, ARC_S_CALL_AUDIT_LOGGING);
}

uint32_t ss_audit_add_record(const struct audit_record *record)
{
	// it is not permitted to add record from normal world
	ARG_UNUSED(record);
	return (uint32_t)-EACCES;
}

uint32_t ss_audit_delete_record(const uint32_t record_index,
										 const struct audit_token *token)
{
	return z_arc_s_call_invoke6(record_index, (uint32_t)token, 0, 0,
				    SS_AUDIT_OP_DELETE_RECORD, 0, ARC_S_CALL_AUDIT_LOGGING);
}

#endif /* CONFIG_ARC_SECURE_FIRMWARE */
