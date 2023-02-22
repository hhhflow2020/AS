
#ifndef __SERVER_LOG_H__
#define __SERVER_LOG_H__

#define LOG_BUFFER_SIZE 1024

/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1<<10) /* Modifier to log without timestamp */
#define LL_STDOUT (1<<11) /* stdout */

int logInit(char *logfile);
void logRelease();
void setLogLevel(int level);
void serverLog(int level, char *fmt, ...);

#endif // __SERVER_LOG_H__