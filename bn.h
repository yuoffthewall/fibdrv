#ifndef _BN_H_
#define _BN_H_
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

#define digits 18  // max digits per big_num node
#define bound 1000000000000000000ULL

MODULE_LICENSE("Dual MIT/GPL");

extern size_t list_len;

typedef struct big_num {
    // unsigned long long upper, lower;
    unsigned long long num;
    struct list_head head;
} big_num;

void new_node(struct list_head *list, unsigned long long num);

void add_big_num(struct list_head *small,
                 struct list_head *large);  // large add to small

struct list_head *fib_big(long long k);

void list_reverse(struct list_head *head);

char *list_to_string(struct list_head *list);

/* ----------- Implementation using char array ----------- */

#define _max(x, y) (x > y ? x : y)
#define _swap(x, y)        \
    ({                     \
        typeof(x) tmp = x; \
        x = y;             \
        y = tmp;           \
    })

typedef struct big_num2 {
    unsigned char *num;
    size_t len;  // strlen
    int sign;
} bn;

bn *bn_alloc(size_t size);

char *reverse_str(char *s);

bn *bn_add(bn *a, bn *b);

// a must > b
bn *bn_sub(bn *a, bn *b);

// s += num start from s[offset]
void bn_mul_add(bn *s, int offset, int num);

// O(n^2) long multiplication
bn *bn_mul(bn *a, bn *b);

void freebn(bn *n);

bn *fast_doubling(long long n);

bn *bcd_encode(bn *ans);

#endif
