/*  $Id: findComment.c,v 1.6 2017/05/25 22:50:16 tanism3 Exp $ */


/* Author: Marshall Tanis
 * CS 352
 * Minishell */

 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <unistd.h>
 #include "proto.h"


/* finds first unquoted instance of comment character and sets the end of line
 * char at the postion of the # key
 */
int findComment(char *line){
  int ix = findChar(line, "#", 1);
  if(ix < 0){
    if(ix == -2){
      return 0;
    }
    else{
      return -1;
    }
  }
  else if(ix != 0){
    if(line[ix - 1] != '$'){
      line[ix] = '\0';
    }
  }
  else{
    line[ix] = '\0';
  }
  return 0;
}
