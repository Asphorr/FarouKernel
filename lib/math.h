#ifndef MATH_H
#define MATH_H

#include <stdlib.h>
#include <stdio.h>

double sqrt(double x);
double pow(double base, double exponent);
double exp(double x);
double log(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);

float fabs(float x);
float round(float x);
float ceil(float x);
float floor(float x);
float trunc(float x);

long double ldexp(long double x, int n);
long double frexp(long double x, int *n);
long double modf(long double x, long double *iptr);

#endif // MATH_H
