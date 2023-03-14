#include "../include/task.h"
#include "../include/function.h"
#include "../include/shell.h"
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int sig = 0, sig_check = 0;
ucontext_t maincontext;
int cancel_current = 0;
int total_task_count = 0;
int fin_check = 0;
fin_cnt = 0;

queue ready_queue, finish_queue, waiting_queue, waiting_res;
task_t main_task;
task_t *current;
task_t *idl;
struct itimerval value, ovalue;

bool resource_check[8];
/*
void sigroutine(){
    ++sig;
    printf("Signal %d\n", sig);
    sleep_count();
    if(current->state == IDLE){
        printf("CPU idle\n");
    }
    else{
        printf("Task ");
        fputs(current->ta_name, stdout);
        printf(" is running.\n");
    }
    if(sig % 3 == 1){
        schedular();
    }

    return;
}*/

void queue_init(queue *q)
{
    q->front = q->tail = NULL;
    q->count = 0;
}

int enqueue(queue *q, task_t *task)
{
    node *tmp = (node *)malloc(sizeof(node));
    tmp->data = task;
    tmp->next = NULL;

    if (q->count == 0)
    {
        // printf("a\n");
        q->tail = tmp;
        q->front = tmp;
    }
    else
    {
        q->tail->next = tmp;
        q->tail = tmp;
    }
    (q->count)++;
    return 1;
}

task_t *dequeue(queue *q)
{
    if (q->front == NULL)
        return NULL;
    node *tmp = (node *)malloc(sizeof(node));
    tmp = q->front;
    q->front = q->front->next;
    task_t *ret = tmp->data;
    free(tmp);
    (q->count)--;
    return ret;
}

int queue_size(queue *q)
{
    return q->count;
}

