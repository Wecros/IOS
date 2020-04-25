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
#include <fcntl.h>          // for O_* constants
#include <unistd.h>         // fork()
#include <time.h>           // nanosleep, sleep

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
void process_immigrant(unsigned enter_time, unsigned getcert_time);

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
int sem_setup();
