#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define make_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <time.h>
#define make_directory(path) mkdir(path, 0775)
#endif

#include "../calculator_core.h"

struct test_case {
    double left;
    char operator;
    double right;
    double expected;
};

static int approximately_equal(double actual, double expected)
{
    return fabs(actual - expected) <= 1e-12 * fmax(1.0, fabs(expected));
}

static int test_error_cases(void)
{
    double result = 0.0;
    if (calculate(1.0, '/', 0.0, &result)) {
        fputs("FAIL: division by zero was accepted\n", stderr);
        return 0;
    }
    if (calculate(1.0, '?', 2.0, &result)) {
        fputs("FAIL: unsupported operator was accepted\n", stderr);
        return 0;
    }
    if (calculate(INFINITY, '+', 1.0, &result)) {
        fputs("FAIL: infinite input was accepted\n", stderr);
        return 0;
    }
    if (calculate(1e308, '*', 1e308, &result)) {
        fputs("FAIL: overflow result was accepted\n", stderr);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    unsigned long iterations_per_operator = 100000;
    if (argc > 1) {
        char *end = NULL;
        errno = 0;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end != '\0'
            || errno == ERANGE || parsed < 10000 || parsed > 100000000) {
            fputs("Usage: calculator-headless-tests.exe [iterations 10000..100000000]\n", stderr);
            return 2;
        }
        iterations_per_operator = parsed;
    }

    const struct test_case cases[] = {
        {7.0, '+', 5.0, 12.0},
        {9.0, '-', 4.0, 5.0},
        {6.0, '*', 7.0, 42.0},
        {8.0, '/', 2.0, 4.0}
    };
    const size_t case_count = sizeof cases / sizeof cases[0];
    if (!test_error_cases()) {
        return 1;
    }

    if (make_directory("test-results") != 0) {
#ifdef _WIN32
        if (errno != EEXIST) {
#else
        if (errno != EEXIST) {
#endif
            perror("Could not create test-results directory");
            return 1;
        }
    }

    FILE *csv = fopen("test-results/c-benchmark.csv", "w");
    if (csv == NULL) {
        perror("Could not open benchmark CSV");
        return 1;
    }
    fputs("operator,iterations,elapsed_seconds,operations_per_second\n", csv);

    volatile double checksum = 0.0;
    unsigned long total_operations = 0;
#ifdef _WIN32
    LARGE_INTEGER performance_frequency;
    if (!QueryPerformanceFrequency(&performance_frequency)) {
        fputs("High-resolution performance counter is unavailable\n", stderr);
        fclose(csv);
        return 1;
    }
#endif
    for (size_t case_index = 0; case_index < case_count; case_index++) {
        const struct test_case *test = &cases[case_index];
        /* Volatile reads keep the compiler from replacing the repeated calls
           with one precomputed constant result in an optimized benchmark build. */
        volatile double input_left = test->left;
        volatile double input_right = test->right;
        double result = 0.0;
#ifdef _WIN32
        LARGE_INTEGER start;
        LARGE_INTEGER finish;
        QueryPerformanceCounter(&start);
#else
        clock_t start = clock();
#endif
        for (unsigned long iteration = 0; iteration < iterations_per_operator;
             iteration++) {
            if (!calculate(input_left, test->operator, input_right, &result)
                || !approximately_equal(result, test->expected)) {
                fprintf(stderr, "FAIL: operator %c at iteration %lu\n",
                        test->operator, iteration);
                fclose(csv);
                return 1;
            }
            checksum += result;
        }
#ifdef _WIN32
        QueryPerformanceCounter(&finish);
        double elapsed_seconds =
            (double)(finish.QuadPart - start.QuadPart)
            / (double)performance_frequency.QuadPart;
#else
        clock_t elapsed_ticks = clock() - start;
        double elapsed_seconds = (double)elapsed_ticks / CLOCKS_PER_SEC;
#endif
        double operations_per_second = elapsed_seconds > 0.0
            ? (double)iterations_per_operator / elapsed_seconds : 0.0;
        fprintf(csv, "%c,%lu,%.9f,%.2f\n", test->operator,
                iterations_per_operator, elapsed_seconds, operations_per_second);
        printf("%c: %lu calls, %.6f seconds, %.2f calls/second\n",
               test->operator, iterations_per_operator, elapsed_seconds,
               operations_per_second);
        total_operations += iterations_per_operator;
    }

    if (fclose(csv) != 0) {
        perror("Could not close benchmark CSV");
        return 1;
    }
    printf("PASS: %lu arithmetic calls plus invalid-input checks; checksum %.0f\n",
           total_operations, checksum);
    puts("Measurements: test-results/c-benchmark.csv");
    return 0;
}
