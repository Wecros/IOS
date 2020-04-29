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

// #define DEBUG    // debug logs define

const char *my_sem_name = "/xfilip46.ios.proj2.semaphore";
sem_t *no_judge;  // turnstile for incoming immigrants, protects entered
sem_t *mutex;     //
sem_t *confirmed;
int *entered;
int *checked;
int *ordernum;
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
    (*ordernum)++;  // increment the order operations count

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
    no_judge = sem_open("wec.no_judge", SEM_MODE, SEM_PERMS, 1);
    mutex = sem_open("wec.mutex", SEM_MODE, SEM_PERMS, 1);
    confirmed = sem_open("wec.confirmed", SEM_MODE, SEM_PERMS, 0);
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
    ordernum = mmap(NULL, sizeof(*checked), SHARED_PROT, SHARED_FLAGS, -1, 0);
    if (entered == MAP_FAILED || checked == MAP_FAILED || ordernum == MAP_FAILED) {
        fprintf(stderr, "ERROR: Initial setup of shared memory failed.\n");
        return -1;
    }
    *entered = *checked = 0;
    *ordernum = 1;
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
    if (sem_close(no_judge) == -1 || sem_close(mutex) == -1 ||
            sem_close(confirmed) == -1) {
        sem_unlink("wec.no_judge");
        sem_unlink("wec.mutex");
        sem_unlink("wec.confirmed");
        fprintf(stderr, "ERROR: Cleanup of semaphores failed.\n");
        return -1;
    }
    int entered_code = munmap(entered, sizeof(*entered));
    int checked_code = munmap(checked, sizeof(*checked));
    int ordernum_code = munmap(ordernum, sizeof(*ordernum));
    if (entered_code == -1 || checked_code == -1 || ordernum_code == -1) {
        fprintf(stderr, "ERROR: Cleanup of shared memory failed.\n");
        return -1;
    }
    // close the file
    fclose(out);

    return 0;
}

void process_immigrant(unsigned enter_time, unsigned getcert_time) {
    // rand_sleep(enter_time);

    // 1. init
    writelog("%d: NAME I: starts\n", *ordernum);

    // 2. wants to enter

    // entered
    writelog("%d: NAME I: enters: NE: NC: NB\n", *ordernum);

    // 3. wants to register

    // registered
    writelog("%d: NAME I: checks: NE: NC: NB\n", *ordernum);

    // 4. waits for judge's confirmation

    // 5. wants certificate
    writelog("%d: NAME I: wants certifiate: NE: NC: NB\n", *ordernum);
    rand_sleep(getcert_time);

    // got certificate
    writelog("%d: NAME I: got certifiate: NE: NC: NB\n", *ordernum);

    // 6. wants to leave

    // left
    writelog("%d: NAME I: leaves: NE: NC: NB\n", *ordernum);
}

void process_judge(unsigned enter_time, unsigned conf_time) {
    rand_sleep(enter_time);
    // 1. init

    // loop:

    // 2. wants to enter
    writelog("%d: NAME: wants to enter\n", *ordernum);

    // 3. entered
    writelog("%d: NAME: enters: NE: NC: NB\n", *ordernum);

    // 4. confirm the naturalization

    // wait for immigrants
    writelog("%d: NAME: waits for imm: NE: NC: NB\n", *ordernum);

    // starts confirmation
    writelog("%d: NAME: starts confirmation: NE: NC: NB\n", *ordernum);
    rand_sleep(conf_time);

    // ends confirmation
    writelog("%d: NAME: ends confirmation: NE: NC: NB\n", *ordernum);

    // wants to leave
    rand_sleep(conf_time);

    // leaves
    writelog("%d: NAME: leaves: NE: NC: NB\n", *ordernum);
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

    int wstatus, retcode;
    pid_t process = fork();
    if (process == 0) {
        // child process - immigrant generator
        pid_t children_arr[args.PI];
        for (size_t i = 0; i < args.PI; i++) {
            rand_sleep(args.IG);  // TODO:  check for IG == 0
            pid_t immigrant = fork();  // immigrant process
            if (immigrant == 0) {
                process_immigrant(args.IG, args.IT);
                return(EXIT_SUCCESS);
            } else {
                children_arr[i] = immigrant;
                ; // generator proceeds
            }
        }

        int wstatusall;
        for (size_t i = 0; i < args.PI; i++) {
            waitpid(children_arr[i], &wstatus, WNOHANG);
            wstatusall |= wstatus;
        }

        // pid_t generator = fork();
        // if (generator == 0) {
        //     // generate_immigrants(int PI, int delay);
        // }
        // wait(&wstatus);

        if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS) {
            fprintf(stderr, "\nIMM GEN - EVERYTHING OK.\n\n");
            retcode = EXIT_SUCCESS;
        } else {
            fprintf(stderr, "\nIMM GEN - SOMETHING PROBABLY HAPPENED.\n\n");
            retcode = EXIT_FAILURE;
        }
        return retcode;
    } else {
        // parent process - judge
        process_judge(args.JG, args.JT);
    }

    wait(&wstatus); // waits for immigrant generator to die
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS) {
        fprintf(stderr, "\nMAIN JUDGE - EVERYTHING OK.\n\n");
        retcode = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "\nMAIN JUDGE - SOMETHING PROBABLY HAPPENED.\n\n");
        retcode = EXIT_FAILURE;
    }
    if (cleanup() == -1) {
        return EXIT_FAILURE;
    }
    return retcode;
}
