#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

int robots_no,stud_no,tables_no;

typedef struct Table{
    pthread_mutex_t lock;
    bool locked;
    bool briyani_over;
    pthread_t table_thread_id;
    pthread_cond_t cond_id;
    int slots;
    int  briyani_amt;
    bool briyani_container;
}table;

typedef struct Chef{
    pthread_t chef_thread_id;
    pthread_mutex_t lock;
    pthread_cond_t cond_lock;

    int vessels;
    int people;

}chef;

typedef struct Student{
    pthread_t stud_thread_id;
}student;

student s[1000];
table t[1000];
chef c[1000];

void wait_for_slot();

void student_in_slot();

void* chef_create(void *arg);

void briyani_ready();

void table_init();

void *student_func(void * arg);

int main(){
    scanf("Enter number of students, tables and robot chefs %d %d %d",&stud_no,&tables_no,&robots_no);

    for (int j = 0; j < tables_no; ++j) {
        pthread_mutex_init(&(t[j].lock), NULL);
        pthread_mutex_lock(&t[j].lock);
        pthread_cond_init(&t[j].cond_id,NULL);
    } ///set all tables to locked state in the beginning

    for (int m = 0; m < robots_no; ++m) {
        pthread_cond_init(&c[m].cond_lock,NULL);
        pthread_mutex_init(&c[m].lock,NULL);
    }

    for (int i = 0; i < robots_no; ++i) { ///interating through chefs
        pthread_create(&c[i].chef_thread_id,NULL,chef_create,&c[i]);

    }

    for (int i1 = 0; i1 < stud_no; ++i1) {
        pthread_create(&s[i1].stud_thread_id,NULL,student_func,&s[i1]);
    }

    for (int l = 0; l < stud_no; ++l) { ///join i.e wait for all students to be over
        pthread_join(s[l].stud_thread_id,0);
    }

    for (int k = 0; k < tables_no; ++k) {
        pthread_mutex_destroy(&t[k].lock);

    }
    printf("Students have been fed");
    return 0;
}

void *student_func(void * arg) {

}


void student_in_slot() {

}

void wait_for_slot() {
    for (int i = 0; i < tables_no ; ++i) {
        if(t[i]){ ///if slot is available
            --t[i];
            return; ///student has been alloted a slot
        }
    }
    ///no slots available

}

void * chef_create(void *arg) {
    chef *current=(chef * )arg;
    while(stud_no){///stops making briyani if number of students left to feed hits 0

        int time=(rand()%4)+2;///2-5 secs
        int vessels=(rand()%10)+1;///1-10
        int p=(rand()%26)+25;///25-50

        printf("Preparing %d vessels, each can feed %d people",vessels,p);
        current->people=p;
        current->vessels=vessels;
        sleep(time); ///sleep for time to prepare stuff
        briyani_ready(current);
    }
    return NULL;
}

void briyani_ready(void *arg) {
    ///table has been initialized with briyani amount
    chef *current=(chef*)arg;
    for (int i = 0; i < current->vessels; ++i) {
        pthread_mutex_lock(&current->lock);
        pthread_cond_wait(&current->cond_lock);
        pthread_mutex_unlock(&current->lock);
    }
}

