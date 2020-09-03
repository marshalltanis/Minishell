/*    $Id: proto.h,v 1.21 2017/06/02 02:13:41 tanism3 Exp $    */



#define LINELEN 200000
#define WAIT 1
#define DONTWAIT 0
#define EXPAND 2
#define DONTEXPAND 0


char **arg_parse(char *line, int *argcp);
int isBuiltIn(char **args, int argcp);
int exBuiltIn(char **args, int argcp, int outfd, int infd, int newErr);
int expand (char *orig, char *new, int newsize);
int findChar(char *line, char *charToFind, int count);
int findComment(char *line);
int processline (char *line , int inFd, int outFd, int flags);
void signalHandler(int signo);
int findRedir(char *newLine, int *cfd, int inFd, int outFd);
int handlePipes(char *line, int first, int infd, int outfd);
int firstWord(char *line);
int doStatement(char *line);





char **myArgv;
int myArgc;
int shiftAmt;
int exVal;
int sigHappened;
int myChild;
