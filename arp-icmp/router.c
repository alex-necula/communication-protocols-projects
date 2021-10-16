#include "router.h"
#include "queue.h"
#include "skel.h"
#include "trie.h"

struct trie_node *root;

struct arp_entry *arp_table;
int arp_table_size;

int get_best_route(uint32_t dest_ip, uint32_t *next_hop) {
    return search_route(root, dest_ip, next_hop);
}

struct arp_entry *get_arp_entry(uint32_t dest_ip) {
    for (int i = 0; i < arp_table_size; i++) {
        if (arp_table[i].ip == dest_ip) {
            return &arp_table[i];
        }
    }
    return NULL;
}

void read_rtable(char *filename) {
    FILE *f;
    f = fopen(filename, "r");
    DIE(f == NULL, "Failed to open rtable file");
    printf("Parsing routing table\n");

    // Initialise trie structure
    init_trie(&root, -1, 0);

    char line[200];
    while (fgets(line, sizeof(line), f)) {
        char prefix_str[50], next_hop_str[50], mask_str[50];
        int interface;

        // Read line
        sscanf(line, "%s %s %s %d", prefix_str, next_hop_str, mask_str, &interface);
        uint32_t prefix = inet_addr(prefix_str);
        uint32_t mask = inet_addr(mask_str);
        uint32_t next_hop = inet_addr(next_hop_str);

        // Insert to trie
        insert_route(root, prefix, mask, next_hop, interface);
    }

    fclose(f);
    printf("Route table successfully read\n");
}

void update_eth_hdr_and_send(packet *m, int interface, uint8_t *dhost) {
    struct ether_header *eth_hdr = (struct ether_header *)m->payload;

    // Update Ethernet address
    get_interface_mac(interface, eth_hdr->ether_shost);
    memcpy(eth_hdr->ether_dhost, dhost, ETH_ALEN);

    // Forward the packet to interface
    printf("Sending packet on interface%d\n", interface);
    send_packet(interface, m);
}

uint16_t add(uint16_t a, uint16_t b) {
    uint16_t res = a + b;
    return res + (res < b); // in case of overflow
}

uint16_t ip_checksum_incremental(uint16_t old_checksum, uint16_t old_field, uint16_t new_field) {
    return ~add(add(~old_checksum, ~old_field), new_field);
}

