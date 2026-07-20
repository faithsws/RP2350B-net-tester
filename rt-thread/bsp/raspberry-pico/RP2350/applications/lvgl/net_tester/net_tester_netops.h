#ifndef NET_TESTER_NETOPS_H
#define NET_TESTER_NETOPS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int net_tester_ops_tcp_probe(const char * host, int port, int timeout_ms,
                             char * out, size_t out_sz);
int net_tester_ops_udp_probe(const char * host, int port, int timeout_ms,
                             char * out, size_t out_sz);
int net_tester_ops_dns_lookup(const char * host, char * out, size_t out_sz);
int net_tester_ops_http_get(const char * host, int port, const char * path,
                            char * out, size_t out_sz);
int net_tester_ops_arp_query(const char * ipstr, char * out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_NETOPS_H */
