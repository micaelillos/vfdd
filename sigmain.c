/*
 * sigmain.c - complement to every main program
 * 
 * include LICENSE
 */
#define _GNU_SOURCE   /* sigaction siginfo_t */
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <msglog.h>
#include <sigmain.h>
#include <strmem.h>

#ifdef USE_BACKTRACE
#include <backtrace.h>
#include <backtrace-supported.h>

#include <symlink.h>

/*
 * local prototypes
 */
void bt_error_callback (void *data, const char *msg, int errnum);

/* */
struct backtrace_state *bt_state;

void bt_error_callback (void *data, const char *msg, int errnum)
{
   msg_error( "ERROR: %s (%d)", msg, errnum);
}

#endif /* USE_BACKTRACE */

static void sigmain_handler (int sig, siginfo_t *siginfo, void *context);

int signalInProgress;
Main_Destroy_FP exit_func;
void * exit_param;

static void sigmain_handler (int sig, siginfo_t *siginfo, void *context)
{
   if (signalInProgress) {
      return;
   }
   signalInProgress = 1;
   
   msg_info( "child terminate pid %ld from PID: %ld, UID: %ld, signal = %d",
             getpid(),
             (long) siginfo->si_pid, (long) siginfo->si_uid, sig );

   if ( sig == SIGPIPE ){ /* 13 */
      signalInProgress = 0;
      return;
   }

#ifdef USE_BACKTRACE
   if ( sig == SIGSEGV ){ /* 11 */
      msg_info( "Calling backtrace_print" );
      backtrace_print ( bt_state, 1, msg_get_msg_fd() );
   }
#endif /* USE_BACKTRACE */
   
   exit_func( exit_param);
   
   fprintf (stderr, " Killed by signal %d.\n", sig);
   exit (-1);
}

int sigmain_sigaction(int sig)
{
   struct sigaction action;

   memset(&action, 0, sizeof(action));
   action.sa_sigaction = sigmain_handler;
   action.sa_flags = SA_SIGINFO;
   if ( sig == SIGSEGV ){
      action.sa_flags |= SA_ONESHOT;
   }
   if (sigaction( sig, &action, NULL) < 0) {
      msg_error("sigaction %s", strerror(errno) );
      return 0;
  }
  return 1;
}

void sigmain_signal_init (Main_Destroy_FP func,  void *user_data,
                         int nochdir, int noclose )
{
   exit_func = func;
   exit_param = user_data;

   if ( noclose >= 0 ){
      if ( daemon(nochdir, noclose) ){ /* chdir close */
         msg_error( "daemon error %s", strerror(errno) );
         return;
      }
   }

   sigmain_sigaction (SIGBUS);
   sigmain_sigaction (SIGSYS);
   sigmain_sigaction (SIGPIPE);
   sigmain_sigaction (SIGINT);
   sigmain_sigaction (SIGHUP);
   sigmain_sigaction (SIGQUIT);
   sigmain_sigaction (SIGSEGV);

#ifdef USE_BACKTRACE
   char *exe = symlink_get_value("/proc/self/exe");
   bt_state = backtrace_create_state ( exe, BACKTRACE_SUPPORTS_THREADS,
                                      bt_error_callback, NULL);
   app_free(exe);
#endif
}
