#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root, int pip[])
{
        int pfd[2]; //create array to become pipe

        printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
        change_pname(root->name);

        int i;
        if (pipe(pfd)<0) {  //create pipe
                perror("error creating pipe");
                exit(1);
        }
        pid_t pid[root->nr_children];
        for (i=0; i<root->nr_children; i++) { //create children (if not leaf)
                printf("%s: creating child %s\n", root->name, (root->children+i)->name);
                pid[i]=fork();
                if (pid[i]<0) { //error in fork()
                        perror(root->name);
                        exit(1);
                }
                if (pid[i]==0) {  //child
                        fork_procs(root->children+i, pfd);
                        exit(1);
                }
        }
        wait_for_ready_children(root->nr_children);

        /*
         * Suspend Self
         */
        raise(SIGSTOP);
        printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
 int first, second;
        int  result;
        if (root->nr_children==2){
                kill(pid[0], SIGCONT); //wake proccesses
                kill(pid[1], SIGCONT);
                pid_t p;
                int status;
                p=wait(&status); //wait for child to exit
                explain_wait_status(p, status);  //explain reason for child's exit
                //kill(pid[1], SIGCONT);
                printf("%d: trying to read first number\n", getpid());
                if (read(pfd[0], &first, sizeof(first))!=sizeof(first)) { //read first arguement from pipe
                        perror("error read from pipe from first child");
                        exit(1);
                }
                printf("%d: just received number: %d\n",getpid(), first);
                //kill(pid[1], SIGCONT);
                p=wait(&status); //wait for second child to exit
                explain_wait_status(p, status); //print reason for child's exit
                printf("%d: trying to read second number\n", getpid());
                if (read(pfd[0],&second, sizeof(second))!=sizeof(second)) {  //read second arguement from pipe
                        perror("error reading from pipe from second child");
                        exit(1);
                }
                printf("%d: just received %d\n",getpid(),  second);
                if (*(root->name)=='+') //determine operator, calculate value
                        result=first+second;
                else if (*(root->name)=='*')
                        result=first*second;
                if (write(pip[1], &result, sizeof(result))!=sizeof(result)){ //send result to father
                        perror("error writing my result to pipe");
                        exit(1);
                }

        }
 //leaf:
        if (root->nr_children==0){
                printf("Child %d is writing %s\n",getpid(), root->name);
                int value=atoi(root->name);
                if (write(pip[1], &value, sizeof(value))!=sizeof(value)){
                        perror("write to pipe from proccess");
                        exit(1);
                }
                printf("Child %d wrote to pipe\n", getpid());
                exit(0);
        }
        exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(int argc, char *argv[])
{
        int pfd[2];
        pid_t pid;
        int status;
        int value;
        struct tree_node *root;

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);
        print_tree(root); //print tree to be created
        if (pipe(pfd)<0) { //create pipe
                perror("initial pipe");
                exit(1);
        }

        /* Fork root of process tree */
        pid = fork(); //create root proccess
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs(root,pfd);
                exit(1);
        }

        /*
         * Father
         */
        wait_for_ready_children(1); //wake for root of tree to suspend itself

        /* Print the process tree root at pid */
        show_pstree(pid);

        kill(pid, SIGCONT); //wake root proccess

        /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);
        if (read(pfd[0],&value,sizeof(value))!=sizeof(value)){
                perror("read from initial pipe");
                exit(1);
        }
        printf("Final output is: %d\n",value);

        return 0;
}
