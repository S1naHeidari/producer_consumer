#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "getline.h"
#include "tands.h"
#include <time.h>

/*
This program provides a possible solution for producer-consumer problem using mutex and semaphore.
I have used 5 producers and 5 consumers to demonstrate the solution. You can always play with these values.
*/
// declare semaphors
sem_t empty;
sem_t full;
// in and out will be used as indexes of our shared buffer
int in = 0;
int out = 0;
// declare the mutex
pthread_mutex_t mutex;
// log number (0 by default)
int log_id = 0;
// variable to store number of task inside the buffer
int pending = 0;
// number of Tranactions
int works = 0;
// number of ask events
int asks = 0;
// number of receive events
int receives = 0;
// number of complete events
int completes = 0;
// number of sleep events
int sleeps = 0;

// argument struct for thread routines
struct arg_struct {
    int cno;
    int BufferSize;
    int *buffer;
    clock_t start;
    int *completes;
};

void *producer(void *arguments)
{
    struct arg_struct *args = arguments;
    // open inputexample file.
    FILE *fp = fopen("inputexample", "r");
    if(fp == NULL) {
        perror("Unable to open file!");
        exit(1);
    }
    // Read lines from a text file using our own a portable getline implementation
    char *line = NULL;
    size_t len = 0;
    int item;
    // store log file name in dest_log variable
    char dest_log[20] = "prodcon";
    if(log_id>0){
        snprintf(dest_log, 20, "prodcon.%d.", log_id);
    }else if(log_id == 0){
        strcat(dest_log, ".");
    }
    strcat(dest_log, "log");
    // Read T(s) and S(s) line by line
    while(my_getline(&line, &len, fp) != -1) {
        if(line[0] == 'T'){
            // add to pending tasks
            pending++;
            // add to total tasks
            works++;
            // open log file in append mode
            FILE *fptr = fopen(dest_log,"a");
            if(fptr == NULL){
              printf("Error!");
              exit(1);
            }
            // write "Work" event to the log file
            fprintf(fptr, "%.3lf ID=%2d Q=%2d %-8s %5c\n", (double)(clock() - args ->start) / CLOCKS_PER_SEC, args->cno, pending, "Work", line[1]);
            fclose(fptr);
            // wait if buffer is full
            sem_wait(&empty);
            pthread_mutex_lock(&mutex);
            item = (int)line[1];
            // store the task id in the buffer
            args -> buffer[in] = item - 48;
            // find index of the new task
            in = (in+1)% args -> BufferSize;
            // unlock mutex so consumers could access the buffer
            pthread_mutex_unlock(&mutex);
            sem_post(&full);
        // if the task is sleep
        }else if(line[0] == 'S'){
            // add to sleep events
            sleeps++;
            FILE *fptr = fopen(dest_log,"a");
            if(fptr == NULL){
              printf("Error!");
              exit(1);
            }
            // write "Sleep" event to the log file
            fprintf(fptr, "%.3lf ID=%2d %4s %-8s %5c\n", (double)(clock() - args ->start) / CLOCKS_PER_SEC, args->cno, "", "Sleep", line[1]);
            fclose(fptr);
            // do the sleeping
            Sleep((int)line[1]);
        }
    }
    FILE *fptr = fopen(dest_log,"a");
    if(fptr == NULL){
      printf("Error!");
      exit(1);
    }
    // write "End" event to the log file
    fprintf(fptr, "%.3lf ID=%2d %4s %-8s\n", (double)(clock() - args ->start) / CLOCKS_PER_SEC, args->cno, "", "End");
    fclose(fp);
}
void *consumer(void *arguments)
{
    // store name of the log file
    char dest_log[20] = "prodcon";
    if(log_id>0){
        snprintf(dest_log, 20, "prodcon.%d.", log_id);
    }else if(log_id == 0){
        strcat(dest_log, ".");
    }
    strcat(dest_log, "log");

    struct arg_struct *args = arguments;
    while(1){
        // add to ask events
        asks++;
        FILE *fptr = fopen(dest_log,"a");
        if(fptr == NULL){
          printf("Error!");
          exit(1);
        }
        // write "Ask" events in the log file
        fprintf(fptr, "%.3lf ID=%2d %4s %-8s\n",(double)(clock() - args ->start) / CLOCKS_PER_SEC, args ->cno,"", "Ask");
        fclose(fptr);
        // wait if the buffer is empty
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        // remove the task id in the buffer
        int item = args ->buffer[out];
        // decrease pending tasks in the buffer
        pending--;
        // increase received tasks by threads
        receives++;
        fptr = fopen(dest_log,"a");
        if(fptr == NULL){
          printf("Error!");
          exit(1);
        }
        char item_str[2];
        snprintf(item_str, 2, "%d", item);
        // write the "Recieve" event to the log file
        fprintf(fptr, "%.3lf ID=%2d Q=%2d %-8s %5s\n", (double)(clock() - args ->start) / CLOCKS_PER_SEC, args->cno, pending, "Receive", item_str);
        fclose(fptr);
        // change the out index of the buffer
        out = (out+1) % args ->BufferSize;
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        completes++;
        // add to completed tasks by the thread
        args ->completes[args ->cno - 1] = args ->completes[args ->cno - 1] + 1;
        fptr = fopen(dest_log,"a");
        if(fptr == NULL){
          printf("Error!");
          exit(1);
        }
        char item_str_complete[2];
        snprintf(item_str_complete, 2, "%d", item);
        // write completed event to the log file
        fprintf(fptr, "%.3lf ID=%2d %4s %-8s %5s\n", (double)(clock() - args ->start) / CLOCKS_PER_SEC, args->cno, "", "Complete", item_str_complete);
        fclose(fptr);
    }
}