int remove_q_ele(queue *q, task_t *t)
{
    node *curr = q->front;
    node *prev = NULL;
    if (curr == NULL)
        return 0;
    if (q->count <= 0)
        return 0;

    while (curr)
    {
        if (curr->data->tid == t->tid)
        {
            if (prev == NULL)
            {
                q->front = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }

            if (curr == q->tail)
            {
                q->tail = prev;
            }
            free(curr);
            (q->count)--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }

    return 0;
}

task_t *get_element_from_queue(queue *q, int num)
{
    node *tmp = q->front;
    if (q->count <= 0)
        return 0;
    int i = 0;
    task_t *data = 0;
    while (tmp)
    {
        if (i == num)
        {
            data = tmp->data;
            return data;
        }
        tmp = tmp->next;
        i++;
    }

    return 0;
}

void queue_sort(queue *q){
    
    int n = q->count;
    /*
    printf("%d\n", n);
    if(n == 0) return;
    for(int i = 0; i < n; i++){
        
        int min_index = -1;
        int min_value = 10000000;
        //printf("b\n");
        for(int j = 0; j < n - i; j++){
            if(q->front->data->pir < min_value)
            {
                min_value = q->front->data->pir;
                min_index = j;
            }
            task_t *tmp = dequeue(q);
            fputs(tmp->ta_name, stdout);
            printf(" pop\n");
            fputs(tmp->ta_name, stdout);
            printf(" in\n");
            enqueue(q, tmp);
        }
        printf("min_v: %d\n", min_value);
        printf("min_i: %d\n", min_index);
        //printf("c\n");
        for(int k = 0; k < min_index; k++){
            task_t *tmp = dequeue(q);
            fputs(tmp->ta_name, stdout);
            printf(" pop\n");
            fputs(tmp->ta_name, stdout);
            printf(" in\n");
            enqueue(q, tmp);
        }
        //printf("d\n");
        task_t *min = dequeue(q);
        fputs(min->ta_name, stdout);
        printf(" pop\n");

        for(int k = min_index + 1; k < n; k++){
            task_t *tmp = dequeue(q);
            fputs(tmp->ta_name, stdout);
            printf(" pop\n");
            fputs(tmp->ta_name, stdout);
            printf(" in\n");
            enqueue(q, tmp);
        }
        //printf("f\n");
        enqueue(q, min);
        fputs(min->ta_name, stdout);
        printf(" in\n");
        printf("end\n");
    }*/
    int index = 0;
    while(n - index > 0){
        int minimumValue = 2147483647;        //A larger number which never gets minimum
        int minimumIndex = -1;
        task_t *min;
        int index2 = 0;
        while(index2 < n - index){
            task_t *dequeued = dequeue(q);
            if(dequeued->pir <  minimumValue){
                minimumValue = dequeued->pir;
                min = dequeued;
                minimumIndex = index2;
            }
            index2++;
            enqueue(q, dequeued);
        }
        while(index2 < n){
            task_t *dequeued = dequeue(q);

            enqueue(q,dequeued);

            index2++;
        }
        index2 = 0;
        while(index2 < n - index){
            task_t *dequeued = dequeue(q);

            if(index2 != minimumIndex){
                enqueue(q, dequeued);
            }
            index2++;
        }

        while(index2 < n){
            task_t *dequeued = dequeue(q);

            enqueue(q,dequeued);

            index2++;
        }
        enqueue(q, min);
        index++;
    }
}

void sleep_count()
{
    int i = queue_size(&waiting_queue);
    if (i == 0)
    {
        // puts("0");
        return;
    }

    int cnt = 0;
    node *no = waiting_queue.front;
    task_t *arr[10000];
    for (int j = 0; j < i; j++)
    {
        (no->data->sleep_time)--;
        if (no->data->sleep_time == 0)
        {
            arr[cnt] = no->data;
            cnt++;
        }
        no = no->next;
    }
    // printf("%d\n", cnt);

    for (int j = 0; j < cnt; j++)
    {
        remove_q_ele(&waiting_queue, arr[j]);
        arr[j]->state = READY;
        enqueue(&ready_queue, arr[j]);
        // puts(ready_queue.front->data->ta_name);
    }
    return;
    // free(arr);
}

void rescources_count()
{
    // printf("a\n");
    int i = queue_size(&waiting_res);
    if (i == 0)
    {
        return;
    }

    int cnt = 0;
    node *no = waiting_res.front;
    task_t *arr[10000];
    for (int j = 0; j < i; j++)
    {
        int k = 0;
        for (k = 0; k < 8; k++)
        {
            if (no->data->need_res[k] == 1)
            {
                if (resource_check[k] == true)
                {
                    break;
                }
            }
        }
        if (k == 8)
        {
            arr[cnt] = no->data;
            cnt++;
        }
        no = no->next;
    }

    if (cnt == 0)
        return;

    for (int j = 0; j < cnt; j++)
    {
        for (int k = 0; k < 8; k++)
        {
            arr[j]->need_res[k] = 0;
        }
        remove_q_ele(&waiting_res, arr[j]);
        arr[j]->state = READY;
        enqueue(&ready_queue, arr[j]);
    }
    return;
}

void schedular()
{
    if (strcmp(alg, "RR") == 0)
    {
        task_t *prev, *next = NULL;
        setitimer(ITIMER_VIRTUAL, 0, 0);

        ++sig;
        //printf("Signal %d\n", sig);
        sleep_count();
        // printf("%d\n", queue_size(&ready_queue));
        // sleep_count();

        if(current != NULL){
            current->running_time++;
            current->turn_around_time++;
        }

        if (sig_check == 3)
        {
            sig_check = 0;
        }
        sig_check++;
        if (sig_check == 1)
        {
            rescources_count();
            // printf("%d\n", queue_size(&ready_queue));

            prev = current;

            if (!cancel_current && current->tid != 0)
            {
                prev->state = READY;
                enqueue(&ready_queue, prev);
            }
            else
            {
                cancel_current = 0;
            }

            next = dequeue(&ready_queue);

            if (next == NULL)
            {
                if (queue_size(&waiting_queue) != 0 || queue_size(&waiting_res) != 0)
                {
                    if(prev->state != IDLE)
                        printf("CPU idle\n");
                    setitimer(ITIMER_VIRTUAL, &value, 0);
                    cancel_current = 1;
                    next = idl;
                    current = next;

                    node* n = ready_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&ready_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_res.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_res); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }

                    swapcontext(&(prev->ctx), &(next->ctx));
                }
                else
                {
                    // puts("fin");
                    fin_check = 1;
                    swapcontext(&(prev->ctx), &(maincontext));
                }
            }
            else
            {

                printf("Task ");
                fputs(next->ta_name, stdout);
                printf(" is running.\n");
                next->state = RUNNING;
                // printf("%d\n",current->get_again);
                if (next->get_again == 1)
                {
                    // printf("a\n");
                    int list[next->res_cnt];
                    int b = 0;
                    for (int a = 0; a < 8; a++)
                    {
                        if (next->ge_res[a] == 1)
                        {
                            list[b] = a;
                            b++;
                        }
                    }
                    next->get_again = 0;

                    int x = 0;
                    for (int i = 0; i < next->res_cnt; i++)
                    {
                        if (resource_check[list[i]] == true)
                        {
                            x = 1;
                        }
                        next->ge_res[list[i]] = 1;
                        next->need_res[list[i]] = 1;
                    }
                    if (x == 1)
                    {
                        printf("Task ");
                        fputs(next->ta_name, stdout);
                        printf(" is waiting resource.\n");
                        next->state = WAITING;
                        next->get_again = 1;
                        enqueue(&waiting_res, next);
                        next = idl;
                        cancel_current = 1;
                    }
                    else
                    {
                        for (int i = 0; i < next->res_cnt; i++)
                        {
                            resource_check[list[i]] = true;
                            next->need_res[list[i]] = 0;
                            next->ge_res[list[i]] = 0;
                            next->have_res[list[i]] = 1;
                            printf("Task ");
                            fputs(next->ta_name, stdout);
                            printf(" gets resource %d.\n", list[i]);
                        }
                    }

                    // printf("b\n");
                }

                current = next;

                node* no = ready_queue.front;
                if(no != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        no->data->waiting_time++;
                        no = no->next;
                    }
                }

                node* n = ready_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_res.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_res); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }

                setitimer(ITIMER_VIRTUAL, &value, 0);
                // printf("c\n");
                swapcontext(&(prev->ctx), &(next->ctx));
            }
        }
        else
        {
            node* n = ready_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_res.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_res); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            node* no = ready_queue.front;
            if(no != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    //puts(no->data->ta_name);
                    no->data->waiting_time++;
                    no = no->next;
                }
                
            }
            setitimer(ITIMER_VIRTUAL, &value, 0);
        }

    }
    else if (strcmp(alg, "FCFS") == 0)
    {
        task_t *prev, *next = NULL;
        setitimer(ITIMER_VIRTUAL, 0, 0);

        ++sig;
        //printf("Signal %d\n", sig);
        sleep_count();

        
        if(current != NULL){
            current->running_time++;
            current->turn_around_time++;
        }

        if (current->state != RUNNING)
        {
            rescources_count();
            // printf("%d\n", queue_size(&ready_queue));

            prev = current;

            if (!cancel_current && current->tid != 0)
            {
                prev->state = READY;
                enqueue(&ready_queue, prev);
            }
            else
            {
                cancel_current = 0;
            }

            next = dequeue(&ready_queue);
            //puts(next->ta_name);

            if (next == NULL)
            {
                if (queue_size(&waiting_queue) != 0 || queue_size(&waiting_res) != 0)
                {
                    if(prev->state != IDLE)
                        printf("CPU idle\n");
                    setitimer(ITIMER_VIRTUAL, &value, 0);
                    cancel_current = 1;
                    next = idl;
                    current = next;
                    node* n = ready_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&ready_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_res.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_res); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    swapcontext(&(prev->ctx), &(next->ctx));
                }
                else
                {
                    //puts("fin");
                    fin_check = 1;
                    swapcontext(&(prev->ctx), &(maincontext));
                }
            }
            else
            {
                //next->waiting_time++;
                printf("Task ");
                fputs(next->ta_name, stdout);
                printf(" is running.\n");
                next->state = RUNNING;
                // printf("%d\n",current->get_again);
                if (next->get_again == 1)
                {
                    // printf("a\n");
                    int list[next->res_cnt];
                    int b = 0;
                    for (int a = 0; a < 8; a++)
                    {
                        if (next->ge_res[a] == 1)
                        {
                            list[b] = a;
                            b++;
                        }
                    }
                    next->get_again = 0;

                    int x = 0;
                    for (int i = 0; i < next->res_cnt; i++)
                    {
                        if (resource_check[list[i]] == true)
                        {
                            x = 1;
                        }
                        next->ge_res[list[i]] = 1;
                        next->need_res[list[i]] = 1;
                    }
                    if (x == 1)
                    {
                        printf("Task ");
                        fputs(next->ta_name, stdout);
                        printf(" is waiting resource.\n");
                        next->state = WAITING;
                        next->get_again = 1;
                        enqueue(&waiting_res, next);
                        next = idl;
                        cancel_current = 1;
                    }
                    else
                    {
                        for (int i = 0; i < next->res_cnt; i++)
                        {
                            resource_check[list[i]] = true;
                            next->need_res[list[i]] = 0;
                            next->ge_res[list[i]] = 0;
                            next->have_res[list[i]] = 1;
                            printf("Task ");
                            fputs(next->ta_name, stdout);
                            printf(" gets resource %d.\n", list[i]);
                        }
                    }

                    // printf("b\n");
                }
                node *no = ready_queue.front;
                if(no != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        no->data->waiting_time++;
                        no = no->next;
                    }
                }

                node* n = ready_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_res.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_res); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }

                current = next;
                
                

                setitimer(ITIMER_VIRTUAL, &value, 0);
                // printf("c\n");
                swapcontext(&(prev->ctx), &(next->ctx));
            }
        }
        else
        {
            node* n = ready_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_res.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_res); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            node *no = ready_queue.front;
            if(no != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    no->data->waiting_time++;
                    no = no->next;
                }
            }
            setitimer(ITIMER_VIRTUAL, &value, 0);
        }
    }
    else if (strcmp(alg, "PP") == 0)
    {
        task_t *prev, *next = NULL;
        setitimer(ITIMER_VIRTUAL, 0, 0);

        ++sig;
        //printf("Signal %d\n", sig);
        queue_sort(&ready_queue);
        sleep_count();

        if(current != NULL){
            current->running_time++;
            current->turn_around_time++;
        }

        if (current->state != RUNNING)
        {
            rescources_count();
            // printf("%d\n", queue_size(&ready_queue));

            prev = current;

            if (!cancel_current && current->tid != 0)
            {
                prev->state = READY;
                enqueue(&ready_queue, prev);
            }
            else
            {
                cancel_current = 0;
            }

            next = dequeue(&ready_queue);
            //puts("a");

            if (next == NULL)
            {
                //puts("c");
                if (queue_size(&waiting_queue) != 0 || queue_size(&waiting_res) != 0)
                {
                    node* n = ready_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&ready_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_queue.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_queue); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }
                    n = waiting_res.front;
                    if(n != NULL){
                        for(int i = 0; i < queue_size(&waiting_res); i++){
                            n->data->turn_around_time++;
                            n = n->next;
                        }
                    }

                    if(prev->state != IDLE)
                        printf("CPU idle\n");
                    setitimer(ITIMER_VIRTUAL, &value, 0);
                    cancel_current = 1;
                    next = idl;
                    current = next;
                    swapcontext(&(prev->ctx), &(next->ctx));
                }
                else
                {
                    //puts("fin");
                    fin_check = 1;
                    swapcontext(&(prev->ctx), &(maincontext));
                }
            }
            else
            {
                printf("Task ");
                fputs(next->ta_name, stdout);
                printf(" is running.\n");
                next->state = RUNNING;
                // printf("%d\n",current->get_again);
                if (next->get_again == 1)
                {
                    // printf("a\n");
                    int list[next->res_cnt];
                    int b = 0;
                    for (int a = 0; a < 8; a++)
                    {
                        if (next->ge_res[a] == 1)
                        {
                            list[b] = a;
                            b++;
                        }
                    }
                    next->get_again = 0;

                    int x = 0;
                    for (int i = 0; i < next->res_cnt; i++)
                    {
                        if (resource_check[list[i]] == true)
                        {
                            x = 1;
                        }
                        next->ge_res[list[i]] = 1;
                        next->need_res[list[i]] = 1;
                    }
                    if (x == 1)
                    {
                        printf("Task ");
                        fputs(next->ta_name, stdout);
                        printf(" is waiting resource.\n");
                        next->state = WAITING;
                        next->get_again = 1;
                        enqueue(&waiting_res, next);
                        next = idl;
                        cancel_current = 1;
                    }
                    else
                    {
                        for (int i = 0; i < next->res_cnt; i++)
                        {
                            resource_check[list[i]] = true;
                            next->need_res[list[i]] = 0;
                            next->ge_res[list[i]] = 0;
                            next->have_res[list[i]] = 1;
                            printf("Task ");
                            fputs(next->ta_name, stdout);
                            printf(" gets resource %d.\n", list[i]);
                        }
                    }

                    // printf("b\n");
                }
                node* n = ready_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_queue.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_queue); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }
                n = waiting_res.front;
                if(n != NULL){
                    for(int i = 0; i < queue_size(&waiting_res); i++){
                        n->data->turn_around_time++;
                        n = n->next;
                    }
                }

                current = next;
                node* no = ready_queue.front;
                if(no != NULL){
                    for(int i = 0; i < queue_size(&ready_queue); i++){
                        no->data->waiting_time++;
                        no = no->next;
                    }
                   
                }
                setitimer(ITIMER_VIRTUAL, &value, 0);
                // printf("c\n");
                swapcontext(&(prev->ctx), &(next->ctx));
            }
        }
        else
        {
            node* n = ready_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_queue.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_queue); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }
            n = waiting_res.front;
            if(n != NULL){
                for(int i = 0; i < queue_size(&waiting_res); i++){
                    n->data->turn_around_time++;
                    n = n->next;
                }
            }

            node* no = ready_queue.front;
            if(no != NULL){
                for(int i = 0; i < queue_size(&ready_queue); i++){
                    no->data->waiting_time++;
                    no = no->next;
                }
            }
            setitimer(ITIMER_VIRTUAL, &value, 0);
        }
    }
}

