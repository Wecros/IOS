/**
 * @file proj2.c
 * @author Marek Filip (xfilip46), FIT BUT
 * @date 2020-Apr-24
 * @brief IOS-project-2
 * @see http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf
 *      https://www.fit.vutbr.cz/study/courses/IOS/private/Lab/projekty/projekt2/projekt2.pdf
 * @details Implementaiton of the Faneuil Hall Problem.
 *          Program returns 0 if everything went OK, 1 if errors encountered.
 *          Compiled: gcc 9.3
 */

#include "proj2.h"

sem_t *noJudge;  // turnstile for incoming immigrants, protects entered
sem_t *mutex;     //
sem_t *confirmed;
sem_t *allSignedIn;
sem_t *logWritten;
sem_t *sharedMutex;
const char* noJudgeName = "wec.noJudge";
const char* mutexName = "wec.mutex";
const char* confirmedName = "wec.confirmed";
const char* allSignedInName = "wec.allSignedIn";
const char* logWrittenName = "wec.printing";
const char* sharedMutexName = "wec.sharedMutex";
int *entered;
int *checked;
int *A;
int *NB;
int *activeImmigrants;
bool *judge;
FILE *out;


void printHelp() {
    fprintf(stderr,
    "USAGE: proj2 <PI> <IG> <JG> <IT> <JT>\n"
    "  PI: Number of processes generated in immigrant category.\n"
    "    PI >= 1\n"
    "  IG: Max time after which is immigrant process generated.\n"
    "    IG >= 0 && IG <= 2000\n"
    "  JG: Max time after which judge enters the hall.\n"
    "    JG >= 0 && JG <= 2000.\n"
    "  IT: Max time for getting the certificate by immigrant.\n"
    "    IT >= 0 && IT <= 2000.\n"
    "  JT: Max time for confiriming the naturalization by judge.\n"
    "    JT >= 0 && JT <= 2000.\n"
    "All arguments are integers.\n"
    );
}

void writelog(const char* NAME, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    sem_wait(logWritten);  // semaphor for synchronizing prints
    fprintf(out, "%3d: %s ", *A, NAME);  // prints the order of ops and name
    vfprintf(out, fmt, ap);  // write the passed message to output file
    *A += 1;  // increment the order operations count
    sem_post(logWritten);

    va_end(ap);
}

bool areArgsValid(int argc, char *argv[], args_t *args) {
    // if number of arguments is not 5, return false
    const int VALID_ARGS_NUM = 5;
    if (argc != VALID_ARGS_NUM + 1) {
        fprintf(stderr, "ERROR: Invalid number of arguments!\n\n");
        return false;
    }

    unsigned arg_values[VALID_ARGS_NUM];
    for (int i = 0; i < VALID_ARGS_NUM; i++) {
        char *endptr;
        const unsigned BASE = 10;
        int arg = strtol(argv[i+1], &endptr, BASE);
        if (*endptr != '\0' || arg < 0) {  // num is invalid or negative
            fprintf(stderr, "ERROR: Arguments must be positive integers!\n\n");
            return false;
        }
        arg_values[i] = arg;
    }
    args->PI = arg_values[0];
    args->IG = arg_values[1];
    args->JG = arg_values[2];
    args->IT = arg_values[3];
    args->JT = arg_values[4];
    if (args->PI < 1 || args->IG > 2000 || args->JG > 2000 ||
            args->IT > 2000 || args->JT > 2000) {
        fprintf(stderr, "ERROR: Argument values out of range!\n\n");
        return false;
    }

    return true;
}

void randSleep(unsigned ms) {
    // if (ms == 0) {
    //     return;
    // }
    srand(time(0));  // set starting point for pseudo-random operations
    ms = rand() % (ms + 1);  // integer in interval <0, ms>
    struct timespec req, rem;
    if (ms < 1000) {
        // lower than 1 second
        req.tv_sec = 0;
        req.tv_nsec = ms * 1000000; // multiply ms to get nanoseconds
    } else {
        // greater than 1 second
        req.tv_sec = ms / 1000;
        req.tv_nsec = (ms - req.tv_sec * 1000) * 1000000;
    }

    // put the process to sleep
    if (nanosleep(&req, &rem) == -1) {
        // nanosleep failed, terminate the process
        fprintf(stderr, "ERROR: nanosleep failed or got interrupted.\n"
                        "       Terminating the process...\n");
        exit(EXIT_FAILURE);
    }
}

