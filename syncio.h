


#ifndef __SYNCIO_H__
#define __SYNCIO_H__
#include <sys/types.h>

/* Synchronous I/O with timeout */
ssize_t syncWrite(int fd, char *ptr, ssize_t size, long long timeout);
ssize_t syncRead(int fd, char *ptr, ssize_t size, long long timeout);
ssize_t syncReadLine(int fd, char *ptr, ssize_t size, long long timeout);

#endif // __SYNCIO_H__