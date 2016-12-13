/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 5
 * Fall 2016
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Yufeng Wang
 * GTID: 903053019
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static int scheduler_type;
static pcb_t *process_head;
static pthread_cond_t idle_cond;
static int time_slice;
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static int cpu_count;
pcb_t* get_next_priority();
pcb_t* deque();
void put_in_readyqueue(pcb_t* current);


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *  you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *  The current array (see above) is how you access the currently running
 *  process indexed by the cpu id. See above for full description.
 *  context_switch() is prototyped in os-sim.h. Look there for more information 
 *  about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    pcb_t* head_b;
    if (scheduler_type == 1 || scheduler_type == 2) {
        head_b = deque(); //if round robin or FCFS
    } else {
        head_b = get_next_priority();
    }

    if(head_b != NULL) {
        head_b -> state = PROCESS_RUNNING;
    }
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = head_b;
    pthread_mutex_unlock(&current_mutex);
    context_switch(cpu_id, head_b, time_slice);
}



/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{

    pthread_mutex_lock(&current_mutex);
    while(process_head == NULL) {
        pthread_cond_wait(&idle_cond, &current_mutex);
    }
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    
    current[cpu_id] -> state = PROCESS_READY;
    pcb_t* current_process = current[cpu_id];
    
    put_in_readyqueue(current_process);
    
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    
    current[cpu_id] -> state = PROCESS_WAITING;
    
    pthread_mutex_unlock(&current_mutex);
    
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    
    current[cpu_id] -> state = PROCESS_TERMINATED;
    
    pthread_mutex_unlock(&current_mutex);
    
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *  To preempt a process, use force_preempt(). Look in os-sim.h for 
 *  its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    process->state = PROCESS_READY;
    put_in_readyqueue(process);
    if (scheduler_type == 3) {
        pthread_mutex_lock(&current_mutex);
        int count = 0;
        int lowest_priority = 10; //initialize the priority with the highest priority
        int cpu_number = -1;
        while (cpu_count > count && current[count] != NULL) {
            if(current[count] -> static_priority < lowest_priority) {
                lowest_priority = current[count] -> static_priority;
                cpu_number = count;
            }
            count++;
        }
        pthread_mutex_unlock(&current_mutex);
        if (cpu_number!=-1 && lowest_priority < process->static_priority) {
            force_preempt(cpu_number);
        }
    }
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{

    /* Parse command-line arguments */
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "CS 2200 Project 5 Fall 2016 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    cpu_count = atoi(argv[1]);

    /* FIX ME - Add support for -r and -p parameters*/
    /********** TODO **************/
    if(argc == 2) {
        scheduler_type = 1; //scheduler type is first come first serve
        time_slice = -1;
    } else {
        if (argv[2][1]=='r') {
            scheduler_type = 2;
            time_slice = atoi(argv[3]);
        } else {
            scheduler_type = 3;
            time_slice = -1;
        }
    }
    /* Allocate the current[] array and its mutex */
    /*********** TODO *************/


    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&queue_mutex,NULL);
    pthread_cond_init(&idle_cond,NULL);
    start_simulator(cpu_count);

    return 0;
}

void put_in_readyqueue (pcb_t *current_process) {
    
    pthread_mutex_lock(&queue_mutex);
    if(process_head == NULL) {
        process_head = current_process;
    } else {
        pcb_t *tmp_head = process_head;
        while (tmp_head -> next != NULL) {
            tmp_head = tmp_head-> next;
        }
        tmp_head -> next = current_process;
    }
    pthread_cond_signal(&idle_cond);
    pthread_mutex_unlock(&queue_mutex);
} 

pcb_t *deque(){
    pthread_mutex_lock(&queue_mutex);
    pcb_t *tmp_head;
    if(process_head == NULL){
        pthread_mutex_unlock(&queue_mutex);
        return NULL;
    } else {
        tmp_head = process_head;
        process_head = process_head -> next;
    }
    pthread_mutex_unlock(&queue_mutex);
    tmp_head -> next = NULL;
    return tmp_head;
}

pcb_t *get_next_priority() {

    pcb_t *max_priority_node;
    if (process_head == NULL) {
        return NULL;
     } else {
         pthread_mutex_lock(&queue_mutex);   
        pcb_t *tmp_node = process_head;
        max_priority_node = process_head;
        pcb_t *tmp_node_prev = NULL;
        pcb_t *max_priority_prev;
        int max_priority = process_head -> static_priority;
        
        //find the max_priority_node;
        while(tmp_node->next != NULL) {
            if (tmp_node -> static_priority > max_priority) {
                max_priority = tmp_node -> static_priority;
                max_priority_node = tmp_node;
                max_priority_prev = tmp_node_prev;
            }
            tmp_node_prev = tmp_node;
            tmp_node = tmp_node -> next;
        }

        //get rid of the max_priority_node
        if (max_priority_node == process_head) {
            process_head = max_priority_node -> next;
        } else {
            max_priority_prev -> next = max_priority_node ->next;
        }
    }
    max_priority_node -> next = NULL;

    pthread_mutex_unlock(&queue_mutex);
    return max_priority_node;
}

