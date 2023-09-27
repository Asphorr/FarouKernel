#include <math.h>

double sqrt(double x) {
    return __sqrt(x);
}

double pow(double base, double exponent) {
    return __pow(base, exponent);
}

double exp(double x) {
    return __exp(x);
}

double log(double x) {
    return __log(x);
}

double sin(double x) {
    return __sin(x);
}

double cos(double x) {
    return __cos(x);
}

double tan(double x) {
    return __tan(x);
}

double asin(double x) {
    return __asin(x);
}

double acos(double x) {
    return __acos(x);
}

double atan(double x) {
    return __atan(x);
}

double atan2(double y, double x) {
    return __atan2(y, x);
}

float fabs(float x) {
    return fabsf(x);
}

float round(float x) {
    return roundf(x);
}

float ceil(float x) {
    return ceilf(x);
}

float floor(float x) {
    return floorf(x);
}

float trunc(float x) {
    return truncf(x);
}

long double ldexp(long double x, int n) {
    return ldexpf(x, n);
}

long double frexp(long double x, int *n) {
    return frexpf(x, n);
}

long double modf(long double x, long double *iptr) {
    return modfpf(x, iptr);
}
