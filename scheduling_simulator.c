#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "scheduling_simulator.h"

//ucontext

#define DEFAULT_STACK_SIZE (4096)
ucontext_t Shell_mode;
ucontext_t Simulate_mode;
ucontext_t Terminate;

int task_count = 0;

//timer
struct itimerval t;

void settimer(int sec, int microsec)
{
    //printf("enter settimer\n");
    t.it_value.tv_usec = microsec;
    t.it_value.tv_sec = sec;
    if(setitimer( ITIMER_REAL, &t, NULL) < 0 ) {
        printf("set timer error\n");
        exit(1);
    }
    return;
}
//thread
typedef struct uthread_t {
    ucontext_t link;
    void (*func)(void);
    enum TASK_STATE state;
    char name[10];
    int  queueing_time;
    char priority;
    char time_quantum;
    int remain_time;
    int pid;
    int suspend_time;
    struct uthread_t *next;
    struct uthread_t *pid_next;
} uthread_t;

uthread_t head;
uthread_t *newtask = &head;
uthread_t *tail = &head;
uthread_t *running_task = &head;
uthread_t *prev_running_task = &head;
//
void hw_suspend(int msec_10)
{
    printf("enter hw suspend, suspend pid:%d  taskname:%s\n", running_task->pid, running_task->name);
    settimer(0,0);

    uthread_t *ptr = running_task;
    uthread_t *save_running_task = running_task;
    //change running_task parameter
    running_task -> state = TASK_WAITING;
    running_task -> remain_time = 0;
    running_task -> suspend_time = 10 * msec_10;

    //switch to next ready task with high priority
    while(ptr -> next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'H') {
            running_task = ptr;
            ptr -> state = TASK_RUNNING;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            //printf("suspend and give to pid:%d, %d\n", ptr->pid, prev_running_task->next->pid);
            settimer(0,10000);
            swapcontext(&(save_running_task->link), &(running_task->link));
            return;
        }

    }

    //can't find ready task after running_task
    //search from head with high priotiry;

    ptr = &head;
    while(ptr -> next != NULL && ptr -> next != running_task) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'H') {
            running_task = ptr;
            ptr -> state = TASK_RUNNING;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            //printf("suspend and give to pid:%d, %d\n", ptr->pid, prev_running_task->next->pid);
            settimer(0,10000);
            swapcontext(&(save_running_task->link), &(running_task->link));
            return;
        }
    }

    //can't find high priority -> search low priority
    ptr = running_task;

    while(ptr -> next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'L') {
            running_task = ptr;
            ptr -> state = TASK_RUNNING;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            //printf("suspend and give to pid:%d, %d\n", ptr->pid, prev_running_task->next->pid);
            settimer(0,10000);
            swapcontext(&(save_running_task->link), &(running_task->link));
            return;
        }

    }
    //can't find ready task after running_task
    //search from head with low priotiry;

    ptr = &head;
    while(ptr -> next != NULL && ptr -> next != running_task) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'L') {
            running_task = ptr;
            ptr -> state = TASK_RUNNING;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            //printf("suspend and give to pid:%d, %d\n", ptr->pid, prev_running_task->next->pid);
            settimer(0,10000);
            swapcontext(&(save_running_task->link), &(running_task->link));
            return;
        }
    }


    //not found any ready task
    printf("in suspend can't find any ready task\n");
    swapcontext(&(save_running_task->link), &Simulate_mode);

    return;
}

void hw_wakeup_pid(int pid)
{
    printf("enter hw_wakenup_pid pid:%d\n", pid);
    uthread_t *ptr = &head;
    uthread_t *pre_ptr = &head;
    while(ptr->next != NULL) {
        pre_ptr = ptr;
        ptr = ptr -> next;
        if(ptr -> pid == pid && ptr -> state == TASK_WAITING) {
            ptr -> state = TASK_READY;
            ptr -> suspend_time = 0;
            pre_ptr -> next = ptr -> next;
            tail -> next = ptr;
            tail = ptr;
            tail -> next = NULL;
            ptr = pre_ptr;
        }
    }
    return;
}

int hw_wakeup_taskname(char *task_name)
{
    printf("enter hw_wakenup_taskname taskname:%s\n", task_name);
    uthread_t *ptr = &head;
    uthread_t *pre_ptr = &head;
    int count = 0;
    while(ptr->next != NULL) {
        pre_ptr = ptr;
        ptr = ptr -> next;
        if(strcmp(task_name, ptr -> name) == 0 && ptr -> state == TASK_WAITING) {
            ptr -> state = TASK_READY;
            ptr -> suspend_time = 0;
            pre_ptr -> next = ptr -> next;
            tail -> next = ptr;
            tail = ptr;
            tail -> next = NULL;
            ptr = pre_ptr;
            ++count;
        }
    }
    return count;
}

