#include <semaphore.h>
#include<stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#define maxcount 1000

int cab_no,rider_no,server_no;

typedef struct Server{
    bool in_use;
    pthread_t server_thread;
    pthread_mutex_t server_lock;
}server;

typedef struct Rider{
    bool cabtype; ///0 if pool 1 if premier
    long int maxwait;
    int ridetime;
    int arrivaltime;

    int pno;
    pthread_t rider_thread;
    pthread_mutex_t rider_lock;
    int s_no;
}rider;

typedef struct Driver{
    int pno;
    bool type; ///1 if cab is premier type, 0 if cab is pool type

    bool waitstate;
    bool onPrem;
    bool onPool1;
    bool onPool2;

    pthread_mutex_t lock;

}driver;


driver d[maxcount];
rider r[maxcount];
server s[maxcount];

void bookcab(rider *current, driver *pDriver,bool *booked);

void* accept_payment(void *arg);

void *rider_f(void *args);

void initialize_driver(int no);

int random_between(int l, int e)
{
    return l + rand() % (e - l + 1);
}

int main(){
    srand(time(NULL));
    printf("Enter number of cabs, riders and servers ");
    scanf("%d %d %d",&cab_no,&rider_no,&server_no);

    for (int m = 0; m < server_no; ++m) {
        s->in_use=false;
        pthread_mutex_init(&s[m].server_lock,NULL);
    }

    for (int k = 1; k <= cab_no; ++k) {  ///initializing the mutex locks of drivers
        pthread_mutex_init(&d[k].lock,NULL);
        d[k].waitstate=true; ///initializes the wait state
        d[k].pno=k; ///stores cab number
        d[k].type=random_between(0,1); ///assingns the cab its type
        printf("Initializing driver %d with cabtype %d\n",k,d[k].type);
    }

    for (int i = 1; i <= rider_no ; ++i) {
        r[i].ridetime=random_between(1,10);
        r[i].maxwait=random_between(1,10);
        r[i].cabtype=random_between(0,1);
        r[i].pno=i;
        r[i].arrivaltime=random_between(1,5);
        pthread_mutex_init(&r[i].rider_lock,NULL);
        printf("Initializing rider %d with cabtype %d,ridetime %d,maxwait time %ld,arrival time %d\n",i,r[i].cabtype,r[i].ridetime,r[i].maxwait,r[i].arrivaltime);
    }

    for (int j = 1; j <= rider_no; ++j) { ///threads for each rider
        pthread_create(&r[j].rider_thread,NULL,rider_f,&r[j]);
    }

    for (int l = 1; l <= rider_no; ++l) { ///wait till all riders are done
        pthread_join(r[l].rider_thread,NULL);
    }

    printf("All riders have been processed \n");
}

void *rider_f(void *args) {
    rider *current=(rider *)args;
    bool booked=false;

    sleep(current->arrivaltime);
    long int timer=-1;

    time_t time1;
    time_t time2;
    time(&time1); ///initializing the time variable
    int i = 1;
    while(current->maxwait > timer && !booked){

        i=1;
        for (; i <= cab_no && current->maxwait > timer; ++i) { ///look for a cab
            if (current->cabtype == d[i].type)
            {

                if(current->cabtype){ ///premier type
                    if(d[i].waitstate){
                        bookcab(current,&d[i],&booked);
                        if(booked)
                            break;
                    }
                }else{///pool type
                    if(d[i].onPool1 || d[i].waitstate){
                        bookcab(current, &d[i], &booked);
                        if (booked)
                            break;
                    }
                }

                pthread_mutex_unlock(&d[i].lock);
            }
            time(&time2);
            timer=time2-time1; ///time elapsed in waiting mode
        }
    }

    if(!booked) {
        printf("The passenger number %d has waited for too long and has hence Timed out \n", current->pno);
        return NULL;
    }

    printf("Rider %d has started his ride with Driver %d\n",current->pno,i);
    sleep(current->ridetime);
    printf("Rider %d has finished his ride with Driver %d\n",current->pno,i);
    initialize_driver(i);

    i = 0;

    while(1){
        if(!s[i].in_use && (pthread_mutex_trylock(&s[i].server_lock)==0)){
            s[i].in_use=true;
            current->s_no=i;
            pthread_create(&s[i].server_thread,NULL,accept_payment,current);
            pthread_mutex_unlock(&s[i].server_lock);
            break;
        }
        i++;
        i%=server_no;
    }
    while (1){
        if(!s[i].in_use){ ///wait till the moment it frees up
            break;
        }
    }

    return NULL;
}

void initialize_driver(int no) {
    pthread_mutex_lock(&d[no].lock); ///lock up the driver
    if(d[no].onPrem || d[no].onPool1){
        d[no].onPrem=false;
        d[no].waitstate=true;
        d[no].onPool1=false;
        d[no].onPool2=false;
    }
    if(d[no].onPool2){
        d[no].onPool2=false;
        d[no].onPool1=true;
    }
    pthread_mutex_unlock(&d[no].lock); ///unlock the driver
}

void * accept_payment(void *arg) {
    rider * current=(rider *) arg;
    printf("Payment of rider %d is being processed \n",current->pno);
    sleep(2);
    printf("Payment of rider %d is completed \n",current->pno);
    s[current->s_no].in_use=false;
    return NULL;
}

void bookcab(rider *current, driver *pDriver,bool * booked) {
    pthread_mutex_lock(&pDriver->lock); ///lock up the driver
    if(current->cabtype){ ///its premier driver
        pDriver->onPrem=true;
        printf("Passenger %d is assigned driver %d for Premier ride\n",current->pno,pDriver->pno);
        *booked=true;
    }
    else if(current->cabtype==false){ ///its share driver
        if(pDriver->onPool1){
            pDriver->onPool2=true;
            pDriver->onPool1=false;
        } else{
            pDriver->onPool1=true;
            pDriver->onPool2=false;
        }
        printf("Passenger %d is assigned driver %d for Pool ride\n",current->pno,pDriver->pno);
        *booked=true;
    }
    else{ ///false positives
        *booked=false;
    }
    pDriver->waitstate=false; ///Driver is booked and is not waiting
    pthread_mutex_unlock(&pDriver->lock);
}


