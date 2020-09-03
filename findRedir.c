/*  $Id: findRedir.c,v 1.4 2017/05/31 19:23:05 tanism3 Exp $ */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "proto.h"


int findRedir(char *newLine, int *cfd, int inFd, int outFd){
  int tmpIx = 0;
  int Err = 0;
  int DoubleR = 0;
  while(newLine[tmpIx] != '\0'){
    int count = 0;
    int spaces = 0;
    char *path;
    int pathStart = 0;
    if(newLine[tmpIx] == '"'){
      tmpIx ++;
      while(newLine[tmpIx] != '"' && newLine[tmpIx] != '\0'){
        tmpIx ++;
      }
      if(newLine[tmpIx] == '\0'){
        dprintf(2, "There was an odd number of quotes \n");
        return -1;
      }
    }
    if(newLine[tmpIx] == '>'){
      int start = tmpIx;
      if(tmpIx != 0){
        if(tmpIx - 1 == 0 && newLine[tmpIx - 1] == '2'){
          Err = 1;
        }
        else if(newLine[tmpIx - 1] == '2' && newLine[tmpIx - 2] == ' '){
          Err = 1;
        }
      }
      if(newLine[tmpIx + 1] == '>'){
        DoubleR = 1;
        spaces ++;
        tmpIx ++;
      }
      tmpIx ++;
      while(newLine[tmpIx] == ' ' && newLine[tmpIx] != '\0'){
        spaces ++;
        tmpIx ++;
      }
      if(newLine[tmpIx] == '\0'){
        dprintf(cfd[2], "Please specify a redirect file\n");
        return -1;
      }
      pathStart = tmpIx;
      while(newLine[tmpIx] != ' ' && newLine[tmpIx] != '\0'){
        if(newLine[tmpIx] == '"'){
          int tmptmpIx=tmpIx;
          while(newLine[tmptmpIx] != '\0'){
            newLine[tmptmpIx] = newLine[tmptmpIx + 1];
            tmptmpIx ++;
          }
          while(newLine[tmpIx] != '"' && newLine[tmpIx] != '\0'){
            tmpIx ++;
            count ++;
          }
          if(newLine[tmpIx] == '\0'){
            dprintf(cfd[2], "There is an odd number of quotes\n");
            return -1;
          }
          tmptmpIx = tmpIx;
          while(newLine[tmptmpIx] != '\0'){
            newLine[tmptmpIx] = newLine[tmptmpIx + 1];
            tmptmpIx ++;
          }
        }
        else{
          tmpIx ++;
          count ++;
        }
      }
      char saved = newLine[tmpIx];
      newLine[tmpIx] = '\0';
      path = (char *)malloc (sizeof(char) * count);
      int worked = snprintf(path, sizeof(char) * count + 1, "%s", &(newLine[pathStart]));
      if(worked == 0){
        perror("snprintf");
        return -1;
      }
      path[strlen(path)] = '\0';
      newLine[tmpIx] = saved;
      tmpIx = start;
      while((count + spaces + 1) > 0){
        newLine[tmpIx] = ' ';
        tmpIx ++;
        count --;
      }
      if(Err == 1){
        newLine[start - 1] = ' ';
        if(cfd[2] != 2){
          close(cfd[2]);
        }
        if(DoubleR == 1){
          cfd[2] = open(path, O_RDWR|O_CREAT | O_APPEND, 0744);
        }
        else{
          cfd[2] = open(path, O_RDWR | O_CREAT | O_TRUNC, 0744);
        }
        if(cfd[2] < 0){
          dprintf(cfd[2], "open");
          return -1;
        }
      }
      else{
        if(cfd[1] != outFd){
          close(cfd[1]);
        }
        if(DoubleR == 1){
          cfd[1] = open(path, O_RDWR | O_CREAT | O_APPEND, 0744);
        }
        else{
          cfd[1] = open(path, O_RDWR | O_CREAT | O_TRUNC, 0744);
        }
        if(cfd[1] < 0){
          dprintf(cfd[2], "open");
          return -1;
        }
      }
      free(path);
    }
    if(newLine[tmpIx] == '<'){
      int start = tmpIx;
      tmpIx ++;
      while(newLine[tmpIx] == ' ' && newLine[tmpIx] != '\0'){
        spaces ++;
        tmpIx ++;
      }
      if(newLine[tmpIx] == '\0'){
        dprintf(cfd[2], "Please specify an input file");
        return -1;
      }
      pathStart = tmpIx;
      while(newLine[tmpIx] != ' ' && newLine[tmpIx] != '\0'){
        if(newLine[tmpIx] == '"'){
          int tmptmpIx=tmpIx;
          while(newLine[tmptmpIx] != '\0'){
            newLine[tmptmpIx] = newLine[tmptmpIx + 1];
            tmptmpIx ++;
          }
          while(newLine[tmpIx] != '"' && newLine[tmpIx] != '\0'){
            tmpIx ++;
            count ++;
          }
          if(newLine[tmpIx] == '\0'){
            dprintf(cfd[2], "There is an odd number of quotes");
            return -1;
          }
          tmptmpIx = tmpIx;
          while(newLine[tmptmpIx] != '\0'){
            newLine[tmptmpIx] = newLine[tmptmpIx + 1];
            tmptmpIx ++;
          }
        }
        else{
          tmpIx ++;
          count ++;
        }
      }
      char saved = newLine[tmpIx];
      newLine[tmpIx] = '\0';
      path = (char *)malloc (sizeof(char) * count);
      int worked = snprintf(path, sizeof(char) * count + 1, "%s", &(newLine[pathStart]));
      if(worked == 0){
        perror("snprintf");
        return -1;
      }
      path[strlen(path)] = '\0';
      newLine[tmpIx] = saved;
      tmpIx = start;
      while((count + spaces + 1) > 0){
        newLine[tmpIx] = ' ';
        tmpIx ++;
        count --;
      }
      if(cfd[0] != inFd){
        close(cfd[0]);
      }
      cfd[0] = open(path, O_RDONLY);
      if(cfd[0] < 0){
        dprintf(cfd[2],"open");
        return -1;
      }
    }
    if(newLine[tmpIx] != '\0'){
      tmpIx ++;
    }
  }
  return 0;
}
