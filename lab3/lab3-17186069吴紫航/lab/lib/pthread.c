#include "pthread.h"
#include "lib.h"
/*
 * pthread lib here
 * 用户态多线程写在这
 */
 
ThreadTable tcb[MAX_TCB_NUM];
int current;

void pthread_initial(void){
    int i;
    for (i = 0; i < MAX_TCB_NUM; i++) {
        tcb[i].state = STATE_DEAD;
        tcb[i].joinid = -1;
    }
    tcb[0].state = STATE_RUNNING;
    tcb[0].pthid = 0;
    //printf("\n");
    current = 0; // main thread
    return;
}

void pthread_schedule(){
    //printf("cur:%x\n",current);
    //for(int i=0;i<MAX_TCB_NUM;i++)
       // printf("state%x:%x,",i,tcb[i].state);


   
    uint32_t i=(current+1)%MAX_TCB_NUM;
    uint32_t j;
    for(j=i;j!=current;j=(j+1)%MAX_TCB_NUM){
        if(tcb[j].state==STATE_RUNNABLE){
            break;
        }
    }
    //if((current!=0)&&(tcb[current].state==STATE_RUNNABLE))
       // j=current;
     //printf("%d->%d\n",current,j);
    if(j!=current){
    current=j;
   // uint32_t index;
   // for(index=tcb[j].cont.ebp;index>tcb[j].cont.esp;index-=4){
   //    printf("%x,",*(uint32_t*)index);
    //}
    //printf(",%x\n",*(uint32_t *)tcb[j].cont.esp);

    tcb[j].state=STATE_RUNNING;
    asm volatile("movl %0, %%eax"::"m"(tcb[j].cont.eax));
	asm volatile("movl %0, %%ecx"::"m"(tcb[j].cont.ecx));
	asm volatile("movl %0, %%edx"::"m"(tcb[j].cont.edx));
	asm volatile("movl %0, %%ebx"::"m"(tcb[j].cont.ebx));
	asm volatile("movl %0, %%esi"::"m"(tcb[j].cont.esi));
	asm volatile("movl %0, %%edi"::"m"(tcb[j].cont.edi));
    asm volatile("movl %0, %%esp"::"m"(tcb[j].cont.esp));
    asm volatile("movl %0, %%ebp"::"m"(tcb[j].cont.ebp));
    //asm volatile("leave");
    return;
    //asm volatile("ret");
    
}
else {
    printf(" ");
    while(1);
    tcb[j].state=STATE_RUNNING;
    asm volatile("movl %0, %%eax"::"m"(tcb[j].cont.eax));
	asm volatile("movl %0, %%ecx"::"m"(tcb[j].cont.ecx));
	asm volatile("movl %0, %%edx"::"m"(tcb[j].cont.edx));
	asm volatile("movl %0, %%ebx"::"m"(tcb[j].cont.ebx));
	asm volatile("movl %0, %%esi"::"m"(tcb[j].cont.esi));
	asm volatile("movl %0, %%edi"::"m"(tcb[j].cont.edi));
    asm volatile("movl %0, %%esp"::"m"(tcb[j].cont.esp));
    asm volatile("movl %0, %%ebp"::"m"(tcb[j].cont.ebp));
    //asm volatile("leave");
    return;
} 
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg){
    int i=(current+1)%MAX_TCB_NUM;
    int j;
    for(j=i;j!=current;j=(j+1)%MAX_TCB_NUM)
        if(tcb[j].state==STATE_DEAD)
            break;

    if(j!=current){
        tcb[j].state=STATE_RUNNABLE;
        tcb[j].joinid=-1;
        tcb[j].pthArg=(uint32_t)arg;
        tcb[j].pthid=j;
        *thread = j;
       // uint32_t eax,ebx,ecx,edi,edx,esi;
        asm volatile("movl %%eax, %0":"=m"(tcb[j].cont.eax));
        asm volatile("movl %%ebx, %0":"=m"(tcb[j].cont.ebx));
        asm volatile("movl %%ecx, %0":"=m"(tcb[j].cont.ecx));
        asm volatile("movl %%edi, %0":"=m"(tcb[j].cont.edi));
        asm volatile("movl %%edx, %0":"=m"(tcb[j].cont.edx));
        asm volatile("movl %%esi, %0":"=m"(tcb[j].cont.esi));

        tcb[j].stackTop=(uint32_t)&(tcb[j].stack[MAX_STACK_SIZE]);
        tcb[j].cont.ebp=tcb[j].stackTop;
        tcb[j].cont.esp=tcb[j].stackTop;
        *(uint32_t *)(tcb[j].cont.ebp)=tcb[j].pthArg;

        tcb[j].cont.eip=(uint32_t )start_routine;
        tcb[j].cont.esp-=8;
        *(uint32_t *)tcb[j].cont.esp=tcb[j].cont.eip;
        tcb[j].cont.esp-=4;
         *(uint32_t *)tcb[j].cont.esp=tcb[j].cont.ebp;
         tcb[j].cont.ebp=tcb[j].cont.esp;
        return 0;
    }

    else return -1;
}

