#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define MAX_ROWS 10
#define MAX_LEN 64
#define K 1 // Кол-во дочерних процессов

// Структура таблицы
typedef struct {
    char rows[MAX_ROWS][MAX_LEN];
} SharedTable;

// Инициализация таблицы
void init_table(SharedTable *table) {
    for (int i = 0; i < MAX_ROWS; i++) {
        snprintf(table->rows[i], MAX_LEN, "row_%d_data", i);
    }
}

// Печать таблицы
void print_table(SharedTable *table) {
    printf("Текущая таблица:\n");
    for (int i = 0; i < MAX_ROWS; i++) {
        if (table->rows[i][0] != '\0') {
            printf("%d: %s\n", i, table->rows[i]);
        }
    }
    printf("\n");
}

// Операции над семафором
void sem_wait(int semid) {
    struct sembuf op = {0, -1, 0}; // P-операция
    semop(semid, &op, 1);
}

void sem_signal(int semid) {
    struct sembuf op = {0, 1, 0}; // V-операция
    semop(semid, &op, 1);
}

int main() {
    // Shared Memory Key
    key_t shm_key = ftok("progfile", 65);
    int shmid = shmget(shm_key, sizeof(SharedTable), 0666 | IPC_CREAT);
    if (shmid == -1) { perror("shmget"); exit(1); }

    SharedTable *table = (SharedTable *)shmat(shmid, NULL, 0);
    if (table == (void *)-1) { perror("shmat"); exit(1); }

    init_table(table);

    printf("Изначальная таблица:\n");
    print_table(table);

    // Semaphore init
    key_t sem_key = ftok("progfile", 75);
    int semid = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (semid == -1) { perror("semget"); exit(1); }

    // Устанавливаем значение семафора в 1 (разрешён вход одному)
    semctl(semid, 0, SETVAL, 1);

    // Создание дочерних процессов
    for (int i = 0; i < K; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            SharedTable *child_table = (SharedTable *)shmat(shmid, NULL, 0);
            if (child_table == (void *)-1) { perror("shmat (child)"); exit(1); }

            sem_wait(semid); // 🔒 Блокировка доступа к таблице

            int row_index = i;
            if (row_index >= 0 && row_index < MAX_ROWS && child_table->rows[row_index][0] != '\0') {
                printf("[Child %d] Удаляю строку %d: %s\n", getpid(), row_index, child_table->rows[row_index]);
                child_table->rows[row_index][0] = '\0';
            } else {
                printf("[Child %d] Ошибка: строка недоступна.\n", getpid());
            }

            sem_signal(semid); // 🔓 Разблокировка доступа

            shmdt(child_table);
            exit(0);
        }
    }

    for (int i = 0; i < K; i++) wait(NULL);

    printf("\nТаблица после удаления:\n");
    print_table(table);

    // Очистка ресурсов
    shmdt(table);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID); // Удаление семафора

    return 0;
}