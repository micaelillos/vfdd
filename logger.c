/**
 *  \file logger.c
 *  \brief Functions for forward messages, warnings and errors to syslog.
 *
 *  This module provides functions to forward messages to the system log
 *  syslog. These functions need not be called directly, but used as callback
 *  to msg_initlog(), see msglog.c.
 *
 * To log to syslog:
 *  - call msg_initlog
 *  - call logger_init
 * If you change your mind
 *  - call logger_close
 *  - or call msg_set_func( NULL );
 * 
 * include LICENSE
 */

#include <logger.h>

void logger_init( int facility)
{
   if ( ! msglogp ){
      return;
   }
   msg_set_facility(facility);
   if ( facility < 0 ){
      msg_set_facility(LOG_DAEMON);
   }
   if ( ! msg_get_logger_is_open() ){
      openlog( "", LOG_PID, msg_get_facility() );
      msg_set_logger_is_open(1);
      msg_set_func( logger );
      atexit(logger_close);
   }
}
void logger_close( void)
{
   if ( msg_get_logger_is_open() ){
      closelog();
      msg_set_func( NULL );
      msg_set_logger_is_open(0);
   }
}
   
int logger(int msgtype, const char *msg)
{
   int priority = 0;

   switch (msgtype) {
    case MSG_T_INFO:
      priority = LOG_INFO;
      break;
    case MSG_T_WARNING:
      priority = LOG_WARNING;
      break;
    case MSG_T_ERROR:
      priority = LOG_ERR;
      break;
    case MSG_T_FATAL:
       priority = LOG_ALERT;
      break;
   }
   syslog( msg_get_facility() | priority, "%s", msg);
   return 0;
}
