// arquivo: stats_threads.c
// Compilação (no WSL / Linux):
//   gcc -O2 -pthread stats_threads.c -lm -o stats_threads
// Execução:
//   ./stats_threads
//
// Objetivo:
//   - Gerar um vetor de 10.000 inteiros no intervalo [0, 100].
//   - Usar 3 threads para calcular: média, mediana e desvio padrão.
//   - Armazenar os resultados em variáveis globais.
//   - Medir tempo de criação das threads e tempo total de execução.
//   - Refazer os cálculos de forma sequencial (1 thread) e comparar tempos.

#define _POSIX_C_SOURCE 199309L  // habilita CLOCK_MONOTONIC e clock_gettime

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define N     10000   // tamanho do vetor
#define RANGE 101     // valores de 0 a 100 (rand() % RANGE)

// Vetor de dados compartilhado
int data[N];

// Resultados globais (acessados pelas threads)
double avg_global;
double med_global;
double stddev_global;

// Função auxiliar: calcula diferença entre dois tempos em milissegundos
static double diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1e3
         + (end.tv_nsec - start.tv_nsec) / 1e6;
}

// ---------------------------------------------------------------------
// 1) Função da thread que calcula a MÉDIA
// ---------------------------------------------------------------------
void *calc_avg(void *arg) {
    (void)arg;  // evita warning de parâmetro não usado

    long long sum = 0;

    for (int i = 0; i < N; i++) {
        sum += data[i];
    }

    avg_global = (double) sum / (double) N;

    pthread_exit(NULL);
}

// Função de comparação para ordenar inteiros com qsort
int cmp_int(const void *a, const void *b) {
    int ia = *(const int *) a;
    int ib = *(const int *) b;

    if (ia < ib) return -1;
    if (ia > ib) return  1;
    return 0;
}

// ---------------------------------------------------------------------
// 2) Função da thread que calcula a MEDIANA
// ---------------------------------------------------------------------
void *calc_med(void *arg) {
    (void)arg;  // evita warning

    // Fazemos uma cópia do vetor para ordenar
    int *copy = (int *) malloc(sizeof(int) * N);
    if (!copy) {
        perror("malloc");
        pthread_exit(NULL);
    }

    for (int i = 0; i < N; i++) {
        copy[i] = data[i];
    }

    // Ordena a cópia
    qsort(copy, N, sizeof(int), cmp_int);

    if (N % 2 == 1) {
        // N ímpar: mediana é o elemento do meio
        med_global = copy[N / 2];
    } else {
        // N par: média dos dois elementos centrais
        med_global = (copy[N / 2 - 1] + copy[N / 2]) / 2.0;
    }

    free(copy);
    pthread_exit(NULL);
}

// ---------------------------------------------------------------------
// 3) Função da thread que calcula o DESVIO PADRÃO
// ---------------------------------------------------------------------
// Observação: aqui a própria thread calcula a média localmente para
// não depender da ordem de execução das outras threads.
void *calc_std(void *arg) {
    (void)arg;  // evita warning

    // 3.1) Calcula a média local
    long long sum = 0;
    for (int i = 0; i < N; i++) {
        sum += data[i];
    }
    double mean = (double) sum / (double) N;

    // 3.2) Calcula a variância
    double var = 0.0;
    for (int i = 0; i < N; i++) {
        double d = data[i] - mean;
        var += d * d;
    }

    stddev_global = sqrt(var / N);

    pthread_exit(NULL);
}

int main(void) {
    struct timespec t_start, t_end;   // tempo total
    struct timespec c_start, c_end;   // tempo de criação das threads
    pthread_t t1, t2, t3;

    // -----------------------------------------------------------------
    // 0) Geração dos dados pela thread principal
    // -----------------------------------------------------------------
    srand(0);  // semente fixa para resultados reprodutíveis
    for (int i = 0; i < N; i++) {
        data[i] = rand() % RANGE; // valores entre 0 e 100
    }

    // -----------------------------------------------------------------
    // Versão com 3 threads
    // -----------------------------------------------------------------
    clock_gettime(CLOCK_MONOTONIC, &t_start);   // início do tempo total
    clock_gettime(CLOCK_MONOTONIC, &c_start);   // início do tempo de criação

    // Cria uma thread para cada cálculo
    pthread_create(&t1, NULL, calc_avg, NULL);
    pthread_create(&t2, NULL, calc_med, NULL);
    pthread_create(&t3, NULL, calc_std, NULL);

    clock_gettime(CLOCK_MONOTONIC, &c_end);     // fim do tempo de criação

    // Espera todas as threads terminarem
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_end);     // fim do tempo total

    printf("=== 3 Threads ===\n");
    printf("Média   = %.3f\n", avg_global);
    printf("Mediana = %.3f\n", med_global);
    printf("Desvio  = %.3f\n", stddev_global);
    printf("Tempo criação threads: %.3f ms\n", diff_ms(c_start, c_end));
    printf("Tempo total (inclui join): %.3f ms\n", diff_ms(t_start, t_end));

    // -----------------------------------------------------------------
    // Versão sequencial (1 thread) para comparação
    // -----------------------------------------------------------------
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    calc_avg(NULL);
    calc_med(NULL);
    calc_std(NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    printf("\n=== Sequencial (1 thread) ===\n");
    printf("Média   = %.3f\n", avg_global);
    printf("Mediana = %.3f\n", med_global);
    printf("Desvio  = %.3f\n", stddev_global);
    printf("Tempo total sequencial: %.3f ms\n", diff_ms(t_start, t_end));

    return 0;
}