int hw_task_create(char *task_name)
{
    //printf("enter task create\n");
    //which task to add
    void (*func)(void);
    if(strcmp(task_name, "task1") == 0) {
        func = task1;
    } else if(strcmp(task_name, "task2") == 0) {
        func = task2;
    } else if(strcmp(task_name, "task3") == 0) {
        func = task3;
    } else if(strcmp(task_name, "task4") == 0) {
        func = task4;
    } else if(strcmp(task_name, "task5") == 0) {
        func = task5;
    } else if(strcmp(task_name, "task6") == 0) {
        func = task6;
    } else {
        return -1;
    }
    //set newtask parameters
    newtask = (struct uthread_t *)malloc(sizeof(uthread_t));
    newtask -> func = func;
    newtask -> state = TASK_READY;
    sprintf(newtask -> name, "%s", task_name);
    newtask -> queueing_time = 0;
    newtask -> priority = 'L';
    newtask -> time_quantum = 'S';
    newtask -> remain_time = 0;
    newtask -> pid = ++task_count;
    newtask -> suspend_time = 0;
    newtask -> next = NULL;
    newtask -> pid_next = NULL;
    //set newtask link
    getcontext(&(newtask->link));
    newtask->link.uc_stack.ss_sp = malloc(DEFAULT_STACK_SIZE);
    newtask->link.uc_stack.ss_size = DEFAULT_STACK_SIZE;
    newtask->link.uc_stack.ss_flags = 0;
    newtask->link.uc_link = &Terminate;
    makecontext(&(newtask->link), newtask->func, 0);

    //change the tail and connect linklist
    uthread_t *ptr =  &head;
    while(ptr->next != NULL) {
        ptr = ptr->next;
    }
    ptr -> next = newtask;
    tail = newtask;
    // remember link by pid
    ptr = &head;
    while(ptr -> pid_next != NULL) {
        ptr = ptr -> pid_next;
    }
    ptr -> pid_next = newtask;

    printf("create pid:%d, name:%s\n", newtask->pid, newtask->name);
    return newtask -> pid;
}

void task_remove(int pid)
{

    uthread_t *ptr_front = &head;
    uthread_t *ptr_prev = &head;

    // for print out
    uthread_t *ptr_pid_front = &head;
    uthread_t *ptr_pid_prev = &head;

    //check if task queue is empty
    if(ptr_front -> next == NULL) {
        printf("task is empty\n");
        return;
    }
    ptr_front = ptr_front -> next;
    //searching
    while(ptr_front->next != NULL && ptr_front->pid != pid) {
        ptr_prev = ptr_front;
        ptr_front = ptr_front -> next;
    }
    //found?
    if(ptr_front->next == NULL && ptr_front->pid != pid) {
        printf("pid not exist\n");
        return;
    } else {
        //for print out queue
        ptr_pid_front = ptr_pid_front -> pid_next;
        while(ptr_pid_front -> pid_next != NULL && ptr_pid_front->pid != pid) {
            ptr_pid_prev = ptr_pid_front;
            ptr_pid_front = ptr_pid_front -> pid_next;
        }
        //
        ptr_pid_prev -> pid_next = ptr_pid_front -> pid_next;
        ptr_prev -> next = ptr_front->next;
        free(ptr_front->link.uc_stack.ss_sp);
        free(ptr_front);
        printf("remove pid:%d\n", pid);
    }
    return;
}

void terminate()
{
    settimer(0,0);
    printf("pid:%d enter terminate\n", running_task->pid);
    running_task -> state = TASK_TERMINATED;
    running_task -> remain_time = 0;
    setcontext(&Simulate_mode);
    return;
}

