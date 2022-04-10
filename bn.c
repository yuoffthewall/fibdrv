#include "bn.h"
#include <linux/slab.h>

size_t list_len = 0;

void new_node(struct list_head *list, unsigned long long num)
{
    big_num *temp = kmalloc(sizeof(big_num), GFP_KERNEL);
    temp->num = num;
    list_add_tail(&temp->head, list);
    list_len++;
}

void add_big_num(struct list_head *small,
                 struct list_head *large)  // large add to small
{
    struct list_head **ptr1 = &small->next, **ptr2 = &large->next;
    for (int carry = 0;; ptr1 = &(*ptr1)->next, ptr2 = &(*ptr2)->next) {
        if (*ptr1 == small && *ptr2 == large) {
            if (carry)
                new_node(small, 1);
            break;
        } else if (*ptr1 == small && *ptr2 != large)
            new_node(small, 0);

        big_num *sn = list_entry(*ptr1, big_num, head);
        big_num *ln = list_entry(*ptr2, big_num, head);

        if (ln->num + carry >= bound - sn->num) {
            sn->num = sn->num + ln->num + carry - bound;
            carry = 1;
        } else {
            sn->num = sn->num + ln->num + carry;
            carry = 0;
        }
    }
}

struct list_head *fib_big(long long k)
{
    struct list_head *f[2];
    list_len = 1;
    for (int i = 0; i < 2; i++) {
        f[i] = kmalloc(sizeof(struct list_head), GFP_KERNEL);
        INIT_LIST_HEAD(f[i]);
        new_node(f[i], i);
    }
    for (int i = 2; i <= k; i++) {
        add_big_num(f[(i & 1)], f[((i - 1) & 1)]);
    }
    return f[(k & 1)];
}

void list_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *ptr1 = head, *ptr2 = ptr1->next;
    do {
        ptr2->prev = ptr2->next;
        ptr2->next = ptr1;
        ptr1 = ptr2;
        ptr2 = ptr2->prev;
    } while (ptr1 != head);
}

char *list_to_string(struct list_head *list)
{
    char *s = kmalloc(digits * list_len + 1, GFP_KERNEL);
    big_num *node;
    list_reverse(list);
    list_for_each_entry (node, list, head) {
        char buf[digits + 1];
        snprintf(buf, digits + 1, "%018llu", node->num);
        strncat(s, buf, digits);
    }
    return s;
}

/* ----------- Implementation using char array ----------- */

bn *bn_alloc(size_t size)
{
    bn *temp = kmalloc(sizeof(bn), GFP_KERNEL);
    temp->num = kmalloc(size, GFP_KERNEL);
    memset(temp->num, 0, size);
    temp->len = size - 1;
    return temp;
}

char *reverse_str(char *s)
{
    int len = strlen(s);
    for (int i = 0; i < len / 2; i++)
        _swap(s[i], s[len - 1 - i]);
    return s;
}

bn *bn_add(bn *na, bn *nb)
{
    /*
     * Make sure the string length of 'a' is always greater than
     * the one of 'b'.
     */
    char *a = na->num, *b = nb->num;
    size_t len_a = na->len, len_b = nb->len;
    size_t len = _max(len_a, len_b);

    bn *buf = bn_alloc(len + 2);

    int i, carry = 0;
    for (i = 0; i < len; i++) {
        int n1 = (i < len_a) ? (a[i] - '0') : 0;
        int n2 = (i < len_b) ? (b[i] - '0') : 0;
        int sum = n1 + n2 + carry;
        buf->num[i] = '0' + sum % 10;
        carry = sum / 10;
    }

    if (carry)
        buf->num[i++] = '0' + carry;
    buf->num[i] = 0;
    buf->len = i;

    return buf;
}

// a must > b
bn *bn_sub(bn *na, bn *nb)
{
    /*
     * Make sure the string length of 'a' is always greater than
     * the one of 'b'.
     */
    char *a = na->num, *b = nb->num;
    size_t len_a = na->len, len_b = nb->len;
    if (len_a < len_b) {
        _swap(a, b);
        _swap(len_a, len_b);
    }

    bn *buf = bn_alloc(len_a + 1);

    int i, carry = 0, borrow = 10;
    for (i = 0; i < len_a; i++) {
        int n1 = (i < len_a) ? (a[i] - '0') : 0;
        int n2 = (i < len_b) ? (b[i] - '0') : 0;
        carry = n1 - n2 - carry;
        if (carry < 0) {
            buf->num[i] = carry + borrow + '0';
            carry = 1;
        } else {
            buf->num[i] = carry + '0';
            carry = 0;
        }
    }
    buf->len = len_a;

    return buf;
}

// s += num start from s[offset]
void bn_mul_add(bn *n, int offset, int num)
{
    char *s = n->num;
    int carry = 0;
    for (int i = offset;; i++) {
        int sum = num % 10 + (s[i] ? s[i] - '0' : 0) + carry;
        s[i] = '0' + sum % 10;
        carry = sum / 10;
        num /= 10;
        if (!num && !carry) {  // done
            n->len = i + 1;
            return;
        }
    }
}

// O(n^2) long multiplication
bn *bn_mul(bn *na, bn *nb)
{
    size_t len_a = na->len, len_b = nb->len;
    char *a = na->num, *b = nb->num;
    bn *buf = bn_alloc(len_a + len_b + 1);
    int prod, n1, n2;
    for (int i = 0; i < len_a; i++) {
        for (int j = 0; j < len_b; j++) {
            n1 = a[i] - '0';
            n2 = b[j] - '0';
            prod = n1 * n2;
            bn_mul_add(buf, i + j, prod);
        }
    }

    return buf;
}

void freebn(bn *n)
{
    kfree(n->num);
    kfree(n);
}

bn *fast_doubling(long long n)
{
    int msb = 64 - __builtin_clzll(n);
    bn *a = bn_alloc(2);
    bn *b = bn_alloc(2);
    a->num[0] = '0';  // F(n) = 0
    b->num[0] = '1';  // F(n + 1) = 1

    bn *n2 = bn_alloc(2);
    n2->num[0] = '2';

    for (long long mask = 1 << (msb - 1); mask; mask >>= 1) {
        // k = floor((n >> j) / 2)
        // then a = F(k), b = F(k + 1) now.

        // F(2k) = F(k) * [ 2 * F(k + 1) – F(k) ]
        bn *tmp1 = bn_mul(n2, b), *tmp2 = bn_sub(tmp1, a);
        bn *c = bn_mul(a, tmp2);
        freebn(tmp1);
        freebn(tmp2);

        // F(2k + 1) = F(k)^2 + F(k + 1)^2
        tmp1 = bn_mul(a, a);
        tmp2 = bn_mul(b, b);
        bn *d = bn_add(tmp1, tmp2);
        freebn(tmp1);
        freebn(tmp2);

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
        freebn(a);
        freebn(b);
        if (mask & n) {
            a = d;
            b = bn_add(c, d);
            freebn(c);
        } else {
            a = c;
            b = d;
        }
    }
    freebn(b);
    return a;
}

bn *bcd_encode(bn *ans)
{
    size_t sz = ans->len / 2 + (ans->len & 1) + 1;
    bn *bin = bn_alloc(sz);
    for (int i = 0; i < bin->len; i++) {
        int offset = i * 2;
        bin->num[i] =
            (ans->num[offset] - '0') | ((ans->num[offset + 1] - '0') << 4);
    }
    return bin;
}
