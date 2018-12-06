#include "type.h"
#include "util.h"

int main(int argc, char *argv[])
{
    int i, cmd;
    char line[128], cname[64], parameter[64];
    init();
    mount_root(argv[1]);
    printf("Dev=%d, inodeBegin=%d, bmap=%d, imap=%d, ninodes=%d\n", dev, inodeBegin, bmap, imap, ninodes); //FOR TESTING
    menu("");
    while(1)
    {
        memset(pathname, 0, 128);
        memset(parameter, 0, 64);
        memset(cname, 0, 64);
        printf("P%d running:\n", running->pid);
        printf("Command_> ");
        fgets(line, 128, stdin);
        line[strlen(line)-1] = 0;  // kill the \r char at end
        if (line[0]==0) continue;

        sscanf(line, "%s %s %s", cname, pathname, parameter);
        //printf("Pathname = %s, parameter = %s\n", pathname, parameter);
        if(parameter[0] != 0)
        {
            strcat(pathname, " ");
            strcat(pathname, parameter);
        }
        //printf("Pathname = %s\n", pathname);
        for(i = 0; i < 15; i++)
        {
            //look for the command in our list of commands, then run itvia the funciton pointer
            if(strcmp(cmnds[i], cname) == 0)
            {
                    fptr[i](pathname);
                    continue;
            }

        }
        putchar(10);

    }
}
