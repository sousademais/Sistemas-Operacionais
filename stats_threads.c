#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define N     10000
#define RANGE 101

int data[N];

double avg_global;
double med_global;
double stddev_global;

static double diff_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
}

void *calc_avg(void *arg) {
    long long sum = 0;
    for (int i = 0; i < N; i++) sum += data[i];
    avg_global = (double) sum / (double) N;
    return NULL;
}

int cmp_int(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return ia - ib;
}

void *calc_med(void *arg) {
    int *copy = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) copy[i] = data[i];
    qsort(copy, N, sizeof(int), cmp_int);

    if (N % 2 == 1) med_global = copy[N/2];
    else med_global = (copy[N/2 - 1] + copy[N/2]) / 2.0;

    free(copy);
    return NULL;
}

void *calc_std(void *arg) {
    long long sum = 0;
    for (int i = 0; i < N; i++) sum += data[i];
    double mean = (double) sum / (double) N;

    double var = 0.0;
    for (int i = 0; i < N; i++) {
        double d = data[i] - mean;
        var += d * d;
    }

    stddev_global = sqrt(var / N);
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;
    clock_t t_start, t_end, c_start, c_end;

    srand(0);
    for (int i = 0; i < N; i++) data[i] = rand() % RANGE;

    // ------------------- 3 THREADS ----------------------
    t_start = clock();
    c_start = clock();

    pthread_create(&t1, NULL, calc_avg, NULL);
    pthread_create(&t2, NULL, calc_med, NULL);
    pthread_create(&t3, NULL, calc_std, NULL);

    c_end = clock();

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    t_end = clock();

    printf("=== 3 Threads ===\n");
    printf("Média   = %.3f\n", avg_global);
    printf("Mediana = %.3f\n", med_global);
    printf("Desvio  = %.3f\n", stddev_global);
    printf("Tempo criação threads: %.3f ms\n", diff_ms(c_start, c_end));
    printf("Tempo total (inclui join): %.3f ms\n", diff_ms(t_start, t_end));

    // ------------------- SEQUENCIAL ----------------------
    t_start = clock();

    calc_avg(NULL);
    calc_med(NULL);
    calc_std(NULL);

    t_end = clock();

    printf("\n=== Sequencial (1 thread) ===\n");
    printf("Média   = %.3f\n", avg_global);
    printf("Mediana = %.3f\n", med_global);
    printf("Desvio  = %.3f\n", stddev_global);
    printf("Tempo total sequencial: %.3f ms\n", diff_ms(t_start, t_end));

    return 0;
}
