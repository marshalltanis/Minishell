/*    $Id: expand.c,v 1.52 2017/05/27 21:35:09 tanism3 Exp $    */


/* Author: Marshall Tanis
 * CS 352
 * Minishell */

 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <unistd.h>
 #include <ctype.h>
 #include <dirent.h>
 #include <errno.h>
 #include <sys/wait.h>
 #include "proto.h"

static int replace(int ix, char *replacement, int maxSize, char *new);
static char *digExp(char *orig, int ix);
static char *envExp(int ix, char *orig, int *ixOld);
static char *pidExp();
static char *exitVal();
static int simpleStar(int ixNew, char *new, int newsize);
static char *getContext(char *orig, int ix);
static int complexStar(char *context, int ixNew, char *new, int newsize);
static char *expressionExp(char *orig, int ix, int *ixOld);
static int addPipe(char *command, char *newLine, int *ixNew, int max);

int expand(char *orig, char *new, int newsize){
  int ixOld = 0;
  int ixNew = 0;
  /* goes char by char */
  while(orig[ixOld] != '\0' && !sigHappened){
    /* if it's a $ expansion */
    if(orig[ixOld] == '$'){
      if(orig[ixOld + 1] == '{'){
        // do environment expansion
        char *env = envExp(ixOld + 2, orig, &ixOld);
        if(env == NULL){
          return -1;
        }
        ixNew = replace(ixNew, env, newsize, new);
        if(ixNew < 0){
          return -1;
        }
      }
      else if(orig[ixOld + 1] == '$'){
        // do pid expansion
        char *ppid = pidExp();
        if(ppid == NULL){
          return -1;
        }
        ixNew = replace(ixNew, ppid, newsize, new);
        if(ixNew < 0){
          return -1;
        }
        ixOld = ixOld + 2;
      }
      else if(isdigit(orig[ixOld + 1])){
        char *expand = digExp(orig, ixOld + 1);
        if(expand == NULL){
          ixOld ++;
          while(isdigit(orig[ixOld]) && orig[ixOld] != '\0'){
            ixOld ++;
          }
        }
        else if(strcmp(expand, "error")==0){
          return -1;
        }
        else{
          ixNew = replace(ixNew, expand, newsize, new);
          if(ixNew < 0){
            return -1;
          }
          free(expand);
          ixOld ++;
          while(isdigit(orig[ixOld]) && orig[ixOld] != '\0'){
            ixOld ++;
          }
        }
      }
      else if(orig[ixOld + 1] == '#'){
        char num[4];
        int argNum = myArgc - shiftAmt;
        if(argNum < 1){
          argNum = 1;
        }
        int res = snprintf(num, sizeof(int) * 4, "%d", argNum);
        if(res < 0){
          perror("snprintf");
          return -1;
        }
        ixNew = replace(ixNew, num, newsize, new);
        if(ixNew < 0){
          return -1;
        }
        ixOld += 2;
      }
      else if(orig[ixOld + 1] == '?'){
        char *expand = exitVal();
        if(expand == NULL){
          return -1;
        }
        ixNew = replace(ixNew, expand, newsize, new);
        if(ixNew < 0){
          return -1;
        }
        ixOld += 2;
      }
      else if(orig[ixOld + 1] == '('){
        char *expansion = expressionExp(orig, ixOld + 2, &ixOld);
        if(expansion == NULL){
          return - 1;
        }
        int worked = addPipe(expansion, new, &ixNew, newsize);
        if(worked < 0){
          return -1;
        }
      }
      else{
        new[ixNew] = orig[ixOld];
        ixOld ++;
        ixNew ++;
      }
    }
    /* else it's a * expansion */
    else if(orig[ixOld] == '*'){
      if((orig[ixOld + 1] == ' ' || (orig[ixOld + 1] == '\0')) && ((orig[ixOld - 1] == ' ') || (orig[ixOld - 1] == '"'))){
        ixNew = simpleStar(ixNew, new, newsize);
        if(ixNew < 0){
          return -1;
        }
        ixOld += 2;
      }
      else if(orig[ixOld - 1] == '\\'){
        new[ixNew - 1] = orig[ixOld];
        ixOld += 1;
      }
      else if(orig[ixOld + 1] != '\0' && orig[ixOld + 1] != ' '){
        char *context = getContext(orig, ixOld);
        if(context == NULL){
          return -1;
        }
        ixOld += (strlen(context) + 1);
        ixNew = complexStar(context, ixNew, new, newsize);
        if(ixNew < 0){
          return -1;
        }
      }
      else{
        new[ixNew] = orig[ixOld];
        ixNew ++;
        ixOld ++;
      }
    }
    /* else its just normal copy */
    else{
      new[ixNew] = orig[ixOld];
      ixOld ++;
      ixNew ++;
    }
  }
  if(sigHappened){
    return -1;
  }
  new[strlen(new)] = '\0';
  return 0;
}

