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

    FILE* X = fopen(argv[1], "r");
    int K = atoi(argv[2]);  // Number of kids
    char N[5] ;   // Number of transaction
    strcpy(N,argv[3]);

    int lines_of_file = file_lines(X);
    int pids[K];

    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    char buffer[BUFSIZ] ;
    int shmid;  

    // Clear Semaphores

    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);

    //  Create Semaphores

    sem_t *semaphore1 = sem_open(SEM_NAME1, O_CREAT | O_RDWR, 0660, 1);
    if (semaphore1 == SEM_FAILED) {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore2 = sem_open(SEM_NAME2, O_CREAT | O_RDWR);
    if (semaphore2 == SEM_FAILED) {
        perror("sem_open(3) failed");
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

    for (int i = 0; i < K; i++) {
        pids[i] = fork();

    //  Child Process 

        if (pids[i] == 0) { 
            
            //if (shared_stuff->written_by_you != 0) {
                sem_wait(semaphore2);
                printf("Transactions: %d\n", atoi(shared_stuff->some_text));
                printf("Num of lines is: %d\n", shared_stuff->written_by_you);
                shared_stuff->written_by_you = rand() % shared_stuff->written_by_you;
                sem_post(semaphore1);
                sem_wait(semaphore2);
                printf("%s", shared_stuff->some_text);
                sem_post(semaphore1);
            //}
            return 0;
        }
       
    //  Parent Process 
         
        sem_wait(semaphore1);
        strncpy(shared_stuff->some_text, N, TEXT_SIZE);
        shared_stuff->written_by_you = lines_of_file;
        sem_post(semaphore2);
        sem_wait(semaphore1);
        printf("Child want line %d\n", shared_stuff->written_by_you);
        rewind(X);
        returned_line(X, shared_stuff->written_by_you, buffer);
        strcpy(shared_stuff->some_text, buffer);
        //printf("%s", buffer);
        sem_post(semaphore2);
    }

    for (int i = 0; i < K; i++) { 
        wait(NULL);
    }

    if (sem_unlink(SEM_NAME1) < 0) {
        perror("sem_unlink(3) failed");
    }
    if (sem_unlink(SEM_NAME2) < 0) {
        perror("sem_unlink(3) failed");
    }
    sem_close(semaphore1);
    sem_close(semaphore2);

    fclose(X);
    return 0;
}