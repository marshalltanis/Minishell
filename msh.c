/*    $Id: msh.c,v 1.47 2017/06/02 02:13:41 tanism3 Exp $    */



/* CS 352 -- Mini Shell!
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001
 *   Modified January 6, 2003
 *
 *
 * Modified by Marshall Tanis
 * on March 28th, 2017
 * for CS352 Spring 2017
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "proto.h"




/* Shell main */

int
main (int argc, char **argv)
{
    /* define  global variables */
    shiftAmt = 1;
    myArgc = argc;
    myArgv = argv;
    sigHappened = 0;
    myChild = 0;
    struct sigaction sigAct;
    sigAct.sa_handler = signalHandler;
    sigemptyset(&sigAct.sa_mask);
    sigAct.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sigAct, NULL);
    char   buffer [LINELEN];
    int    len;
    FILE *myStream;
    /* tests for script file after initial start up and then sets thescript
     * to standard in */
    if(argc > 1){
      /* Switched to using fopen to create FILE *stream that can be used to assign
       * myStream to either stdin or to the open script */
      myStream = fopen(argv[1],"r");
      if(myStream == NULL){
        perror("fopen");
        exit(127);
      }
    }
    else{
      myStream = stdin;
    }
    while (1) {
        /* prompt and get line */
        sigHappened = 0;
      	if(argc == 1){
          if(getenv("P1") == NULL){
            fprintf (stderr, "%% ");
          }
          else{
            fprintf(stderr, "%s ", getenv("P1"));
          }
        }
      	if (fgets (buffer, LINELEN, myStream) != buffer)
      	  break;

              /* Get rid of \n at end of buffer. */
      	len = strlen(buffer);
      	if (buffer[len-1] == '\n')
      	    buffer[len-1] = 0;

      	/* Run it ... */
        if(firstWord(buffer)){
          doStatement(buffer);
        }
        else{
      	   processline (buffer, 0,1, WAIT | EXPAND);
        }
    }
    if (!feof(myStream))
      perror ("read");

    return 0;		/* Also known as exit (0); */
}


int processline (char *line , int inFd, int outFd, int flags)
{
    pid_t  cpid;
    int status;
    while((cpid = waitpid(-1, &status, WNOHANG)) > 0){
      if(WIFEXITED(status)){
        exVal = WEXITSTATUS(status);
      }
    }
    int cfd[3] = {inFd, outFd, 2};
    int argcp;
    char newLine[LINELEN]= {0};
    int result = 0;
    if(flags & EXPAND){
      result = findComment(line);
      if(result < 0){
        exVal = 127;
        return -1;
      }
      result = expand(line, newLine, LINELEN);
      if(result < 0){
        exVal = 127;
        return -1;
      }
    }
    else{
      result = snprintf(newLine, LINELEN, "%s", line);
      if(result < 0){
        perror("snprintf");
        return -1;
      }
    }
    result = findChar(newLine, "|", 1);
    if(result == -1){
      return -1;
    }
    else if(result >= 0){
      result = handlePipes(newLine, result, inFd, outFd);
      if(result < 0){
        return -1;
      }
      return 0;
    }
    result = findRedir(newLine, cfd, inFd, outFd);
    if(result < 0){
      exVal = 127;
      return -1;
    }
    char **argList = arg_parse(newLine, &argcp);
    if(argList == 0){
      exVal = 127;
      return -1;
    }
    int built = isBuiltIn(argList, argcp);
    if(!built){
      /* Start a new process to do the job. */
      if(argcp == 0){
        return -1;
      }
      cpid = fork();
      if (cpid < 0) {
        perror("fork");
        exVal = 127;
        return  -1;
      }

      /* Check for who we are! */
      if (cpid == 0) {
        /* We are the child! */
        for(int i = 0; i < 3; i ++){
          if(cfd[i] != i){
            int worked = dup2(cfd[i], i);
            if(worked < 0){
              perror("dup2");
              return -1;
            }
            close(cfd[i]);
          }
        }
        execvp (argList[0], argList);
        perror("exec");
        close(0);
        exit (127);
      }
      if(flags & WAIT){
        /* Have the parent wait for child to complete */
        if (waitpid (cpid, &status, 0) < 0)
          perror( "waitpid");

        /* tests to see if the program exited normally */
        if(WIFEXITED(status)){
          /* gets the exit value for the last normal exit */
          exVal = WEXITSTATUS(status);
        }
        /* code is interupted, sets exit to abnormal val */
        else{
          exVal = 127;
        }
      }
      else{
        return cpid;
      }
    }
    /* sets exVal to 0 on success and 1 on failure */
    else{
      exVal = exBuiltIn(argList, argcp, cfd[1], cfd[0], cfd[2]);
      for(int i = 0; i < 3; i ++){
        if(cfd[i] != i){
          close(cfd[i]);
        }
      }
    }
    if(cfd[0] != inFd && cfd[0] != 0){
      close(cfd[0]);
    }
    if(cfd[1] != outFd && cfd[1] != 1){
      close(cfd[1]);
    }
    if(cfd[2] != 2){
      close(cfd[2]);
    }
    free(argList);
    return 0;

}
