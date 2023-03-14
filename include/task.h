#ifndef TASK_H
#define TASK_H
#include <ucontext.h>

enum {
    READY = 0,
	WAITING,
    RUNNING,
    TERMINATED,
    IDLE
};

typedef void (*func)();

typedef struct task {
    ucontext_t ctx;
    int tid;
    int state;
    char *ta_name;
    int need_res[8];
    int res_cnt;
    int ge_res[8];
    int have_res[8];
    int get_again;
    char ta_stack[1024 * 128];
    void *retval;
    int sleep_time;
    int pir;
    int running_time;
    int waiting_time;
    int turn_around_time;
} task_t;

typedef struct node {
    struct node *next;
    task_t *data;
} node;

typedef struct queue {
    struct node *front;
	struct node *tail;
	int count;
} queue; 

/*Functions to modify thread queue*/
void queue_init (queue *q); /*queue initialization*/

int queue_size(queue *q); /* returns size of queue*/

int enqueue (queue *q, task_t *t); /*adds a node to rear*/
task_t * dequeue (queue *q); /*removes a node from front*/

int remove_q_ele (queue *q, task_t *t); /* removes a particular node*/

task_t * get_element_from_queue(queue *q, int num); /* gets element from queue*/

void ini();

void start_simulated();

void task_create(void *(*start_routine)(void), char* name, int pir);

void task_kill(char* name);

task_t task_self(void);

int task_cancel(task_t t);

void task_sleep(int);

void task_break(void *retval);

void task_exit();

void get_re(int, int *);

void release_re(int, int *);

void print();

void restart();

extern int fin_cnt;

#endif
