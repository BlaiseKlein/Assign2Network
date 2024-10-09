//
// Created by blaise-klein on 9/30/24.
//
#include "text_modify_server.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>

ssize_t send_modified_text(struct thread_arguments *arguments)
{
    ssize_t bytes_written = 0;
    int     fd            = open(arguments->fifo_out, O_WRONLY | O_CLOEXEC | O_TRUNC);

    if(fd == -1)
    {
        free(arguments->fifo_out);
        free(arguments->msg);
        free(arguments);
        perror("Failed to open /tmp/response_fifo");
        exit(1);
    }

    bytes_written = write(fd, arguments->msg, arguments->msg_length);
    close(fd);
    if(bytes_written == -1)
    {
        free(arguments->fifo_out);
        free(arguments->msg);
        free(arguments);
        perror("Failed to write to /tmp/response_fifo");
        exit(1);
    }

    return bytes_written;
}

static void *handle_fifo_connection(void *args)
{
    struct thread_arguments *arguments = (struct thread_arguments *)args;
    // printf("%c", arguments->msg[0]);

    for(size_t i = 0; i < arguments->msg_length; i++)
    {
        arguments->msg[i] = char_mods[arguments->option](arguments->msg[i]);
    }

    send_modified_text(arguments);
    free(arguments->msg);
    free(arguments->fifo_out);
    free(arguments);
    arguments = NULL;
    return NULL;
}

enum Text_Modifications option_character(char c)
{
    if(c == 'l')
    {
        return lower;
    }
    if(c == 'u')
    {
        return upper;
    }
    if(c == 'n')
    {
        return none;
    }
    exit(EXIT_FAILURE);
}

noreturn void await_fifo_connection(const char *filename, const char *filename_out, const size_t file_name_length)
{
    ssize_t    bytes_read;
    int        fd;
    pthread_t *thread_ids;
    size_t     msg_size;
    char      *msg_buffer;

    const size_t             out_size             = 19;
    const size_t             initial_thread_count = 8;
    size_t                   buffer               = 0;
    int                      thread_id            = 0;
    struct thread_arguments *thread_args          = NULL;

    fd         = open(filename, O_RDONLY | O_CLOEXEC);
    thread_ids = (pthread_t *)calloc(initial_thread_count, sizeof(pthread_t));
    if(thread_ids == NULL)
    {
        perror("Failed to allocate thread array");
        exit(1);
    }

    if(fd == -1)
    {
        perror("Failed to open /tmp/write_fifo");
        exit(1);
    }
    while(true)
    {
        ssize_t first_read = 0;
        first_read         = read(fd, &buffer, sizeof(size_t));
        // printf("here\n");
        if(first_read == -1)
        {
            perror("Failed to read from /tmp/write_fifo");
            exit(1);
        }
        // ERR check here

        if(first_read == sizeof(size_t))
        {
            msg_size   = buffer;
            msg_buffer = (char *)malloc(msg_size * sizeof(char) + 2);
            // printf("here");
            if(msg_buffer == NULL)
            {
                perror("Failed to allocate buffer");
                exit(1);
            }
            // Err check here
            // printf("here");
            bytes_read = read(fd, msg_buffer, sizeof(char) * msg_size + 2);
            if(bytes_read == -1)
            {
                perror("Failed to read from /tmp/write_fifo");
                free(msg_buffer);
                exit(1);
            }
            // printf("%s", msg_buffer);
            // Err check here
            thread_args = (struct thread_arguments *)malloc(sizeof(struct thread_arguments));
            if(thread_args == NULL)
            {
                perror("Failed to allocate thread arguments");
                free(msg_buffer);
                exit(1);
            }
            thread_args->msg = (char *)malloc(msg_size * sizeof(char));
            if(thread_args->msg == NULL)
            {
                perror("Failed to allocate thread arguments");
                free(msg_buffer);
                free(thread_args);
                exit(1);
            }
            strlcpy(thread_args->msg, msg_buffer + 1, msg_size + 1);
            thread_args->msg_length = msg_size;
            thread_args->option     = option_character(msg_buffer[0]);
            thread_args->fifo_out   = (char *)malloc(sizeof(char) * file_name_length + 1);
            if(thread_args->fifo_out == NULL)
            {
                perror("Failed to allocate thread arguments");
                free(msg_buffer);
                msg_buffer = NULL;
                free(thread_args->msg);
                free(thread_args);
                thread_args = NULL;
                exit(1);
            }
            strlcpy(thread_args->fifo_out, filename_out, out_size);

            if(bytes_read == (ssize_t)buffer + 1)
            {
                int thread_success = 0;
                // printf("creating thread\n");
                thread_success = pthread_create(thread_ids + thread_id, NULL, handle_fifo_connection, thread_args);
                // printf("thread created\n");
                if(thread_success < 0)
                {
                    // HANDLE ERROR PROPERLY
                    perror("Failed to create thread");
                    free(msg_buffer);
                    msg_buffer = NULL;
                    free(thread_args->msg);
                    free(thread_args->fifo_out);
                    free(thread_args);
                    thread_args = NULL;
                    exit(1);
                }
                // thread_id++; //Assume no concurrency, repeat first thread
                free(msg_buffer);
                msg_buffer = NULL;
                // free(thread_args->msg);
                // free(thread_args->fifo_out);
                // free(thread_args);
                // thread_args = NULL;
            }
            else
            {
                // printf("but why, %zd, %zd", first_read, bytes_read);
                free(msg_buffer);
                msg_buffer = NULL;
                free(thread_args->msg);
                free(thread_args->fifo_out);
                free(thread_args);
                thread_args = NULL;
                exit(1);
            }
        }
    }
    // close(fd);
}

// void *basic_thread_func(void *c)
// {
//     printf("First character: %s", (char *)c);
//
//     return NULL;
// }
//
// void basic_fifo_connection(const char *filename)
// {
//     int       fd = open(filename, O_RDONLY | O_CLOEXEC);
//     uint8_t   size;
//     pthread_t thread_id;
//
//     if(fd == -1)
//     {
//         exit(1);
//     }
//
//     // Read and print words from the client
//     while(read(fd, &size, sizeof(uint8_t)) > 0)
//     {
//         int  result = 0;
//         char word[UINT8_MAX + 1];
//
//         read(fd, word, size);
//         word[size] = '\0';
//         // printf("Word Size: %u, Word: %s\n", size, word);
//         result = pthread_create(&thread_id, NULL, basic_thread_func, word);
//         if(result == 0)
//         {
//             exit(1);
//         }
//     }
//
//     close(fd);
// }

void handle_signal(int signal)
{
    if(signal == SIGINT)
    {
        int fd = open("/tmp/response_fifo", O_RDWR | O_CLOEXEC);
        if(fd != -1)
        {
            const char *msg = "Server closed";
            write(fd, msg, strlen(msg));
            close(fd);
        }
        abort();
    }
}

int main(void)
{
    int          response_test_fd;
    const char  *fifo_in          = "/tmp/write_fifo";
    const char  *fifo_out         = "/tmp/response_fifo";
    const size_t file_name_length = 18;

    perror("starting");
    if(signal(SIGINT, handle_signal) == SIG_ERR)
    {
        perror("Error setting up signal handler");
        return 1;
    }

    response_test_fd = open(fifo_out, O_WRONLY | O_CLOEXEC);
    if(response_test_fd == -1)
    {
        perror("Failed to open /tmp/response_fifo");
        exit(1);
    }
    close(response_test_fd);
    perror("starting");

    // printf("starting server\n");
    await_fifo_connection(fifo_in, fifo_out, file_name_length);
}
