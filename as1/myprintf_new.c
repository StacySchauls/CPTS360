#include <string.h>
#include <stdio.h>
typedef unsigned int u32;
char *ctable = "0123456789ABCDEF";
int  BASE = 10; // for decimal numbers
int BASE8 = 8; //for octa numbers.
int rpu(u32 x)
{
    char c;
    if (x){
       c = ctable[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   putchar(' ');
}
//2-1 My Own PrintS Function.
int prints(char *s)
{
  int i;
  for(i = 0; i<strlen(s); i++){
    putchar(s[i]);
  }
}

//2-2 My Own Functions

int printd(int x){
  if(x<0){
    putchar('-');
    rpu(abs(x));
  }else if(x == 0){
    printu(x);
  }else{
    rpu(x);
  }
}

int printo(u32 x){
  BASE = 8;
  (x==0)? putchar('0') : putchar('0'); rpu(x);
  putchar(' ');
}

int printx(u32 x){
  BASE = 16;
  (x==0) ? putchar('0x0') : putchar ('0x'); rpu(x);
}
int myprintf(char *fmt, ...)
{
  int index; //index for our fmt
  char *cp = &fmt; //argument / stack pointer
  cp += sizeof(char*); //increment it to move to the next argument
//  printf("%s\n", fmt);
//  printf("%s",fmt);
//  va_start(arg, fmt); //gets the arguments
  for(index = 0; index < strlen(fmt); index++) //goes until the end of the input string
    {

      if(fmt[index] == '%'){

          // we found a '%', increment, and check what the value is.
        switch(fmt[index + 1]){ //switch along the arguments

      	case 'c': //character
        //  printf("c");
          putchar(*cp);
          cp += sizeof(char*);
      	  break;
      	case 's': //string
        //  printf("s");
          prints(*((char**)cp)); //gets the strig argument
          cp += sizeof(char*);
      	  break;
      	case 'd': //int
        //  printf("d");
          printd(*((int*)cp)); //gets the int argument
          cp += sizeof(char*);
          break;
      	case 'o': //octal
          printo(*((u32*)cp)); //gets the argument to convert to octal
          cp += sizeof(char*);
      	  break;
      	case 'x': //hex
          printx(*((u32*)cp)); //gets the argument to convert to hex
          cp += sizeof(char*);
      	  break;
      	case 'u': //unsigned int
        //  printf("u");
          printu(*((u32*)cp)); //gets the argument for the unsigned int
          cp += sizeof(char*);
      	  break;
      	}
        putchar(' '); //put a space after each argument. Tidy-ness
        index++; //advance index of fmt to next
    }else{
      putchar(fmt[index]); //print anything else
    }
  }

}
/*

int main(int argc, char* argv[], char *env[])
{
  myprintf("argc = %d", argc);
  int i;
  for(i = 0; i < argc; i++){
    printf("Arg %d = %s\n", i, argv[i]);
  }
  myprintf("cha=%c string=%s dec=%d hex=%x oct=%o neg=%d\n",'A', "This is a test", 100, 100, 100, -100);
}
*/
