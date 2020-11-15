/*
 * vfddmain.c - main for vfdd daemon
 * 
 * include LICENSE
 */
#define _GNU_SOURCE    /* unistd.h daemon */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libgen.h>        /* basename */

#include <msglog.h>
#include <duprintf.h>
#include <logger.h>
#include <sigmain.h>

#include <vfdd.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

typedef struct _UserData UserData;
/*
 * local prototypes
 */
int vfddmain_process(UserData *ud );
void vfddmain_destroy(void *aps);
void usage(UserData *ud);
/* */

#define APPVFDDRC_FILE "/etc/vfdd.conf"
const char version[] = PJVERSION;


struct _UserData {
   char *prog;        /* program name */
   MsgFunc logfunc;   /* logger function */
   int verbose;       /* verbose level */
   int do_fork;       /* if set, go in background */
   int serverMode;    /* standalone, inetd  */
   char *log_file;    /* filename to log messages */
   char *conffile;    /* configuration filename  */
   char *word;   /* define the word to print */
   int   colon; /* should I show a colon */
   Vfdd *vfdd;        /* vfdd object  */
};

UserData *userData;

void usage(UserData *ud)
{
   fprintf( stderr,
"vfdd is a demon program to drive a led display\n"
"vfdd [options]\n"
"  -v              : verbose\n"
"  -dm level list  : set debug mask : -dm 8,9\n"
"  -h              : print this help message\n"
"  -C <conffile>   : read configuration from conffile -default %s\n"
"  -D              : daemonize the process\n"
"  -F <facility>   : log facility number 0-23: 3 = daemon, 16 local0, ...\n"
"  -L <log_file>   : log messages to file instead of syslog\n"
"  -V              |\n"
"  --version       : print version number and exit.\n"
"  ex: vfdd -D     : run in background\n",
    ud->conffile );
}

int main(int argc, char **argv, char **envp)
{
   int i ;
   int ret = 0;
   UserData *ud;
   int log_facility = LOG_DAEMON;

//   tmem_init( NULL );
   ud = userData = app_new0(UserData, 1);
   ud->prog = basename(argv[0]);
   ud->serverMode = LM_STANDALONE;
   ud->word = argv[1];
   ud->colon = atoi(argv[2]);
     printf("%s word \n", argv[1]);
 int num = atoi(argv[2]);
  printf("%d colon \n", num);
   msg_initlog( ud->prog , MSG_F_NO_DATE | MSG_F_COLOR, NULL, NULL );
    printf("%d net \n", atoi(argv[3]));
    if(atoi(argv[3]) == 1)
   ud->conffile = app_strdup( "/etc/net.conf" );
    else
   ud->conffile = app_strdup( "/etc/vfdd.conf" );

   for (i = 1 ; i < argc ; i++) {
      if (*argv[i] == '-') {
         if (strcmp(argv[i], "-d") == 0) {
            prog_debug++ ;
         } else if (strcmp(argv[i], "-dm") == 0) {
            /* set dbg mask: -dm 8,9 */
            prog_debug++ ;
#ifdef MSG_DEBUG
            msg_set_dbg_msk_str(argv[++i]);
#endif 
         } else if (strcmp(argv[i], "-C") == 0) {
             app_dup_str(&ud->conffile, argv[++i]);
         } else if (strcmp(argv[i], "-F") == 0) {
            log_facility = atoi(argv[++i]) << 3;
         } else if (strcmp(argv[i], "-L") == 0) {
            ud->log_file = app_strdup(argv[++i]);
         } else if (strcmp(argv[i], "-D") == 0) {
            ud->do_fork = 1;
         } else if (strcmp(argv[i], "-h") == 0) {
            usage(ud);
            goto enderr;
         } else if (strcmp(argv[i], "-v") == 0) {
            ud->verbose++;
         } else if (strcmp(argv[i], "-V") == 0 ||
                    strcmp(argv[i], "--version") == 0 ) {
            fprintf( stdout, "%s\n", PJVERSION );
            goto enderr;
	 }
      }
   }
   msg_set_level(ud->verbose + 1);

   if ( ud->log_file ){
      msg_openlog(ud->log_file , "a"); /* send messages to file */
   } else if ( ud->do_fork ) {
      logger_init(log_facility);
      ud->logfunc = logger;
   }

   ret = vfddmain_process(ud);

enderr:
   vfddmain_destroy(ud);
   return ret;
}

void vfddmain_destroy( void *xud )
{
   UserData *ud = (UserData *) xud;

   vfdd_destroy(ud->vfdd);
   app_free(ud->conffile);
   app_free(ud->log_file);
   app_free(ud);
//   msg_atexit();  /* for clean log before tracemem */
//   tmem_destroy( NULL);
}

int vfddmain_process(UserData *ud )
{
   int ret = 0;
   int noclose = -1;

   if ( ud->do_fork ) {
      if ( ud->logfunc ){
         /* send message to syslog */
         msg_set_func( ud->logfunc );
      }
      noclose = 0;
   }
   sigmain_signal_init(vfddmain_destroy, ud, 0, noclose );

   ud->vfdd = vfdd_new( ud->conffile );
   ret = ud->vfdd->status;
   ud->vfdd->word = ud->word;
   ud->vfdd->nocolon = ud->colon;
   if ( ret == 0 ){
      msg_info( "Running Micael's vfd scheme" );
      loop_run( ud->vfdd->loop );
      
   }
   return ret;
}
