/***********************
 
 File: msyncFileStats.c
 Compile: gcc msyncFileStats.c -o msyncFileStats -lpthread
 Run: ./msyncFileStats [file1] [file2] ...
 
 Description: Multi-thread version of fileStats.c
 Similar to Unix utility wc
 [file1], [file2] etc are text files. mtFileStats counts the
 number of lines, words and characters in each file, reports
 the counts for each file, and total counts over all the files.
 
 ***********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

void *childFunc(void *ptr);

#define MAXCHARS 80

struct FileInfo {
    char *name;    /* name of file */
    int numLines;  /* number of lines in file */
    int numWords;  /* number of words in file */
    int numChars;  /* number of characters in file */
} fileInfo;

struct FileInfo *info; /* array of counts for each file */
int countWords(char *);

void *threadFunc(void *ptr);
sem_t sema;
int numLines = 0, numWords = 0, numChars = 0; /* total counts */
sem_t semaLock;


int main(int argc, char **argv) {
    /* initialize the semaphore to the number of children that will be created*/
    sem_init(&sema, 0, (argc - 1));
    sem_init(&semaLock, 0, 1);
    
    char inString[MAXCHARS];
    
    pthread_t *child;
    int  iret1;
    int *tid;
    
    /* allocate array of structs containing counts for each file */
    info = (struct FileInfo *) malloc((argc-1) * sizeof(struct FileInfo));
    
    for (int i=0; i<argc-1; i++) {
        info[i].name = (char *) malloc(20 * sizeof(char));
        strncpy(info[i].name, argv[i+1], 20);
    }
    
    child = (pthread_t *) malloc((argc-1) * sizeof(pthread_t));
    
    tid = (int *) malloc((argc-1) * sizeof(int));
    for (int i=0; i<argc-1; i++) {
        tid[i] = i;
    }
    
    for (int i=0; i<argc-1; i++) {
        /* spawn a thread */
        if ((iret1 = pthread_create(&child[i], NULL, childFunc, &tid[i]))) {
            printf("Error creating thread\n");
            exit(0);
        }
    }
    /* this value will keep on changing*/
    int semaValue;
    
    
    /*
     check if the children are done, if they're not done, keep on looping until they are
     once they are done, print the total numbers and
     */
    while (true) {
        sem_getvalue(&sema, &semaValue);
        
        if (semaValue == 0) {
            printf("Total: %d lines, %d words, %d characters\n", numLines, numWords, numChars);
            
            
            /* cleaning up the memory since there's no garbage collection */
            free(info);
            free(tid);
            free(child);
            
            sem_destroy(&semaLock);
            sem_destroy(&sema);
            exit(0);
        }
    }
    
    
}

/***********************
 
 int countWords(char *inS)
 
 inS: input null-terminated string
 
 returns number of words in string, delimited by spaces
 
 ***********************/

int countWords(char *inS) {
    char *token;
    int numTokens = 0;
    int i=0;
    
    for (i=1; i<strlen(inS); i++) {
        if ((isalnum(inS[i-1]) || ispunct(inS[i-1])) && (inS[i] == ' ')) {
            numTokens++;
        }
    }
    
    if (isalnum(inS[strlen(inS)-2]) || ispunct(inS[strlen(inS)-2])) {
        numTokens++;
    }
    return numTokens;
}

void *childFunc(void *ptr) {
    FILE *fp;
    char *rs;
    char inString[MAXCHARS];
    int myIndex = *((int *) ptr);
    /* open an input file, initialize struct of counts */
    fp = fopen(info[myIndex].name, "r");
    
    /* add a sem_wait to prevent an infinite loop */
    if (fp == NULL) {
        printf("Error: cannot open file\n");
        sem_wait(&sema);
        return 0;
    }
    
    info[myIndex].numLines = 0;
    info[myIndex].numWords = 0;
    info[myIndex].numChars = 0;
    
    /* read each line, update counts */
    rs = fgets(inString, MAXCHARS, fp);
    
    while (rs != NULL) {
        info[myIndex].numLines++;
        info[myIndex].numChars += strlen(inString);
        info[myIndex].numWords += countWords(inString);
        rs = fgets(inString, MAXCHARS, fp);
    }
    
    
    printf("Thread %ld %s: %d lines, %d words, %d characters\n",
           (long) pthread_self(), info[myIndex].name,
           info[myIndex].numLines, info[myIndex].numWords, info[myIndex].numChars);
    
    /* do all of our race-condition operations in the semaphore lock*/
    sem_wait(&semaLock);
    numLines += info[i].numLines;
    numChars += info[i].numChars;
    numWords += info[i].numWords;
    sem_post(&semaLock);
    
    /* */
    sem_wait(&sema);
    fclose(fp);
    pthread_exit(0);
}
