
#include "server.h"
#include "networking.h"

struct Server server;


void closeSocketListeners(socketFds *sfd) {
    int j;
    for (j = 0; j < sfd->count; j++) {
        if (sfd->fd[j] == -1) continue;
        aeDeleteFileEvent(server.el, sfd->fd[j], AE_READABLE);
        close(sfd->fd[j]);
    }
    sfd->count = 0;
}

/* Create an event handler for accepting new connections in TCP or TLS domain sockets.
 * This works atomically for all socket fds */
int createSocketAcceptHandler(socketFds *sfd, aeFileProc *accept_handler) {
    int j;

    for (j = 0; j < sfd->count; j++) {
        if (aeCreateFileEvent(server.el, sfd->fd[j], AE_READABLE, accept_handler,NULL) == AE_ERR) {
            /* Rollback */
            for (j = j-1; j >= 0; j--) aeDeleteFileEvent(server.el, sfd->fd[j], AE_READABLE);
            return C_ERR;
        }
    }
    return C_OK;
}

/* Initialize a set of file descriptors to listen to the specified 'port'
 * binding the addresses specified in the Redis server configuration.
 *
 * The listening file descriptors are stored in the integer array 'fds'
 * and their number is set in '*count'.
 *
 * The addresses to bind are specified in the global server.bindaddr array
 * and their number is server.bindaddr_count. If the server configuration
 * contains no specific addresses to bind, this function will try to
 * bind * (all addresses) for both the IPv4 and IPv6 protocols.
 *
 * On success the function returns C_OK.
 *
 * On error the function returns C_ERR. For the function to be on
 * error, at least one of the server.bindaddr addresses was
 * impossible to bind, or no bind addresses were specified in the server
 * configuration but the function is not able to bind * for at least
 * one of the IPv4 or IPv6 protocols. */
int listenToPort(int port, socketFds *sfd) {
    int j;
    char **bindaddr = server.bindaddr;

    /* If we have no bind address, we don't listen on a TCP socket */
    if (server.bindaddr_count == 0) return C_OK;

    for (j = 0; j < server.bindaddr_count; j++) {
        char* addr = bindaddr[j];
        int optional = *addr == '-';
        if (optional) addr++;
        if (strchr(addr,':')) {
            /* Bind IPv6 address. */
            sfd->fd[sfd->count] = anetTcp6Server(server.neterr,port,addr,server.tcp_backlog);
        } else {
            /* Bind IPv4 address. */
            sfd->fd[sfd->count] = anetTcpServer(server.neterr,port,addr,server.tcp_backlog);
        }
        if (sfd->fd[sfd->count] == ANET_ERR) {
            int net_errno = errno;
            serverLog(LL_WARNING,
                "Warning: Could not create server TCP listening socket %s:%d: %s",
                addr, port, server.neterr);
            if (net_errno == EADDRNOTAVAIL && optional)
                continue;
            if (net_errno == ENOPROTOOPT     || net_errno == EPROTONOSUPPORT ||
                net_errno == ESOCKTNOSUPPORT || net_errno == EPFNOSUPPORT ||
                net_errno == EAFNOSUPPORT)
                continue;

            /* Rollback successful listens before exiting */
            closeSocketListeners(sfd);
            return C_ERR;
        }
        if (server.socket_mark_id > 0) anetSetSockMarkId(NULL, sfd->fd[sfd->count], server.socket_mark_id);
        anetNonBlock(NULL,sfd->fd[sfd->count]);
        anetCloexec(sfd->fd[sfd->count]);
        sfd->count++;
    }
    return C_OK;
}

void initServer() {
    server.pid = getpid();
    const char *clk_msg = monotonicInit();
    serverLog(LL_NOTICE, "monotonic clock: %s", clk_msg);
    server.el = aeCreateEventLoop(server.maxclients);
    if (server.el == NULL) {
        serverLog(LL_WARNING, "Failed creating the event loop. Error message: '%s'", strerror(errno));
        exit(1);
    }
    /* Open the TCP listening socket for the user commands. */
    if (server.port != 0 &&
        listenToPort(server.port,&server.ipfd) == C_ERR) {  // listen
        /* Note: the following log text is matched by the test suite. */
        serverLog(LL_WARNING, "Failed listening on port %u (TCP), aborting.", server.port);
        exit(1);
    }

    if (createSocketAcceptHandler(&server.ipfd, acceptTcpHandler) != C_OK) {    // acceptTcpHandler
        serverPanic("Unrecoverable error creating TCP socket accept handler.");
    }
    
}


int main(int argc, char *argv[]) {
    logInit("./mytemp.log");

    server.executable = getAbsolutePath(argv[0]);
    server.exec_argv = zmalloc(sizeof(char*)*(argc+1));
    server.exec_argv[argc] = NULL;
    for (int i = 0; i < argc; i++) server.exec_argv[i] = zstrdup(argv[i]);

    server.maxclients = 1024;
    server.port = 7000;
    server.tcp_backlog = 511;
    server.bindaddr[0] = "127.0.0.1";
    server.bindaddr_count = 1;

    initServer();


    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    logRelease();
    return 0;
}