void task_run(void *(*start_routine)(void))
{
    task_break(start_routine());
    return;
}

task_t *find(int id){
    node *no = ready_queue.front;
    if(no != NULL){
        for(int i = 0; i < queue_size(&ready_queue); i++){
            if(no->data->tid == id){
                return no->data;
            }
            no = no->next;
        }
    }
    no = waiting_queue.front;
    if(no != NULL){
        for(int i = 0; i < queue_size(&waiting_queue); i++){
            if(no->data->tid == id){
                return no->data;
            }
            no = no->next;
        }
    }
    no = waiting_res.front;
    if(no != NULL){
        for(int i = 0; i < queue_size(&waiting_res); i++){
            if(no->data->tid == id){
                return no->data;
            }
            no = no->next;
        }
    }
    no = finish_queue.front;
    if(no != NULL){
        for(int i = 0; i < queue_size(&finish_queue); i++){
            if(no->data->tid == id){
                return no->data;
            }
            no = no->next;
        }
    }

    if(current->tid == id){
        return current;
    }
}

void print(){
    printf(" TID|       name|      state| running| waiting| turnaround| resources| priority\n");
    printf("-------------------------------------------------------------------------------\n");
    for(int i = 0; i < total_task_count; i++){
        task_t *t = find(i + 1);
        printf("%4d|%11s", t->tid, t->ta_name);
        //fputs(t->ta_name, stdout);
        if(t->state == RUNNING){
            fputs("|    RUNNING|", stdout);
        }
        else if(t->state == WAITING){
            fputs("|    WAITING|", stdout);
        }
        else if(t->state == TERMINATED){
            fputs("| TERMINATED|", stdout);
        }
        else if(t->state == READY){
            fputs("|      READY|", stdout);
        }

        printf("%8d|", t->running_time);
        printf("%8d|", t->waiting_time);
        if(t->state == TERMINATED){
            printf("%11d|", t->turn_around_time);
        }
        else{
            fputs("       none|", stdout);
        }
        
        char* str = malloc(10 * sizeof(char));
        int c = 8;
        for(int j = 0; j < 8; j++){
            if(t->have_res[j] == 1){
                char a[2];
                sprintf(a, "%d", j);
                a[1] = ' ';
                strcat(str, a);
                c--;
            }
        }
        if(c == 8){
            fputs("      none|", stdout);
        }
        else{
            printf("%10s|", str);
        }

        if(strcmp(alg, "PP") == 0){
            printf("%9d\n", t->pir);
        }
        else{
            puts("     none");
        }
    }
    
}

