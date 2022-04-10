#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

#define _swap(x, y)        \
    ({                     \
        typeof(x) tmp = x; \
        x = y;             \
        y = tmp;           \
    })

char *reverse_str(unsigned char *s)
{
    int len = strlen(s);
    for (int i = 0; i < len / 2; i++)
        _swap(s[i], s[len - 1 - i]);
    return s;
}

char *bin_to_str(unsigned char *bin, size_t sz)
{
    int len = 2 * sz;
    char *str = malloc(len + 1);
    int i;
    for (i = 0; i < len / 2; i++) {
        char mask_low = 0x0F;
        str[i * 2] = (bin[i] & mask_low) + '0';
        str[i * 2 + 1] = (bin[i] >> 4) + '0';
    }
    str[i * 2] = 0;
    if (str[i * 2 - 1] == '0')
        str[i * 2 - 1] = 0;
    return str;
}

int main(int argc, char *argv[])
{
    long long sz;  // return value of read() or write() from kernel space

    char write_buf[] = "testing writing";

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    if (argc == 1) {
        int offset = 100; /* TODO: try test something bigger than the limit */
        for (int i = 0; i <= offset; i++) {
            sz = write(fd, write_buf, strlen(write_buf));
            printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
        }

        char buf[1024];
        for (int i = 0; i <= offset; i++) {
            lseek(fd, i, SEEK_SET);
            sz = read(fd, buf, 40);
            char *ans = reverse_str(bin_to_str(buf, sz));
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, ans);
        }
        for (int i = offset; i >= 0; i--) {
            lseek(fd, i, SEEK_SET);
            sz = read(fd, buf, 40);
            char *ans = reverse_str(bin_to_str(buf, sz));
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, ans);
        }
    } else {
        struct timespec tt1, tt2;
        // printf("consumes %ld nanoseconds!\n", tt2.tv_nsec - tt1.tv_nsec);
        int offset_test = 10000;
        for (int i = 0; i <= offset_test; i++) {
            lseek(fd, i, SEEK_SET);
            clock_gettime(CLOCK_REALTIME, &tt1);
            sz = write(fd, write_buf, atoi(argv[1]));
            clock_gettime(CLOCK_REALTIME, &tt2);
            long user_time = tt2.tv_nsec - tt1.tv_nsec;
            long diff = user_time - sz;
            printf("%d %lld %ld %ld\n", i, sz, user_time, diff);
        }
    }

    close(fd);
    return 0;
}
