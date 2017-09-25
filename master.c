#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// global varaibles
double total = 0.0;
int progress = 0;
int finished = 0;
int **fds, *nregistrar;
fd_set rfds;
struct epoll_event *events;

// argument variables
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

void spawn_a_worker(int idx, int epoll_fd, struct arguments *argsp)
{
    int rfd, wfd, n;
    fds[idx] = malloc(2 * sizeof(int *));
    n = nregistrar[idx];
    // create pipe
    if (pipe(fds[idx]) < 0)
        printf("Error when piping worker idx: %d (error:%s)\n", idx,
               strerror(errno));
    rfd = fds[idx][0];
    wfd = fds[idx][1];

    // fork
    pid_t pid = fork();
    if (pid < 0)
        printf("Error when forking worker idx: %d (error:%s)\n", idx,
               strerror(errno));
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
            printf("Error when executing worker %d (error:%s)\n", idx,
                   strerror(errno));
            exit(1);
        }
    } else {
        close(wfd);
    }

    if (strcmp(argsp->wait_mechanism, "epoll") == 0) {
        events[idx].events = EPOLLIN;  // set up event
        events[idx].data.fd = rfd;     // listen to read file descriptor
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rfd,
                  &events[idx]);  // add to epoll_fd
    }
    return;
}

void on_rfd_ready(int cur_fd, int epoll_fd, struct arguments *argsp, int nw)
{
    int i, idx, cur_n;
    char *str = (char *)malloc(10 * sizeof(char *));
    ssize_t bytes = read(cur_fd, str, 10);
    if (bytes <= 0) {
        if (strcmp(argsp->wait_mechanism, "epoll") == 0)
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, NULL);
        return;
    }

    // check out
    double out = strtod(str, NULL);
    total += out;
    progress += 1;
    if (progress >= argsp->n) finished = 1;
    for (i = 0; i < nw; i++) {
        if (fds[i][0] == cur_fd) {
            idx = i;
            break;
        }
    }
    cur_n = nregistrar[idx];
    printf("worker %d: %d^%d / %d! : %.4f\n", idx, argsp->x, cur_n, cur_n, out);
    nregistrar[idx] = progress + nw - 1;

    // if job is fullfilled, do not spawn anymore
    if (progress + nw > argsp->n) {
        return;
    }
    spawn_a_worker(idx, epoll_fd, argsp);
    return;
}

void scan_workers(int epoll_fd, struct arguments *argsp, int nw)
{
    int i;
    FD_ZERO(&rfds);
    for (i = 0; i < nw; i++) FD_SET(fds[i][0], &rfds);

    if (strcmp(argsp->wait_mechanism, "epoll") == 0) {
        struct epoll_event ev;
        epoll_wait(epoll_fd, &ev, 1, -1);
        on_rfd_ready(ev.data.fd, epoll_fd, argsp, nw);
    } else if (strcmp(argsp->wait_mechanism, "select") == 0) {
        int max_fd = 1;
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        for (i = 0; i < nw; i++) {
            max_fd = (max_fd > fds[i][0]) ? max_fd : fds[i][0];
            max_fd = (max_fd > fds[i][1]) ? max_fd : fds[i][1];
        }
        ++max_fd;
        int ret = select(max_fd, &rfds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("select()");
        } else if (ret) {
            for (i = 0; i < nw; i++) {
                if (FD_ISSET(fds[i][0], &rfds)) {
                    on_rfd_ready(fds[i][0], epoll_fd, argsp, nw);
                }
            }
        } else {
            return;
        }
    }
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
    struct arguments args;
    int i, num_workers, epoll_fd;
    epoll_fd = epoll_create(1);  // for epoll
    FD_ZERO(&rfds);              // for select

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
    nregistrar = malloc(num_workers * sizeof(int *));
    events = malloc(num_workers * sizeof(struct epoll_event *));

    for (i = 0; i < num_workers; i++) {
        nregistrar[i] = i;
        spawn_a_worker(i, epoll_fd, &args);
    }

    while (!finished) {
        scan_workers(epoll_fd, &args, num_workers);
    }
    printf("Final Result : %.4f\n", total);
    return 0;
}
