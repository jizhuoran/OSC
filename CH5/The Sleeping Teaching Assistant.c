#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define SEAT_NUMBER 3
#define MAX_SLEEP_TIME 3
#define com_to_ask() srand_sleep();
#define teaching() srand_sleep();

void* ta_helping();
void* stu_listening(void *i);
void srand_sleep();

sem_t *sem_stu;
sem_t *sem_ta;

pthread_mutex_t lock;

int chair[3] = {-1,-1,-1};
int count = 0;
int next_student = 0;
int next_seat = 0;
int student_has_teached;

int main() {

    int number_of_student;
    printf("How many student? ");
    scanf("%d", &number_of_student);
    
    student_has_teached = number_of_student;
    
    srand(time(NULL));
    
    int *studentID;
    pthread_t ta;
    pthread_t *stu;
    
    studentID = malloc(sizeof(int) * number_of_student);
    stu = malloc(sizeof(pthread_t) * number_of_student);
    
    for(int i = 0; i < number_of_student; ++i) {studentID[i] = i;}
    
    //sem_init(sem_ta, 0, 1);
    //sem_init(sem_stu, 0, 0);
    //sem
    sem_ta = sem_open("sem_ta",O_CREAT, 0644, 1);
    sem_stu = sem_open("&sem_stu",O_CREAT, 0644, 0);
    
    pthread_mutex_init(&lock, NULL);
    
    pthread_create(&ta, NULL, ta_helping, NULL);
    for(int i = 0; i < number_of_student; ++i) {
        pthread_create(&stu[i], NULL, stu_listening, (void *)&studentID[i]);
    }
    
    pthread_join(ta, NULL);
    for (int i = 0; i < number_of_student; ++i) {
        pthread_join(stu[i], NULL);
    }
    
    return 0;
}

void *ta_helping() {
    while(1) {
        sem_wait(sem_stu);
        pthread_mutex_lock(&lock);
        
        printf("****\n TA is teaching student %d \n****\n", chair[next_student]);
        chair[next_student] = -1;
        count--;
        next_student = (next_student + 1) % SEAT_NUMBER;
        printf("waiting students : [1] %d [2] %d [3] %d\n",chair[0],chair[1],chair[2]);
        
        teaching();
        
        
        pthread_mutex_unlock(&lock);
        
        sem_post(sem_ta);
        
        student_has_teached--;
        if(!student_has_teached) {
            printf("teaching finish.\n");
            return NULL;
        }
    }
}

void *stu_listening(void *i) {
    int ID = *(int *) i;
    
    while(1) {
        com_to_ask();
        pthread_mutex_lock(&lock);
        
        if(count < SEAT_NUMBER) {
            count++;
            printf("student %d is waiting\n", ID);
            chair[next_seat] = ID;
            next_seat = (next_seat + 1) % SEAT_NUMBER;
            printf("waiting students : [1] %d [2] %d [3] %d\n",chair[0],chair[1],chair[2]);
            pthread_mutex_unlock(&lock);
            sem_post(sem_stu);
            sem_wait(sem_ta);
            return NULL;
        }else {
            pthread_mutex_unlock(&lock);
        }
    }
}

void srand_sleep() {
    int stime = rand() % MAX_SLEEP_TIME + 1;
    sleep(stime);
}
