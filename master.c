#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
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

void spawn_a_worker(int idx, int n, int epoll_fd, int *fd, struct arguments *argsp,
                    struct epoll_event *evp)
{
    int pres, rfd, wfd;
    // create pipe
    pres = pipe(fd);
    if (pres < 0)
        printf("Error when piping idx: %d (error:%s)\n", idx, strerror(errno));
    rfd = fd[0];
    wfd = fd[1];

    // fork
    pid_t pid = fork();
    if (pid < 0)
        printf("Error when forking idx: %d (error:%s)\n", idx, strerror(errno));

    if (pid == 0) {
        dup2(wfd, STDOUT_FILENO);
        close(rfd);
        char *arg_x = malloc(10 * sizeof(char *));
        char *arg_n = malloc(10 * sizeof(char *));
        sprintf(arg_x, "%d", argsp->x);
        sprintf(arg_n, "%d", n);
        int res = execl(argsp->worker_path, argsp->worker_path, "-x", arg_x,
                        "-n", arg_n, NULL);
        if (res < 0) {
            printf("Error when executing (error:%s)\n", strerror(errno));
            exit(1);
        }
    } else {
        close(wfd);
    }

    /* Setup the epoll_event for this process */
    evp->events = EPOLLIN;

    /* Parent process listening to read file descriptor */
    evp->data.fd = rfd;

    /* Add rfd to evp or (Read) event on epoll_fds -- evp */
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rfd, evp);
    return;
}

void on_worker_done(int epoll_fd, struct arguments *argsp, int nw, int **fds,
                      struct epoll_event *evts, double *total_p, int *pro_p,
                      int *fin_p) {
    int i, idx;
    struct epoll_event ev, cur_ev;
    char *str = (char *)malloc(10 * sizeof(char *));

    epoll_wait(epoll_fd, &ev, 1, -1);
    ssize_t bytes = read(ev.data.fd, str, 10);

    if (bytes <= 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, NULL);
        return;
    }

    // check out and spawn
    double out = strtod(str, NULL);
    (*total_p) += out;
    (*pro_p) += 1;
    if ((*pro_p) >= argsp->n)
        (*fin_p) = 1;
    if ((*pro_p) + nw > argsp->n)
        return;

    for (i=0; i<nw; i++) {
        cur_ev = evts[i];
        if (cur_ev.data.fd == ev.data.fd) {
            idx = i;
            break;
        }
    }
    spawn_a_worker(idx, ((*pro_p) + nw - 1), epoll_fd, fds[idx], argsp, &evts[idx]);
    return;
}

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
    double total;
    struct arguments args;
    int i, num_workers, epoll_fd, **fds, finished, progress;
    epoll_fd = epoll_create(1);

    args.x = 0;
    args.n = 0;
    args.num_workers = 0;
    args.worker_path = '\0';
    args.wait_mechanism = '\0';

    argp_parse(&argp, argc, argv, 0, 0, &args);
    if (args.n == 0) {
        printf("x^n / !n : 1.0000\n");
        return 0;
    }
    if (strcmp(args.wait_mechanism, "sequential") == 0 ||
        strcmp(args.wait_mechanism, "poll") == 0) {
        printf("%s mechanism is not supported yet.\n", args.wait_mechanism);
        return 0;
    }

    // where the pool is stored
    num_workers = (args.n < args.num_workers) ? args.n : args.num_workers;
    fds = malloc(num_workers * sizeof(int *));
    struct epoll_event events[num_workers];

    for (i = 0; i < num_workers; i++) {
        fds[i] = malloc(2 * sizeof(int *));
        spawn_a_worker(i, i, epoll_fd, fds[i], &args, &events[i]);
    }

    total = 0.0;
    progress = 0;
    finished = 0;

    while (!finished) {
        on_worker_done(epoll_fd, &args, num_workers, fds, events, &total,
                         &progress, &finished);
    }
    printf("x^n / !n : %.4f\n", total);
    return 0;
}
