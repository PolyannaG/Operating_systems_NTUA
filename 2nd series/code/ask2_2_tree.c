#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "tree.h"
#include "proc-common.h"

#define LEAF_SLEEP 10
#define SLEEP_SEC 5

void rec(struct tree_node *ptr){
        pid_t child;
        change_pname(ptr->name);
        int i;
        for(i=0;i<ptr->nr_children;i++){
                printf("%s: Forks %d children\n",ptr->name,ptr->nr_children-i);
                printf("Specifically %s: is forking now child with %s \n",ptr->name,ptr->children[i].name);
                child=fork();
                if (child<0){
                        perror("fork");
                        exit(1);
                }
                if(child==0){
                //      printf("Child with %ld created\n",getpid());
                        rec(ptr->children+i);
                        exit(1);
                }
        }

                for (i = 0; i < ptr->nr_children; ++i) {
                int status;
                pid_t pid;
                pid=wait(&status);
                explain_wait_status(pid, status);

        }
        if(ptr->nr_children==0) sleep(LEAF_SLEEP);
        exit(0);
}
int main(int argc,char *argv[]){
        struct tree_node *root;
        pid_t p;
        int status;

        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }
        if(open(argv[1], O_RDONLY) < 0){
                perror("Error");
        }

        root=get_tree_from_file(argv[1]);
        printf("Creating the following process tree: \n");
        print_tree(root);

        //Create the first child
        p=fork();
        if(p<0){
                perror("Error in the main\n");
                exit(1);
        }
        else if(p==0){
                rec(root);
                exit(1);
        }
        else {
                sleep(SLEEP_SEC);
                show_pstree(p);
                p=wait(&status);
                explain_wait_status(p,status);
                return 0;
        }
}
