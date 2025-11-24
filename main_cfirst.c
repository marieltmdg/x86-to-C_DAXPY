// BAYBAYON & TAMONDONG S20A
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>

extern double* daxpy_asm(int n, double A, double* X, double* Y);    // Assembly kernel (returns  Z)
extern double* daxpy_c(int n, double A, double* X, double* Y);      // C version (returns Z)

static LARGE_INTEGER qpc_freq;
static void init_timer(void) {
    QueryPerformanceFrequency(&qpc_freq);
}

static double now_ms(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)qpc_freq.QuadPart;
}

int main() {
    init_timer();
    srand(12345);

    const int runs = 30;
    const double A = 6.7;
    int sizes[3] = { 1048576, 16777216, 268435456 };
    int num_sizes = 3;

    printf("DAXPY benchmark: Z = A*X + Y\n");
    printf("Scalar A = %.9f\n", A);
    printf("Runs per version = %d\n\n", runs);

    for (int j = 0; j < num_sizes; ++j) {
        int n = sizes[j];
        printf("|========================================= Vector length: %d =========================================|\n", n);

        // allocate X and Y
        double* X = (double*)malloc(n * sizeof(double));
        double* Y = (double*)malloc(n * sizeof(double));
        if (!X || !Y) {
            fprintf(stderr, "Allocation for size %d failed. Skipping this size.\n\n", n);
            free(X); free(Y);
            continue;
        }

        // initialize inputs (random in [-100,100])
        for (int i = 0; i < n; ++i) {
            X[i] = ((double)rand() / (double)RAND_MAX) * 200.0 - 100.0;
            Y[i] = ((double)rand() / (double)RAND_MAX) * 200.0 - 100.0;
        }

        int printCap = (n < 10) ? n : 10;

        // Temporary storage for the first elements captured during timing runs
        double first10_asm[10] = { 0 };
        double first10_c[10] = { 0 };
        int captured_asm = 0;
        int captured_c = 0;

        // Time C version (print each run's time)
        double total_c_ms = 0.0;
        int c_success = 0;
        printf("C runs (run #: time ms):\n");
        for (int r = 0; r < runs; ++r) {
            double start = now_ms();
            double* Z = daxpy_c(n, A, X, Y);
            double end = now_ms();
            if (!Z) {
                fprintf(stderr, "daxpy_c returned NULL on run %d for size %d\n", r + 1, n);
                break;
            }
            double elapsed = end - start;
            if (!((r + 1) % 5 == 0)) printf("  %2d: %9.6f ms | ", r + 1, elapsed);
            else printf("  %2d: %9.6f ms\n", r + 1, elapsed);
            total_c_ms += elapsed;
            c_success++;
            if (!captured_c) {
                for (int i = 0; i < printCap; ++i) first10_c[i] = Z[i];
                captured_c = 1;
            }
            free(Z);
        }
        if (c_success == 0) {
            fprintf(stderr, "C runs failed for size %d. Skipping.\n\n", n);
            free(X); free(Y);
            continue;
        }
        double avg_c = total_c_ms / c_success;

        // Time assembly version (print each run's time)
        double total_asm_ms = 0.0;
        int asm_success = 0;
        printf("Assembly runs (run #: time ms):\n");
        for (int r = 0; r < runs; ++r) {
            double start = now_ms();
            double* Z = daxpy_asm(n, A, X, Y);
            double end = now_ms();
            if (!Z) {
                fprintf(stderr, "daxpy_asm returned NULL on run %d for size %d\n", r + 1, n);
                break;
            }
            double elapsed = end - start;
            if (!((r + 1) % 5 == 0)) printf("  %2d: %9.6f ms | ", r + 1, elapsed);
            else printf("  %2d: %9.6f ms\n", r + 1, elapsed);
            total_asm_ms += elapsed;
            asm_success++;
            if (!captured_asm) {
                for (int i = 0; i < printCap; ++i) first10_asm[i] = Z[i];
                captured_asm = 1;
            }
            free(Z);
        }
        if (asm_success == 0) {
            fprintf(stderr, "Assembly runs failed for size %d. Skipping.\n\n", n);
            free(X); free(Y);
            continue;
        }
        double avg_asm = total_asm_ms / asm_success;

        printf("\nRESULTS FOR VECTOR SIZE = %d\n", n);

        printf("C        AVG TIME: %.6f ms\n", avg_c);
        printf("ASSEMBLY AVG TIME: %.6f ms\n", avg_asm);

        printf("Speedup (C/ASM): %.4fx\n", avg_c / avg_asm);

        // Print first elements (Assembly | C)
        printf("\nFirst %d elements of Z (Assembly | C):\n", printCap);
        for (int i = 0; i < printCap; ++i) {
            if (!((i + 1) % 2 == 0)) printf("[%d] Assembly: %15.10f | C: %15.10f\t", i, first10_asm[i], first10_c[i]);
            else printf("[%d] Assembly: %15.10f | C: %15.10f\n", i, first10_asm[i], first10_c[i]);
        }

        // Sanity check: compare outputs exactly for the captured elements
        int ok = 1;
        for (int i = 0; i < printCap; ++i) {
            double a = first10_asm[i];
            double b = first10_c[i];
            double diff = fabs(a - b);
            if (diff != 0.0) {
                ok = 0;
                printf("Mismatch at index %d: asm=%.12f c=%.12f diff=%.12f\n", i, a, b, diff);
            }
        }
        printf("Sanity check: %s\n\n", ok ? "PASSED" : "FAILED");

        free(X);
        free(Y);
    }

    return 0;
}