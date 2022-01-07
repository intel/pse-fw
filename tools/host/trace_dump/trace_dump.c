/*
 * Copyright (c) 2020-2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

struct shmem_mgnt {
	/* host to PSE, must be (n * CACHE_LINE_SIZE) */
	uint32_t head; /* Loop Buffer Head Index */
	uint32_t padding[7];

	/* PSE to host */
	uint32_t size; /* Loop Buffer Size */
	uint32_t tail; /* Loop Buffer Tail Index */
	uint32_t padding1[6];

	/* Loop Buffer */
	uint8_t lbuf[];
};

#define SHMEM_PAGE_SIZE (4096)
#define TRACE_DUMP_FNAME "channel0_0"
#define PCI_CONFIG_FNAME "/sys/bus/pci/devices/0000:00:1d.0/config"
#define PCI_CONFIG_FSIZE (256)
#define PCI_BAR0_BASE_OFF (0x10)
#define PCI_BAR_BASE_MASK (~(uint64_t)(0xF))
#define ATT_ENTRY_4_MMIO_OFF ((uint64_t)0x100000)
#define PTDUMP_SLEEP_US (100 * 1000)

#define PERROR(fmt, arg...) printf("PTDUMPER ERR: " fmt, ##arg)
#define PINFO(fmt, arg...) printf("PTDUMPER INF: " fmt, ##arg)
#define PPBAR(fmt, arg...) printf(fmt, ##arg)
/* #define PDEBUG(fmt, arg...) printf("PTDUMPER DBG: "fmt, ## arg) */
#define PDEBUG(...) ((void)0)

static int dump_stop = 0;

void sig_handler(int sig_num)
{
	dump_stop = 1;
}

int main(int argc, char *argv[])
{
	int fd, ret;
	FILE *fdump;
	struct shmem_mgnt *shmem;
	uint32_t shmem_size;
	uint8_t buf[PCI_CONFIG_FSIZE];
	uint64_t base;
	uint32_t woff = 0;

	PINFO("PSE Trace Dumper ...\n");

	signal(SIGINT, sig_handler);
	/* Get SHMEM MMIO Base Addr */
	fd = open(PCI_CONFIG_FNAME, O_RDONLY);
	if (fd < 0) {
		PERROR("failed to open file, %s\n", PCI_CONFIG_FNAME);
		return -errno;
	}
	ret = read(fd, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		return -errno;
	}
	close(fd);
	base = *((uint64_t *)&buf[PCI_BAR0_BASE_OFF]);
	PINFO("PCI BAR0 Base = 0x%lx\n", base);
	base = (base & PCI_BAR_BASE_MASK) + ATT_ENTRY_4_MMIO_OFF;
	PINFO("SHMEM Base Address = 0x%lx\n", base);

	/* Get actual SHMEM size from PSE */
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	shmem = mmap(NULL, SHMEM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		     fd, (off_t)base);
	if (shmem == MAP_FAILED) {
		PERROR("mmap failed, base=0x%lx, size=%d\n", base,
		       SHMEM_PAGE_SIZE);
		return -errno;
	}
	shmem_size = sizeof(*shmem) + shmem->size;
	munmap(shmem, SHMEM_PAGE_SIZE);
	if (!shmem_size || (shmem_size % SHMEM_PAGE_SIZE)) {
		PERROR("wrong SHMEM size, 0x%x\n", shmem_size);
		return -EXDEV;
	}
	PINFO("SHMEM size = 0x%x\n", shmem_size);

	shmem = mmap(NULL, shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		     (off_t)base);
	if (shmem == MAP_FAILED) {
		PERROR("mmap failed, base=0x%lx, size=%d\n", base, shmem_size);
		return -errno;
	}
	shmem_size -= sizeof(*shmem);
	close(fd);

	fdump = fopen(TRACE_DUMP_FNAME, "wb");
	if (fdump == NULL) {
		PERROR("failed to open file, %s\n", TRACE_DUMP_FNAME);
		return -errno;
	}

	PINFO("SHMEM start info: (0x%x, 0x%x, 0x%x)\n", shmem->size,
	      shmem->head, shmem->tail);
	PINFO("Press CTRL+C to stop dumping");

	do {
		uint32_t head = shmem->head;
		uint32_t tail = shmem->tail;
		uint32_t len;
		uint8_t lbuf[SHMEM_PAGE_SIZE];
		volatile uint8_t *ptr;
		static int pbar_cnt;

		if (head == tail) {
			/* empty, sleep and wait */
			usleep(PTDUMP_SLEEP_US);
			continue;
		}

		if ((head > shmem->size) || (tail >= shmem->size) ||
		    (shmem->size != shmem_size)) {
			PERROR("wrong SHMEM (%u, %u, %u), %u\n", head, tail,
			       shmem->size, shmem_size);
			break;
		}

		if (!(pbar_cnt++ % 80)) {
			PPBAR("\n %010u:", woff);
		}
		PDEBUG("!!! (0x%x, 0x%x)\n", head, tail);

		/* data size @tail */
		if (head < tail)
			len = tail - head;
		else
			len = shmem_size - head;
		PDEBUG("!!! (0x%x, 0x%x)\n", head, len);

		/* need this operation to flush cache, or cached data
		 * will be written into dump file by fwrite
		 */
		if (len > SHMEM_PAGE_SIZE) {
			len = SHMEM_PAGE_SIZE;
		}
		memcpy(lbuf, &shmem->lbuf[head], len);

		PDEBUG("fwrite(%d, %d)\n", len, woff);
		woff += fwrite(lbuf, 1, len, fdump);
		fflush(fdump);

		head += len;
		if (head > shmem_size) {
			PERROR("out of range!\n");
			return -EFAULT;
		} else if (head == shmem_size)
			head = 0;

		/* update Head Index */
		shmem->head = head;
		PPBAR(".");
		fflush(stdout);
	} while (!dump_stop);

	munmap(shmem, shmem_size);
	fclose(fdump);
	PPBAR("\nPTDUMPER: exit, dumped %d bytes\n", woff);
	return 0;
}
