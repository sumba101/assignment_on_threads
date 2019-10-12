#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <f2fs_fs.h>

int robo_prep[1000], robo_v[1000]; /// prep time, number of vessels
pthread_mutex_t mutex[100]; ///
pthread_mutex_t mutex_wait;
pthread_mutex_t mutex_serve[100];

int front = -1, rear = -1, size = 100; ///queue of students
int table_free[100] /*container empty or not*/, portion_alot[100]/*number of portions left in container*/, table_cap[100]/*total number slots on table*/, stud_queue[100]/*the queue*/, stud_stat[100]/*status of student 1 for served 0 for unserved*/;
int bir_cooked[100]/*boolean 1 when finished cooks, 0 when that robots vessels are empty */, no_fed = 0/*number of students fed*/;
int m, n, k;

struct cap //no.of vessels made by chef with array containing people it can feed
{
    int a[15];
};
struct cap *ves_cap[100];
struct info ///fore each table data of chef and vessel number
{
    int chef;
    int v_no;
};
struct info *tab_info[100];

void enQueue(int value, int arr[])
{
    if ((front == 0 && rear == size - 1) ||
        (rear == (front - 1) % (size - 1)))
    {
        printf("\nQueue is Full");
        return;
    }

    else if (front == -1) /* Insert First Element */
    {
        front = rear = 0;
        arr[rear] = value;
    }

    else if (rear == size - 1 && front != 0)
    {
        rear = 0;
        arr[rear] = value;
    }

    else
    {
        rear++;
        arr[rear] = value;
    }
    //sleep(1);
} ///putting into queue
int total_inq()
{
    int sz;
    if (front == -1 && rear == -1)

    {

        sz = 0;
    }

    else

    {

        sz = front > rear ? (size - front + rear + 1) : (rear - front + 1);
    }
    return sz;
} ///size of queue
// Function to delete element from Circular Queue
int deQueue(int arr[])
{
    if (front == -1)
    {
        printf("\nQueue is Empty");
        return -1;
    }

    int data = arr[front];
    arr[front] = -1;
    if (front == rear)
    {
        front = -1;
        rear = -1;
    }
    else if (front == size - 1)
        front = 0;
    else
        front++;

    return data;
}

bool serving_status[100]; ///

struct serve ///which chef id
{
    int num;
};

struct serve *table_total[100]; /// total number of slots available in table

void wait_for_slot(int i)
{
    while (1)
    {
        if (i == stud_queue[front])
        {
            return;
        }
    }
} //just for requirements
void biryani_ready(int i)
{
  ///i is chef index
    pthread_mutex_lock(&mutex_serve[i]);
    bir_cooked[i] = 1; ///briyani ready
    int c = 0; ///count of vessels alloted

    while (c < robo_v[i])
    {

        for (int j = 1; j <= n && c < robo_v[i]; j++)
        {
            if (table_free[j] == 0) ///0 means free
            {
                portion_alot[j] = ves_cap[i]->a[c++]; ///number of portions the container can do
                table_free[j] = 1; ///table is now full
                printf("Service table %d has been given %d portions from %d chef container %d\n", j, portion_alot[j], i, c);
                tab_info[j]->chef = i;
                tab_info[j]->v_no = c;
            }
        }
        if (c < robo_v[i]) ///means all tables full but chef has containers with him
        {
            int x = 1; ///table id
            while (1)
            {
                while (x <= n)
                {
                    if (table_free[x] == 0)
                    {
                        break;
                    }
                    x++;
                }
                if (table_free[x] == 0)
                {
                    break;
                }
            }

        }
    }

    int v_emp=0;
    while (v_emp < robo_v[i])
    {
        v_emp = 0;
        for (int x = 0; x <= robo_v[i]; x++)
        {
            if (ves_cap[i]->a[x] == 0)
            {
                v_emp++;
            }
        }
    }
    printf("chef %d finished vessels are %d\n", i, v_emp);
    bir_cooked[i] = 0; ///means briyani must be cooked again
    pthread_mutex_unlock(&mutex_serve[i]);
}
void *robothread(void *i)
{
    pthread_mutex_lock(&mutex_wait);
    struct serve *ki = i;
    int num = ki->num; ///chef id
    pthread_mutex_unlock(&mutex_wait);

    while (1) ///change to break if all are fed
    {
        robo_prep[num] = rand() % 4 + 2;
        robo_v[num] = rand() % 10 + 1;
        int cap_count = 0;
        ves_cap[num] = malloc(sizeof(struct cap));

        while (cap_count < robo_v[num])
            ves_cap[num]->a[cap_count++] = rand() % 26 + 25;
        printf("robot %d, vessels %d, prep time %d\n", ki->num, robo_v[num], robo_prep[num]);

        sleep(robo_prep[num]);
        printf("Robot %d finished cooking %d vessels in %d time.\n", num, robo_v[num], robo_prep[num]);
        biryani_ready(num);
    }
}
void ready_to_serve(int i, int no) ///table id,no.of students to be served, no.of students served
{

    if (front == -1 || portion_alot[i] == 0) ///if queue is empty of no.of people it can feed
    {
        return;
    }

    table_total[i]->num -= no; ///to see how many seats in table are still
    portion_alot[i] -= no;
    table_cap[i] = 0; ///because all are alloted
    no_fed += no;
    for (int j = 0; j < no; j++)
    {
        int temp = deQueue(stud_queue);
        stud_stat[temp] = 1; ///gonna get food
        printf("Student %d allocated to table %d\n", temp, i);

    }
}

