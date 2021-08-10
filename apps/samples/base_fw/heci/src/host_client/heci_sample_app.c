/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <linux/types.h>
#include <linux/uuid.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GET_VERSION 0x1
#define GET_VERSION_RD_LEN 0xC

/* the IOCTL parameters to connect the fw client */
#define IOCTL_HECI_CONNECT_CLIENT \
	_IOWR('H', 0x01, struct HECI_connect_client_data)

/*
 * smhi is FW side sample heci client.
 * SMHI client UUID: bb579a2e-cc54-4450-b1d0-5e7520dcad25
 */
static const uuid_le HECI_client_smhi_guid = UUID_LE(
	0xbb579a2e, 0xcc54, 0x4450, 0xb1, 0xd0, 0x5e,
	0x75, 0x20, 0xdc, 0xad, 0x25);

/*
 * client information struct
 */
struct HECI_client {
	__u32 max_msg_length;
	__u8 protocol_version;
	__u8 reserved[3];
};

/*
 * IOCTL Connect client data structure
 */
struct HECI_connect_client_data {
	union {
		uuid_le in_client_uuid;
		struct HECI_client out_client_properties;
	};
};

/*
 * HECI message header
 */
typedef struct {
	uint8_t command : 7;
	uint8_t is_response : 1;
	uint8_t reserved[2];
	uint8_t status;
} MSG_HEADER;

int heci_client_conn(void)
{
	int fd, result;
	char *device_name;

	/* ish-smhi is FW side sample heci client.*/
	device_name = "/dev/ish-smhi";
	struct HECI_connect_client_data connect_data;

	memcpy(&(connect_data.in_client_uuid), &(HECI_client_smhi_guid),
	       sizeof(HECI_client_smhi_guid));

	/* open handle of sample heci client */
	fd = open(device_name, O_RDWR);
	if (fd > 0) {
		printf("Host client created: fd:%d\n>", fd);
	} else {
		printf("failed to create a new host client!\n>");
		return -1;
	}

	/* connect heci client */
	result = ioctl(fd, IOCTL_HECI_CONNECT_CLIENT, &connect_data);
	if (result == 0) {
		printf("connected to %s\n>", device_name);
	} else {
		printf("Failed to connect to %s\n>", device_name);
		return -1;
	}

	return fd;
}

/* the main function of the heci demo */
int main(void)
{
	int wr_length, rd_length, fd;
	uint8_t *rd_buffer;
	fd_set heci_file_set;
	struct timeval timeout;

	/* connect to fw test client */
	fd = heci_client_conn();
	if (fd == -1) {
		printf("Failed to establish connection with heci client!\n");
		return -1;
	}

	/* prepare sample message to send */
	MSG_HEADER sample_header;

	memset(sample_header.reserved, 0, sizeof(sample_header.reserved));
	sample_header.status = 0;
	sample_header.is_response = 0;
	sample_header.command = GET_VERSION;

	/* send message to fw client */
	wr_length = sizeof(MSG_HEADER);
	printf("HECIcmdwrite: fd:%d, write length:%d\n", fd, wr_length);
	write(fd, (uint8_t *)&sample_header, wr_length);

	/* allocate rx buffer to receive response from fw */
	rd_length = GET_VERSION_RD_LEN;
	rd_buffer = (uint8_t *)calloc(rd_length, sizeof(char));
	if (!rd_buffer) {
		printf("Failed to allocate memory\n");
		return -1;
	}

	FD_ZERO(&heci_file_set);
	FD_SET(fd, &heci_file_set);
	timeout.tv_sec = 10;
	timeout.tv_usec = 00;

	/* read response from fw side */
	printf("HECIcmdread: fd:%d, read length:%d, cmd: version\n", fd,
	       rd_length);
	select(fd + 1, &heci_file_set, NULL, NULL, &timeout);
	if (FD_ISSET(fd, &heci_file_set)) {
		read(fd, rd_buffer, rd_length);
		printf("read length is %d, read content is:\n", rd_length);
		for (int i = 0; i < rd_length; i++) {
			printf("%x.", rd_buffer[i]);
		}
		printf("\n");
	}

	/* free buffer and close connection */
	printf("close this connection\n");
	free(rd_buffer);
	close(fd);

	return 0;
}
