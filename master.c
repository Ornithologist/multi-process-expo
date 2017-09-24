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

    // fork
    pid_t pid = fork();
    if (pid < 0)
        printf("Error when forking (error:%s)\n",
               strerror(errno));

    if (pid == 0) {
        char *worker_args[6];  // FIXME: there must be a better way
        worker_args[0] = args.worker_path;
        worker_args[1] = "-x";
        sprintf(worker_args[2], "%d", args.x);
        worker_args[3] = "-n";
        sprintf(worker_args[4], "%d", args.x);
        worker_args[5] = NULL;
        execvp(worker_args[0], worker_args);
    } else {
        int wc = wait(NULL);
        printf("pid: %d\n", wc);
    }

    return 0;

    // num_workers = (args.n < args.num_workers) ? args.n : args.num_workers;
    // int pres, rfd, wfd;

    // fds = malloc(num_workers * sizeof(int *));

    // for (i = 0; i < num_workers; i++) {
    //     // create pipe
    //     fds[i] = malloc(2 * sizeof(int *));
    //     pres = pipe(fds[i]);
    //     if (pres < 0)
    //         printf("Error when piping idx: %d (error:%s)\n", i,
    //                strerror(errno));
    //     rfd = fds[i][0];
    //     wfd = fds[i][1];

    //     // fork
    //     pid_t pid = fork();
    //     if (pid < 0)
    //         printf("Error when forking idx: %d (error:%s)\n", i,
    //                strerror(errno));

    //     if (pid == 0) {
    //         close(rfd);        // ?
    //         char *worker_args[6];  // FIXME: there must be a better way
    //         worker_args[0] = args.worker_path;
    //         worker_args[1] = "-x";
    //         sprintf(worker_args[2], "%d", args.x);
    //         worker_args[3] = "-n";
    //         sprintf(worker_args[4], "%d", args.x);
    //         worker_args[5] = NULL;
    //         execvp(worker_args[0], worker_args);
    //     } else {
    //         close(wfd);  // ?
    //     }

    //     /* Setup the epoll_event for this process */
    //     event[i].events = EPOLLIN;

    //     /* Parent process listening to read file descriptor */
    //     event[i].data.fd = rfd;

    //     /* Add rfd to event[i] or (Read) event on epoll_fds -- event[i] */
    //     epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rfd, &event[i]);
    // }

    // finished = 0;
    // int buffer_size = 7;

    // while (!finished) {
    //     struct epoll_event ev;
    //     epoll_wait(epoll_fd, &ev, 1, -1);

    //     char str[buffer_size];
    //     ssize_t bytes = read(ev.data.fd, &str, buffer_size);

    //     /* If the ev.data.fd has bytes added print, else wait */
    //     if (bytes > 0)
    //         printf("Read: %s\n", str);
    //     else
    //         epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, NULL);
    // }

    // return 0;
}