void task_create(void *(*start_routine)(void), char *name, int pir)
{

    printf("Task ");
    fputs(name, stdout);
    printf(" is ready.\n");
    task_t *t = malloc(sizeof(task_t));
    t->tid = ++total_task_count;
    t->ta_name = malloc(sizeof(char *));
    strcpy(t->ta_name, name);
    t->pir = pir;
    //printf("%d\n", pir);
    t->state = READY;
    t->running_time = 0;
    t->waiting_time = 0;
    t->turn_around_time = 0;
    t->sleep_time = 0;
    t->get_again = 0;
    t->res_cnt = 0;
    for (int i = 0; i < 8; i++)
    {
        t->need_res[i] = 0;
        t->ge_res[i] = 0;
        t->have_res[i] = 0;
    }
    getcontext(&(t->ctx));

    t->ctx.uc_link = &maincontext;
    t->ctx.uc_stack.ss_sp = malloc(1024 * 248);
    t->ctx.uc_stack.ss_size = 1024 * 248;

    makecontext(&(t->ctx), (void (*)())task_run, 1, start_routine);
    enqueue(&ready_queue, t);
}

void task_break(void *retval)
{
    current->retval = retval;
    enqueue(&finish_queue, current);
    task_cancel(task_self());
}

int task_cancel(task_t t)
{
    task_t *tmp;
    int found = 0;

    for (int i = 0; i < queue_size(&ready_queue); i++)
    {
        tmp = get_element_from_queue(&ready_queue, i);
        if (tmp->tid == t.tid)
        {
            if (!remove_q_ele(&ready_queue, tmp))
            {
                return 1;
            }
        }
        else
        {
            found = 1;
            break;
        }
    }

    if (t.tid == current->tid)
    {
        cancel_current = 1;
        sig_check = 3;
        schedular();
        return 0;
    }
}

