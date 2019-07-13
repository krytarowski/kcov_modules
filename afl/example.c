/*	$NetBSD$	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioccom.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/kcov.h>

int
main(void)
{
	/* AFL by default uses 64kB SHM region for tracing see:
	 * http://lcamtuf.coredump.cx/afl/technical_details.txt */
	uint64_t size = (1 << 16);
        char *cover, n;
	int i = 0;
        int fd;
        int mode;

        fd = open("/dev/kcov", O_RDWR);
        if (fd == -1)
                err(1, "open");
        if (ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == -1)
                err(1, "ioctl: KCOV_IOC_SETBUFSIZE");
        cover = mmap(NULL, size,
                     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (cover == MAP_FAILED)
                err(1, "mmap");
        mode = KCOV_MODE_TRACE_PC;
        if (ioctl(fd, KCOV_IOC_ENABLE, &mode) == -1)
                err(1, "ioctl: KCOV_IOC_ENABLE");
	/* Start of code to be traced */
        read(-1, NULL, 0);
	printf("#: PRINTF TRACE!\n");
	/* End of traced code */

        if (ioctl(fd, KCOV_IOC_DISABLE) == -1)
                err(1, "ioctl: KCOV_IOC_DISABLE");
	/* Print SHM region after the tracing */
	for (i = 0; i < size; ++i) {
		unsigned char x = (unsigned char)cover[i];
		printf("%03x ", x);
		if (i % 32 == 31) printf("\n");
	}
	printf("\n");
        if (munmap(cover, size) == -1)
                err(1, "munmap");
        close(fd);

        return 0;
}