void pthread_exit(void *retval){
    //printf(" exit ");
    tcb[current].state=STATE_DEAD;
    //awake the pthread joining current pthread
    uint32_t joinid=tcb[current].joinid;
    //printf("%d dead,joinid:%d",current,joinid);
    if(joinid!=-1)
        tcb[joinid].state=STATE_RUNNABLE;
    pthread_schedule();
    return;
}


int pthread_join(pthread_t thread, void **retval){
   //printf("%d join %d ",current,thread);
   if(tcb[thread].state==STATE_DEAD)
        return -1;
    else{
    tcb[current].state=STATE_BLOCKED;
    tcb[thread].joinid=current;
    asm volatile("movl %%eax, %0":"=m"(tcb[current].cont.eax));
    asm volatile("movl %%ebx, %0":"=m"(tcb[current].cont.ebx));
    asm volatile("movl %%ecx, %0":"=m"(tcb[current].cont.ecx));
    asm volatile("movl %%edi, %0":"=m"(tcb[current].cont.edi));
    asm volatile("movl %%edx, %0":"=m"(tcb[current].cont.edx));
    asm volatile("movl %%esi, %0":"=m"(tcb[current].cont.esi));
    asm volatile("movl %%ebp, %0":"=m"(tcb[current].cont.ebp));
    asm volatile("movl %%esp, %0":"=m"(tcb[current].cont.esp));

    //tcb[current].cont.eip=*(uint32_t*)(tcb[current].cont.ebp+4);
    //tcb[current].cont.esp-=4;
   // *(uint32_t *)tcb[current].cont.esp=tcb[current].cont.eip;
   // tcb[current].cont.esp=tcb[current].cont.ebp+16;
   // tcb[current].cont.ebp=*(uint32_t*)tcb[current].cont.ebp;
    pthread_schedule();
    return 0;
    }
}

int pthread_yield(void){
    //printf(" yield ");
    tcb[current].state=STATE_RUNNABLE;
    asm volatile("movl %%eax, %0":"=m"(tcb[current].cont.eax));
    asm volatile("movl %%ebx, %0":"=m"(tcb[current].cont.ebx));
    asm volatile("movl %%ecx, %0":"=m"(tcb[current].cont.ecx));
    asm volatile("movl %%edi, %0":"=m"(tcb[current].cont.edi));
    asm volatile("movl %%edx, %0":"=m"(tcb[current].cont.edx));
    asm volatile("movl %%esi, %0":"=m"(tcb[current].cont.esi));
    asm volatile("movl %%ebp, %0":"=m"(tcb[current].cont.ebp));
    asm volatile("movl %%esp, %0":"=m"(tcb[current].cont.esp));
    //tcb[current].cont.eip=*(uint32_t*)(tcb[current].cont.ebp+4);
    //tcb[current].cont.esp-=4;
    //*(uint32_t *)tcb[current].cont.esp=tcb[current].cont.eip;
    //tcb[current].cont.esp=tcb[current].cont.ebp+8;
    //tcb[current].cont.ebp=*(uint32_t*)tcb[current].cont.ebp;
    pthread_schedule();
    return 0;
}