/* perform piping using command and place output into *newLine */
static int addPipe(char *command, char *newLine, int *ixNew, int max){
  int fd[2];
  int status;
  pipe(fd);
  /* actually runs command */
  int cpid = processline(command, 0, fd[1], DONTWAIT | EXPAND);
  /* sets global id of child */
  myChild = cpid;
  close(fd[1]);
  int val = 0;
  while((val = read(fd[0], &newLine[*ixNew], 1)) > 0 && *ixNew < LINELEN){
    if(val < 0){
      perror("read");
      return -1;
    }
    *ixNew += val;
    if(newLine[*ixNew - 1] == '\n'){
      newLine[*ixNew - 1] = ' ';
    }
  }
  /* if buffer is overflowed, print error and kill child */
  if(*ixNew >= LINELEN){
    dprintf(2, "You overflowed the buffer!\n");
    kill(myChild, SIGKILL);
  }
  /* close pipe */
  close(fd[0]);
  /* wait on child */
  if(myChild > 0){
    if(waitpid(myChild, &status, 0) < 0){
      perror("wait");
      return -1;
    }
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
  /* if the buffer was overflowed, return error */
  if(*ixNew >= LINELEN){
    return -1;
  }
  /* else remove extra last character */
  if(newLine[*ixNew - 1] == ' '){
    *ixNew = *ixNew - 1;
  }

  return 0;
}

/* expand expressions */
static char *expressionExp(char *orig, int ix, int *ixOld){
  int count = 1;
  int howLong = 0;
  int tmpIx = ix;
  char *expression;
  char saved;
  /* looks for matching end parenthesis */
  while(orig[tmpIx] != '\0' && count > 0){
    if(orig[tmpIx] == '('){
      count ++;
    }
    else if(orig[tmpIx] == ')'){
      count --;
      if(count == 0){
        saved = orig[tmpIx];
        orig[tmpIx] = '\0';
        break;
      }
    }
    tmpIx ++;
    howLong ++;
  }
  if(count != 0 && orig[tmpIx] == '\0'){
    dprintf(2, "Could not expand $(), missing closing parenthesis\n");
    return NULL;
  }
  /* change the value of ixOld to the end parenthesis */
  *ixOld = tmpIx + 1;
  expression = (char *) malloc (sizeof(char) * howLong + 1);
  if(expression < 0){
    perror("malloc");
    return NULL;
  }
  /* print the command to expression variable */
  int worked = snprintf(expression, (sizeof(char) * howLong + 1), "%s", &orig[ix]);
  expression[howLong + 1] = '\0';
  orig[tmpIx] = saved;
  if(worked < 0){
    perror("snprintf");
    return NULL;
  }
  return expression;
}

/* gets the context for the * with trailing char expansion */
static char *getContext(char *orig, int ix){
  char *context;
  int counter = 0;
  int tmpIx = ix + 1;
  while(orig[tmpIx] != ' ' && orig[tmpIx] != '\0' && orig[tmpIx] != '"'){
    if(orig[tmpIx] == '/'){
      return NULL;
    }
    tmpIx ++;
    counter ++;
  }
  char saved = orig[tmpIx];
  orig[tmpIx] = '\0';
  context = (char *)malloc(sizeof(char) * (counter + 1));
  if(context < 0){
    perror("malloc");
    return NULL;
  }
  int val = snprintf(context,(sizeof(char) * counter + 1), "%s", &orig[ix + 1]);
  if(val < 0){
    perror("snprintf");
    return NULL;
  }
  orig[tmpIx] = saved;
  context[counter + 1] = '\0';
  return context;
}
/* calculates and replaces the file names that match with the previously found context */
static int complexStar(char *context, int ixNew, char *new, int newsize){
  char *cwd = getcwd(NULL, 0);
  DIR *currDir = opendir(cwd);
  if(currDir == NULL){
    perror("opendir");
    return -1;
  }
  free(cwd);
  struct dirent *currFile;
  errno = 0;
  int counter = 0;
  while((currFile = readdir(currDir)) != NULL){
    if(currFile->d_name[0] != '.'){
      int nameLen = strlen(currFile->d_name);
      if(strcmp(context, &(currFile->d_name[nameLen - (strlen(context))])) == 0){
        ixNew = replace(ixNew, currFile->d_name, newsize, new);
        new[ixNew] = ' ';
        ixNew ++;
        counter ++;
      }
    }
  }
  if(counter == 1){
    ixNew --;
  }
  else if(counter == 0){
    ixNew = replace(ixNew, "*", newsize, new);
    ixNew = replace(ixNew, context, newsize,new);
  }
  if(currFile == NULL && errno != 0){
    perror("readdir");
    return -1;
  }
  int val = closedir(currDir);
  if(val < 0){
    perror("closedir");
    return -1;
  }
  return ixNew;
}
/* calculates and replaces all the file names in the current working directory */
static int simpleStar(int ixNew, char *new, int newsize){
  char *cwd = getcwd(NULL, 0);
  DIR *currDir = opendir(cwd);
  if(currDir == NULL){
    perror("opendir");
    return -1;
  }
  free(cwd);
  struct dirent *currFile;
  errno = 0;
  while((currFile = readdir(currDir))!= '\0'){
    if(currFile->d_name[0] != '.'){
      ixNew = replace(ixNew, currFile->d_name, newsize, new);
      new[ixNew] = ' ';
      ixNew ++;
    }
  }
  if(currFile == NULL && errno != 0){
    perror("readdir");
    return -1;
  }
  int val = closedir(currDir);
  if(val < 0){
    perror("closedir");
    return -1;
  }
  return ixNew;
}
/* expands the exitValue in the new string based on the global var */
static char *exitVal(){
  char *expansion;
  expansion = (char *) malloc(sizeof(char) * 4);
  if(expansion < 0){
    perror("malloc");
    return NULL;
  }
  int result = snprintf(expansion, sizeof(char) * 4, "%d", exVal);
  if(result < 0){
    perror("snprintf");
    return NULL;
  }
  int len = strlen(expansion);
  expansion[len] = '\0';
  return expansion;
}
/* expands the $ digit expansion by calculating the shift amt and adding to it */
static char *digExp(char *orig, int ix){
  char *expansion;
  int tmpIX = ix;
  while(orig[tmpIX] != ' ' && orig[tmpIX] != '\0'){
    tmpIX ++;
  }
  char saved = orig[tmpIX];
  orig[tmpIX] = '\0';
  char num[5];
  int worked = snprintf(num,(sizeof(int) * 5), "%s", &orig[ix]);
  if(worked < 0){
    perror("snprintf");
    return "error";
  }
  orig[tmpIX] = saved;
  int arg = atoi(num) + shiftAmt;
  if(myArgc == 1 && (atoi(num) < myArgc)){
    int argLen = strlen(myArgv[0]);
    expansion = (char *)malloc(sizeof(char) * (argLen + 1));
    if(expansion < 0){
      perror("malloc");
    }
    worked = snprintf(expansion,argLen + 1, "%s", myArgv[0]);
    if(worked < 0){
      perror("snprintf");
      return "error";
    }
  }
  else if(arg >= myArgc){
    return NULL;
  }
  else{
    int argLen;
    if(atoi(num)==0){
      argLen = strlen(myArgv[1]);
    }
    else{
      argLen = strlen(myArgv[arg]);
    }
    expansion = (char *)malloc(sizeof(char) * (argLen + 1));
    if(expansion < 0){
      perror("malloc");
    }
    if(atoi(num) == 0){
      worked = snprintf(expansion,argLen + 1, "%s", myArgv[1]);
    }
    else{
      worked = snprintf(expansion,argLen + 1, "%s", myArgv[arg]);
    }
    if(worked < 0){
      perror("snprintf");
      return "error";
    }
  }
  int len = strlen(expansion);
  expansion[len] = '\0';
  return expansion;
}
/* expands $$ to be the current pid */
static char *pidExp(){
  char *expansion;
  int count = 0;
  int pid = getpid();
  int size = 1;
  while(pid % size != pid){
    count ++;
    size *= 10;
  }
  expansion = (char *)malloc (sizeof(char) * count + 1);
  if(expansion < 0){
    perror("malloc");
    return NULL;
  }
  int tmp = count;
  while(count > 0){
    count --;
    expansion[count] = (pid % 10) + '0';
    pid /= 10;
  }
  expansion[tmp] = '\0';
  return expansion;
}
/* expands environment variables */
static char *envExp(int ix, char *orig, int *ixOld){
  char *envName;
  char *expansion;
  int count = 0;
  int tmpIX = ix;
  while(orig[tmpIX] != '}' && orig[tmpIX] != '\0'){
    count ++;
    tmpIX ++;
  }
  *ixOld = tmpIX + 1;
  if(orig[tmpIX] == '\0'){
    dprintf(2, "There is a missing bracket\n");
    return NULL;
  }
  envName = (char *)malloc(sizeof(char) * count + 1);
  if(envName < 0){
    perror("malloc");
    return NULL;
  }
  int i = 0;
  tmpIX = ix;
  while(orig[tmpIX] != '}'){
    envName[i] = orig[tmpIX];
    i ++;
    tmpIX ++;
  }
  envName[i] = '\0';
  expansion = getenv(envName);
  free(envName);
  if(expansion == NULL){
    expansion="";
  }
  return expansion;
}
/* replaces the string at the given index with replacement string and updates the new index pointer */
static int replace(int ix, char *replacement, int maxSize, char *new){
  int i = 0;
  while(ix < maxSize && replacement[i] != '\0'){
    new[ix] = replacement[i];
    i ++;
    ix ++;
  }
  if(ix == maxSize){
    dprintf(2, "The replacement string was too long\n");
    return -1;
  }
  return ix;
}
