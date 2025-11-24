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

        // allocate per-run timing arrays
        double* asm_times = (double*)malloc(runs * sizeof(double));
        double* c_times = (double*)malloc(runs * sizeof(double));
        if (!asm_times || !c_times) {
            fprintf(stderr, "Allocation failed for timing storage. Skipping size %d.\n\n", n);
            free(X); free(Y); free(asm_times); free(c_times);
            continue;
        }
        for (int r = 0; r < runs; ++r) { asm_times[r] = -1.0; c_times[r] = -1.0; }

        // Paired runs: run both implementations each iteration, alternate order to reduce bias
        for (int r = 0; r < runs; ++r) {
            if (r % 2 == 0) {
                // C then ASM
                double t0 = now_ms();
                double* Zc = daxpy_c(n, A, X, Y);
                double t1 = now_ms();
                if (Zc) {
                    c_times[r] = t1 - t0;
                    if (!captured_c) {
                        for (int k = 0; k < printCap; ++k) first10_c[k] = Zc[k];
                        captured_c = 1;
                    }
                    free(Zc);
                }
                else {
                    c_times[r] = -1.0;
                }

                double t2 = now_ms();
                double* Zasm = daxpy_asm(n, A, X, Y);
                double t3 = now_ms();
                if (Zasm) {
                    asm_times[r] = t3 - t2;
                    if (!captured_asm) {
                        for (int k = 0; k < printCap; ++k) first10_asm[k] = Zasm[k];
                        captured_asm = 1;
                    }
                    free(Zasm);
                }
                else {
                    asm_times[r] = -1.0;
                }
            }
            else {
                // ASM then C
                double t0 = now_ms();
                double* Zasm = daxpy_asm(n, A, X, Y);
                double t1 = now_ms();
                if (Zasm) {
                    asm_times[r] = t1 - t0;
                    if (!captured_asm) {
                        for (int k = 0; k < printCap; ++k) first10_asm[k] = Zasm[k];
                        captured_asm = 1;
                    }
                    free(Zasm);
                }
                else {
                    asm_times[r] = -1.0;
                }

                double t2 = now_ms();
                double* Zc = daxpy_c(n, A, X, Y);
                double t3 = now_ms();
                if (Zc) {
                    c_times[r] = t3 - t2;
                    if (!captured_c) {
                        for (int k = 0; k < printCap; ++k) first10_c[k] = Zc[k];
                        captured_c = 1;
                    }
                    free(Zc);
                }
                else {
                    c_times[r] = -1.0;
                }
            }
        }

        // Compute averages (skip failed runs)
        double total_asm_ms = 0.0; int asm_count = 0;
        double total_c_ms = 0.0;   int c_count = 0;
        for (int r = 0; r < runs; ++r) {
            if (asm_times[r] >= 0.0) { total_asm_ms += asm_times[r]; asm_count++; }
            if (c_times[r] >= 0.0) { total_c_ms += c_times[r];   c_count++; }
        }
        if (asm_count == 0 || c_count == 0) {
            fprintf(stderr, "One implementation failed all runs for size %d. Skipping.\n\n", n);
            free(X); free(Y); free(asm_times); free(c_times);
            continue;
        }
        double avg_asm = total_asm_ms / asm_count;
        double avg_c = total_c_ms / c_count;

        // Print per-run times grouped 5 per row for ASM
        printf("Assembly runs (grouped 5 per row):\n");
        for (int row = 0; row * 5 < runs; ++row) {
            int start = row * 5;
            int end = start + 5;
            if (end > runs) end = runs;
            for (int r = start; r < end; ++r) {
                if (asm_times[r] >= 0.0) printf("  %2d: %9.6f ms   ", r + 1, asm_times[r]);
                else printf("  %2d:   (ERR)     ", r + 1);
            }
            printf("\n");
        }

        // Print per-run times grouped 5 per row for C
        printf("C runs (grouped 5 per row):\n");
        for (int row = 0; row * 5 < runs; ++row) {
            int start = row * 5;
            int end = start + 5;
            if (end > runs) end = runs;
            for (int r = start; r < end; ++r) {
                if (c_times[r] >= 0.0) printf("  %2d: %9.6f ms   ", r + 1, c_times[r]);
                else printf("  %2d:   (ERR)     ", r + 1);
            }
            printf("\n");
        }

        printf("\nRESULTS FOR VECTOR SIZE = %d\n", n);
        printf("ASSEMBLY AVG TIME: %.6f ms (based on %d runs)\n", avg_asm, asm_count);
        printf("C        AVG TIME: %.6f ms (based on %d runs)\n", avg_c, c_count);
        printf("Speedup (C/ASM): %.4fx\n", avg_c / avg_asm);

        // Print first elements (Assembly | C)
        printf("\nFirst %d elements of Z (Assembly | C):\n", printCap);
        for (int i = 0; i < printCap; ++i) {
            printf("[%d] Assembly: % 15.10f | C: % 15.10f\n", i, first10_asm[i], first10_c[i]);
        }

        // Exact sanity check on captured first elements
        int ok = 1;
        for (int i = 0; i < printCap; ++i) {
            double a = first10_asm[i];
            double b = first10_c[i];
            double diff = fabs(a - b);
            if (diff != 0) {
                ok = 0;
                printf("Mismatch at index %d: asm=%.12f c=%.12f\n", i, a, b);
            }
        }
        printf("Sanity check: %s\n\n", ok ? "PASSED" : "FAILED");

        free(asm_times);
        free(c_times);
        free(X);
        free(Y);
    }

    return 0;
}