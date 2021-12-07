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

#define CHILD_PROGRAM "./sem_child"

#define SEM_NAME1 "Prod"
#define SEM_NAME2 "Cons"
#define SEM_NAME3 "Mutex"

#define TEXT_SIZE 100

struct shared_use_st {     // Shared memory variables
    int written_by_you;
    char some_text[TEXT_SIZE];
};

int file_lines(FILE* file) {    // Fuction to count the lines of a file
    char line[TEXT_SIZE];
    int count = 0;
    while (fgets(line, sizeof(line), file))
        count++;
    return count;
}

void returned_line(FILE* file, int line_num, char buffer[BUFSIZ]) {     // Function which loads to the buffer a specific line from a file
    int count = 0;
    while(count != line_num) {
        fgets(buffer, BUFSIZ, file);
        count++;
    }
}


int main(int argc, char **argv) {

    FILE* X = fopen(argv[1], "r");  // Open my file 
    int K = atoi(argv[2]);  // Number of kids
  
    int lines_of_file = file_lines(X);  // Find file number of lines
    char str[10];
    sprintf(str, "%d", lines_of_file);  // Convert integer to string and pass it as an argument to child process

    char* args[] = {str, argv[3], NULL};    // Arguments file child process

    int pids[K];

    // Shared memory variables

    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    char buffer[BUFSIZ] ;
    int shmid;  

    // Clear Semaphores

    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);

    //  Create Semaphores

    sem_t *semaphore1 = sem_open(SEM_NAME1, O_CREAT | O_RDWR, 0660, 0);
    if (semaphore1 == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore2 = sem_open(SEM_NAME2, O_CREAT | O_RDWR, 0660, 1);
    if (semaphore2 == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    sem_t *semaphore3 = sem_open(SEM_NAME3, O_CREAT | O_RDWR, 0660, 1);
    if (semaphore3 == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    sem_close(semaphore3);  //  Close the semaphore as we won't be using it in the parent process

    
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

    // Create K number of child processes

    for (int i = 0; i < K; i++) {
        pids[i] = fork();

        //  Child Process 

        if (pids[i] == 0) {     // If fork returns 0 we are on the child process
            if (execv(CHILD_PROGRAM, args) < 0) {
                perror("execl(10) failed");
                exit(EXIT_FAILURE);
            }
        }
       
        //  Parent Process 
         
        for (int i = 0; i < atoi(argv[3]); i++) {   //  Repeat the parent process to match the number of transactions of every child
         
            sem_wait(semaphore1);   // Wait for child to request a line
            rewind(X);  // Rewind my file to pass it to my function
            returned_line(X, shared_stuff->written_by_you, buffer);     // Call function to load to buffer the requested line 
            strcpy(shared_stuff->some_text, buffer);    // Copy line to my shared memory
            sem_post(semaphore2);   // Signal child once i reply his request
        }
    }

    for (int i = 0; i < K; i++) {   // Wait for every Child to end 
        wait(NULL);
    }

    // Unlink and close semaphores

    if (sem_unlink(SEM_NAME1) < 0) {
        perror("sem_unlink(3) failed");
    }
    if (sem_unlink(SEM_NAME2) < 0) {
        perror("sem_unlink(3) failed");
    }
    if (sem_unlink(SEM_NAME3) < 0) {
        perror("sem_unlink(3) failed");
    }
    sem_close(semaphore1);
    sem_close(semaphore2);

    // Detach shared memory

    if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

    fclose(X);  // Close my file

    exit(EXIT_SUCCESS);
}