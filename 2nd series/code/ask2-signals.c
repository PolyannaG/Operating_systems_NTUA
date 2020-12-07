#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{

        printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
        change_pname(root->name);

        int i;
        pid_t pid[root->nr_children];
        for (i=0; i<root->nr_children; i++) {
                printf("%s: creating child %s\n", root->name, (root->children+i)->name);
                pid[i]=fork();
                if (pid[i]<0) { //error in fork
                        perror(root->name);
                        exit(1);
                }
                if (pid[i]==0) { //is child
                        fork_procs(root->children+i);
                        exit(1);
                }

        //is father:
        wait_for_ready_children(1); //wait for all childrem to suspend themselves
        }
        raise(SIGSTOP); //Suspend self until SIGCONT is received
        printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name); //print message after proccess is resumed

        /* ... */
        for (i=0; i<root->nr_children; i++){
                kill(pid[i], SIGCONT); //send signal to child to wake up
                pid_t pid;
                int status;
                pid=wait(&status); //wait for child to exit
                explain_wait_status(pid, status);  //print reason for child's exit
                //repeat for next child
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
        pid_t pid;
        int status;
        struct tree_node *root;

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);
        printf("Tree to be created:\n");
        print_tree(root);
        /* Fork root of process tree */
        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs(root);
                exit(1);
        }

        //Father:
        wait_for_ready_children(1); //wait for root of tree to suspend itself

        //Print the process tree root at pid:
        show_pstree(pid);

        kill(pid, SIGCONT); //send signal to root of tree to resume

  /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);

        return 0;
}
