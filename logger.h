#ifndef LOGGER_H
#define LOGGER_H

/*
 * logger.h -  Functions for forward messages to syslog
 *
 * include LICENSE
 */
#include <syslog.h>

#include <msglog.h>

/*
 * prototypes
 */
void logger_init( int facility);
void logger_close( void);
int logger(int msgtype, const char *msg);

#endif /* LOGGER_H */
