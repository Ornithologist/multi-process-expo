#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *argp_program_version = "multi-process- v0.1";
const char *argp_program_bug_address = "<liu.ron@husky.neu.edu>";
static char doc[] =
    "A multi-process program that calculates an exponential function.";
static char args_doc[] = "BASE, POWER";
static struct argp_option options[] = {
    {"base", 'x', "BASE", 0, "BASE number."},
    {"power", 'n', "POWER", 0, "POWER number."},
    {"worker_path", 'p', "PATH", 0, "PATH to find the worker program."},
    {"num_workers", 'w', "NUM", 0, "NUM of workers to fork."},
    {"wait_mechanism", 'm', "WAIT", 0, "Use WAIT mechanism."},
};

struct arguments {
    int x, n, num_workers;
    char *worker_path, *wait_mechanism;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key) {
        case 'x':
            if (arg && (atoi(arg) || strcmp(arg, "0") == 0))
                arguments->x = atoi(arg);
            else
                argp_error(state, "Invalid argument 'x'.");
            break;
        case 'n':
            if (arg && (atoi(arg) || strcmp(arg, "0") == 0))
                arguments->n = atoi(arg);
            else
                argp_error(state, "Invalid argument 'n'.");
            break;
        case 'p':
            if (arg && access(arg, X_OK) == 0)
                arguments->worker_path = arg;
            else
                argp_error(state, "Invalid argument 'worker_path'.");
            break;
        case 'w':
            if (arg && atoi(arg))
                arguments->num_workers = atoi(arg);
            else
                argp_error(state, "Invalid argument 'num_workers'.");
            break;
        case 'm':
            if (arg &&
                (strcmp(arg, "sequential") == 0 || strcmp(arg, "select") == 0 ||
                 strcmp(arg, "poll") == 0 || strcmp(arg, "epoll") == 0))
                arguments->wait_mechanism = arg;
            else
                argp_error(state, "Invalid argument 'wait_mechanism'.");
            break;

        case ARGP_KEY_ARG:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv)
{
    struct arguments args;
    // int i, num_workers, epoll_fd, **fds, finished;

    /* Default values. */
    args.x = 0;
    args.n = 0;
    args.num_workers = 0;
    args.worker_path = '\0';
    args.wait_mechanism = '\0';

    argp_parse(&argp, argc, argv, 0, 0, &args);

    if (strcmp(args.wait_mechanism, "sequential") == 0 ||
        strcmp(args.wait_mechanism, "poll") == 0) {
        printf("%s mechanism is not supported yet.\n", args.wait_mechanism);
        return 0;
    }

    // pipe
    int fd[2];
    if (pipe(fd) < 0) {
        printf("Error when piping (error:%s)\n", strerror(errno));
        return 0;
    }

    // fork
    pid_t pid = fork();
    if (pid < 0)
        printf("Error when forking (error:%s)\n",
               strerror(errno));

    if (pid == 0) {
        dup2 (fd[1], STDOUT_FILENO);
        close(fd[0]);
        char *arg_x = malloc(10* sizeof(char *));
        char *arg_n = malloc(10* sizeof(char *));
        sprintf(arg_x, "%d", args.x);
        sprintf(arg_n, "%d", args.n);
        int res = execl(args.worker_path, args.worker_path, "-x", arg_x, "-n", arg_n, NULL);
        if (res < 0) {
            printf("Error when executing (error:%s)\n",
                strerror(errno)); 
            return 0;
        }
    } else {
        close(fd[1]);
        int wc = wait(NULL);
        if (wc < 0) {
            printf("Wait error\n");
        }
        char c[7];
        int r = read(fd[0], c, sizeof(char) * 7);
        if(r > 0)
            printf("PIPE INPUT = %s\n", c);
    }

    return 0;
}