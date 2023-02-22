
#ifndef __SERVER_H__
#define __SERVER_H__


#include "fmacros.h"
#include "config.h"
#include "solarisfixes.h"
#include "atomicvar.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <syslog.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>

#include "ae.h"      /* Event driven programming library */
#include "sds.h"     /* Dynamic safe strings */
#include "adlist.h"  /* Linked lists */
#include "zmalloc.h" /* total memory usage aware version of malloc/free */
#include "anet.h"    /* Networking the easy way */
#include "util.h"    /* Misc functions useful in many places */
#include "connection.h" /* Connection abstraction */
#include "redisassert.h"
#include "log.h"

typedef long long mstime_t; /* millisecond time type. */
typedef long long ustime_t; /* microsecond time type. */

/* Error codes */
#define C_OK                    0
#define C_ERR                   -1

#define UNUSED(V) ((void) V)

#define CONFIG_BINDADDR_MAX 16/* Anti-warning macro... */
#define ANET_ERR_LEN 256




typedef struct socketFds {
    int fd[CONFIG_BINDADDR_MAX];
    int count;
} socketFds;


struct Server {
    /* General */
    pid_t pid;                  /* Main process pid. */
    pthread_t main_thread_id;         /* Main thread id */
    char *configfile;           /* Absolute config file path, or NULL */
    char *executable;           /* Absolute executable file path. */
    char **exec_argv;           /* Executable argv vector (copy). */
    aeEventLoop *el;
    char *pidfile;              /* PID file path */
    int arch_bits;              /* 32 or 64 depending on sizeof(long) */
    int cronloops;              /* Number of times the cron function run */
    size_t initial_memory_usage; /* Bytes used after initialization. */

    /* Networking */
    int port;                   /* TCP listening port */
    int tls_port;               /* TLS listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    char *bindaddr[CONFIG_BINDADDR_MAX]; /* Addresses we should bind to */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
    char *bind_source_addr;     /* Source address to bind on for outgoing connections */
    char *unixsocket;           /* UNIX socket path */
    unsigned int unixsocketperm; /* UNIX socket permission (see mode_t) */
    socketFds ipfd;             /* TCP socket file descriptors */
    socketFds tlsfd;            /* TLS socket file descriptors */
    int sofd;                   /* Unix socket file descriptor */
    uint32_t socket_mark_id;    /* ID for listen socket marking */
    socketFds cfd;              /* Cluster bus listening socket */
    list *clients;              /* List of active clients */
    list *clients_to_close;     /* Clients to close asynchronously */
    list *clients_pending_write; /* There is to write or install handler. */
    list *clients_pending_read;  /* Client has pending read socket buffers. */
    void *current_client;     /* Current client executing the command. */
    redisAtomic uint64_t next_client_id; /* Next client unique ID. Incremental. */
    redisAtomic long long stat_net_input_bytes; /* Bytes read from network. */
    redisAtomic long long stat_net_output_bytes; /* Bytes written to network. */
    long long stat_numconnections;  /* Number of connections received */
    long long stat_rejected_conn;   /* Clients rejected because of maxclients */
    char neterr[ANET_ERR_LEN];   /* Error buffer for anet.c */
    /* Logging */
    char *logfile;                  /* Path of log file */
    /* time cache */
    redisAtomic time_t unixtime; /* Unix time sampled every cron cycle. */
    time_t timezone;            /* Cached timezone. As set by tzset(). */
    int daylight_active;        /* Currently in daylight saving time. */
    mstime_t mstime;            /* 'unixtime' in milliseconds. */
    ustime_t ustime;            /* 'unixtime' in microseconds. */
    /* Limits */
    unsigned int maxclients;            /* Max number of simultaneous clients */
};


struct client {
    connection *conn;
    sds querybuf;
    size_t qb_pos;
    
};


extern struct Server server;



#endif // __SERVER_H__