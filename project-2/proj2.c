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

// #define SEM_OPEN(name, val) sem_open(name, O_CREAT | O_EXCL, 0666, 1)

const char *my_sem_name = "/xfilip46.ios.proj2.semaphore";
sem_t *no_judge;  // turnstile for incoming immigrants, protects entered
sem_t *mutex;     //
sem_t *confirmed;
sem_t *allSignedIn;
const char* no_judge_name = "wec.no_judge";
const char* mutex_name = "wec.mutex";
const char* confirmed_name = "wec.confirmed";
const char* allSignedIn_name = "wec.allSignedIn";
int *entered;
int *checked;
int *A;
int *NE;
int *NC;
int *NB;
bool *judge;
int immigrantID = 1;
sem_t *exited;
sem_t *allGone;
FILE *out;


void print_help() {
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

void writelog(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    vfprintf(out, fmt, ap);  // write the passed message to output file
    (*A)++;  // increment the order operations count

    va_end(ap);
}

bool are_args_valid(int argc, char *argv[], args_t *args) {
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

void rand_sleep(unsigned ms) {
    srand(time(0));  // set starting point for pseudo-random operations
    ms = rand() % (ms + 1);  // integer in interval <0, ms>
    struct timespec req, rem;
    if (ms < 1000) {
        // lower than 1 second
        req.tv_sec = 0;
        req.tv_nsec = ms * 1000000; // multiply ms to get nanoseconds
    } else {
        // greater than 1 second
        rem.tv_sec = ms / 1000;
        req.tv_nsec = (ms - rem.tv_sec * 1000) * 1000000;
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
    // semaphore setup
    mode_t SEM_MODE = O_CREAT | O_EXCL;
    unsigned int SEM_PERMS = 0666;
    no_judge = sem_open(no_judge_name, SEM_MODE, SEM_PERMS, 1);
    mutex = sem_open(mutex_name, SEM_MODE, SEM_PERMS, 1);
    confirmed = sem_open(confirmed_name, SEM_MODE, SEM_PERMS, 0);
    allSignedIn = sem_open(confirmed_name, SEM_MODE, SEM_PERMS, 0);
    // sem_init(&no_judge, 0, 1);
    // sem_init(&mutex, 0, 1);
    // sem_init(&confirmed, 0, 0);
    if (no_judge == SEM_FAILED || mutex == SEM_FAILED || confirmed == SEM_FAILED) {
        fprintf(stderr, "ERROR: Initial setup of semaphores failed.\n");
        return -1;
    }

    // shared memory setup
    int SHARED_PROT = PROT_READ | PROT_WRITE;  // write and read permissions
    int SHARED_FLAGS = MAP_SHARED | MAP_ANONYMOUS; // handling of map data
    entered = mmap(NULL, sizeof(*entered), SHARED_PROT, SHARED_FLAGS, -1, 0);
    checked = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    A = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    NE = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    NC = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    NB = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    judge = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);

    if (entered == MAP_FAILED || checked == MAP_FAILED || A == MAP_FAILED) {
        fprintf(stderr, "ERROR: Initial setup of shared memory failed.\n");
        return -1;
    }
    *entered = *checked = *NE = *NC = *NB = 0;
    *judge = false;
    *A = 1;
    // open the output file for writing, create if does not exist
    out = fopen("proj2.out", "w");
    if (out ==  NULL) {
        fprintf(stderr, "ERROR: could not open proj2.out for writing.\n");
        return -1;
    }
    setbuf(out, NULL);  // make file unbuffered so writing in it is instant

    return 0;
}

int cleanup() {
    // semaphor cleanup
    int sem_retcode = sem_close(no_judge);
    sem_retcode |= sem_close(mutex);
    sem_retcode |= sem_close(confirmed);
    sem_retcode |= sem_close(allSignedIn);
    sem_retcode |= sem_unlink(no_judge_name);
    sem_retcode |= sem_unlink(mutex_name);
    sem_retcode |= sem_unlink(confirmed_name);
    sem_retcode |= sem_unlink(allSignedIn_name);
    // if (sem_retcode == -1) {
    //     fprintf(stderr, "ERROR: Cleanup of semaphores failed.\n");
    //     return -1;
    // }
    int mem_retcode = munmap(entered, sizeof(*entered));
    mem_retcode |= munmap(checked, sizeof(*checked));
    mem_retcode |= munmap(A, sizeof(*A));
    mem_retcode |= munmap(NE, sizeof(*NE));
    mem_retcode |= munmap(NC, sizeof(*NC));
    mem_retcode |= munmap(NB, sizeof(*NB));
    mem_retcode |= munmap(judge, sizeof(*judge));
    // if (mem_retcode == -1) {
    //     fprintf(stderr, "ERROR: Cleanup of shared memory failed.\n");
    //     return -1;
    // }
    // close the file
    fclose(out);

    return 0;
}

void immEnter(const char* NAME, unsigned ID) {
    writelog("%3d: %s %2d: enters:              %d: %d: %d\n", *A, NAME, ID, *NE, *NC, *NB);
    (*NE)++;
    (*NB)++;
}

void immCheckIn(const char* NAME, unsigned ID) {
    writelog("%3d: %s %2d: checks:              %d: %d: %d\n", *A, NAME, ID, *NE, *NC, *NB);
    (*NC)++;
}

void immLeave(const char* NAME, unsigned ID) {
    writelog("%3d: %s %2d: leaves:              %d: %d: %d\n", *A, NAME, ID, *NE, *NC, *NB);
    (*NB)--;
}

void process_immigrant(unsigned ID, unsigned getcert_time) {
    // 1. init
    const char* NAME = "IMM";
    writelog("%3d: %s %2d: starts\n", *A, NAME, ID);

    // 2. wants to enter
    sem_wait(no_judge);
    // entered
    immEnter(NAME, ID);
    (*entered)++;
    sem_post(no_judge);

    // 3. wants to register
    sem_wait(mutex);
    // registered
    immCheckIn(NAME, ID);
    (*checked)++;

    // 4. waits for judge's confirmation
    if (judge == true && *entered == *checked) {
        sem_post(allSignedIn);
    } else {
        // pass the mutex
        sem_post(mutex);
    }

    // 4.5 waits for judge to confirm
    sem_wait(confirmed);

    // 5. wants certificate
    writelog("%3d: %s %2d: wants certifiate:    %d: %d: %d\n", *A, NAME, ID, *NE, *NC, *NB);
    rand_sleep(getcert_time);
    // got certificate
    writelog("%3d: %s %2d: got certifiate:      %d: %d: %d\n", *A, NAME, ID, *NE, *NC, *NB);
    (*NE)--;
    (*NC)--;

    // 6. wants to leave
    sem_wait(no_judge);
    // left
    immLeave(NAME, ID);
    sem_post(no_judge);

}

void judgeEnter(const char* NAME) {
    writelog("%3d: %6s: enters:              %d: %d: %d\n", *A, NAME, *NE, *NC, *NB);
}

void process_judge(unsigned enter_time, unsigned conf_time) {
    // 1. init
    const char* NAME = "JUDGE";

    // loop:

    // 2. wants to enter
    rand_sleep(enter_time);
    writelog("%3d: %6s: wants to enter\n", *A, NAME);
    sem_wait(no_judge);
    sem_wait(mutex);

    // 3. entered
    judgeEnter(NAME);
    (*judge) = true;

    // 4. confirm the naturalization

    // wait for immigrants
    if (*entered > checked) {
        writelog("%3d: %6s: waits for imm:       %d: %d: %d\n", *A, NAME, *NE, *NC, *NB);
        sem_post(mutex);
        sem_wait(allSignedIn);
    } // and get the mutex back

    // starts confirmation
    writelog("%3d: %6s: starts confirmation: %d: %d: %d\n", *A, NAME, *NE, *NC, *NB);
    rand_sleep(conf_time);


    // ends confirmation
    writelog("%3d: %6s: ends confirmation:   %d: %d: %d\n", *A, NAME, *NE, *NC, *NB);

    // wants to leave
    rand_sleep(conf_time);

    // leaves
    writelog("%3d: %6s: leaves:              %d: %d: %d\n", *A, NAME, *NE, *NC, *NB);
}


int main(int argc, char *argv[]) {
    args_t args;
    if (!are_args_valid(argc, argv, &args)) {
        print_help();
        return EXIT_FAILURE;
    }
    if (setup() == -1) {
        cleanup();
        fprintf(stderr, "Terminating the program...\n");
        return EXIT_FAILURE;
    }

    #ifdef DEBUG
    fprintf(stderr, "Args: PI = %u\n"
        "      IG = %u\n"
        "      JG = %u\n"
        "      IT = %u\n"
        "      JT = %u\n",
        args.PI, args.IG, args.JG, args.IT, args.JT);
    #endif

    int wstatus = 0, retcode = 0;
    pid_t process = fork();
    if (process == 0) {
        // child process - immigrant generator
        pid_t children_arr[args.PI];
        for (size_t i = 0; i < args.PI; i++, immigrantID++) {
            rand_sleep(args.IG);  // TODO:  check for IG == 0
            pid_t immigrant = fork();  // immigrant process
            if (immigrant == 0) {
                process_immigrant(immigrantID, args.IT);
                exit(EXIT_SUCCESS);
            } else {
                // generator proceeds
                children_arr[i] = immigrant;
            }
        }

        int wstatusall = 0;
        for (size_t i = 0; i < args.PI; i++) {
            waitpid(children_arr[i], &wstatus, WNOHANG);
            wstatusall |= wstatus;
            #ifdef DEBUG
            printf("%d\n", children_arr[i]);
            printf("%d, %d\n", wstatus, wstatusall);
            #endif
        }

        // pid_t generator = fork();
        // if (generator == 0) {
        //     // generate_immigrants(int PI, int delay);
        // }
        // wait(&wstatus);

        if (WIFEXITED(wstatus) && WEXITSTATUS(wstatusall) == EXIT_SUCCESS) {
            fprintf(stderr, "IMM GEN - EVERYTHING OK.\n");
            retcode = EXIT_SUCCESS;
        } else {
            fprintf(stderr, "IMM GEN - SOMETHING PROBABLY HAPPENED.\n");
            retcode = EXIT_SUCCESS;
        }
        return retcode;
    } else {
        // parent process - judge
        process_judge(args.JG, args.JT);
    }

    wait(&wstatus); // waits for immigrant generator to die
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS) {
        fprintf(stderr, "MAIN JUDGE - EVERYTHING OK.\n");
        retcode = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "MAIN JUDGE - SOMETHING PROBABLY HAPPENED.\n");
        retcode = EXIT_FAILURE;
    }
    if (cleanup() == -1) {
        return EXIT_FAILURE;
    }
    return retcode;
}
