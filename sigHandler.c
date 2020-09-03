/*  $Id: sigHandler.c,v 1.8 2017/05/18 17:48:30 tanism3 Exp $*/


#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "proto.h"
#include <errno.h>


/* sets exit value if there was a signal interrupt, and then kill's the child.
 * also informs the process that a sigHappened
 */
void signalHandler(int signo){
  if(signo == SIGINT){
    exVal = EINTR;
    if(myChild != 0){
      kill((pid_t)myChild, SIGINT);
    }
    sigHappened = 1;
    return;
  }
}
