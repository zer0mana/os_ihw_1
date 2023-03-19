#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 5000

int logic(char *buffer, int n) {

    // Основная логика.
    // Чтобы получить последнюю последовательность
    // иду с конца, но тогда она запишется в обратном порядке,
    // поэтому в конце ее разворачиваю.
    
    char reverse_answer[n];
    int answer_index = 0;
    for (int i = strlen(buffer); i > -1; i--) {
        if (answer_index == n) {
            break;
        }
        if (answer_index == 0) {
            reverse_answer[answer_index] = buffer[i];
            answer_index++;
        } else {
            if (reverse_answer[answer_index - 1] > buffer[i]) {
                reverse_answer[answer_index] = buffer[i];
                answer_index++;
            } else {
                reverse_answer[0] = buffer[i];
                answer_index = 1;
            }
        }
    }

    // Если последовательность есть, то
    // разворачиваю ее и записываю в буффер,
    
    if (answer_index == n) {
        char answer[n];
        for (int i = 0; i < n; i++) {
            answer[i] = reverse_answer[n - i - 1];
        }
        strcpy(buffer, answer);
        
        // иначе буффер пустой.
        
    } else {
        strcpy(buffer, "");
    }
}

int main(int argc, char* argv[]) {

    int pipe_rdr[2];
    int pipe_wrtr[2];
    int process_rdr;
    int process_wrtr;
    int status;

    size_t size;

    char buffer[BUFFER_SIZE];

    if (argc < 4) {
        printf("Not enough arguments\n");
        exit(1);
    }

    if (argc > 4) {
        printf("Too much arguments\n");
        exit(1);
    }

    char *input_file = argv[1];
    char *output_file = argv[2];
    int n = atoi(argv[3]);

    if (pipe(pipe_rdr) < 0 || pipe(pipe_wrtr) < 0) {
        printf("Can\'t create pipe\n");
        exit(-1);
    }

    // Создаем процесс-читатель.
    process_rdr = fork();

    if (process_rdr < 0) {

        printf("Can\'t fork child\n");
        exit(-1);

    } else if (process_rdr == 0) { // Сюда попадает процесс-читатель.

        // Закрываем трубу на чтение.
        close(pipe_rdr[0]);

        // Записываем файл в буффер.
        int file = open(input_file, O_RDONLY);
        ssize_t read_bytes = read(file, buffer, BUFFER_SIZE);
        close(file);

        if (read_bytes == -1) {
            printf("Can\'t read file\n");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        size = write(pipe_rdr[1], buffer, strlen(buffer) + 1);
        if (size < 0) {
            printf("Can\'t write all string to pipe\n");
            exit(-1);
        }

        // Процесс-читатель вышел.
        printf("Reader exit\n");

    } else { // Сюда попадает main поток.

        // Создаем новый процесс-писатель.
        process_wrtr = fork();
        if (process_wrtr < 0) {
            printf("Can\'t fork child\n");
            exit(-1);

        } else if (process_wrtr == 0) { // Сюда попадает процесс-писатель.

            // Закрываем трубу на запись.
            close(pipe_wrtr[1]);

            size = read(pipe_wrtr[0], buffer, BUFFER_SIZE);
            if (size < 0) {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            // Записываем из буффера в файл.
            int file = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            ssize_t size = write(file, buffer, strlen(buffer));
            close(file);

            if (size == -1) {
                printf("Can\'t write in file\n");
                exit(1);
            }

            // Процесс-писатель вышел.
            printf("Writer exit\n");
        } else {

            // Сюда попадает main процесс.
            close(pipe_rdr[1]);
            close(pipe_wrtr[0]);

            size = read(pipe_rdr[0], buffer, BUFFER_SIZE);
            if (size < 0) {
                printf("Can\'t read string from pipe\n");
                exit(-1);
            }

            logic(buffer, n);
            size = write(pipe_wrtr[1], buffer, strlen(buffer) + 1);
            if (size < 0) {
                printf("Can\'t write all string to pipe\n");
                exit(-1);
            }

            // main процесс вышел.
            printf("Parent exit\n");
        }
    }

    // Ожидаем завершения дочерних процессов.
    waitpid(process_rdr, &status, 0);
    waitpid(process_wrtr, &status, 0);

    exit(0);
};


