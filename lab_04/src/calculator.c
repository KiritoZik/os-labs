#include "calculator.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static int read_number(const char** p, int* number) {
    char* end;
    errno = 0;
    long value = strtol(*p, &end, 10);

    if (end == *p || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        return -1;
    }
    if (*end != '\0' && !isspace((unsigned char)*end)) {
        return -1;
    }
    *number = (int)value;
    *p = end;
    return 0;
}

char calculate_line(const char *line, int *answer) {
    const char *p = line;
    int result;

    if (read_number(&p, &result) == -1) {
        return 'I';
    }

    while (1) {
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            *answer = result;
            return 'O';
        }

        int divisor;
        if (read_number(&p, &divisor) == -1) {
            return 'I';
        }
        if (divisor == 0) {
            return 'Z';
        }
        if (result == INT_MIN && divisor == -1) {
            return 'I';
        }
        result /= divisor;
    }
}