task_t task_self()
{
    return *current;
}

void ini()
{
    queue_init(&ready_queue);
    queue_init(&finish_queue);
    queue_init(&waiting_queue);
    queue_init(&waiting_res);
}

void ctz(int sig_num){
    signal(SIGTSTP, ctz);
    //printf("Cannot execute Ctrl+Z\n");
    setitimer(ITIMER_VIRTUAL, 0, 0);
    fin_check = 2;
    if(current->state != IDLE)
        enqueue(&ready_queue, current);
    swapcontext(&(current->ctx), &(maincontext));
}
/*
void restart(){
    fin_cnt = 0;
    fin_check = 0;
    setitimer(ITIMER_VIRTUAL, &value, 0);
    while(1);
    //setcontext(&(current->ctx));
}*/

void start_simulated()
{
    //puts(alg);
    //printf("%d\n", fin_check);
    puts("Start simulation.");
    getcontext(&maincontext);

    if (fin_check == 1)
    {
        printf("Simulation over.\n");
        fin_check = 0;
        return;
    }
    else if(fin_check == 2){
        fin_check = 0;
        return;
    }
    fin_cnt = 0;
    for (int i = 0; i < 8; i++)
    {
        resource_check[i] = false;
    }
    fin_check = 0;
    // puts(algo);
    idl = malloc(sizeof(task_t));

    getcontext(&(idl->ctx));
    idl->ctx.uc_link = &maincontext;
    idl->ctx.uc_stack.ss_sp = malloc(1024 * 248);
    idl->ctx.uc_stack.ss_size = 1024 * 248;

    makecontext(&(idl->ctx), (void (*)())idle, 0);

    signal(SIGTSTP, ctz);

    idl->tid = 0;
    idl->ta_name = "i";
    idl->state = IDLE;
    signal(SIGVTALRM, schedular);
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 10000;
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = 10000;
    setitimer(ITIMER_VIRTUAL, &value, &ovalue);

    main_task.tid = 0;
    main_task.ta_name = "m";
    getcontext(&main_task.ctx);
    current = &main_task;
    /*
        signal(SIGVTALRM, sigroutine);

        value.it_value.tv_sec = 0;

        value.it_value.tv_usec = 500000;

        value.it_interval.tv_sec = 0;

        value.it_interval.tv_usec = 500000;

        setitimer(ITIMER_VIRTUAL, &value, &ovalue);
    */
    for(;;);
}

