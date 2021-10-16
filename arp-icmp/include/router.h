#pragma once
#include <stdint.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include "skel.h"

#define MAX_TABLE_SIZE 100

struct arp_entry {
	uint32_t ip;
	uint8_t mac[ETH_ALEN];
};

struct queue_entry {
	packet m;
	int interface;
};

/**
 * @brief Get the arp entry object
 *
 * @param dest_ip
 * @return Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
 for the given dest_ip or NULL if there is no matching entry.
 */
struct arp_entry *get_arp_entry(uint32_t dest_ip);

/**
 * @brief Returns the best route interface (-1 if not found)
 *
 * @param dest_ip
 * @param next_hop return value if next hop found
 * @return interface or -1 if not found
 */
int get_best_route(uint32_t dest_ip, uint32_t *next_hop);

/**
 * @brief Reads the routing table from text file
 *
 * @param rtable
 * @param filename
 */
void read_rtable(char* filename);

/**
 * @brief Updates ethernet header and sends packet
 *
 * @param m packet
 * @param interface to be sent to
 * @param dhost destination MAC address
 */
void update_eth_hdr_and_send(packet *m, int interface, uint8_t *dhost);

/**
 * @brief Adds 2 short numbers (2 bytes) - handles overflow
 *
 * @param a
 * @param b
 * @return uint16_t
 */
uint16_t add(uint16_t a, uint16_t b);

/**
 * @brief RFC 1624 (Incremental Internet Checksum)
 * HC' = ~(~HC + ~m + m')
 *
 * @param old_checksum HC
 * @param old_field m
 * @param new_field m'
 * @return uint16_t HC'
 */
uint16_t ip_checksum_incremental(uint16_t old_checksum, uint16_t old_field, uint16_t new_field);


