#include <string.h>
#include <stdio.h>
#include <stdarg.h>
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
  unsigned int i;
  char *cp; //cpersal pointer
  va_list arg;
//  printf("%s",fmt);
  va_start(arg, fmt); //gets the arguments
  for(cp = fmt; *cp != '\0'; cp++) //goes until the end of the input string
    {
      while(*cp != '%') //checks for the '%'
    	{
    	  putchar(*cp);
    	  cp++;
    	}
      cp++; // we found a '%', increment, and check what the value is.
      switch(*cp)
    	{

    	case 'c': //character
    	  i = va_arg(arg,int);
    	  putchar(i);
    	  break;
    	case 's': //string
    	  i = va_arg(arg, int);
    	  prints(i);
    	  break;
    	case 'd': //int
    	  i = va_arg(arg, int);
    	  printd(i);

    	  break;
    	case 'o': //octal
    	  i = va_arg(arg,int);
    	  printo(i);

    	  break;
    	case 'x': //hex
    	  i = va_arg(arg, int);
    	  printx(i);

    	  break;
    	case 'u': //unsigned int
    	  i = va_arg(arg, int);
    	  printu(i);
    	  break;
    	}
      putchar(' '); //put a space after each argument. Tidy-ness
      cp++; //advance cp to next
    }

}

va_end(arg);

int main(char* argv, int argc)
{
  myprintf("cha=%c string=%s dec=%d hex=%x oct=%o neg=%d\n",'A', "This is a test", 100, 100, 100, -100);
}
