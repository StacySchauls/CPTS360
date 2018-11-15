#include <stdio.h>
#include <stdlib.h>
#include "myprintf_new.c"

int *FP;
//int myprintf(char *fmt, ...);
int main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  myprintf("enter main\n");

  myprintf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
  myprintf("&a=%x &b=%x &c=%x\n", &a, &b, &c);

//(1). Write C code to print values of argc and argv[] entries

  a=1; b=2; c=3;
  A(a,b);
  myprintf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  myprintf("enter A\n");
  // PRINT ADDRESS OF d, e, f
  myprintf("&d=%x &e=%x &d=%x\n", &d,&e,&f);
  d=4; e=5; f=6;
  B(d,e);
  myprintf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  myprintf("enter B\n");
  // PRINT ADDRESS OF g,h,i
  myprintf("&g=%x &h=%x &i=%x\n", &g,&h,&i);
  g=7; h=8; i=9;
  C(g,h);
  myprintf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p;

  myprintf("enter C\n");
  // PRINT ADDRESS OF u,v,w,i,p;
  myprintf("&u=%x &v=%x &w=%x &i=%x &p=%x\n", &u ,&v, &w, &i, p);
  u=10; v=11; w=12; i=13;
  FP = (int *)getebp();
//(2). Write C code to print the stack frame link list.

//do this for p instead
  myprintf("\n\n");
  myprintf("Stack Frame Linked List\n");
  myprintf("-----------------------------------\n");
  while(FP){
    myprintf("fp=%x      fp*=%x\n", FP, *FP);
    FP = (int *) *FP;
  }


/*(3). Print the stack contents from p to the frame of main()
     YOU MAY JUST PRINT 128 entries of the stack contents. */
     myprintf("\n\n");
     myprintf("Print the stack contents\n -------------------------------\n");
     p = (int *)&p;
     int k;
     for ( k = 0; k < 128; k++){
        myprintf("%x : %x \n", p, *p); //prints address p and what p points to
        p++;
     }



/*(4). On a hard copy of the print out, identify the stack contents
     as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.
*/
}
