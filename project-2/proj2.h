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
void print_help();

/**
 * @brief Validates arguments.
 *
 * @details If the args are valid, assign them to args structure.
 * @returns True if args are valid, false if they aren't.
 */
bool are_args_valid(int argc, char *argv[], args_t *args);

/**
 * @brief Supsends a thread for a random time in an <0, ms> int interval.
 *
 * @details Uses 'nanosleep()' function. If the nanosleep function fails
 *          or gets interrupted an error is raised and the process killed.
 * @param ms The upper bound for the time interval in milliseconds.
 */
void rand_sleep(unsigned ms);

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
void writelog(int *A, const char* NAME, const char* fmt, ...);

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
 * @brief Function to signify that a judge entered the building.
 */
void judgeStartConfirm(const char* NAME);

/**
 * @brief Function to signify that a judge entered the building.
 */
void judgeConfirm(const char* NAME);

/**
 * @brief Function to signify that a judge entered the building.
 */
void judgeLeave(const char* NAME);