int setup() {
    int retcode = 0;
    // semaphore setup
    mode_t SEM_MODE = O_CREAT | O_EXCL;
    unsigned int SEM_PERMS = 0666;
    noJudge = sem_open(noJudgeName, SEM_MODE, SEM_PERMS, 1);
    mutex = sem_open(mutexName, SEM_MODE, SEM_PERMS, 1);
    confirmed = sem_open(confirmedName, SEM_MODE, SEM_PERMS, 0);
    allSignedIn = sem_open(allSignedInName, SEM_MODE, SEM_PERMS, 0);
    logWritten = sem_open(logWrittenName, SEM_MODE, SEM_PERMS, 1);
    sharedMutex = sem_open(sharedMutexName, SEM_MODE, SEM_PERMS, 1);
    if (noJudge == SEM_FAILED || mutex == SEM_FAILED || confirmed == SEM_FAILED ||
            allSignedIn == SEM_FAILED || logWritten == SEM_FAILED ||
            sharedMutex == SEM_FAILED) {
        fprintf(stderr, "ERROR: Initial setup of semaphores failed.\n");
        retcode = -1;
    }
    // shared memory setup
    int SHARED_PROT = PROT_READ | PROT_WRITE;  // write and read permissions
    int SHARED_FLAGS = MAP_SHARED | MAP_ANONYMOUS; // handling of map data
    entered = mmap(NULL, sizeof(*entered), SHARED_PROT, SHARED_FLAGS, -1, 0);
    checked = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    NB = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    activeImmigrants = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    judge = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    A = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);

    if (entered == MAP_FAILED || checked == MAP_FAILED || A == MAP_FAILED ||
            activeImmigrants == MAP_FAILED || judge == MAP_FAILED || NB == MAP_FAILED) {
        fprintf(stderr, "ERROR: Initial setup of shared memory failed.\n");
        retcode = -1;
    }
    *entered = *checked = *NB = *activeImmigrants = 0;
    *judge = false;
    *A = 1;
    // open the output file for writing, create if does not exist
    out = fopen("proj2.out", "w");
    if (out ==  NULL) {
        fprintf(stderr, "ERROR: could not open proj2.out for writing.\n");
        retcode = -1;
    }
    setbuf(out, NULL);  // make file unbuffered so writing in it is instant

    return retcode;
}

int cleanup() {
    // semaphor cleanup
    int retcode, sem_retcode, mem_retcode;
    retcode = sem_retcode = mem_retcode = 0;
    sem_retcode |= sem_close(noJudge);
    sem_retcode |= sem_close(mutex);
    sem_retcode |= sem_close(confirmed);
    sem_retcode |= sem_close(allSignedIn);
    sem_retcode |= sem_close(logWritten);
    sem_retcode |= sem_close(sharedMutex);
    sem_retcode |= sem_unlink(noJudgeName);
    sem_retcode |= sem_unlink(mutexName);
    sem_retcode |= sem_unlink(confirmedName);
    sem_retcode |= sem_unlink(allSignedInName);
    sem_retcode |= sem_unlink(logWrittenName);
    sem_retcode |= sem_unlink(sharedMutexName);
    if (sem_retcode == -1) {
        fprintf(stderr, "ERROR: Cleanup of semaphores failed.\n");
        retcode = -1;
    }
    // shared memory cleanup
    mem_retcode |= munmap(entered, sizeof(*entered));
    mem_retcode |= munmap(checked, sizeof(*checked));
    mem_retcode |= munmap(A, sizeof(*A));
    mem_retcode |= munmap(NB, sizeof(*NB));
    mem_retcode |= munmap(activeImmigrants, sizeof(*activeImmigrants));
    mem_retcode |= munmap(judge, sizeof(*judge));
    if (mem_retcode == -1) {
        fprintf(stderr, "ERROR: Cleanup of shared memory failed.\n");
        retcode = -1;
    }
    // close the file
    fclose(out);

    return retcode;
}

void immEnter(const char* NAME, unsigned ID) {
    sem_wait(sharedMutex);
    (*NB)++;
    (*entered)++;
    sem_post(sharedMutex);
    writelog(NAME, "%2d: enters:              %d: %d: %d\n", ID, *entered, *checked, *NB);
}

void immCheckIn(const char* NAME, unsigned ID) {
    sem_wait(sharedMutex);
    (*checked)++;
    sem_post(sharedMutex);
    writelog(NAME, "%2d: checks:              %d: %d: %d\n", ID, *entered, *checked, *NB);
}

void immLeave(const char* NAME, unsigned ID) {
    sem_wait(sharedMutex);
    (*NB)--;
    sem_post(sharedMutex);
    writelog(NAME, "%2d: leaves:              %d: %d: %d\n", ID, *entered, *checked, *NB);
}

