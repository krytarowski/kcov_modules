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
	 * https://github.com/mirrorer/afl/blob/master/docs/technical_details.txt#L37 */
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
		if (i % 32 == 31) printf("\n");
	
		printf("%03x ", (char)cover[i]);
	}
        if (munmap(cover, size) == -1)
                err(1, "munmap");
        close(fd);

        return 0;
}
