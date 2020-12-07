#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "mandel-lib.h"
/*
 * POSIX thread functions do not return error numbers in errno,
 * but in the actual return value of the function call instead.
 * This macro helps with error reporting in this case.
 */
#define perror_pthread(ret, msg) \
	do { errno = ret; perror(msg); } while (0)
#define MANDEL_MAX_ITERATION 100000
/***************************
 * Compile-time parameters *
 ***************************/
struct thread_info_struct {
	pthread_t threadid; /* POSIX thread id, as returned by the library */
	int N; /* Application-defined thread id */

};
int NTHREADS;
/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = 50;
int x_chars = 90;
/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;
/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
sem_t *sem;
struct thread_info_struct *thrptr;
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;
	int n;
	int val;
	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;
	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {
		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;
		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}
/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	char point ='@';
	char newline='\n';
	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}
	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}
void compute_and_output_mandel_line(int fd, int line, int N)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];
	compute_mandel_line(line, color_val);
	sem_wait(&sem[((line)%N)]);
	output_mandel_line(fd, color_val);
	sem_post(&sem[((line+1)%N)]);
}
int safe_fn(char *s)
{
	long l;
	char* endp;

	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		return l;
	}
	else
		return -1;
}

void *thread_solution(void *arg)
{
	struct thread_info_struct *thr = arg;
	int line;
	for (line = thr->N; line < y_chars; line+= NTHREADS) {
		compute_and_output_mandel_line(1, line, NTHREADS);
	}
	return NULL;
}

//We can put here the function defined  in the asnwer of the 4th question
int main(int argc, char *argv[])
{
	int ret;
	if (argc != 2){
		perror("Heyyy, You must type the following structure ./mandel <N_THREADS>\n");
		exit(1);
	}

	NTHREADS = safe_fn(argv[1]);
	if (NTHREADS <= 0) {
		fprintf(stderr, "%s not valid for 'NTHREADS'\n", argv[2]);
		exit(2);
	}
	printf("Running for N_THREADS = %d\n", NTHREADS);
	printf("Lets create the threads\n");
        if ((thrptr = malloc(NTHREADS*sizeof(*thrptr))) == NULL) {
                fprintf(stderr, "We have problem in allocation of %zd bytes \n",NTHREADS*sizeof(*thrptr));
                exit(1);
        }	
	printf("threads are OK\n");
	printf("Lets create semaphores\n");
	if ((sem = malloc(NTHREADS*sizeof(*sem))) == NULL) {
        fprintf(stderr, "We have problem in allocation of  %zd bytes \n",NTHREADS*sizeof(*sem));
            exit(1);
        }
	printf("sem is OK\n");
	int i;
	// Initializing semaphores
	for (i = 0; i < NTHREADS; i++){
		sem_init(&sem[i], 0, 0);
	}
	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;
	/*
	 * draw the Mandelbrot Set, one line at a time.
	 * Output is sent to file descriptor '1', i.e., standard output.
	 */
	// Incrementing the first semaphore
	sem_init(&sem[0], 0, 1);
	for (i = 0; i < NTHREADS; i++) {
		thrptr[i].N = i;

		/* Spawn new thread(s) */
		ret = pthread_create(&thrptr[i].threadid, NULL, thread_solution, &thrptr[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}
	}
	/*
	 * Wait threads to terminate
	 */
	for (i = 0; i < NTHREADS; i++) {
		ret = pthread_join(thrptr[i].threadid, NULL);
		if (ret) {
			perror_pthread(ret, "pthread_join");
			exit(1);
		}
	}

	reset_xterm_color(1);
	return 0;
}

