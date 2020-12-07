#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

typedef struct proc_struct {  //pcb struct
	pid_t pid;
	int id;
	char process_name[TASK_NAME_SZ];
	char priority[5];
	}process;

static int number_of_processes;  //list node struct

typedef struct proc_list{
	process *proc_pointer;
	struct proc_list *next;
	}proc_node;

static proc_node *running_process;  //running process is kept here, list of procesess is a circular list
static int num_high=0;

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{	
	printf("Running process:id:%d Pid:%d name:%s priority:%s \n",(running_process->proc_pointer)->id, (running_process->proc_pointer)->pid, (running_process->proc_pointer)->process_name, (running_process->proc_pointer)->priority);
	proc_node *p=running_process;
	p=p->next;
	while (p!=running_process) {
	printf("Proccess in proccess list:id:%d Pid:%d name %s priority: %s\n",(p->proc_pointer)->id, (p->proc_pointer)->pid, (p->proc_pointer)->process_name, (p->proc_pointer)->priority);
	p=p->next;
	}
	//assert(0 && "Please fill me!");
}


/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{	
	proc_node *p=running_process->next;
	while (p!=running_process && (p->proc_pointer)->id!=id)
		p=p->next;
	if (p==running_process && id!=0) {
		printf("Coud not find process that was asked to kill, exiting...\n");
		printf("asked id:%d\n", id);
		return 1;
	}
	if ((p->proc_pointer)->id==id){
		kill((p->proc_pointer)->pid,SIGKILL);
		printf("Process with id:%d, and pid:%d killed succesfully\n", (p->proc_pointer)->id, (p->proc_pointer)->pid);
		return 0;
	}

	//assert(0 && "Please fill me!");
	//return -ENOSYS;
return 0;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{
	printf("Asked to create process with name:%s and id:%d\n", executable, number_of_processes+1);
	pid_t pid=fork();
	if (pid<0){
		perror("sched_create_task fork error");
	}
	if (pid==0) {  //child replaces itself
		char *newargv[]={executable,NULL,NULL,NULL};
		char *newenviron[]={NULL};
		printf("Process with pid:%ld replacing itself with the executable: %s ...\n", (long)getpid(), executable);
		raise(SIGSTOP);
		execve(executable, newargv, newenviron);
		perror("shed_create_task execve error");
		exit(1);
	}
	if (pid>0){  //parent keeps count of processes, adds child to the processes' list
		number_of_processes++;
		proc_node *pn=( proc_node *) malloc(sizeof(proc_node));
		process *p=(process *) malloc(sizeof(process));
		p->id=number_of_processes;
		p->pid=pid;	
		strcpy(p->process_name, executable);
		proc_node *tmp=running_process;
		while (tmp->next!=running_process) {  //add process to the end of the list, so before the running process
			tmp=tmp->next;
		}
		tmp->next=pn;
		pn->next=running_process;
		pn->proc_pointer=p;
		strcpy(pn->proc_pointer->priority, "LOW");
		
	}	
		
	
//	assert(0 && "Please fill me!");
}
static void sched_set_low_priority(int id) {
	proc_node *p=running_process;
	if (num_high>1 && id==0) {
		printf("Cannot change shells priority to LOW, because shell control will not be able to be regained.\n");
		return;
	}
	while ((p->proc_pointer)->id!=id){
		p=p->next;
		if (p==running_process) {
			printf("Error finding process to set priority to LOW\n");
			return;
		}
	}
	if (strcmp(p->proc_pointer->priority, "HIGH")==0)  //if process had high priority before, number of processes with high priority (shell excluded) is decreased;
		num_high--;
	if (num_high==1 && id!=0) //if there is only one high priority process it is the shell, shell must have low priority (no nead to call again for shell)
		sched_set_low_priority(0);
	strcpy(p->proc_pointer->priority,"LOW");

	printf("Successfully set priority of process with id: %d pid:%d and name:%s to LOW\n", (p->proc_pointer)->id, (p->proc_pointer)->pid, (p->proc_pointer)->process_name);
}
static void sched_set_high_priority(int id){
	proc_node *p=running_process;
	while ((p->proc_pointer)->id!=id){
		p=p->next;
		if (p==running_process) {
			printf("Error finding process to set priority to HIGH\n");
			return;
		}
	}
	if (num_high==0 && id!=0) //if it is the first process getting high priority, the shell must also get high priority (we don't need to call the function again if we are already changing shell's priority)
		sched_set_high_priority(0);
	if (strcmp(p->proc_pointer->priority, "LOW")==0)  //if process had low priority before, number of processes with high priority is increased
		num_high++;
	strcpy(p->proc_pointer->priority,"HIGH");
	printf("Successfully set priority of process with id %d pid:%d and name:%s to HIGH\n", (p->proc_pointer)->id, (p->proc_pointer)->pid, (p->proc_pointer)->process_name);
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		case REQ_LOW_TASK:
			sched_set_low_priority(rq->task_arg);
			return 0;
		case REQ_HIGH_TASK:
			sched_set_high_priority(rq->task_arg);
			return 0;
		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	printf("Process with id %d and pid %d to be stopped \n", (running_process->proc_pointer)->id, (running_process->proc_pointer)->pid);
	if (kill((running_process->proc_pointer)->pid, SIGSTOP) <0) {
		perror("scheduler kill error");
		exit(1);
	}

//	assert(0 && "Please fill me!");
}

proc_node* get_process_with_pid( pid_t pid) {
	proc_node *pr=running_process;
	while ((pr->proc_pointer)->pid!=pid) {	
		pr=pr->next;
		if (pr==running_process) {
			return NULL;
		}
	}
	return pr;
	
}
/*
* SIGCHILD handler
*/
static void
sigchld_handler(int signum)
{			
	pid_t p;
	int run_next;  //helper variables
	int status;

	for (;;) {
		run_next=0;
		p=waitpid(-1, &status, WUNTRACED | WNOHANG );
		if (p<0) {
			perror("Waitpid<0 in sigchld_handler");
			exit(1);			
		}
		if (p==0)
			break;
		if (WIFSTOPPED(status)) {
			if (p==(running_process->proc_pointer)->pid){ 
				printf("Currently running process with id:%d pid:%d and priority:%s has been stopped\n",(running_process->proc_pointer)->id, (running_process->proc_pointer)->pid, (running_process->proc_pointer)->priority);	
				run_next=1;
				running_process=running_process->next;
			}
			else {	//should not go here: only the running task is stopped, by the scheduler
				proc_node *pr=get_process_with_pid(p);
				if (pr==NULL) 
					printf("Error getting data of stopped process.");
				else {
			
					printf("Process (not running) with id:%d pid:%d and priority:%s has received stop signal.\n", (pr->proc_pointer)->id, (pr->proc_pointer)->pid, (pr->proc_pointer)->priority);
				}
			}		
		}  
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
		
			if (p==(running_process->proc_pointer)->pid) {
				printf("Currently running process with id:%d pid:%d and priority:%s has exited or been killed.\n", (running_process->proc_pointer)->id, (running_process->proc_pointer)->pid, running_process->proc_pointer->priority);
				run_next=1;
			if (strcmp(running_process->proc_pointer->priority,"HIGH")==0) {  //if priority of process was high
				num_high--;						//reduce the number of processes with high priority
				if (num_high==1 && running_process->proc_pointer->id!=0) sched_set_low_priority(0);    //if only one process has high priority (the shell) set shell's priority to low
			}
			if (running_process==running_process->next) {
				printf("Scheduler: No more processes to run.\n");
				exit(0);
			}
			}
			else {
				proc_node *pr=get_process_with_pid(p);
				printf("Process not running with id:%d pid:%d and priority:%s has exited or been killed.\n",(pr->proc_pointer)->id, (pr->proc_pointer)->pid, pr->proc_pointer->priority);
				if (strcmp(pr->proc_pointer->priority,"HIGH")==0) {
					num_high--;
					if (num_high==1 && pr->proc_pointer->id!=0) sched_set_low_priority(0);
				}
				if (pr==pr->next) {
					printf("Scheduler: No more processes to run.\n");
					exit(0);
				}
			}
			proc_node *rem=running_process;
			while (((rem->next)->proc_pointer)->pid!=p)  //remove killed process from process list
				rem=rem->next;
			proc_node *tmp=rem->next;
			rem->next=(rem->next)->next;
			free(tmp->proc_pointer);
			free(tmp);
			if (run_next==1) 	
				running_process=running_process->next;
		}		
		if (run_next==1) {   //if killed process is the current process, find the next one
			proc_node *pr=running_process;
			//if ((pr->proc_pointer)->id!=0) {  //if id=0 it is the shell, we don't care about its priority
			if (num_high!=0) {  //if there are processes with high priority, choose the next one with high prioriry
			while ((strcmp(pr->proc_pointer->priority, "LOW")==0)){
					pr=pr->next;
				}
			}
			running_process=pr;
			printf("Waking up process with id:%d and pid:%d\n", (running_process->proc_pointer)->id, (running_process->proc_pointer)->pid);
			kill((running_process->proc_pointer)->pid,SIGCONT);
			alarm(SCHED_TQ_SEC);
		}
	}
	//assert(0 && "Please fill me!");
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	
	running_process=(proc_node *) malloc(sizeof(proc_node));  //shell is added to the process list as the running process
	process *proc=(process *) malloc(sizeof(process));
	running_process->proc_pointer=proc;
	proc->id=0;
	proc->pid=p;
	running_process->next=running_process;
	strcpy(proc->process_name,SHELL_EXECUTABLE_NAME);
	strcpy(running_process->proc_pointer->priority, "LOW");
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	printf("start");
	int nproc;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	nproc=argc-1;  // number of input processes  
	number_of_processes=nproc; //input process, global variable to be used for pid's
	/* Create the shell. */
	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	//nproc = 0; /* number of proccesses goes here */

	int i;
	for (i=1; i<=nproc; i++) {
		pid_t pid;
		pid=fork();
		char *argvs[] = { argv[i], NULL};
		char *newenviron[]={NULL};
		if (pid<0) {
			perror("main: fork");
			exit(1);
		}
		if (pid==0) {
			raise(SIGSTOP);
			execve( argv[i], argvs, newenviron);
			perror("main: execve");
			exit(1);
		}
		if (pid>0) {
			proc_node *pn=(proc_node*)malloc(sizeof(proc_node));
			process *pr=(process*)malloc(sizeof(process));
			pr->id=i;
			pr->pid=pid;
			strcpy(pr->process_name, argv[i]);
			strcpy(pr->priority, "LOW");
			pn->next=running_process->next;
			pn->proc_pointer=pr;
			running_process->next=pn;
			running_process=pn;

		}
	}

	running_process=running_process->next; //running process points to the last process we added,point to the first one:FIFO

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

//	if (nproc == 0) { 							//We don't need this part: There is always at least one  process to make running, the shell.
//		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
//		exit(1);
//	}
//	if (nproc>0) {
		kill((running_process->proc_pointer)->pid,SIGCONT);  //wake up process which entered the fifo queue fisrt, running_process is the one we added last
		alarm(SCHED_TQ_SEC);
//	}
	

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
