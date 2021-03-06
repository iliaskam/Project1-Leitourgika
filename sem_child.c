#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include<time.h>

#define SEM_NAME1 "Prod"
#define SEM_NAME2 "Cons"
#define SEM_NAME3 "Mutex"

#define TEXT_SIZE 100

struct shared_use_st {      // Shared memory variables
    int written_by_you;
    char some_text[TEXT_SIZE];
};

int main(int argc, char **argv) {

    int num_of_lines = atoi(argv[0]);   // Number of lines
    int tran = atoi(argv[1]);   // Number of transactions

    // Shared memory variables

    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    int shmid;  

    //  Create Semaphores

    sem_t *semaphore1 = sem_open(SEM_NAME1, 0);
    if (semaphore1 == SEM_FAILED) {
        perror("sem_open(4) failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore2 = sem_open(SEM_NAME2, 0);
    if (semaphore2 == SEM_FAILED) {
        perror("sem_open(5) failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore3 = sem_open(SEM_NAME3, 0);
    if (semaphore3 == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }
    
    //  Create Shared Memory 

    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
    if (shmid == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        fprintf(stderr, "shmat failes\n");
        exit(EXIT_FAILURE);
    }
    //printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);

    shared_stuff = (struct shared_use_st *)shared_memory;

    // Start clock and set srand

    clock_t begin = clock();
    srand(getpid());

    // Repeat the loop to match the number of transactions


    for (int i = 0; i < tran; i++) {   
        sem_wait(semaphore3);   // Lock
        sem_wait(semaphore2);   // Lock transaction
        shared_stuff->written_by_you = 1 + (rand() % num_of_lines);
        printf("Child %d requested line %d from Parent \n", getpid(), shared_stuff->written_by_you);
        sem_post(semaphore1);   // Post when child make his request
        sem_wait(semaphore2);   // Wait the response from parent
        printf("Child %d received line : %s", getpid(), shared_stuff->some_text);
        sem_post(semaphore2);   // We post so the value of semaphore2 is 1 and the next transaction/child start immediatly
        sem_post(semaphore3);   // Unlock
    }

    // Stop the clock when child process end and print results

    clock_t end = clock();
    double time= ((double)end - (double)begin)/CLOCKS_PER_SEC;
    printf("Average serve time for child %d : %f\n", getpid(), time);

    // Close semaphores

    sem_close(semaphore1);
    sem_close(semaphore2);
    sem_close(semaphore3);

    // Detach shared memory

    if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

    exit(EXIT_SUCCESS);
}
