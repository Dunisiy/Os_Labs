#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <aio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

struct aio_operation {
    struct aiocb aio;
    char *buffer;
    int write_operation;
};

pthread_mutex_t offset_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t active_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t active_cond = PTHREAD_COND_INITIALIZER;

off_t file_size;
off_t next_read_offset = 0;
int read_fd, write_fd;
size_t chunk_size;

int active_ops = 0;

void start_aio_read(struct aio_operation *op);
void start_aio_write(struct aio_operation *op);

void increment_ops() {
    pthread_mutex_lock(&active_mutex);
    active_ops++;
    pthread_mutex_unlock(&active_mutex);
}

void decrement_ops() {
    pthread_mutex_lock(&active_mutex);
    active_ops--;
    if (active_ops == 0 && next_read_offset >= file_size) {
        pthread_cond_signal(&active_cond);
    }
    pthread_mutex_unlock(&active_mutex);
}

void aio_completion_handler(sigval_t sigval) {
    struct aio_operation *op = (struct aio_operation *)sigval.sival_ptr;

    int err = aio_error(&op->aio);
    if (err != 0) {
        fprintf(stderr, "AIO error: %s\n", strerror(err));
        decrement_ops();
        return;
    }

    ssize_t ret = aio_return(&op->aio);

    if (ret <= 0) {
        decrement_ops();
        return;
    }

    if (op->write_operation) {

        pthread_mutex_lock(&offset_mutex);
        if (next_read_offset >= file_size) {
            pthread_mutex_unlock(&offset_mutex);
            decrement_ops();
            return;
        }

        off_t offset = next_read_offset;
        size_t len = (size_t)min((off_t)chunk_size, file_size - offset);
        next_read_offset += len;
        pthread_mutex_unlock(&offset_mutex);

        op->aio.aio_fildes = read_fd;
        op->aio.aio_offset = offset;
        op->aio.aio_nbytes = len;
        op->write_operation = 0;

        increment_ops();
        if (aio_read(&op->aio) == -1) {
            perror("aio_read");
            decrement_ops();
        }

        decrement_ops(); 
    } else {
        op->aio.aio_fildes = write_fd;
        op->write_operation = 1;

        increment_ops();
        if (aio_write(&op->aio) == -1) {
            perror("aio_write");
            decrement_ops();
        }

        decrement_ops(); 
    }
}

void start_aio_read(struct aio_operation *op) {
    pthread_mutex_lock(&offset_mutex);
    if (next_read_offset >= file_size) {
        pthread_mutex_unlock(&offset_mutex);
        return;
    }

    off_t offset = next_read_offset;
    size_t len = (size_t)min((off_t)chunk_size, file_size - offset);
    next_read_offset += len;
    pthread_mutex_unlock(&offset_mutex);

    op->aio.aio_fildes = read_fd;
    op->aio.aio_offset = offset;
    op->aio.aio_nbytes = len;
    op->write_operation = 0;

    increment_ops();
    if (aio_read(&op->aio) == -1) {
        perror("aio_read initial");
        decrement_ops();
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "Usage: %s <src> <dst> [n=8] [block_kb=64]\n", argv[0]);
        return 1;
    }

    char *src = argv[1];
    char *dst = argv[2];

    int n = 8;                
    size_t block_kb = 64;      

    if (argc >= 4) {
        n = atoi(argv[3]);
        if (n <= 0) n = 8;
    }

    if (argc == 5) {
        block_kb = atoi(argv[4]);
        if (block_kb == 0) block_kb = 64;
    }

    chunk_size = block_kb * 1024;

    read_fd = open(src, O_RDONLY);
    if (read_fd < 0) {
        perror("open read");
        return 1;
    }

    write_fd = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (write_fd < 0) {
        perror("open write");
        return 1;
    }

    struct stat st;
    if (fstat(read_fd, &st) == -1) {
        perror("fstat");
        return 1;
    }

    file_size = st.st_size;
    if (file_size == 0) {
        printf("Empty file\n");
        return 0;
    }

    struct aio_operation **ops = (struct aio_operation **)malloc(n * sizeof(*ops));

    for (int i = 0; i < n; i++) {
        ops[i] = (struct aio_operation *)malloc(sizeof(struct aio_operation));
        ops[i]->buffer = (char *)malloc(chunk_size);

        memset(&ops[i]->aio, 0, sizeof(struct aiocb));

        ops[i]->aio.aio_buf = ops[i]->buffer;
        ops[i]->aio.aio_sigevent.sigev_notify = SIGEV_THREAD;
        ops[i]->aio.aio_sigevent.sigev_notify_function = aio_completion_handler;
        ops[i]->aio.aio_sigevent.sigev_value.sival_ptr = ops[i];
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n; i++) {
        start_aio_read(ops[i]);
    }

    pthread_mutex_lock(&active_mutex);
    while (active_ops > 0 || next_read_offset < file_size) {
        pthread_cond_wait(&active_cond, &active_mutex);
    }
    pthread_mutex_unlock(&active_mutex);

    fsync(write_fd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double t = (end.tv_sec - start.tv_sec) +
               (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Done in %.6f sec (n=%d, block=%zu KB)\n", t, n, block_kb);

    close(read_fd);
    close(write_fd);

    for (int i = 0; i < n; i++) {
        free(ops[i]->buffer);
        free(ops[i]);
    }
    free(ops);

    return 0;
}