void simulate_mode()
{
    settimer(0,0);
    //printf("enter simulate_mode \n");

    uthread_t *ptr = &head;
    //check if task queue empty
    if(ptr->next == NULL) {
        printf("task queue empty!\n");
        setcontext(&Shell_mode);
        return;
    }
    //check whether pause from a running task;
    while(ptr-> next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_RUNNING) {
            running_task = ptr;
            settimer(0,10000);
            //printf("setcontext to pid:%d\n", ptr->pid);
            setcontext(&(ptr->link));
            return;
        }
    }

    ptr = &head;

    //find ready task to work  (high priority)
    while(ptr->next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'H') {
            ptr -> state = TASK_RUNNING;
            running_task = ptr;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            settimer(0,10000);
            //printf("setcontext (ready->work) to pid:%d\n", ptr->pid);
            setcontext(&(ptr->link));
            return;
        }
    }

    /*
    //no task is ready (high priority)  -> find a waiting task to wait
    ptr = &head;

    while(ptr->next != NULL) {
    	prev_running_task = ptr;
    	ptr = ptr -> next;
    	if(ptr -> state == TASK_WAITING && ptr -> priority == 'H') {
    		running_task = ptr;
    		settimer(0, 10000);
    		//wait timer interrupt
    		while(1);
    	}
    }
    */

    ptr = &head;

    //find ready task to work  (low priority)
    while(ptr->next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_READY && ptr -> priority == 'L') {
            ptr -> state = TASK_RUNNING;
            running_task = ptr;
            if(running_task -> time_quantum == 'L') {
                running_task -> remain_time = 20;
            } else {
                running_task -> remain_time = 10;
            }
            settimer(0,10000);
            //printf("setcontext (ready->work) to pid:%d\n", ptr->pid);
            setcontext(&(ptr->link));
            return;
        }
    }

    //no task is ready  -> find a waiting task to wait
    ptr = &head;

    while(ptr->next != NULL) {
        prev_running_task = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_WAITING) {
            running_task = ptr;
            settimer(0, 10000);
            //wait timer interrupt
            while(1);
        }
    }



    //no ready && no waiting -> all task done
    printf("all task done\n");
    setcontext(&Shell_mode);

    return;
}

void shell_mode()
{
    settimer(0,0);
    printf("enter shell mode\n");
    /*
       t.it_interval.tv_usec = 0;
       t.it_interval.tv_sec = 1;
       t.it_value.tv_usec = 0;
       t.it_value.tv_sec = 0;

       if( setitimer( ITIMER_REAL, &t, NULL) < 0 ) {
       printf("set timer error\n");
       exit(1);
       }
       */
    while(1) {
        //get input
        char first[40] = {};
        fflush(stdin);
        fgets(first, 40, stdin);
        printf("input: %s\n", first);
        //check input
        //add
        if(strstr(first, "add") != NULL) {
            printf("add\n");
            char task_name[10], time_quantum, priority;
            sscanf(first+4, "%s ", task_name);
            int pid = hw_task_create(task_name);

            if(strstr(first, "-t") != NULL && pid >= 0) {
                time_quantum = *(strstr(first, "-t") + 3);
                if(time_quantum == 'L') {
                    tail -> time_quantum = 'L';
                }
                //printf("quantum:%c\n", time_quantum);
            }
            if(strstr(first, "-p") != NULL && pid >= 0) {
                priority = *(strstr(first, "-p") + 3);
                if(priority == 'H') {
                    tail -> priority = 'H';
                }
                //printf("priority:%c\n", priority);
            }
        }
        //remove
        else if(strstr(first, "remove") != NULL) {
            int pid = atoi(first+7);
            printf("pid: %d\n", pid);
            task_remove(pid);
        }
        //ps
        else if(strstr(first, "ps") != NULL) {

            uthread_t *ptr = &head;
            while(ptr->pid_next != NULL) {
                ptr = ptr -> pid_next;
                char state[24] = {};
                //check state
                if(ptr -> state == TASK_RUNNING) {
                    sprintf(state, "%s", "TASK_RUNNING");
                } else if(ptr -> state == TASK_READY) {
                    sprintf(state, "%s", "TASK_READY");
                } else if(ptr -> state == TASK_WAITING) {
                    sprintf(state, "%s", "TASK_WAITING");
                } else if(ptr -> state == TASK_TERMINATED) {
                    sprintf(state, "%s", "TASK_TERMINATED");
                }
                printf("%d %s %s %d %c %c\n", ptr->pid, ptr->name, state, ptr->queueing_time, ptr->priority, ptr->time_quantum);
            }
            printf("ps\n");

        }
        //start
        else if(strstr(first, "start") != NULL) {
            printf("start\n");
            break;
        }
    }

    setcontext(&Simulate_mode);
    return;
}

