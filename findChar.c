/*  $Id: findChar.c,v 1.9 2017/05/28 00:55:21 tanism3 Exp $ */

/* Author: Marshall Tanis
 * CS 352
 * Minishell */

 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <unistd.h>
 #include "proto.h"


/* finds first instance of unquoted character and returns ix of found char */
int findChar(char *line, char *charToFind, int count){
  int ix = 0;
  int ixChar = 0;
  int num = 0;
  int len = strlen(charToFind);
  while(line[ix] != '\0' && num != count){
    if(line[ix] == '"'){
      ix ++;
      while(line[ix] != '"' && line[ix] != '\0'){
        ix ++;
      }
      if(line[ix] == '\0'){
        dprintf(2, "There was an odd number of quotes");
        return -1;
      }
    }
    while(ixChar < len){
      if(line[ix] == charToFind[ixChar]){
        num ++;
        if(num == count){
          return ix;
        }
        else{
          count --;
        }
      }
      ixChar ++;
    }
    ix ++;
    ixChar = 0;
  }
  return -2;
}
