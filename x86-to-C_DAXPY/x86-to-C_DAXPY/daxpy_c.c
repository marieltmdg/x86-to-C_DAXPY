// BAYBAYON & TAMONDONG S20A
#include <stdio.h>
#include <stdlib.h>

double* daxpy_c(int n, double A, double* X, double* Y) {
    double* Z = malloc(n * sizeof(double));

    // Perform daxpy: Z[i] = A * X[i] + Y[i]
    for (int i = 0; i < n; i++) {
        Z[i] = A * X[i] + Y[i];
    }

    return Z;
}