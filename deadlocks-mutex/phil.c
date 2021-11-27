#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define PHILOSOPHER_NUM 5
#define MAX_MEALS 5
#define MAX_THINK_SEC 10

enum {
    THINKING, HUNGRY, EATING
} state[PHILOSOPHER_NUM];

pthread_t ph[PHILOSOPHER_NUM];
pthread_mutex_t mutex;
pthread_cond_t cond[PHILOSOPHER_NUM];

void* pickup_forks(void* philosopher_number)
{
    int loop_iterations = 0;
    int pnum = *(int*)philosopher_number;

    while (meals_eaten[pnum] < MAX_MEALS)
    {
        printf("Philosoper %d is thinking.\n", pnum);
        sleep(rand() % MAX_THINK_SEC) + 1);

        pthread_mutex_lock(&mutex);
        state[pnum] = HUNGRY;
        test(pnum);

        while (state[pnum] != EATING)
        {
            pthread_cond_wait(&cond[pnum], &mutex);
        }
        pthread_mutex_unlock(&mutex);

        (meals_eaten[pnum])++;

        printf("Philosoper %d is eating meal %d.\n", pnum, meals_eaten[pnum]);
        sleep(rand() % MAX_THINK_SEC) + 1);

        return_forks(philosopher_number);

        loop_iterations++;
    }
}

void* return_forks(void* philosopher_number)
{
    pthread_mutex_lock(&mutex);
    int pnum = *(int*)philosopher_number;

    state[pnum] = THINKING;

    test(left_neighbor(pnum));
    test(right_neighbor(pnum));

    pthread_mutex_unlock(&mutex);
}

int left_neighbor(int philosopher_number)
{
    return ((philosopher_number + (PHILOSOPHER_NUM - 1)) % 5);
}

int right_neighbor(int philosopher_number)
{
    return ((philosopher_number + 1) % 5);
}

void test(int philosopher_number)
{
    if ((state[left_neighbor(philosopher_number)] != EATING) &&
        (state[philosopher_number] == HUNGRY) &&
        (state[right_neighbor(philosopher_number)] != EATING))
    {
        state[philosopher_number] = EATING;
        pthread_cond_signal(&cond[philosopher_number]);
    }
}

int main(int argc, char* argv[]) {
    int ph_num[PHILOSOPHER_NUM];

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("can not init mutex");
        return 1;
    }
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        state[i] = THINKING;
        pthread_cond_init(&cond[i], NULL);
    }
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        ph_num[i] = i;
        pthread_create(&ph[i], NULL, pickup_forks, (void*)&(ph_num[i]));
    }
    sleep(60);
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        pthread_join(ph[i], NULL);
    }


    return 0;
}

