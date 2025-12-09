// arquivo: stats_processes.c
//
// Objetivo:
//   - Gerar um vetor de 10.000 inteiros no intervalo [0,100].
//   - Calcular média, mediana e desvio padrão usando PROCESSOS.
//   - CASO 1: um único processo filho calcula tudo e envia ao pai.
//   - CASO 2: três processos filhos, cada um calcula um valor.
//   - Medir o tempo de criação dos processos e o tempo total de execução.
//
// Observação:
//   - Usamos clock() (padrão C) ao invés de clock_gettime/CLOCK_MONOTONIC,
//     para evitar problemas de compatibilidade e facilitar a compilação
//     em diferentes ambientes (como WSL + VS Code).

#include <stdio.h>
#include <stdlib.h>
#include <time.h>      // clock(), CLOCKS_PER_SEC
#include <math.h>      // sqrt()
#include <unistd.h>    // fork(), pipe(), read(), write(), close()
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // waitpid()

#define N     10000    // tamanho do vetor
#define RANGE 101      // valores entre 0 e 100

// Vetor global de dados. Após o fork(), cada processo terá
// sua própria cópia desse vetor na memória.
int data[N];

// Struct usada no caso 1 para enviar os três resultados de uma vez
typedef struct {
    double avg;
    double med;
    double stddev;
} stats_t;

// Função auxiliar: converte diferença de clock() para milissegundos
static double diff_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
}

// Função de comparação usada pelo qsort para ordenar inteiros
int cmp_int(const void *a, const void *b) {
    int ia = *(const int *) a;
    int ib = *(const int *) b;

    if (ia < ib) return -1;
    if (ia > ib) return  1;
    return 0;
}

// ------------------------ Cálculo da MÉDIA ------------------------
static double calc_avg_proc(void) {
    long long sum = 0;

    // Soma todos os elementos do vetor
    for (int i = 0; i < N; i++)
        sum += data[i];

    // Retorna a média como double
    return (double) sum / (double) N;
}

// ---------------------- Cálculo da MEDIANA ------------------------
static double calc_med_proc(void) {
    // Cria uma cópia do vetor, pois vamos ordenar
    int *copy = (int *) malloc(N * sizeof(int));
    if (!copy) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N; i++)
        copy[i] = data[i];

    // Ordena o vetor auxiliar
    qsort(copy, N, sizeof(int), cmp_int);

    double med;
    if (N % 2 == 1) {
        // N ímpar → elemento central
        med = copy[N / 2];
    } else {
        // N par → média dos dois centrais
        med = (copy[N / 2 - 1] + copy[N / 2]) / 2.0;
    }

    free(copy);
    return med;
}

// ------------------ Cálculo do DESVIO PADRÃO ----------------------
static double calc_std_proc(void) {
    // Primeiro calcula a média
    double mean = calc_avg_proc();
    double var  = 0.0;

    // Depois calcula a variância
    for (int i = 0; i < N; i++) {
        double d = data[i] - mean;
        var += d * d;
    }

    // Desvio padrão = raiz da variância
    return sqrt(var / N);
}