int main(int argc, char *argv[]) {
    packet m;
    int rc;

    DIE(argc != 5, "Wrong number of arguments");

    // Initialization
    init(argc - 2, argv + 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    arp_table = malloc(sizeof(struct arp_entry) * MAX_TABLE_SIZE);
    DIE(arp_table == NULL, "memory");
    read_rtable(argv[1]);

    queue q = queue_create();

    while (1) {
        rc = get_packet(&m);
        DIE(rc < 0, "get_message");

        struct ether_header *eth_hdr = (struct ether_header *)m.payload;
        struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));

        uint32_t router_addr = inet_addr(get_interface_ip(m.interface));

        // Check if packet is ICMP echo request
        struct icmphdr *icmp_hdr = parse_icmp(m.payload);

        if (icmp_hdr != NULL) {
            if (icmp_hdr->type == ICMP_ECHO && ip_hdr->daddr == router_addr) {
                printf("Received router ICMP echo request\n");
                send_icmp(ip_hdr->saddr, router_addr, eth_hdr->ether_dhost, eth_hdr->ether_shost,
                          ICMP_ECHOREPLY, 0, m.interface,
                          getpid(), icmp_hdr->un.echo.sequence);
                printf("Sent ICMP echo reply\n");
                continue;
            }
        }

        // Check if packet is ARP
        struct arp_header *arp_hdr = parse_arp(m.payload);

        if (arp_hdr != NULL) {
            if (ntohs(arp_hdr->op) == ARPOP_REQUEST) {
                if (arp_hdr->tpa == router_addr) {
                    // ARP request for router
                    printf("Received ARP request for router\n");
                    memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, ETH_ALEN);
                    get_interface_mac(m.interface, eth_hdr->ether_shost);
                    send_arp(arp_hdr->spa, router_addr, eth_hdr, m.interface, ARPOP_REPLY);
                    printf("Sent ARP reply\n");
                } else {
                    // Forward ARP request
                    printf("Received ARP request for someone else\n");
                    uint32_t next_hop;
                    int next_interface = get_best_route(arp_hdr->tpa, &next_hop);
                    if (next_interface == -1 || m.interface == next_interface)
                        continue;
                    send_arp(arp_hdr->tpa, arp_hdr->spa, eth_hdr, next_interface, ARPOP_REQUEST);
                    printf("Forwarded ARP request\n");
                }

                continue;

            } else if (ntohs(arp_hdr->op) == ARPOP_REPLY) {
                // Add entry to ARP table
                printf("Received ARP reply\n");
                arp_table[arp_table_size].ip = arp_hdr->spa;
                memcpy(arp_table[arp_table_size].mac, arp_hdr->sha, ETH_ALEN);
                arp_table_size++;

                if (!queue_empty(q)) {
                    // Dequeue and send packet
                    printf("Dequeueing packet\n");

                    struct queue_entry q_entry;
                    memcpy(&q_entry, (struct queue_entry *)queue_deq(q), sizeof(struct queue_entry));
                    update_eth_hdr_and_send(&(q_entry.m), q_entry.interface, arp_hdr->sha);
                    continue;
                }

                // Forward ARP reply if not for this router
                uint32_t next_hop;
                int next_interface = get_best_route(arp_hdr->tpa, &next_hop);
                if (next_interface == -1 || m.interface == next_interface)
                    continue;
                send_arp(arp_hdr->tpa, arp_hdr->spa, eth_hdr, next_interface, ARPOP_REPLY);
                printf("Forwarded ARP reply\n");
                continue;
            }

            continue;
        }

        // Check the checksum
        uint16_t packet_check = ip_hdr->check;
        ip_hdr->check = 0;
        uint16_t received_check = ip_checksum(ip_hdr, sizeof(struct iphdr));
        if (packet_check == received_check) {
            printf("Checksum OK\n");
        } else {
            printf("Checksum ERROR %d %d\n", packet_check, received_check);
            continue;
        }

        // Check TTL > 1
        if (ip_hdr->ttl > 1) {
            printf("TTL OK\n");
        } else {
            printf("TTL ERROR\n");
            get_interface_mac(m.interface, eth_hdr->ether_dhost);
            send_icmp_error(ip_hdr->saddr, router_addr, eth_hdr->ether_dhost, eth_hdr->ether_shost,
                      ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, m.interface);
            continue;
        }

        // Find best matching route
        uint32_t next_hop;
        int next_interface = get_best_route(ip_hdr->daddr, &next_hop);
        if (next_interface == -1) {
            printf("Route not found\n");
            get_interface_mac(m.interface, eth_hdr->ether_dhost);
            send_icmp_error(ip_hdr->saddr, router_addr, eth_hdr->ether_dhost, eth_hdr->ether_shost,
                      ICMP_DEST_UNREACH, ICMP_NET_UNREACH, m.interface);
            continue;
        }

        // Update TTL and recalculate the checksum using RFC 1624
        ip_hdr->ttl--;
        ip_hdr->check = ip_checksum_incremental(packet_check, ip_hdr->ttl + 1, ip_hdr->ttl);

        // Find matching ARP entry
        struct arp_entry *arp_entry = get_arp_entry(next_hop);

        if (arp_entry == NULL) {
            // Enqueue packet
            printf("Enqueueing packet\n");
            struct queue_entry q_entry;
            memcpy(&(q_entry.m), &m, sizeof(packet));
            q_entry.interface = next_interface;
            queue_enq(q, &q_entry);

            // Send ARP request
            printf("Sending ARP request on interface%d\n", next_interface);

            eth_hdr->ether_type = htons(ETHERTYPE_ARP);
            get_interface_mac(next_interface, eth_hdr->ether_shost);
            memset(eth_hdr->ether_dhost, 0xff, ETH_ALEN); // broadcast

            uint32_t next_interface_addr = inet_addr(get_interface_ip(next_interface));
            send_arp(next_hop, next_interface_addr, eth_hdr, next_interface, ARPOP_REQUEST);
            continue;
        }

        update_eth_hdr_and_send(&m, next_interface, arp_entry->mac);
    }
}
