/*    $Id: builtin.c,v 1.27 2017/05/30 21:22:12 tanism3 Exp $    */


/* Author: Marshall Tanis
 * CS 352
 * Minishell */




#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <bsd/string.h>
#include "proto.h"

/* built in args[0]s array */
static char *builtIns[] = {"exit", "aecho", "envset", "envunset", "cd", "shift", "unshift", "sstat", "read"};

/* tests to see if the args[0] is a builtin, returns 1 for yes, 0 for no */
int isBuiltIn(char **args, int argcp){
  int ix = 0;
  /* test and set which args[0] is being called */
  while(builtIns[ix] != '\0'){
    if(strcmp(builtIns[ix], args[0]) == 0){
      return 1;
    }
    ix ++;
  }
  return 0;
}

int exBuiltIn(char **args, int argcp, int outfd, int infd, int newErr){
  /* execute the args[0] based on testing the first letter of the args[0]*/
  int ix = 1;
  switch (args[0][0]){
    case 'e':
      if(strcmp(args[0], builtIns[0]) == 0){
        if(args[1] != '\0'){
          exit(atoi(args[1]));
        }
        else{
          exit(0);
        }
        break;
      }
      else if(strcmp(args[0], builtIns[2]) == 0){
        if(args[1] == NULL || args[2] == NULL){
          dprintf(newErr, "Please provide a name and a value in order to set the variable\n");
          return 1;
        }
        else{
          int result = setenv(args[1], args[2], 1);
          if(result < 0){
            dprintf(newErr,"setenv");
            return 1;
          }
          break;
        }
      }
      else if(strcmp(args[0], builtIns[3]) == 0){
        if(args[1] == NULL){
          dprintf(newErr, "Please provide the name of the variable to unset\n");
          return 1;
        }
        else{
          int result = unsetenv(args[1]);
          if(result < 0){
            dprintf(newErr,"unsetenv");
            return 1;
          }
          break;
        }
      }

    case 'a':
      while(ix < argcp - 1){
        if(strcmp(args[ix], "-n") != 0){
          dprintf(outfd, "%s ",args[ix]);
          ix ++;
        }
        else{
          ix ++;
        }
      }
      if(argcp > 1){
        if(strcmp(args[1], "-n") == 0){
          dprintf(outfd, "%s", args[ix]);
          break;
        }
        else{
          dprintf(outfd, "%s\n", args[ix]);
        }
      }
      if(argcp == 1){
        dprintf(outfd,"\n");
      }
      break;
    case 'c':
      if(args[1] == NULL){
        char *home = getenv("HOME");
        if(home == NULL){
          dprintf(newErr, "The HOME environment variable is not set, and no directory provided\n");
          return 1;
        }
        int result = chdir(home);
        if(result < 0){
          dprintf(newErr, "chdir\n");
          return 1;
        }
      }
      else{
        int result = chdir(args[1]);
        if(result < 0){
          dprintf(newErr,"chdir\n");
          return 1;
        }
      }
      break;
    case 's':
      /* shifts by 1 if no amount is specified, else shifts by specified amount */
      if(strcmp(builtIns[5], args[0]) == 0){
        if(argcp == 1){
          if(1 < (myArgc -shiftAmt)){
            shiftAmt ++;
          }
          else{
            dprintf(newErr, "The shift amount was greater than the number of arguments\n");
            return 1;
          }
        }
        /* won't let you shift past end of string */
        else if(argcp > 1){
          if(atoi(args[1]) < (myArgc - shiftAmt)){
            shiftAmt += atoi(args[1]);
          }
          else{
            dprintf(newErr, "The shift amount was greater than the number of arguments\n");
            return 1;
          }
        }
      break;
    }
    else{
      /* format sstat string that gets printed */
      struct stat statRes;
      struct passwd *userName;
      struct group *groupName;
      char *permis;
      char *modTime;
      struct tm *currTime;
      int ix = 1;
      while(ix < argcp){
        int val = stat(args[ix], &statRes);
        if(val < 0){
          dprintf(newErr,"stat\n");
          return 1;
        }
        userName = getpwuid(statRes.st_uid);
        groupName = getgrgid(statRes.st_gid);
        currTime = localtime(&statRes.st_mtime);
        modTime = asctime(currTime);
        permis = (char *) malloc(sizeof(char) * 12);
        if(permis < 0){
          dprintf(newErr,"malloc\n");
          return 1;
        }
        strmode(statRes.st_mode, permis);
        /* tests if user and group are null */
        if(userName == NULL){
          if(groupName == NULL){
            dprintf(outfd,"%s %d %d %s%d %ld %s",args[ix], statRes.st_uid, statRes.st_gid, permis, (int)statRes.st_nlink, statRes.st_size, modTime);
          }
          /* just the user is null */
          else{
            dprintf(outfd,"%s %d %s %s%d %ld %s",args[ix], statRes.st_uid, groupName->gr_name, permis, (int)statRes.st_nlink, statRes.st_size, modTime);
          }
        }
        /* just the group is null */
        else if(groupName == NULL){
          dprintf(outfd,"%s %s %d %s%d %ld %s",args[ix], userName->pw_name, statRes.st_gid, permis, (int)statRes.st_nlink, statRes.st_size, modTime);
        }
        else{
          dprintf(outfd,"%s %s %s %s%d %ld %s",args[ix], userName->pw_name, groupName->gr_name, permis, (int)statRes.st_nlink, statRes.st_size, modTime);
        }
        ix ++;
      }
    }
    /* read builtin that reads from stdin the value of the environment variable
     * pointed to by arg[1].
     */
    case 'r':
      if(strcmp(args[0],builtIns[8]) == 0){
        if(args[1] == '\0'){
          dprintf(newErr, "Please provide an environment variable name to populate\n");
          return -1;
        }
        char envVar[LINELEN];
        errno = 0;
        int tmpIx = 0;
        while(read(infd ,&envVar[tmpIx], 1) > 0 && envVar[tmpIx] != '\n'){
            tmpIx ++;
        }
        if(errno != 0){
          dprintf(newErr,"read\n");
          return -1;
        }
        int howMuch = strlen(envVar);
        envVar[howMuch - 1] = '\0';
        int result = setenv(args[1], envVar , 1);
        if(result < 0){
          dprintf(newErr,"setenv\n");
          return 1;
        }
        break;
      }
    /* Unshift the arguments by the specified number */
    case 'u':
      if(argcp == 1){
        shiftAmt = 1;
      }
      else if(argcp > 1){
        if(atoi(args[1]) < shiftAmt){
          shiftAmt -= atoi(args[1]);
        }
        else{
          dprintf(newErr, "The unshift amount was greater than the number of shifted arguments\n");
          return 1;
        }
      }
      break;
  }
  if(infd != 0){
    close(infd);
  }
  return 0;
}
