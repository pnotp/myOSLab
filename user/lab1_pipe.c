// #include <stdio.h>
// #include <unistd.h>
// #include <string.h>
// #include <memory.h>
// #include <stdlib.h>
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
// #include <sys/types.h>
// #include <sys/wait.h>

// from FreeBSD.
int
do_rand(unsigned long *ctx)
{
/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * without overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
    long hi, lo, x;

    /* Transform to [1, 0x7ffffffe] range. */
    x = (*ctx % 0x7ffffffe) + 1;
    hi = x / 127773;
    lo = x % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    /* Transform to [0, 0x7ffffffd] range. */
    x--;
    *ctx = x;
    return (x);
}

unsigned long rand_next = 1;

int
rand(void)
{
    return (do_rand(&rand_next));
}

void Consumer(int rfd){
    // close(1);
    // int fd = open("lab1_output.txt", O_CREATE | O_WRONLY);
    // if(fd < 0){
    //     return;
    // }
    char buffer[1024];
    char tmp_buffer[1024];
    int sum = 0;

    int i = 0;
    printf("Consumer: Receiving numbers...\n");
    while(1){
        buffer[0] = 0;
        
        int n = read(rfd, buffer, sizeof(buffer));
        int number = 0;
        int tmp_i = 0;

        if(n > 0){
            buffer[n] = 0;
            
            for(int k = 0; k < n; k++){
                if(buffer[k] == 0){
                    tmp_buffer[tmp_i] = 0;
                    number = atoi(tmp_buffer);
                    sum += number;
                    printf("Consumer: Received %d\n", number);
                    tmp_i = 0;
                    i++;
                }
                else{
                    tmp_buffer[tmp_i] = buffer[k];
                    tmp_i++;
                }
            }
        }
        else{
            break;
        }       
        
    }
    printf("Consumer: Total sum = %d\n", sum);
}

void Producer(int wfd){
    // srand(0);

    char buffer[1024];
    char tmp_buffer[1024];
    int i = 0;
    printf("Producer: Sending numbers...\n");
    while(i < 5){
        int n = rand() % 100;
        int num = n;
        // int n = 5;
        // buffer[0] = 0;
        int tmp_i = 0;
        while(n > 0){
            tmp_buffer[tmp_i] = n % 10 + '0';
            n = n / 10;
            tmp_i ++;
        }
        for(int k = 0; k < tmp_i; k++){
            buffer[k] = tmp_buffer[tmp_i - k - 1];
        }
        buffer[tmp_i] = 0;
        // buffer[0] = n + '0';
        // buffer[1] = 0;
        // snprintf(buffer, sizeof(buffer), "%d", n);

        int ret = write(wfd, buffer, strlen(buffer) + 1);
        if(ret < 0){
            // printf("error!");
        }
        printf("Producer: Sent %d\n", num);
        sleep(1);
        i++;
    }
}

int main(){
    int fd[2] = {0};
    int n = pipe(fd);
    if(n<0)
        return 1;
    int id = fork();
    if(id<0)
        return 2;
    if(id == 0){ //子进程
        close(fd[0]);
        Producer(fd[1]);
        close(fd[1]);
        exit(0);
    }
    close(fd[1]);
    // pid_t rid = waitpid(id, NULL, 0);
    int rid = wait(0);
    if(rid < 0)
        return -1;
    Consumer(fd[0]);

    close(fd[0]);
    return 0;
}