void timer_handle(int err)
{
    settimer(0,0);
    //printf("pid: %d, %denter timer_handler\n", running_task->pid, prev_running_task-> next -> pid);

    uthread_t *ptr = &head;
    uthread_t *pre_ptr = &head;
    /*
    int save = 0;
    while(ptr->next != NULL){
    	++save;
    	ptr = ptr -> next;
    	printf("%d, ", ptr->pid);
    	if(save > 30) exit(1);
    }
    printf("\n");
    ptr = &head;
    */
    // add queueing time
    while(ptr->next != NULL) {
        pre_ptr = ptr;
        ptr = ptr -> next;
        //printf("pid in timer_handle:%d\n", ptr -> pid);
        if(ptr -> state == TASK_READY) {
            ptr -> queueing_time += 10;
        }
    }
    // dec suspend time
    ptr = &head;
    pre_ptr = &head;
    while(ptr->next != NULL) {
        pre_ptr = ptr;
        ptr = ptr -> next;
        if(ptr -> state == TASK_WAITING) {
            ptr -> suspend_time -= 10;
            if(ptr->suspend_time <= 0) {
                ptr -> state = TASK_READY;
                ptr -> suspend_time = 0;
                pre_ptr -> next = ptr -> next;
                tail -> next = ptr;
                tail = ptr;
                tail -> next = NULL;
                ptr = pre_ptr;
            }
        }
    }

    //printf("after add queueing_time\n");

    if(running_task -> state == TASK_RUNNING) {
        running_task -> remain_time -= 10;
        if(running_task -> remain_time <= 0) {
            //change running to ready
            running_task -> remain_time = 0;
            running_task -> state = TASK_READY;
            if(running_task -> next != NULL) {
                //move this progress to tail
                prev_running_task -> next = running_task -> next;
                tail -> next = running_task;
                tail = running_task;
                running_task -> next = NULL;
                //printf("time out -> swap from timer_hander to simulate\n");
                swapcontext(&(running_task->link), &Simulate_mode);
                return;
            }
        } else {
            //printf("time remaining keep going\n");
            settimer(0, 10000);
            return;
        }

    }
    //printf("no task running -> go to simulate\n");
    if(running_task -> state == TASK_WAITING || running_task -> state == TASK_READY) {
        setcontext(&Simulate_mode);
        return;
    }
    return;
}

void ctrlZ_handle(int err)
{
    settimer(0,0);
    printf("enter ctrlZ_handler\n");
    if(running_task -> state == TASK_RUNNING) {
        swapcontext(&(running_task->link), &Shell_mode);
        return;
    }
    setcontext(&Shell_mode);
    return;
}

int main()
{
    //set signal
    struct sigaction signal1, signal2;

    signal1.sa_handler = ctrlZ_handle;
    signal2.sa_handler = timer_handle;

    sigemptyset(&signal1.sa_mask);
    sigemptyset(&signal2.sa_mask);
    sigaddset(&signal1.sa_mask, SIGTSTP);
    sigaddset(&signal1.sa_mask, SIGALRM);
    sigaddset(&signal2.sa_mask, SIGTSTP);
    sigaddset(&signal2.sa_mask, SIGALRM);

    signal1.sa_flags = 0;
    signal2.sa_flags = 0;

    if (sigaction(SIGTSTP, &signal1, NULL) < 0) {
        printf("set ctrlZ_handle error\n");
        exit(1);
    }
    if (sigaction(SIGALRM, &signal2, NULL) < 0) {
        printf("set timer_handle error\n");
        exit(1);
    }
    /*
    //set ctrl + Z signal
    signal(SIGTSTP, ctrlZ_handle);
    //set timer signal
    signal(SIGALRM, timer_handle);
    */

    //big location
    //shell location
    getcontext(&Shell_mode);
    Shell_mode.uc_stack.ss_sp = malloc(DEFAULT_STACK_SIZE);
    Shell_mode.uc_stack.ss_size = DEFAULT_STACK_SIZE;
    Shell_mode.uc_stack.ss_flags = 0;
    Shell_mode.uc_link = 0;
    //simulate location
    getcontext(&Simulate_mode);
    Simulate_mode.uc_stack.ss_sp = malloc(DEFAULT_STACK_SIZE);
    Simulate_mode.uc_stack.ss_size = DEFAULT_STACK_SIZE;
    Simulate_mode.uc_stack.ss_flags = 0;
    Simulate_mode.uc_link = 0;
    //terminate location (when thread finish)
    getcontext(&Terminate);
    Terminate.uc_stack.ss_sp = malloc(DEFAULT_STACK_SIZE);
    Terminate.uc_stack.ss_size = DEFAULT_STACK_SIZE;
    Terminate.uc_stack.ss_flags = 0;
    Terminate.uc_link = 0;
    //set function to ucontext
    makecontext(&Shell_mode, &shell_mode, 0);
    makecontext(&Simulate_mode, &simulate_mode, 0);
    makecontext(&Terminate, &terminate, 0);
    //go to shell mode
    setcontext(&Shell_mode);

    return 0;
}
