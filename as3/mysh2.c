#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Function Prototypes */
int hasSpace(char *string); //check if there is a space in a pipeLine
int sysCall(void);
int hasRedirect(char *string);
int parsePath(void);
int parseLine(void);
int resetEnv(void);
int hasPipe(char *string);
int paresePipe(char *string);
int hasSlash(char *string);
/*global Variables*/

char line[128];
char command[16];
char *arguments[10];					//  list of indiviudal argument strings
char argument[112];					// Single string with all arguments
char *env[128];
char*envTest; 	         // environment for execve
char redirectFile[128];					// File name used in case of IO redirectioni
char redirectFileH[128];				// Redirect file for head argument
char redirectFileT[128];				// Redirect file for tail argument
char *head;						// head of piped argument
char *tail;						// tail of piped argument
char *headArgs[10];					// Dynamic list of individual argument strings from the head argument
char *tailArgs[10];
int lineHasSpace;
char cwd[256];
/* Main Function */
int main(int argc, char*argv[], char* envp[]){
  int i = 0;


  parsePath();

  getcwd(cwd, sizeof(cwd));

  while(1){
    resetEnv();

    printf("%s >_ ", cwd);
    fgets(line, 128, stdin);        // get user input
    line[strlen(line)-1] = 0;       //remove \n at the end

    if(hasPipe(line)){            //check for pipe
      //printf("There is a pipe!\n");
      parsePipe(line);
      sysCall();
      continue;
    }else{
      if(!hasSpace(line)){            //if the command doesnt hav arguments
          strcpy(command, line);
          //printf("line is: %s\n",line);
          arguments[0] = (char *)malloc(strlen(line));
          arguments[0] = line;
          //printf("arguments[0] %s\n", arguments[0]);
          //printf("argument[0]: %s\n",arguments[0]);
          sysCall();
          continue;
        }

      }


    parseLine();                //populate arguments[]
  //  printf("returned from parse\n");
    sysCall();                  //send arguments to system

  }

  return 0;
}

int sysCall(void){

  char commandLine[128]= {0};
  int pid,pid1,pid2, ret, fd, i = 0;
  char binPath[128] = "/bin/";
  char cdPathArg[128] = {0};
  cdPathArg[0] = '/';

  memset(commandLine, 0, 128);
  strcat(commandLine, command);
  //printf("commandLine: %s\n", commandLine);


  if(hasSpace(line)){
    strcat(commandLine, " ");
    strcat(commandLine, argument);
  }

  if(strcmp(command, "cd") == 0){      //cd
    if(hasSpace(line)){
      //printf("commandline here: %s\n", commandLine);
      if(getcwd(cwd, sizeof(cwd)) != NULL){
        //printf("cwd: %s\n", cwd);
      }else{
        printf("error getting cwd\n");
      }

      strcat(cdPathArg, argument);
      cwd[strlen(cwd)] = '/';               //appened a slash to the end of the cwd
      strcat(cwd, argument);              //add the argument to end of cwd

      chdir(cwd);                       //cd to argument

      return 1;
    }
    chdir(getenv("HOME"));       //cd to home if there is no arguments with cd
    return 1;
  }

  if(strcmp(commandLine, "exit") == 0){
    exit(1);
    return 1;
  }




  /*forking processes*/

  int status;
  pid = fork();
  if(pid){
    //printf("in the pid here\n");
    while(wait(&status) > 0) {}

  }else{
    if(hasPipe(line)){
      int pd[2];
      pipe(pd); //create pipe. pd[0] read from pipe, pd[1] write to pipe;
      //printf("pid before fork: %d\n", pid);

      pid1 = fork(); //fork a child process for command 1
    //  printf("pid after fork: %d\n", pid);

      if(pid1){ //parent
        //printf("running %s\n", head);
        //printf("first head arg is: %s", *headArgs[0]);
        /*
        close(1);
        close(pd[0]);
        dup2(pd[1], 1);
        //close(1);
        */
        close(pd[0]);
        dup2(pd[1],1);
        close(pd[1]);
        strcat(binPath, head);
        execve(binPath, &headArgs[0], env);
        printf("Error on parent");
        exit(-1);
      }else{ //child
        //printf("running %s\n", tail);
        //printf("here %s\n", tail);
        //printf("first tail arg is: %s", *tailArgs[0]);
        /*
        close(0);
        close(pd[1]);
        dup2(pd[0], 0);
        */
        close(pd[1]);
        dup2(pd[0], 0);
        close(pd[0]);
        strcat(binPath, tail);

        execve(binPath, &tailArgs[0], env);
        printf("error on child %s\n", tail);
        exit(-1);

      }
      exit(1);

    }



    if(hasRedirect(line)){
      fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd, 1);           //create copy of file descriptor
      close(fd);
                // close the file descriptor
    }
    if(!hasSlash(command)){
      strcat(binPath, command);
      execve(binPath, &arguments[0], env);
    }else{
      execve(command, &arguments[0], env);
    }
    exit(1);

  }
  return 1;
}

int hasRedirect(char *string){

	int i = 0;

	while (string[i])
	{
		if (string[i] == '>' || string[i] == '<') return 1;
		i++;
	}
	return 0;
}


