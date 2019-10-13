#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define max_size 1000
#define min(x,y) (x < y) ? x : y;

int M, N, K, loaded[max_size], slots[max_size];
pthread_t chef_id[max_size], student_id[max_size], table_id[max_size];
pthread_mutex_t chef_table_lock[max_size], table_lock[max_size];

int random_between(int l, int e);

void biryani_ready(int index, int r, int p);

void *chef(void *index);

void ready_to_serve(int index);

void *serving_table(void *index);

void student_in_slot(int index, int table);

void wait_for_slot(int index);

void *student(void *index);

void initialize_random(){
    srand(time(NULL));
}

int main() {
    initialize_random();
    printf("Enter number of chefs, serving tables and students\n");
    scanf("%d %d %d", &M, &N, &K);


    int chefin[max_size], servein[max_size], studentin[max_size];
    for (int i = 0; i < M; i++) {
        chefin[i] = i;
        pthread_create(&(chef_id[i]), NULL, &chef, &chefin[i]);
        usleep(100);
    }

    for (int i = 0; i < N; i++) {
        servein[i] = i;
        pthread_mutex_init(&chef_table_lock[i], NULL);

        pthread_mutex_init(&table_lock[i], NULL);

        pthread_mutex_lock(&table_lock[i]);
        pthread_create(&(table_id[i]), NULL, &serving_table, &servein[i]);

        usleep(100);
    }

    for (int i = 0; i < K; i++) {
        studentin[i] = i;
        pthread_create(&(student_id[i]), NULL, &student, &studentin[i]);
        usleep(100);
    }

    for (int i = 0; i < K; ) ///wait for the threads to join
    {
        pthread_join(student_id[i], NULL);
        i+=1;
    }
    for (int i = 0; i < N; ) ///handling garbage collection
    {
        pthread_mutex_destroy(&table_lock[i]);
        pthread_mutex_destroy(&chef_table_lock[i]);
        i+=1;
    }

    printf("All students have been fed briyani\n");
}

void *student(void *index) {
    int *temp=((int*)index);
    wait_for_slot(*temp);

    return NULL;
}

void wait_for_slot(int index) {
    bool served = false;
    printf("Student number %d is waiting for a slot in a table\n", index+1);
    while(!served)
    {
        int i = 0;
        for(;i<N;)
        {
            if(!pthread_mutex_trylock(&table_lock[i]))
            {
                if(slots[i]==0){ ///if slots become 0
                    pthread_mutex_unlock(&table_lock[i]);
                    continue;
                }

                student_in_slot(index, i);
                served = true;
                pthread_mutex_unlock(&table_lock[i]);
                break;
            }
            i+=1;
        }
    }
}

void student_in_slot(int index, int table) {
    printf("Student number %d is having briyani at table %d\n", index+1, table+1);
    sleep(2);
    slots[table]-=1;
}

void *serving_table(void *index) {
    int *temp=((int*)index);

    while(1)
    {
        while(!loaded[(*temp)]);

        slots[(*temp)] = min(random_between(1,10), loaded[(*temp)]);
        printf("Serving table %d has made %d slot(s) available\n", (*temp)+1, slots[(*temp)]);

        ready_to_serve((*temp));

        loaded[(*temp)] -= slots[(*temp)];

        if(!loaded[(*temp)])
            pthread_mutex_unlock(&chef_table_lock[(*temp)]);

        sleep(1);

    }

}

void ready_to_serve(int index) {
    pthread_mutex_unlock(&table_lock[index]);

    for(;slots[index];){
        ;//spinlock
        ;
    } ///spin lock till slots of index hits 0

    pthread_mutex_lock(&table_lock[index]);
}

void *chef(void *index) {
    int *temp=((int*)index);

    int w = random_between(2,5), r = random_between(1,10), p =random_between(2,5);

    printf("Chef %d will now take %d seconds time to prepare %d vessel(s) each with portions to feed for %d student(s)\n", (*temp)+1, w, r, p);
    sleep(w); ///sleep for random time i.e preparation time
    biryani_ready((*temp), r, p);
    return NULL;
}

void biryani_ready(int index, int r, int p) {
    for(int i = 0;i<r;){
        int served = 0;
        for(int j = 0;j<N;)
        {
            if(!pthread_mutex_trylock(&chef_table_lock[j]))
            {
                served = 1;
                printf("Vessel %d by chef number %d is served in table %d\n", i+1, index+1, j+1);
                loaded[j] = p;
                break;
            }
            j+=1;
        }
        i-=1-served;
        i+=1;
    }
}

int random_between(int l, int e) {
    return l + rand() % (e - l + 1);
}
