/* $Id: handlePipes.c,v 1.6 2017/05/30 21:17:49 tanism3 Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include "proto.h"

int handlePipes(char *line, int first, int infd, int outfd){
  int ix = 0;
  int start = 0;
  int pipeix = first;
  int fd[2];
  int out = infd;
  int dontpipe = 0;
  int count = 0;
  char *command;
  int val = 0;
  while(line[ix] != '\0'){
    count = 0;
    if(dontpipe != 1){
      val = pipe(fd);
      if (val < 0){
        perror("pipe");
        return -1;
      }
    }
    line[pipeix] = '\0';
    int tmpIx = ix;
    while(line[tmpIx] != '\0'){
      count ++;
      tmpIx ++;
    }
    command = (char *)malloc(sizeof(char) * count + 1);
    if(command < 0){
      perror("malloc");
      return -1;
    }
    val = snprintf(command, sizeof(char) * count + 1, "%s", &line[start]);
    if(val < 0){
      perror("snprintf");
      return -1;
    }
    ix = pipeix + 1;
    start = ix;
    tmpIx = ix;
    while(line[tmpIx] != '|' && line[tmpIx] != '\0'){
      tmpIx ++;
    }
    if(dontpipe == 0){
      int cpid = processline(command, out, fd[1], DONTWAIT | DONTEXPAND);
      if(cpid < 0){
        return -1;
      }

      close(fd[1]);
    }
    else{
      int cpid = processline(command, out, outfd, WAIT | DONTEXPAND);
      close(out);
      if(cpid < 0){
        return -1;
      }
    }
    if(out != 0){
      close(out);
    }
    if(dontpipe != 1){
      line[pipeix] = '|';
      pipeix = tmpIx;
      out = fd[0];
    }
    else{
      ix = tmpIx;
    }
    if(line[tmpIx] == '\0' && dontpipe != 1){
      dontpipe = 1;
    }
  }
  free(command);
  return 0;
}
