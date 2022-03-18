#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

int main(int argc, char *argv[])
{
    long long sz;  // return value of read() or write() from kernel space

    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    if (argc == 1) {
        for (int i = 0; i <= offset; i++) {
            sz = write(fd, write_buf, strlen(write_buf));
            printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
        }

        char buf[40];
        for (int i = 0; i <= offset; i++) {
            lseek(fd, i, SEEK_SET);
            sz = read(fd, buf, 40);
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, buf);
        }
        for (int i = offset; i >= 0; i--) {
            lseek(fd, i, SEEK_SET);
            sz = read(fd, buf, 40);
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, buf);
        }
    } else {
        struct timespec tt1, tt2;
        // printf("consumes %ld nanoseconds!\n", tt2.tv_nsec - tt1.tv_nsec);
        for (int i = 0; i <= offset; i++) {
            lseek(fd, i, SEEK_SET);
            clock_gettime(CLOCK_REALTIME, &tt1);
            sz = write(fd, write_buf, atoi(argv[1]));
            clock_gettime(CLOCK_REALTIME, &tt2);
            long user_time = tt2.tv_nsec - tt1.tv_nsec;
            long diff = user_time - sz;
            printf("%d %lld %ld, %ld\n", i, sz, user_time, diff);
        }
    }

    close(fd);
    return 0;
}
