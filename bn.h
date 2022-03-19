#ifndef _BN_H_
#define _BN_H_
#include <linux/list.h>
#include <linux/slab.h>

#define digits 18  // max digits per big_num node
#define bound 1000000000000000000ULL

size_t len = 0;

typedef struct big_num {
    // unsigned long long upper, lower;
    unsigned long long num;
    struct list_head head;
} big_num;

void new_node(struct list_head *list, unsigned long long num)
{
    big_num *temp = kmalloc(sizeof(big_num), GFP_KERNEL);
    temp->num = num;
    list_add_tail(&temp->head, list);
    len++;
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
    len = 1;
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
    char *s = kmalloc(digits * len + 1, GFP_KERNEL);
    big_num *node;
    list_reverse(list);
    list_for_each_entry (node, list, head) {
        char buf[digits + 1];
        snprintf(buf, digits + 1, "%018llu", node->num);
        strncat(s, buf, digits);
    }
    return s;
}

#endif
