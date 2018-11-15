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
  char *trav;
  va_list arg;
  printf("%s",fmt);
  va_start(arg, fmt);
  for(trav = fmt; *trav != '\0'; trav++)
    {
      while(*trav != '%')
	{
	  putchar(*trav);
	  trav++;
	}
      trav++;
      switch(*trav)
	{
	case 'c':
	  i = va_arg(arg,int);
	
	  putchar(i);
	  break;
	case 's':
	  i = va_arg(arg, int);
	  printf("%s", i);
        
	  break;
	case 'd':
	  i = va_arg(arg, int);
	  printf("%d", i);
	 
	  break;
	case 'o':
	  i = va_arg(arg,int);
	  printf("%o", i);
	  ;
	  break;
	case 'x':
	  i = va_arg(arg, int);
	  printf("%x", i);
     
	  break;
	case 'u':
	  i = va_arg(arg, int);
	  printf("%u", i);
      
	  break;
	}
    }
 
}

va_end(arg);

int main(char* argv, int argc)
{
  myprintf("cha=%c string=%s dec=%d hex=%x oct=%o neg=%d\n",'A', "This is a test", 100, 100, 100, -100);
}
