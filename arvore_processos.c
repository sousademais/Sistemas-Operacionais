#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid_F1, pid_F2;

    // Processo P1 (processo inicial)
    printf("P1 (pai) iniciado. PID = %d\n", getpid());

    // 1) Cria o processo F1 (filho de P1)
    pid_F1 = fork();
    if (pid_F1 < 0) {
        perror("Erro ao criar F1");
        exit(1);
    }

    if (pid_F1 == 0) {
        // Está no processo F1

        // Criando N1
        if (fork() == 0) {
            // N1 executa: ls -l
            execlp("ls", "ls", "-l", NULL);
            perror("execlp N1 falhou");
            exit(1);
        }

        // Criando N2
        if (fork() == 0) {
            // N2 executa: date
            execlp("date", "date", NULL);
            perror("execlp N2 falhou");
            exit(1);
        }

        // F1 aguarda o término de N1 e N2
        wait(NULL);
        wait(NULL);

        // Mensagem de F1 após os filhos concluírem
        printf("F1: PID = %d, PPID = %d -> N1 e N2 concluídos.\n",
               getpid(), getppid());

        // F1 termina aqui
        exit(0);
    }

    // 2) Volta ao processo P1 e agora cria o processo F2
    pid_F2 = fork();
    if (pid_F2 < 0) {
        perror("Erro ao criar F2");
        exit(1);
    }

    if (pid_F2 == 0) {
        // Está no processo F2

        // Criando N3
        if (fork() == 0) {
            // N3 executa: whoami
            execlp("whoami", "whoami", NULL);
            perror("execlp N3 falhou");
            exit(1);
        }

        // Criando N4
        if (fork() == 0) {
            // N4 executa: pwd
            execlp("pwd", "pwd", NULL);
            perror("execlp N4 falhou");
            exit(1);
        }

        // F2 aguarda o término de N3 e N4
        wait(NULL);
        wait(NULL);

        // Mensagem de F2 após os filhos concluírem
        printf("F2: PID = %d, PPID = %d -> N3 e N4 concluídos.\n",
               getpid(), getppid());

        // F2 termina aqui
        exit(0);
    }

    // 3) Processo P1 espera F1 e F2 terminarem
    waitpid(pid_F1, NULL, 0);
    waitpid(pid_F2, NULL, 0);

    // Mensagem final de P1
    printf("P1 (pai): PID = %d -> todos os filhos concluídos. Encerrando.\n",
           getpid());

    return 0;
}
