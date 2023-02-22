

#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>


/* This is a safe version of localtime() which contains no locks and is
 * fork() friendly. Even the _r version of localtime() cannot be used safely
 * in Redis. Another thread may be calling localtime() while the main thread
 * forks(). Later when the child process calls localtime() again, for instance
 * in order to log something to the Redis log, it may deadlock: in the copy
 * of the address space of the forked process the lock will never be released.
 *
 * This function takes the timezone 'tz' as argument, and the 'dst' flag is
 * used to check if daylight saving time is currently in effect. The caller
 * of this function should obtain such information calling tzset() ASAP in the
 * main() function to obtain the timezone offset from the 'timezone' global
 * variable. To obtain the daylight information, if it is currently active or not,
 * one trick is to call localtime() in main() ASAP as well, and get the
 * information from the tm_isdst field of the tm structure. However the daylight
 * time may switch in the future for long running processes, so this information
 * should be refreshed at safe times.
 *
 * Note that this function does not work for dates < 1/1/1970, it is solely
 * designed to work with what time(NULL) may return, and to support Redis
 * logging of the dates, it's not really a complete implementation. */
static int is_leap_year(time_t year) {
    if (year % 4) return 0;         /* A year not divisible by 4 is not leap. */
    else if (year % 100) return 1;  /* If div by 4 and not 100 is surely leap. */
    else if (year % 400) return 0;  /* If div by 100 *and* not by 400 is not leap. */
    else return 1;                  /* If div by 100 and 400 is leap. */
}

static void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst) {
    const time_t secs_min = 60;
    const time_t secs_hour = 3600;
    const time_t secs_day = 3600*24;

    t -= tz;                            /* Adjust for timezone. */
    t += 3600*dst;                      /* Adjust for daylight time. */
    time_t days = t / secs_day;         /* Days passed since epoch. */
    time_t seconds = t % secs_day;      /* Remaining seconds. */

    tmp->tm_isdst = dst;
    tmp->tm_hour = seconds / secs_hour;
    tmp->tm_min = (seconds % secs_hour) / secs_min;
    tmp->tm_sec = (seconds % secs_hour) % secs_min;

    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
     * where sunday = 0, so to calculate the day of the week we have to add 4
     * and take the modulo by 7. */
    tmp->tm_wday = (days+4)%7;

    /* Calculate the current year. */
    tmp->tm_year = 1970;
    while(1) {
        /* Leap years have one day more. */
        time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
        if (days_this_year > days) break;
        days -= days_this_year;
        tmp->tm_year++;
    }
    tmp->tm_yday = days;  /* Number of day of the current year. */

    /* We need to calculate in which month and day of the month we are. To do
     * so we need to skip days according to how many days there are in each
     * month, and adjust for the leap year that has one more day in February. */
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);

    tmp->tm_mon = 0;
    while(days >= mdays[tmp->tm_mon]) {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }

    tmp->tm_mday = days+1;  /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1900;   /* Surprisingly tm_year is year-1900. */
}

struct LOG {
    char *logfile;
    FILE *logfp;
    int level;
    long int timezone;
    int daylight_active;
};

static int g_init = 0;
static struct LOG g_log;

int logInit(char *logfile) {
    g_init = 1;
    setLogLevel(LL_DEBUG);
    time_t t = time(NULL);
    struct tm *aux = localtime(&t);
    g_log.daylight_active = aux->tm_isdst;

    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    g_log.timezone = tz.tz_minuteswest * 60L;

    g_log.logfile = logfile;
    g_log.logfp = logfile ? fopen(logfile, "a") : NULL;
    if (!g_log.logfp) return -1;
    return 0;
}
void logRelease() {
    if (g_init && g_log.logfp) {
        fflush(g_log.logfp);
        fclose(g_log.logfp);
    }
}

void setLogLevel(int level) {
    g_log.level = level;
}

void serverLog(int level, char *fmt, ...) {
    va_list vl;
    char msg[LOG_BUFFER_SIZE];
    va_start(vl, fmt);
    vsnprintf(msg, sizeof(msg), fmt, vl);
    va_end(vl);

    FILE *fp = g_log.logfp;
    int rawmode = (level & LL_RAW);
    int log_to_stdout = (fp == NULL || level & LL_STDOUT);
    level &= 0xff; /* clear flags */
    if (level < g_log.level) return;

    if (log_to_stdout) fp = stdout;
    if (rawmode) {
        fprintf(fp, "%s", msg);
    } else {
        const char *c = ".-*#";
        char buf[64];
        int off;
        struct timeval tv;
        gettimeofday(&tv,NULL);
        struct tm tm;
        nolocks_localtime(&tm,tv.tv_sec,g_log.timezone, g_log.daylight_active);
        off = strftime(buf,sizeof(buf),"%d %b %Y %H:%M:%S.",&tm);
        snprintf(buf+off,sizeof(buf)-off,"%03d",(int)tv.tv_usec/1000);
        fprintf(fp,"%s %c %s\n", buf,c[level],msg);
    }
    fflush(fp);
}