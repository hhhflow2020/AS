
#ifndef __NETWORKING_H__
#define __NETWORKING_H__
#include "ae.h"

#define NET_IP_STR_LEN 46 /* INET6_ADDRSTRLEN is 46, but we need to be sure */
#define MAX_ACCEPTS_PER_CALL 1000

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);


#endif // __NETWORKING_H__