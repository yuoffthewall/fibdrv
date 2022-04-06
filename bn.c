#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _max(x, y) (x > y ? x : y)
#define _swap(x, y)        \
    ({                     \
        typeof(x) tmp = x; \
        x = y;             \
        y = tmp;           \
    })

char *bn_alloc(size_t size)
{
    char *temp = malloc(size);
    memset(temp, 0, size);
    return temp;
}

static char *reverse_str(char *s)
{
    int len = strlen(s);
    for (int i = 0; i < len / 2; i++)
        _swap(s[i], s[len - 1 - i]);
    return s;
}

static char *bn_add(char *a, char *b)
{
    /*
     * Make sure the string length of 'a' is always greater than
     * the one of 'b'.
     */
    size_t size_a = strlen(a), size_b = strlen(b);
    size_t len = _max(size_a, size_b);

    char *buf = bn_alloc(len + 2);

    int i, carry = 0;
    for (i = 0; i < len; i++) {
        int n1 = (i < size_a) ? (a[i] - '0') : 0;
        int n2 = (i < size_b) ? (b[i] - '0') : 0;
        int sum = n1 + n2 + carry;
        buf[i] = '0' + sum % 10;
        carry = sum / 10;
    }

    if (carry)
        buf[i++] = '0' + carry;
    buf[i] = 0;

    return buf;
}

// a must > b
static char *bn_sub(char *a, char *b)
{
    /*
     * Make sure the string length of 'a' is always greater than
     * the one of 'b'.
     */
    size_t size_a = strlen(a), size_b = strlen(b);
    if (size_a < size_b) {
        _swap(a, b);
        _swap(size_a, size_b);
    }

    char *buf = bn_alloc(size_a + 1);

    int i, carry = 0, borrow = 10;
    for (i = 0; i < size_a; i++) {
        int n1 = (i < size_a) ? (a[i] - '0') : 0;
        int n2 = (i < size_b) ? (b[i] - '0') : 0;
        carry = n1 - n2 - carry;
        if (carry < 0) {
            buf[i] = carry + borrow + '0';
            carry = 1;
        } else {
            buf[i] = carry + '0';
            carry = 0;
        }
    }

    return buf;
}

// s += num start from s[offset]
static void bn_mul_add(char *s, int offset, int num)
{
    int carry = 0;
    for (int i = offset;; i++) {
        int sum = num % 10 + (s[i] ? s[i] - '0' : 0) + carry;
        s[i] = '0' + sum % 10;
        carry = sum / 10;
        num /= 10;
        if (!num && !carry)  // done
            return;
    }
}

// O(n^2) long multiplication
static char *bn_mul(char *a, char *b)
{
    size_t size_a = strlen(a), size_b = strlen(b);
    char *buf = bn_alloc(size_a + size_b + 1);
    for (int i = 0; i < size_a; i++) {
        for (int j = 0; j < size_b; j++) {
            int n1 = a[i] - '0';
            int n2 = b[j] - '0';
            int prod = n1 * n2;
            bn_mul_add(buf, i + j, prod);
        }
    }

    return buf;
}

static char *fast_doubling(long long n)
{
    int msb = 64 - __builtin_clzll(n);
    char *a = bn_alloc(2);
    char *b = bn_alloc(2);
    a[0] = '0';  // F(n) = 0
    b[0] = '1';  // F(n + 1) = 1

    for (long long mask = 1 << (msb - 1); mask; mask >>= 1) {
        // k = floor((n >> j) / 2)
        // then a = F(k), b = F(k + 1) now.

        // F(2k) = F(k) * [ 2 * F(k + 1) – F(k) ]
        char *tmp1 = bn_mul("2", b), *tmp2 = bn_sub(tmp1, a);
        char *c = bn_mul(a, tmp2);
        free(tmp1);
        free(tmp2);

        // F(2k + 1) = F(k)^2 + F(k + 1)^2
        tmp1 = bn_mul(a, a);
        tmp2 = bn_mul(b, b);
        char *d = bn_add(tmp1, tmp2);
        free(tmp1);
        free(tmp2);

        // char *c = bn_mul(a, bn_sub(bn_mul("2", b), a));
        // char *d = bn_add(bn_mul(a, a),  bn_mul(b, b));

        /*
         * n >> j is odd: n >> j = 2k + 1
         * F(n >> j) = F(2k + 1)
         * F(n >> j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
         *
         * n >> j is even: n >> j = 2k
         * F(n >> j) = F(2k)
         * F(n >> j + 1) = F(2k + 1)
         */
        free(a);
        free(b);
        if (mask & n) {
            a = d;
            b = bn_add(c, d);
            free(c);
        } else {
            a = c;
            b = d;
        }
    }
    free(b);
    return a;
}

int main(int argc, char **argv)
{
    /*
    char *n1 = "70", *n2 = "73";
    char *a = malloc(strlen(n1) + 1);
    char *b = malloc(strlen(n2) + 1);
    strncpy(a, n1, strlen(n1));
    strncpy(b, n2, strlen(n2));
    reverse_str(a);
    reverse_str(b);
    // printf("%s\n", reverse_str(bn_mul("2", "8")));
    */
    char *ans = fast_doubling(atoll(argv[1]));
    printf("%s\n", reverse_str(ans));
    // free(a);
    // free(b);
    free(ans);
}
