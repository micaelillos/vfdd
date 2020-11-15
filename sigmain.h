#ifndef SIGMAIN_H
#define SIGMAIN_H

/*
 * sigmain.h - sigmain.h interface
 *
 * include LICENSE
 */
#include <signal.h>
typedef void (*Main_Destroy_FP)( void *user_data );

/*
 * prototypes
 */
int sigmain_sigaction(int sig);
void sigmain_signal_init (Main_Destroy_FP func,  void *user_data,
                          int nochdir, int noclose );
#endif /* SIGMAIN_H */
