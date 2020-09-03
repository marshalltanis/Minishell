/* $Id: stmts.c,v 1.4 2017/06/02 03:37:36 tanism3 Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "proto.h"

struct stmtLine{
  int kind;
  int elseEndIx;
  char *command;
};

struct stmt_list{
  int numLines;
  int addLines;
  struct stmtLine *stmts;
};
enum firstWord {W_IF = 1, W_WHILE = 2, W_ELSE = 4, W_END = 8, W_OTHER = 16};
static char word[15];
static int tmpKind = 0;
void addStatement(struct stmtLine *currLine, struct stmt_list *list);
void readStmt(struct stmtLine *fLine, int kind, struct stmt_list *list);
struct stmt_list * init();
struct stmtLine *createLine(char *line);

int runStmt(int stIx, struct stmtLine *list){
  
}
void readStmt(struct stmtLine *fLine, int kind, struct stmt_list *list){
  addStatement(fLine, list);
  int numElse = 0;
  if(kind == W_IF){
    numElse = 1;
  }
  else{
    numElse = 0;
  }
  struct stmtLine *nextLine = (struct stmtLine *)malloc(sizeof(struct stmtLine));
  if(nextLine < 0){
    perror("malloc");
    return;
  }
  nextLine->command = (char *)malloc(sizeof(char) * LINELEN);
  fgets(nextLine->command, LINELEN + 1, stdin);
  firstWord(nextLine->command);
  int nextKind = tmpKind;
  while(nextKind != W_END){
    switch (nextKind){
      case W_IF || W_WHILE:
        readStmt(nextLine, nextKind, list);
        break;
      case W_ELSE:
        if(numElse > 0){
          nextLine->elseEndIx = list->numLines;
          addStatement(nextLine, list);
          numElse --;
        }
        else{
          dprintf(2, "Else in a while or without a corresponding If\n");
        }
        break;
      case W_OTHER:
        addStatement(nextLine,list);
        break;
    }
    fgets(nextLine->command, LINELEN + 1, stdin);
    nextLine->command[strlen(nextLine->command) - 1] = '\0';
    firstWord(nextLine->command);
    nextKind = tmpKind;
  }
  nextLine->elseEndIx = list->numLines;
  addStatement(nextLine, list);
}
void addStatement(struct stmtLine *currLine, struct stmt_list *list){
  list->stmts[list->addLines] = *currLine;
  list->numLines ++;
  list->addLines ++;
}
struct stmt_list * init(){
  struct stmt_list *list = (struct stmt_list *)malloc(sizeof(struct stmt_list));
  if(list < 0){
    perror("malloc");
    return NULL;
  }
  list->stmts = (struct stmtLine *)malloc(sizeof(struct stmtLine) * LINELEN);
  list->numLines = 0;
  list->addLines = 0;
  return list;
}

struct stmtLine *createLine(char *line){
  struct stmtLine *tmpLine = (struct stmtLine *)malloc (sizeof(struct stmtLine));
  if(tmpLine < 0){
    perror("malloc");
    return NULL;
  }
  tmpLine->command = (char *) malloc (sizeof(char) * LINELEN);
  if(tmpLine->command < 0){
    perror("malloc");
    return NULL;
  }
  tmpLine->kind = tmpKind;
  tmpLine->elseEndIx = 0;
  tmpLine->command = line;
  return tmpLine;
}
int doStatement(char *line){
  struct stmt_list *myList = init();
  struct stmtLine *myLine = createLine(line);
  readStmt(myLine, myLine->kind, myList);
  return 0;
}
int firstWord(char *line){
  int ix = 0;
  while(line[ix] != '\0' && line[ix] == ' '){
    ix ++;
  }
  if(line[ix] == '"'){
    return 0;
  }
  else if(line[ix] == '\0'){
    return 0;
  }
  int wordIx = 0;
  while(line[ix] != ' ' && line[ix] != '\0'){
    word[wordIx] = line[ix];
    ix ++;
    wordIx ++;
  }
  word[wordIx] = '\0';
  if(strcmp("if", word) == 0){
    tmpKind = W_IF;
    return 1;
  }
  else if(strcmp("while", word) == 0){
    tmpKind = W_WHILE;
    return 1;
  }
  else{
    if(strcmp(word,"else") == 0){
      tmpKind = W_ELSE;
      return 0;
    }
    else if(strcmp(word, "end") == 0){
      tmpKind = W_END;
      return 0;
    }
    else{
      tmpKind = W_OTHER;
      return 0;
    }
  }
  if(strcmp("if", word) == 0 || strcmp("while", word) == 0){
    return 1;
  }
  else{
    return 0;
  }
}
