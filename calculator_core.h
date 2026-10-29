#ifndef CALCULATOR_CORE_H
#define CALCULATOR_CORE_H

#include <stdbool.h>
#include <math.h>

/* This is the single arithmetic implementation shared by the GUI and tests.
    It has no Win32 dependencies, so headless tests can compile it without creating
    a window. Keeping it here prevents the tested math rules from drifting away
    from the production calculator. */
static inline bool calculate(double left, char operator, double right,
                             double *result)
{
    switch (operator) {
        case '+':
            *result = left + right;
            break;
        case '-':
            *result = left - right;
            break;
        case '*':
            *result = left * right;
            break;
        case '/':
            if (right == 0.0) {
                return false;
            }
            *result = left / right;
            break;
        default:
            return false;
    }

    return isfinite(*result);
}

#endif
