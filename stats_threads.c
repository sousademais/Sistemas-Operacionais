#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define N     10000    // tamanho do vetor
#define RANGE 101      // valores entre 0 e 100 (rand() % RANGE)

// Vetor global de dados
int data[N];

// Variáveis globais onde os resultados serão armazenados
double avg_global, med_global, stddev_global;

// Função auxiliar: converte diferença de clock() para milissegundos
static double diff_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
}

// Thread média
void *calc_avg(void *arg) {
    long long sum = 0;

    // Soma todos os elementos do vetor
    for (int i = 0; i < N; i++)
        sum += data[i];

    // Calcula a média e armazena na variável global
    avg_global = (double) sum / (double) N;

    return NULL;
}

// Função de comparação usada pelo qsort para ordenar inteiros
int cmp_int(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return ia - ib;
}

// Thread mediana 
void *calc_med(void *arg) {
    // Cria uma cópia do vetor, pois será necessário ordená-lo
    int *copy = malloc(N * sizeof(int));
    if (!copy) {
        perror("malloc");
        return NULL;
    }

    for (int i = 0; i < N; i++)
        copy[i] = data[i];

    // Ordena a cópia do vetor
    qsort(copy, N, sizeof(int), cmp_int);

    // Se N é ímpar, a mediana é o elemento do meio;
    // se N é par, a mediana é a média dos dois elementos centrais
    if (N % 2 == 1)
        med_global = copy[N/2];
    else
        med_global = (copy[N/2 - 1] + copy[N/2]) / 2.0;

    free(copy);
    return NULL;
}

// Thread desvio padrão
void *calc_std(void *arg) {
    long long sum = 0;

    // Primeiro calcula a média local (independente de avg_global)
    for (int i = 0; i < N; i++)
        sum += data[i];

    double mean = (double) sum / (double) N;

    // Calcula a variância
    double var = 0.0;
    for (int i = 0; i < N; i++) {
        double d = data[i] - mean;
        var += d * d;
    }

    // Desvio padrão = raiz quadrada da variância
    stddev_global = sqrt(var / N);

    return NULL;
}

//Main 
int main(void) {
    pthread_t t1, t2, t3;          // identificadores das três threads
    clock_t t_start, t_end;        // tempo total
    clock_t c_start, c_end;        // tempo de criação das threads

    // Geração dos dados pela thread principal
    srand(0);
    for (int i = 0; i < N; i++)
        data[i] = rand() % RANGE;  // valores entre 0 e 100

    //1 – Execução em 3 threads

    t_start = clock();   // início do tempo total
    c_start = clock();   // início do tempo de criação

    // Cria as três threads, uma para cada cálculo
    pthread_create(&t1, NULL, calc_avg, NULL);
    pthread_create(&t2, NULL, calc_med, NULL);
    pthread_create(&t3, NULL, calc_std, NULL);

    c_end = clock();     // fim do tempo de criação

    // Aguarda o término de todas as threads
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    t_end = clock();     // fim do tempo total

    // Exibe resultados da versão com 3 threads
    printf("=== 3 Threads ===\n");
    printf("Média   = %.3f\n", avg_global);
    printf("Mediana = %.3f\n", med_global);
    printf("Desvio  = %.3f\n", stddev_global);
    printf("Tempo criação threads: %.3f ms\n", diff_ms(c_start, c_end));
    printf("Tempo total (inclui join): %.3f ms\n", diff_ms(t_start, t_end));

    
    // 2 – Execução sequencial (1 thread)
    
    t_start = clock();

    // Chamadas diretas das funções, sem criar threads
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
