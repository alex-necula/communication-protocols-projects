#include "trie.h"
#include <stdio.h>
#include <stdlib.h>

void init_trie(struct trie_node **t, int interface, uint32_t next_hop) {
    *t = (struct trie_node *)malloc(sizeof(struct trie_node));
    if ((*t) == NULL) {
        printf("Memory error\n");
        return;
    }
    (*t)->l = (*t)->r = NULL;
    (*t)->interface = interface;
    (*t)->next_hop = next_hop;
}

void insert_left(struct trie_node **t, int interface, uint32_t next_hop) {
    if ((*t) == NULL) {
        init_trie(t, interface, next_hop);
        return;
    } else {
        insert_left(&((*t)->l), interface, next_hop);
    }
}

void insert_right(struct trie_node **t, int interface, uint32_t next_hop) {
    if ((*t) == NULL) {
        init_trie(t, interface, next_hop);
        return;
    } else {
        insert_right(&((*t)->r), interface, next_hop);
    }
}

int get_bit_count_from_mask(uint32_t mask) {
    return __builtin_popcount(mask);
}

void insert_route(struct trie_node *root, uint32_t prefix, uint32_t mask, uint32_t next_hop, int interface) {
    int cidr = get_bit_count_from_mask(mask);

    prefix = htonl(prefix); // Convert to Big Endian

    for (int i = 0; i < cidr - 1; i++) {
        if (((prefix >> (31 - i)) & 1) == 1) {
            if (root->r == NULL) {
                insert_right(&root, -1, 0);
            }
            root = root->r;
        } else {
            if (root->l == NULL) {
                insert_left(&root, -1, 0);
            }
            root = root->l;
        }
    }

    // Add interface to last bit
    (((prefix >> (32 - cidr)) & 1) == 1) ? insert_right(&root, interface, next_hop)
                                         : insert_left(&root, interface, next_hop);
}

int search_route(struct trie_node *root, uint32_t ip, uint32_t *next_hop) {
    int interface = -1;

    ip = htonl(ip); // Convert to Big Endian

    for (int i = 0; i < 32; i++) {
        (((ip >> (31 - i)) & 1) == 1) ? (root = root->r) : (root = root->l);
        if (root == NULL) {
            break; // end of path
        }
        if (root->interface != -1) {
            interface = root->interface; // found match
            *next_hop = root->next_hop;
        }
    }

    return interface;
}