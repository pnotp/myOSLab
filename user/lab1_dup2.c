// #include <stdio.h>
// #include <unistd.h>
// #include <string.h>
// #include <memory.h>
// #include <stdlib.h>
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
// #include <sys/types.h>
// #include <sys/wait.h>

// 两个函数的定义我写在user/syscall.c中了，在这里附一下。
// int dup(int fd){
//   struct proc *p = myproc();
//   p->trapframe->a7 = SYS_dup;
//   p->trapframe->a0 = fd;
//   syscall();
//   return p->trapframe->a0;
// }

// int dup2(int oldfd, int newfd){
//   struct proc *p = myproc();
//   p->trapframe->a7 = SYS_dup2;
//   p->trapframe->a0 = oldfd;
//   p->trapframe->a1 = newfd;
//   syscall();
//   return p->trapframe->a0;
// }

int main(){
    int fd1 = -1, fd2 = -1;

    // 打开两个文件，分别对应fd1和fd2。
    if((fd1 = open("dup_output.txt",O_RDWR|O_CREATE|O_TRUNC)) == -1 || (fd2 = open("dup2_output.txt",O_RDWR|O_CREATE|O_TRUNC)) == -1) 
    {
        return -1;
    }

    // 先在两个fd指向的原文件中进行输出。
    fprintf(fd1, "This is from fd1 (original). \n");
    fprintf(fd2, "First line. (From original fd2) \n");

    // 记下fd2原文件的指向。
    int old_fd2 = dup(fd2);

    // 调用dup2()使得fd2也指向fd1对应的文件。
    dup2(fd1, fd2);
    // 测试fd2是否重定向成功，也即此条输出应该出现在dup_output.txt中。
    fprintf(fd2, "This is from fd2 (dup2 of fd1). \n");

    // 调用dup2()使得fd2重新指向old_fd2对应的文件。
    dup2(old_fd2, fd2);
    // 测试fd2是否重定向恢复成功，也即此条输出应该出现在dup2_output.txt中。
    fprintf(fd2, "Second line. (Recovery test) \n");

    // 测试完毕，关闭所有文件句柄。
    close(fd1);
    close(fd2);
    close(old_fd2);
    return 0;
}

