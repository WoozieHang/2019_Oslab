#include "lib.h"
#include "types.h"

int data = 0;
#define MY_TEST 1
#ifdef MY_TEST
#define INTERVAL 10

void produce(int pid,sem_t full,sem_t mutex){
    for(int i=1;i<9;i++){
        printf("pid %d,producer %d,try lock\n",pid,pid-1);
       sleep(INTERVAL);
        sem_wait(&mutex);
       sleep(INTERVAL);
        printf("pid %d,producer %d,locked\n",pid,pid-1);
       sleep(INTERVAL);
        printf("pid %d,producer %d,produce product %d\n",pid,pid-1,i);
        sleep(INTERVAL);
        sem_post(&full);
        sem_post(&mutex);
        sleep(INTERVAL);
        printf("pid %d,producer %d,unlock\n",pid,pid-1);
       
    }
     printf("pid %d,producer %d,finished\n",pid,pid-1);
     sleep(INTERVAL);
}

void consume(int pid,sem_t full,sem_t mutex){
    for(int i=1;i<5;i++){
        printf("pid %d,consumer %d,try consume product %d\n",pid,pid-3,i);
        sem_wait(&full);
       sleep(INTERVAL);
        printf("pid %d,consumer %d,try lock\n",pid,pid-3);
        sem_wait(&mutex);
        sleep(INTERVAL);
        printf("pid %d,consumer %d,locked\n",pid,pid-3);
       sleep(INTERVAL);
        printf("pid %d,consumer %d,consumed product %d\n",pid,pid-3,i);
        sleep(INTERVAL);
        sem_post(&mutex);
       
        
        printf("pid %d,consumer %d,unlock\n",pid,pid-3);
    }
    printf("pid %d,consumer %d,finished\n",pid,pid-3);
    sleep(INTERVAL);
}
#endif


#ifndef MY_TEST   
int uEntry(void) { 
    int i = 4;
    int ret = 0;
    sem_t sem;
    printf("Father Process: Semaphore Initializing.\n");
    ret = sem_init(&sem, 2);
    if (ret == -1) {
        printf("Father Process: Semaphore Initializing Failed.\n");
        exit();
    }

    ret = fork();
    if (ret == 0) {
        while( i != 0) {
            i --;
            printf("Child Process: Semaphore Waiting.\n");
            sem_wait(&sem);
            printf("Child Process: In Critical Area.\n");
        }
        printf("Child Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
    }
    else if (ret != -1) {
        while( i != 0) {
            i --;
            printf("Father Process: Sleeping.\n");
            sleep(128);
            printf("Father Process: Semaphore Posting.\n");
            sem_post(&sem);
        }
        printf("Father Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
        
    }
    return 0;
}
#else
int uEntry(void) {
    sem_t full;
    sem_t mutex;
    sem_init(&full, 0);
    sem_init(&mutex,1);
    int ret=1;
    for(int i=0;i<6;i++){
        if(ret!=0){
            ret=fork();
        } 
        else {
            break;
        }
    }
    if(ret==0){
        
        int pid=getpid();
        //printf("child pid:%d\n",pid);
        if(pid==2||pid==3)
          produce(pid,full,mutex);
    
        else if(pid>3) consume(pid,full,mutex);
        //printf("pid:%d eixt\n",pid);
       exit();
    }

    else {
       while(1);
       exit();
    }
    return 0;
}
#endif