int main(int argc, char * argv[])
{   // store current time
    clock_t begin = clock();
    int con_count, i;
    // store prodcon arguments
    if(argc == 2){
        con_count = atoi(argv[1]);
    // if the log id is passed
    }else if(argc == 3){
        con_count = atoi(argv[1]);
        log_id = atoi(argv[2]);
    }
    char dest_log[20] = "prodcon";
    if(log_id>0){
        snprintf(dest_log, 20, "prodcon.%d.", log_id);
    }else if(log_id == 0){
        strcat(dest_log, ".");
    }
    strcat(dest_log, "log");
    // store buffer size
    int buffer_size = atoi(argv[1]) * 2;
    // create buffer dynamicaly
    int *buffer = (int*)malloc(buffer_size*sizeof(int));
    // create the array for storing completed tasks for each thread
    int *thread_completes = (int*)malloc(con_count*sizeof(int));
    // declare threads
    pthread_t pro[1],con[con_count];
    pthread_mutex_init(&mutex, NULL);
    // init semaphors
    sem_init(&empty,0,buffer_size);
    sem_init(&full,0,0);
    // declare arg_struct for producer
    struct arg_struct args_producer;
    args_producer.cno = 0;
    args_producer.BufferSize = buffer_size;
    args_producer.buffer = buffer;
    args_producer.start = begin;
    pthread_create(&pro[0], NULL, (void *)producer, (void *)&args_producer);
    // declare arg_struct array for consumers
    struct arg_struct args_consumer[con_count];
    for(int i = 0; i < con_count; i++) {
        args_consumer[i];
        args_consumer[i].cno = i + 1;
        args_consumer[i].BufferSize = buffer_size;
        args_consumer[i].buffer = buffer;
        args_consumer[i].start = begin;
        args_consumer[i].completes = thread_completes;
        pthread_create(&con[i], NULL, (void *)consumer, (void *)&args_consumer[i]);
    }
    pthread_join(pro[0], NULL);
    // store finish time
    double time_spent = (double)(clock() - begin) / CLOCKS_PER_SEC;
    // calculate "number of transaction per second"
    double tpers = completes/time_spent;
    for(int i = 0; i < con_count; i++){
        pthread_cancel(con[i]);
    }
    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    // open log file for writing summary
    FILE *fptr = fopen(dest_log,"a");
    if(fptr == NULL){
      printf("Error!");
      exit(1);
    }
    fprintf(fptr, "Summary\n");
    fprintf(fptr, "    %-8s %4d\n","Work", works);
    fprintf(fptr, "    %-8s %4d\n","Ask", asks);
    fprintf(fptr, "    %-8s %4d\n","Receive", receives);
    fprintf(fptr, "    %-8s %4d\n","Complete", completes);
    fprintf(fptr, "    %-8s %4d\n","Sleep", sleeps);
    for(i = 0; i < con_count; i++){
        fprintf(fptr, "    %-8s%d%4d\n","Thread",i+1,thread_completes[i]);
    }
    fprintf(fptr, "Transactions per second: %.2lf\n", tpers);
    fclose(fptr);
    return 0;
}
