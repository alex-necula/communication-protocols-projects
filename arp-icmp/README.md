# Router - ARP, ICMP, Forwarding

## Necula Alexandru - 322CD

### Included files

- all files from checker (including skel)
- router source file (.c and .h)
- trie source file (.c and .h)
- modified Makefile

For implementing the router, I followed the guidelines from the statemenent.

### Preprocessing

- reading the routing table and creating a binary trie for Longest Prefix
  Matching
- left side of trie represents a bit of "0"
- right side of trie represents a bit of "1"
- converting mask to CIDR prefix is done using built in x86 operation in O(1)
- any lookup will be done in O(1) when searching the trie (max depth: 32)

- I used the built-in API for sending ARP/ICMP packets

### Infinite loop

- receive a packet
- check if packet is ICMP ECHO request for the router -> send ICMP reply

- check if packet is ARP request for the router -> send ARP reply
- check if packet is ARP reply for the router -> update ARP table, dequeue and
  send packet

- check TTL >= 1 -> otherwise send ICMP timeout
- check checksum -> in case of failure, drop packet
- decrement TTL and update checksum using RFC 1624

- find best matching route using LPM (trie)
- find MAC address of next hop in arp_table -> if not found, queue packet and send ARP
  request, wait for ARP reply
- update Ethernet header and send packet

### Bonus

I implemented the bonus task by calculating the checksum using RFC 1624
