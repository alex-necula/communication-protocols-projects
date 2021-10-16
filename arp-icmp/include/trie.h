#pragma once
#include <arpa/inet.h>

struct trie_node {
    int interface;
    uint32_t next_hop;
    struct trie_node *l;
    struct trie_node *r;
};

/**
 * @brief Allocates memory for node
 *
 * @param t pointer where node will be allocated
 * @param x element to fill in node
 */
void init_trie(struct trie_node **t, int interface, uint32_t next_hop);

/**
 * @brief Insert element to left of node t
 *
 * @param t root
 * @param x element to add to left
 */
void insert_left(struct trie_node **t, int interface, uint32_t next_hop);

/**
 * @brief Insert element to right of node t
 *
 * @param t root
 * @param x element to add to right
 */
void insert_right(struct trie_node **t, int interface, uint32_t next_hop);

/**
 * @brief Get the bit count from mask
 *
 * @param mask
 * @return int - CIDR prefix
 */
int get_bit_count_from_mask(uint32_t mask);

/**
 * @brief Inserts route to trie
 *
 * @param root of trie
 * @param prefix
 * @param mask
 * @param next_hop
 * @param interface
 */
void insert_route(struct trie_node *root, uint32_t prefix, uint32_t mask, uint32_t next_hop, int interface);

/**
 * @brief Searches for best match in trie
 *
 * @param root
 * @param ip
 * @param next_hop return value if found next hop
 * @return -1 if not found, corresponding interface otherwise
 */
int search_route(struct trie_node *root, uint32_t ip, uint32_t *next_hop);