void *tablethread(void *i)
{
    pthread_mutex_lock(&mutex_wait);
    struct serve *ki = i;
    int num = ki->num;

    pthread_mutex_unlock(&mutex_wait);
    while (1)
    {
        /*table total is total capacoity of slots and table cap is the assigned one by random
         * */
        if (table_free[num] == 1) ///if table has container or not
        {
            table_cap[num] = rand() % 10 + 1; ///slots for container
            if (table_cap[num] > portion_alot[num]/*people it serve*/)
            {
                table_cap[num] = portion_alot[num];
            }
            if (table_cap[num] > total_inq()/*no of students*/)
            {
                table_cap[num] = total_inq();
            }
            ready_to_serve(num, table_cap[num]);

            printf("For table %d portion left = %d, table_total = %d, no__fed = %d\n", num, portion_alot[num], table_total[num]->num, no_fed);
            if (portion_alot[num] == 0) ///container over
            {
                int temp = tab_info[num]->chef;
                int ves_num = tab_info[num]->v_no;
                ves_cap[temp]->a[ves_num] = 0;
            }
            if (portion_alot[num] == 0 && table_total[num]->num != 0 && no_fed != k)
            {
                table_free[num] = 0; ///free for allotment
            }
        }

        if (table_free[num] == 0) ///redundant, comment out and see
        {
            portion_alot[num] = 0;
            table_cap[num] = 0;

        }

        if (table_total[num]->num == 0 || no_fed == k)
        {
            break;
        }
        else if (front == -1)
        {
            while (front == -1)
            { ;
            }
        }
    }
    return NULL;
}

void *studentthread(void *i)
{
    pthread_mutex_lock(&mutex_wait);
    struct serve *ki = i;
    int num = ki->num;
    printf("Enter student %d\n", num);
    enQueue(num, stud_queue);

    pthread_mutex_unlock(&mutex_wait);
    wait_for_slot(num);
    return NULL;
}
int main()
{

    scanf("%d %d %d", &m, &n, &k);
    pthread_t robo[m], table[n], stud[k];

    for (int i = 1; i <= m; i++)
    {
        bir_cooked[i] = 0;
        pthread_mutex_init(&mutex[i], NULL);       ///for tables
        pthread_mutex_init(&mutex_serve[i], NULL);/// for briyani ready
    }

    for (int i = 1; i <= n; i++)
    {
        table_total[i] = (struct serve *)malloc(sizeof(struct serve));
        table_total[i]->num = 100;
        table_free[i] = 0;
        tab_info[i] = (struct info *)malloc(sizeof(struct info));
    }

    pthread_mutex_init(&mutex_wait, NULL);///for each thread
    for (int i = 1; i <= m; i++)
    {

        struct serve *ki;
        ki = (struct serve *)malloc(sizeof(struct serve));
        ki->num = i;

        pthread_create(&robo[i], NULL, robothread, ki);

        sleep(2); ///if removed, tanvi shall explain
    }
    struct serve *se;
    se = (struct serve *)malloc(sizeof(struct serve));

    se->num = 0;

    for (int i = 1; i <= n; i++)
    {

        serving_status[i] = 1;
        se->num = i;
        pthread_create(&table[i], NULL, tablethread, se);
        sleep(2);
    }
    for (int i = 1; i <= k; i++)
    {
        stud_stat[i] = 1;
        se->num = i;
        pthread_create(&stud[i], NULL, studentthread, se);
        sleep(2);
    }
}