void task_sleep(int ms)
{
    task_t *t = current;
    printf("Task ");
    fputs(t->ta_name, stdout);
    printf(" goes to sleep.\n");
    t->sleep_time = ms;
    t->state = WAITING;
    cancel_current = 1;
    remove_q_ele(&ready_queue, t);
    enqueue(&waiting_queue, t);
    sig_check = 3;
    schedular();
}

void task_exit()
{
    task_t *t = current;
    printf("Task ");
    fputs(t->ta_name, stdout);
    printf(" has terminated.\n");
    t->state = TERMINATED;
    enqueue(&finish_queue, t);
    remove_q_ele(&ready_queue, t);
    cancel_current = 1;
    sig_check = 3;
    schedular();
}

void task_kill(char *name){
    printf("Task ");
    fputs(name, stdout);
    printf(" is killed.\n");
    int n = queue_size(&ready_queue);
    node *no = ready_queue.front;
    for(int i = 0; i < n; i++){
        if(strcmp(no->data->ta_name, name) == 0){
            remove_q_ele(&ready_queue, no->data);
            no->data->state = TERMINATED;
            enqueue(&finish_queue, no->data);
            return;
        }
        no = no->next;
    }

    n = queue_size(&waiting_queue);
    no = waiting_queue.front;
    for(int i = 0; i < n; i++){
        if(strcmp(no->data->ta_name, name) == 0){
            remove_q_ele(&waiting_queue, no->data);
            no->data->state = TERMINATED;
            enqueue(&finish_queue, no->data);
            return;
        }
        no = no->next;
    }
}

