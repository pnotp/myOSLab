#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{       int pid;
        int data[8];
        int i,j,k;
        pid=fork();
        pid = getpid();
        printf("pid: %d started \n", pid);
        for(i=0;i<2;i++)
        {
            printf("pid: %d running \n", pid);
            for(j=0;j<1024;j++)
                for(k=0;k<1024*64;k++)
                    data[k%8]=pid*k;
         }
         printf("pid: %d done, %d \n", pid, data[0]);
         exit(0);
}

