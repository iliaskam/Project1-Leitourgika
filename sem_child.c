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

#define SEM_NAME1 "prod"
#define SEM_NAME2 "cons"

#define TEXT_SIZE 100

struct shared_use_st {
    int written_by_you;
    char some_text[TEXT_SIZE];
};

int file_lines(FILE* file) {
    char line[100];
    int count = 0;
    while (fgets(line, sizeof(line), file))
        count++;
    return count;
}

void returned_line(FILE* file, int line_num, char buffer[BUFSIZ]) {
    int count = 0;
    while(count != line_num) {
        fgets(buffer, BUFSIZ, file);
        count++;
    }
}



int main(int argc, char **argv) {

    printf("%s %s\n", argv[0], argv[1]);

    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    int shmid;  

    //  Create Semaphores

    sem_t *semaphore1 = sem_open(SEM_NAME1, 0);
    if (semaphore1 == SEM_FAILED) {
        perror("sem_open(4) failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore2 = sem_open(SEM_NAME2, 1);
    if (semaphore2 == SEM_FAILED) {
        perror("sem_open(5) failed");
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
    printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);

    shared_stuff = (struct shared_use_st *)shared_memory;
            
        sem_wait(semaphore2);
        printf("Num of lines is: %d\n", atoi(argv[0]));
        printf("Transactions: %d\n", atoi(argv[1]));
        srand(time(NULL) + getpid());
        shared_stuff->written_by_you = 1 + (rand() % atoi(argv[0]));
        sem_post(semaphore1);
        sem_wait(semaphore2);
        printf("%s", shared_stuff->some_text);
        sem_post(semaphore2);
       
    
    sem_close(semaphore1);
    sem_close(semaphore2);

    return 0;
}