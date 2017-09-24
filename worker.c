#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char *argp_program_version = "multi-process- v0.1";
const char *argp_program_bug_address = "<liu.ron@husky.neu.edu>";
static char doc[] =
    "A multi-process program that calculates an exponential function.";
static char args_doc[] = "BASE, POWER";
static struct argp_option options[] = {
    {"base", 'x', "BASE", 0, "BASE number."}, {"power", 'n', "POWER", 0, "POWER number."},
};


struct arguments {
    int x, n;
};

double exponentiate(int x, int n);
unsigned long long factorial(int n);
unsigned long long my_pow(int base, int expo);

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
    double out;

    /* Default values. */
    arguments.x = 0;
    arguments.n = 0;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    out = exponentiate(arguments.x, arguments.n);
    printf("x^n / n! : %.4f\n", out);
    return 0;
}

double exponentiate(int x, int n) {
    if (n == 0)
        return 1.0;
    unsigned long long denominator = factorial(n);
    unsigned long long nominator = my_pow(x, n);
    return nominator / (double) denominator;
}

unsigned long long factorial(int n) {
    unsigned long long base = 1;
    int i = 0;
    while ((++i) <= n)
        base = base * i;
    return base;
}

unsigned long long my_pow(int base, int expo) {
    unsigned long long result = 1;
    while ((--expo) >= 0)
        result = result * base;
    return result;
}
