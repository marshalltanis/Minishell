/*    $Id: arg_parse.c,v 1.2 2017/04/17 21:42:56 tanism3 Exp $    */


/* Author: Marshall Tanis
 * CS 352
 * Minishell */




#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "proto.h"


char **arg_parse(char *line, int *argcp){
  /* declare commonly used variables */
  char **args;
  int numArgs = 0;
  int count = 0;
  int let=0;
  /* count the number of arguments in the line */
  while(line[let] != '\0'){
    /* switch on the character, space, quote or other */
    switch (line[let]){
      /* if it's a space, skip it */
      case ' ':
        let ++;
        break;
      /* if it's a quote, find the matching quote, otherwise return error */
      case '"':
        let ++;
        numArgs ++;
        while(line[let] != '"'){
          if(line[let] == '\0'){
            dprintf(2, "There was an odd number of quotes\n");
            *argcp = 0;
            return NULL;
          }
          let ++;
        }
        while(line[let] != ' ' && line[let] != '\0'){
          let ++;
        }
        let ++;

        break;
      /* if it's an end of string character, break out of the switch */
      case '\0':
        break;
      /* otherwise increment value of numArgs and find next space */
      default:
        numArgs ++;
        while(line[let] != ' ' && line[let] != '\0'){
          if(line[let] == '"'){
            let ++;
            while(line[let] != '"'){
              if(line[let] == '\0'){
                dprintf(2, "There was an odd number of quotes\n");
                *argcp = 0;
                return NULL;
              }
              let ++;
            }
          }
          let ++;
        }
        let ++;
        break;
      }
  }
  *argcp = numArgs;
  if(numArgs == 0){
    return NULL;
  }
  /* free space for those arguments */
  if((args = (char **) malloc (sizeof(char *) * (numArgs + 1)))< 0){
    perror("malloc");
  }
  let = 0;
  /* point to the address of the first letter of each argument */
  while(line[let] != '\0'){
    /* same switch statement as before */
    switch (line[let]){
      case ' ':
        let ++;
        break;
      /* if it's a quote, remove the first quote, take the arg, then skip till last quote */
      case '"':
        let ++;
        args[count] = &line[let];
        while(line[let] != '"' && line[let] != '\0'){
          let ++;
        }
        if(line[let] != '\0'){
          int tmpIX = let;
          while(line[tmpIX] != '\0'){
            line[tmpIX] = line[tmpIX + 1];
            tmpIX ++;
          }
        }
        while(line[let] != ' ' && line[let] != '\0'){
          let ++;
        }
        line[let] = '\0';
        let ++;
        count ++;
        break;
      case '\0':
        break;
      /* otherwise just take arg as standard input, while looking for quotes */
      default:
        args[count] = &line[let];
        while(line[let] != ' ' && line[let] != '\0'){
          if(line[let] == '"'){
            int tmpIX = let;
            while(line[tmpIX] != '\0'){
              line[tmpIX] = line[tmpIX + 1];
              tmpIX ++;
            }
            while(line[let] != '"' && line[let] != '\0'){
              let ++;
            }
            tmpIX = let;
            while(line[tmpIX] != '\0'){
              line[tmpIX] = line[tmpIX + 1];
              tmpIX ++;
            }
          }
          let ++;
        }
        line[let] = '\0';
        let ++;
        count ++;
        break;
      }
  }
  args[numArgs] = NULL;
  return args;
}