int hasSpace(char *string){
    int i = 0;
    while(string[i]){
      if(string[i] == ' '){
        lineHasSpace = 1;
        return 1;
      }
      i++;
    }
    return 0;
}

int parseLine(){
  int i, j = 0;
  char *token;
  char tempLine[128];
  char *pipeLine;
  char copyLine[128];
  strcpy( tempLine, line);
  //check for pipe Here
  //printf("here");
  while(argument[i] != ' ') i++; //pass the command, look for first space
    i++;                         // move passed the teh space, to the first argument
  strcpy(argument, tempLine);    //copy line into arguments
  memmove(argument, argument + i, strlen(argument));  //copy everything but the command into argument
  token = strtok(tempLine, " ");                      //tokenize the input string. First is command
  if(strlen(token) > 16){
    printf("Command too long.\n");
    return 0;
  }

  strcpy(command, token);
  arguments[0] = (char*)malloc(strlen(token));
  strcpy(arguments[0], token);

  i = 1;
  while(token = strtok(0, " ")){
    if((strcmp(token, "<") == 0) || (strcmp(token, ">") ==0) || (strcmp(token, ">>")==0)){
      token = strtok(0, " "); //move to next token
      strcpy(redirectFile, token); //save token after <,>,>> as redirect file name
      continue;
    }

    arguments[i] = (char *)malloc(strlen(token)); //llocate memory for argument string
    strcpy(arguments[i], token);
    printf("argument %d : %s\n", i,token);
           //copy argument into argument array
    i++;
  }
}

void prepend(char* s, const char* t){
    size_t len = strlen(t);
    size_t i;

    memmove(s + len, s, strlen(s) + 1);

    for (i = 0; i < len; ++i)
    {
        s[i] = t[i];
    }
}
// Reset global environment variables
int resetEnv(void){
	int i = 0;

	memset(line, 0, 128);						// Reset user inputs
	memset(command, 0, 16);
	memset(argument, 0, 112);
	memset(redirectFile, 0, 128);
  memset(cwd, 0 ,128);
	while(arguments[i])						// Reset arguments
	{
		memset(arguments[i], 0, strlen(arguments[i]));
		arguments[i] = 0;
		i++;
	}

	i = 0;
	while(headArgs[i])						// Reset head arguments
	{
		memset(headArgs[i], 0, strlen(headArgs[i]));
		headArgs[i] = 0;
		i++;
	}

        i = 0;
        while(tailArgs[i])						// Reset tail arguments
        {
                memset(tailArgs[i], 0, strlen(tailArgs[i]));
		tailArgs[i] = 0;
                i++;
        }

	if (head) memset(head, 0, strlen(head));			// Reset head and tail
	if (tail) memset(head, 0, strlen(tail));
}							// Reset globals so old commands don't interfere with new ones

int parsePath(){
  int i =0;
  char *path = getenv("PATH");
  char *token;
  //printf("%s\n", path);
  token = strtok(path, ":");
  printf("token: %s\n", token);
  env[i] = token;
  i++;
  while(token = strtok(0, ":")){
    //ls
    printf("token: %s\n",token);
    env[i] = token;
    i++;
  }
}

int hasPipe(char *string){
  int i = 0;
  while(string[i]){
    if(string[i] == '|') return 1;
    i++;
  }
  return 0;
}
//parses the pipe line. can only handle one pipe right now.
int parsePipe(char *string){
  char * token;
  char copyLine[128];
  int i = 0;
  strcpy(copyLine, string);

  token = strtok(copyLine, "|");
  head = (char *)malloc(strlen(token));
  strcpy(head,token);
  head[strlen(head)-1] = 0;  //remove space at the end
  //printf("head: %s\n", head);
  token = strtok(0, "|");
  token++;                  //remove spce at the begining

  tail = (char *)malloc(strlen(token));
  strcpy(tail,token);
  //printf("tail: %s\n", tail);

  //get head and tail arguments
  //strcpy(copyLine, head); //new copy of string
  token = strtok(head, " ");
  headArgs[i] = (char *)malloc(strlen(token));
  headArgs[i] = token;
  while(token = strtok(0, " ")){
    //printf("head arg %s: \n", token);
  //  printf("%s: \n", token);
    i++;
    headArgs[i] = (char *)malloc(strlen(token));
    headArgs[i] = token;
  }
  head[strlen(head)] = 0;
  //get tail args

  i = 0;
  //strcpy(copyLine, tail);

  token = strtok(tail, " ");

  tailArgs[i] = (char *)malloc(strlen(token));
  //printf("tail arg %d: %s\n",i , token);
  tailArgs[i] = token;
  i++;
  while(token = strtok(0, " ")){
  //  printf("tail arg: %d %s\n",i, token);
    tailArgs[i] = (char *)malloc(strlen(token));
    tailArgs[i] = token;
    i++;
  }
  return 0;

}
// check if its a local file or an absolute path
int hasSlash(char *string){
  int i = 0;
  while(string[i]){
    if(string[i] == '/') return 1;
    i++;
  }
  return 0;
}
