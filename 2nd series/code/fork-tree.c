#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_SEC 5
#define LEAF_SLEEP 10
void child(){
        //initial process is A
        pid_t B,C,D, p;
        int status;
        fprintf(stderr,"A creats B...\n");
        B=fork();
        if (B<0){
                perror("Error when in B");
                exit(1);
        }
        else if (B==0){
                fprintf(stderr, "B creats D...\n");
                D=fork();
                if(D<0){
                        perror("Error when in D\n");
                        exit(1);
                }
                else if(D==0){
                        change_pname("D");
                        fprintf(stderr, "D Sleeping...\n");
                        sleep(LEAF_SLEEP);
                        fprintf(stderr, "D Exiting...\n");
                        exit(13);
                }
                else{
                        change_pname("B");
                        fprintf(stderr,"B Sleeping...\n");
                        p=wait(&status);
                        explain_wait_status(D,status);
                        sleep(LEAF_SLEEP);
                        printf("B Exiting...\n");
                        exit(19);
                }
        }
 else{
                fprintf(stderr,"A creats C ...\n");
                C=fork();
                if(C<0){
                        perror("When in C/n");
                        exit(1);
                }
                else if(C==0){
                        change_pname("C");
                        fprintf(stderr, "C Sleeping...\n");
                        sleep(LEAF_SLEEP);
                        fprintf(stderr,"C Exiting...\n");
                        exit(17);
                }
                else{
                        change_pname("A");
                        p=wait(&status);
                        explain_wait_status(p,status);
                        p=wait(&status);
                        explain_wait_status(p,status);
                        sleep(LEAF_SLEEP);
                        printf("A Exiting...\n");
                        exit(16);
                }
        }
}
int main(void)
{
        pid_t p;
        int status;

        fprintf(stderr, "Parent, PID = %ld: Creating child...\n",
                        (long)getpid());
        p = fork();
        if (p < 0) {
                /* fork failed */
                perror("fork");
                exit(1);
        }
        if (p == 0) {
                printf("A created...\n");
                child();
                exit(1);
        }

        //      change_pname("father");
        /*
         * In parent process. Wait for the child to terminate
         * and report its termination status.
         */
        sleep(SLEEP_SEC);
        show_pstree(p);
        p = wait(&status);
        explain_wait_status(p, status);

        printf("All done, exiting...\n");

        return 0;
}