// =================================================================
//                        FUNÇÃO PRINCIPAL
// =================================================================
int main(void) {
    clock_t t_start, t_end;   // tempo total
    clock_t c_start, c_end;   // tempo de criação dos processos

    // -------------------------------------------------------------
    // Geração dos dados pelo processo PAI
    // -------------------------------------------------------------
    srand(0);  // semente fixa para reprodutibilidade
    for (int i = 0; i < N; i++)
        data[i] = rand() % RANGE;   // valores entre 0 e 100

    // =============================================================
    //                CASO 1 – UM PROCESSO FILHO ÚNICO
    // =============================================================
    int fd_single[2];   // pipe para comunicação pai <-> filho único

    if (pipe(fd_single) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    // Marca início do tempo total e de criação
    t_start = clock();
    c_start = clock();

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // ------------------- FILHO ÚNICO --------------------
        // Fecha o lado de leitura da pipe (vai apenas escrever)
        close(fd_single[0]);

        // Calcula as três estatísticas
        stats_t s;
        s.avg    = calc_avg_proc();
        s.med    = calc_med_proc();
        s.stddev = calc_std_proc();

        // Escreve a struct na pipe para o pai
        if (write(fd_single[1], &s, sizeof(s)) != sizeof(s)) {
            perror("write filho único");
        }

        close(fd_single[1]); // fecha a escrita
        _exit(0);            // termina o processo filho
    }

    // ------------------------ PAI (CASO 1) ------------------------
    // Tempo de criação do processo (até o retorno do fork no pai)
    c_end = clock();

    // Pai não irá escrever nessa pipe, apenas ler
    close(fd_single[1]);

    // Struct onde o pai irá receber os resultados do filho
    stats_t s_single;
    if (read(fd_single[0], &s_single, sizeof(s_single)) != sizeof(s_single)) {
        perror("read pai (single)");
    }

    close(fd_single[0]);

    // Aguarda o término do filho
    waitpid(pid, NULL, 0);

    // Tempo total do caso 1
    t_end = clock();

    printf("=== 1 Processo (filho único) ===\n");
    printf("Média   = %.3f\n", s_single.avg);
    printf("Mediana = %.3f\n", s_single.med);
    printf("Desvio  = %.3f\n", s_single.stddev);
    printf("Tempo criação processo: %.3f ms\n", diff_ms(c_start, c_end));
    printf("Tempo total (inclui wait): %.3f ms\n\n", diff_ms(t_start, t_end));

    // =============================================================
    //                CASO 2 – TRÊS PROCESSOS FILHOS
    // =============================================================
    int fds[3][2];   // cada filho terá uma pipe própria
    pid_t pids[3];

    // Cria as 3 pipes
    for (int i = 0; i < 3; i++) {
        if (pipe(fds[i]) == -1) {
            perror("pipe");
            return EXIT_FAILURE;
        }
    }

    // Marca início do tempo total e de criação
    t_start = clock();
    c_start = clock();

    // Cria 3 processos filhos
    for (int i = 0; i < 3; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return EXIT_FAILURE;
        }

        if (pids[i] == 0) {
            // ---------------- FILHO i ----------------
            // Fecha o lado de leitura: vai apenas escrever
            close(fds[i][0]);

            double result;

            // Decide o cálculo de acordo com o índice i
            if (i == 0) {
                result = calc_avg_proc();   // filho 0 -> média
            } else if (i == 1) {
                result = calc_med_proc();   // filho 1 -> mediana
            } else {
                result = calc_std_proc();   // filho 2 -> desvio padrão
            }

            // Escreve o resultado na pipe correspondente
            if (write(fds[i][1], &result, sizeof(result)) != sizeof(result)) {
                perror("write filho[i]");
            }

            close(fds[i][1]);
            _exit(0);   // termina o processo filho
        } else {
            // ---------------- PAI (enquanto cria) ---------------
            // Fecha o lado de escrita para esse filho:
            // o pai só vai ler o resultado depois.
            close(fds[i][1]);
        }
    }

    // Tempo de criação de todos os processos filhos
    c_end = clock();

    double avg = 0.0, med = 0.0, stddev = 0.0;

    // Pai lê um double de cada pipe e espera cada filho
    for (int i = 0; i < 3; i++) {
        double value;

        // Lê o valor enviado pelo filho i
        if (read(fds[i][0], &value, sizeof(value)) != sizeof(value)) {
            perror("read pai[i]");
        }

        close(fds[i][0]);

        // Aguarda término do processo filho i
        waitpid(pids[i], NULL, 0);

        // Associa o resultado ao campo correto
        if      (i == 0) avg    = value;
        else if (i == 1) med    = value;
        else             stddev = value;
    }

    // Marca o fim do tempo total do caso 2
    t_end = clock();

    printf("=== 3 Processos ===\n");
    printf("Média   = %.3f\n", avg);
    printf("Mediana = %.3f\n", med);
    printf("Desvio  = %.3f\n", stddev);
    printf("Tempo criação processos: %.3f ms\n", diff_ms(c_start, c_end));
    printf("Tempo total (inclui wait): %.3f ms\n", diff_ms(t_start, t_end));

    return 0;
}
