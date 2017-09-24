#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
            if (arg && atoi(arg))                   // test number
                arguments->x = atoi(arg);
            else
                argp_error(state, "Invalid argument 'x'.");
            break;
        case 'n':
            if (arg && atoi(arg))                   // test number
                arguments->n = atoi(arg);
            else
                argp_error(state, "Invalid argument 'n'.");
            break;
        case 'p':
            if (arg && access(arg, X_OK) == 0)      // test if executable
                arguments->worker_path = arg;
            else
                argp_error(state, "Invalid argument 'worker_path'.");
            break;
        case 'w':
            if (arg && atoi(arg))                   // test number
                arguments->num_workers = atoi(arg);
            else
                argp_error(state, "Invalid argument 'num_workers'.");
            break;
        case 'm':
            if (arg && (strcmp(arg, "sequential") == 0 || strcmp(arg, "select") ==0 ||
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
    struct arguments arguments;

    /* Default values. */
    arguments.x = 0;
    arguments.n = 0;
    arguments.num_workers = 0;
    arguments.worker_path = '\0';
    arguments.wait_mechanism = '\0';

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    return 0;
}