/**
 * @file proj2.h
 * @author Marek Filip (xfilip46), FIT BUT
 * @date 2020-Apr-24
 * @brief IOS-project-2
 * @see http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf
 * @details Header file for proj2.c program.
 *          Compiled: gcc 9.3
 */

#ifndef _PROJ2_H
#define _PROJ2_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        // bool
#include <semaphore.h>      // semaphores
#include <sys/stat.h>       // for mode constants
#include <sys/mman.h>       // shared memory
#include <sys/wait.h>       // waitpid(), wait()
#include <fcntl.h>          // for O_* constants
#include <unistd.h>         // fork()
#include <time.h>           // nanosleep, sleep
#include <stdarg.h>         // fprintf arguments

// #define DEBUG               // debug logs define

extern sem_t *noJudge;      // turnstile for incoming immigrants, protects entered
extern sem_t *mutex;        // mutex protecting checked variable
extern sem_t *confirmed;    // semaphor singaling that judge has confirmed imms
extern sem_t *allSignedIn;  // semaphor if all entered imms have checked in
extern sem_t *logWritten;   // mutex for log writing
extern sem_t *sharedMutex;  // mutex shared memory manipulation
extern const char* noJudgeName;     // noJudge     semaphor name
extern const char* mutexName;       // mutex       semaphor name
extern const char* confirmedName;   // confirmed   semaphor name
extern const char* allSignedInName; // allSignedIn semaphor name
extern const char* logWrittenName;  // logWritten  semaphor name
extern const char* sharedMutexName; // sharedMutex semaphor name
extern int *entered;        // entered immigrants, not certified yet
extern int *checked;        // checked immigrants, not certified yet
extern int *A;              // order of logs counter
extern int *NB;             // counts of immigrants inside the building
extern int *activeImmigrants;   // total number of imm processes yet to die
extern bool *judge;         // is judge inside the building?
extern FILE *out;           // pointer for the output file
#endif


/**
 * @brief Args structure for program arguments.
 * All variables are unsigned numbers.
 */
typedef struct args {
    unsigned PI; /**< Number of generated immigrant processes. */
    unsigned IG; /**< Max time after which is immigrant process generated. */
    unsigned JG; /**< Max time after which judge enters the hall. */
    unsigned IT; /**< Max time for getting the certificate by immigrant. */
    unsigned JT; /**< Max time for confiriming the naturalization by judge. */
} args_t;

/**
 * @brief Prints the usage message.
 *
 * @details Used when invalid arguments entered.
 */
void printHelp();

/**
 * @brief Validates arguments.
 *
 * @details If the args are valid, assign them to args structure.
 * @returns True if args are valid, false if they aren't.
 */
bool areArgsValid(int argc, char *argv[], args_t *args);

/**
 * @brief Supsends a thread for a random time in an <0, ms> int interval.
 *
 * @details Uses 'nanosleep()' function. If the nanosleep function fails
 *          or gets interrupted an error is raised and the process killed.
 * @param ms The upper bound for the time interval in milliseconds.
 */
void randSleep(unsigned ms);

/**
 * @brief Processes immigrants.
 *
 */
void process_immigrant(unsigned ID, unsigned getcert_time);

/**
 * @brief Processes judge.
 *
 */
void process_judge(unsigned enter_time, unsigned conf_time);

/**
 * @brief Initializes all global semaphores and shared variables.
 *
 * @returns 0 if everything was initialized, -1 if semaphor already exists.
 */
int setup();

/**
 * @brief Destroys all global semaphores and shared variables.
 *
 * @returns 0 if everything freed succesfully, -1 if error occured.
 */
int cleanup();

/**
 * @brief Writes a line into the output file.
 *
 * @details Uses variable number of arguments for printing.
 *          Increments the order of operations variable.
 */
void writelog(const char* NAME, const char* fmt, ...);

/**
 * @brief Function to signify that an immigrant entered the building.
 */
void immEnter(const char* NAME, unsigned ID);

/**
 * @brief Function to signify that an immigrant was registered.
 */
void immCheckIn(const char* NAME, unsigned ID);

/**
 * @brief Function to signify that an immigrant has left the building.
 */
void immLeave(const char* NAME, unsigned ID);

/**
 * @brief Function to signify that a judge entered the building.
 */
void judgeEnter(const char* NAME);

/**
 * @brief Function to signify that a judge started confirmation process.
 */
void judgeStartConfirm(const char* NAME, unsigned conf_time);

/**
 * @brief Function to signify that a judge confirmed all immigrants.
 */
void judgeConfirm(const char* NAME);

/**
 * @brief Function to signify that a judge left the building.
 */
void judgeLeave(const char* NAME, unsigned conf_time);