void process_immigrant(unsigned ID, unsigned getcert_time) {
    // 1. init
    const char* NAME = "IMM";
    writelog(NAME, "%2d: starts\n", ID);

    // 2. wants to enter
    sem_wait(noJudge);
    immEnter(NAME, ID);     // entered
    sem_post(noJudge);

    // 3. wants to register
    sem_wait(mutex);
    immCheckIn(NAME, ID);   // registered

    // 4. waits for judge's confirmation
    if (*judge == true && *entered == *checked) {
        sem_post(allSignedIn);
    } else {
        sem_post(mutex);    // pass the mutex
    }
    // 4.5 waits for judge to confirm
    sem_wait(confirmed);

    // 5. wants certificate
    sem_wait(sharedMutex);  // mutex until judge confirms all
    writelog(NAME, "%2d: wants certificate:   %d: %d: %d\n", ID, *entered, *checked, *NB);
    sem_post(sharedMutex);
    randSleep(getcert_time);
    // got certificate
    writelog(NAME, "%2d: got certificate:     %d: %d: %d\n", ID, *entered, *checked, *NB);
    // 6. wants to leave
    sem_wait(noJudge);
    immLeave(NAME, ID);     // left
    sem_post(noJudge);
    // sem_post(mutex);  // FIXME: NOT IN PSEUDOCODE
}

void judgeEnter(const char* NAME) {
    sem_wait(sharedMutex);
    *judge = true;
    sem_post(sharedMutex);
    writelog(NAME, ": enters:              %d: %d: %d\n", *entered, *checked, *NB);
}

void judgeStartConfirm(const char* NAME, unsigned conf_time) {
    writelog(NAME, ": starts confirmation: %d: %d: %d\n", *entered, *checked, *NB);
    randSleep(conf_time);
}

void judgeConfirm(const char* NAME) {
    writelog(NAME, ": ends confirmation:   %d: %d: %d\n", *entered, *checked, *NB);
}

void judgeLeave(const char* NAME, unsigned conf_time) {
    randSleep(conf_time);
    writelog(NAME, ": leaves:              %d: %d: %d\n", *entered, *checked, *NB);
    *judge = false;
}

void process_judge(unsigned enter_time, unsigned conf_time) {
    // 1. init
    const char* NAME = "JUDGE";

    // loop until all immigrant processes were registered:
    while (*activeImmigrants != 0) {
        // 2. wants to enter
        randSleep(enter_time);
        writelog(NAME, ": wants to enter\n");
        sem_wait(noJudge);
        sem_wait(mutex);

        judgeEnter(NAME);  // 3. entered

        // 4. confirm the naturalization
        // wait for immigrants
        if (*entered > *checked) {
            writelog(NAME, ": waits for imm:       %d: %d: %d\n", *entered, *checked, *NB);
            sem_post(mutex);
            sem_wait(allSignedIn);
        } // and get the mutex back
        // starts confirmation
        judgeStartConfirm(NAME, conf_time);
        sem_wait(sharedMutex);
        for (int i = 0; i < *checked; i++) { // confirmed.signal(checked)
            sem_post(confirmed);
        }
        *activeImmigrants -= *entered;
        *entered = *checked = 0;
        judgeConfirm(NAME);
        sem_post(sharedMutex); // ends confirmation
        // 5. wants to leave
        judgeLeave(NAME, conf_time);  // leaves

        sem_post(mutex);
        sem_post(noJudge);
    }
    writelog(NAME, ": finishes\n");  // 7. finishes
}

int main(int argc, char *argv[]) {
    args_t args;
    if (!areArgsValid(argc, argv, &args)) {
        printHelp();
        return EXIT_FAILURE;
    }
    if (setup() == -1) {
        cleanup();  // clean so next start is okay
        fprintf(stderr, "Terminating the program...\n");
        return EXIT_FAILURE;
    }

    int immigrantID = 1;
    *activeImmigrants = args.PI;
    pid_t wpid;
    int wstatus = 0, retcode = 0;
    pid_t process = fork();
    if (process == 0) {
        // child process - immigrant generator
        for (size_t i = 0; i < args.PI; i++, immigrantID++) {
            randSleep(args.IG);  // TODO:  check for IG == 0
            pid_t immigrant = fork();  // immigrant process
            if (immigrant == 0) {
                process_immigrant(immigrantID, args.IT);
                return EXIT_SUCCESS;
            }
        }
        // wait for all the immigrant processes to die
        while ((wpid = wait(&wstatus)) > 0) {
            if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS) {
                retcode = EXIT_SUCCESS;
            } else {
                fprintf(stderr, "ERROR: Immigrant proccess exited with failure");
                retcode = EXIT_FAILURE;
            }
        }
        return retcode;
    } else {
        // parent process - judge
        process_judge(args.JG, args.JT);
    }
    // wait for the immigrant generator process to die
    while ((wpid = wait(&wstatus)) > 0) {
        if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS) {
            retcode = EXIT_SUCCESS;
        } else {
            fprintf(stderr, "ERROR: Immigrant generator exited with failure.");
            retcode = EXIT_FAILURE;
        }
    }

    #ifdef DEBUG
    fprintf(stderr, "DONE\n");
    #endif

    if (cleanup() == -1) {
        return EXIT_FAILURE;
    }
    return retcode;
}