void get_re(int count, int *resources)
{
    task_t *t = current;
    bool b = false;
    int i = 0;
    t->res_cnt = count;
    for (i = 0; i < count; i++)
    {
        if (resource_check[resources[i]] == true)
        {
            b = true;
        }
        t->ge_res[resources[i]] = 1;
        t->need_res[resources[i]] = 1;
    }

    if (b)
    {
        printf("Task ");
        fputs(t->ta_name, stdout);
        printf(" is waiting resource.\n");
        t->state = WAITING;
        t->get_again = 1;
        cancel_current = 1;
        remove_q_ele(&ready_queue, t);
        enqueue(&waiting_res, t);
        sig_check = 3;
        schedular();
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            resource_check[resources[i]] = true;
            t->need_res[resources[i]] = 0;
            t->ge_res[resources[i]] = 0;
            t->have_res[resources[i]] = 1;
            printf("Task ");
            fputs(t->ta_name, stdout);
            printf(" gets resource %d.\n", resources[i]);
        }
    }
}

void release_re(int count, int *resources)
{
    task_t *t = current;
    for (int i = 0; i < count; i++)
    {
        resource_check[resources[i]] = false;
        t->have_res[resources[i]] = 0;
        printf("Task ");
        fputs(t->ta_name, stdout);
        printf(" releases resource %d.\n", resources[i]);
    }
}