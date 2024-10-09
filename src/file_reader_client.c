//
// Created by blaise-klein on 9/30/24.
//

#include "file_reader_client.h"
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void cleanup_context(struct context *ctx)
{
    // if(ctx->arguments->input_file != NULL)
    // {
    //     free(ctx->arguments->input_file);
    //     ctx->arguments->input_file = NULL;
    // }
    // if(ctx->arguments->output_file != NULL)
    // {
    //     free(ctx->arguments->output_file);
    //     ctx->arguments->output_file = NULL;
    // }
    if(ctx != NULL)
    {
        if(ctx->fifo_in != NULL)
        {
            free(ctx->fifo_in);
            ctx->fifo_in = NULL;
        }
        if(ctx->fifo_out != NULL)
        {
            free(ctx->fifo_out);
            ctx->fifo_out = NULL;
        }
    }
    if(ctx != NULL && ctx->arguments != NULL)
    {
        // cleanup_string(ctx->arguments->input);
        // cleanup_string(ctx->arguments->output);
        if(ctx->arguments->output != NULL)
        {
            free(ctx->arguments->output);
            ctx->arguments->output = NULL;
        }
        free(ctx->arguments);
        ctx->arguments = NULL;
    }
    free(ctx);
}

void cleanup_file(int fd)
{
    close(fd);
}

noreturn void usage(const char *err)
{
    fprintf(stderr, "%s\nUsage: main.c -i input_filename -o output_filename -f [lower/upper/none] \n", err);
    fprintf(stderr,
            "-i input_file: Enter the filename to read from\n"
            "-o output_filename: Enter the file to write to. Will create it if it does not exist\n"
            "-f [lower/upper/none]: Enter the character modification option.\n"
            "\tlower: Makes all letters lowercase\n"
            "\tupper: Makes all letters uppercase\n"
            "\tnone: Copies characters unchanged\n");
    exit(EXIT_FAILURE);
}

void parse_arguments(int argc, char **argv, struct context *ctx)
{
    int       opt;
    const int valid_argument_count = 5;
    const int help_argument_count  = 2;

    // struct context* ctx = malloc(sizeof(struct context));
    if(ctx == NULL)
    {
        usage("Called to get context");
    }
    if(argc != valid_argument_count && argc != help_argument_count)
    {
        ctx->arguments = NULL;
        cleanup_context(ctx);
        usage("Wrong number of arguments");
    }

    ctx->arguments = (struct arguments *)malloc(sizeof(struct arguments));
    if(ctx->arguments == NULL)
    {
        usage("Could not allocate memory for arguments");
    }
    ctx->arguments->input_length        = 0;
    ctx->arguments->output              = NULL;
    ctx->arguments->modification_option = invalid;    // Used to check if the value is set properly later

    opterr = 0;

    while((opt = getopt(argc, argv, "hi:f:")) != -1)
    {
        switch(opt)
        {
            case 'h':
            {
                usage("Printing usage instructions");
            }
            case 'i':
            {
                // char *input_filename = optarg;
                // ctx->arguments->input_file = (char *)malloc(strlen(input_filename) + 1);
                // ctx->arguments->input_file = strdup(input_filename);
                ctx->arguments->input        = optarg;
                ctx->arguments->input_length = strlen(optarg);
                if(ctx->arguments->input == NULL)
                {
                    // free(ctx->arguments->output);
                    cleanup_context(ctx);
                    usage("Input string evaluation failed");
                }
                // ctx->arguments->output = (char *)malloc(sizeof(char) * ctx->arguments->input_length + 1);
                // free(input_filename);
                break;
            }
            case 'f':
            {
                const char *modification_option = optarg;
                if(strcmp(modification_option, "lower") == 0)
                {
                    ctx->arguments->modification_option = lower;
                    break;
                }
                if(strcmp(modification_option, "upper") == 0)
                {
                    ctx->arguments->modification_option = upper;
                    break;
                }
                if(strcmp(modification_option, "none") == 0)
                {
                    ctx->arguments->modification_option = none;
                    break;
                }
                cleanup_context(ctx);
                usage("Invalid mod option");
            }
            case '?':
            {
                if(optopt == 'i')
                {
                    // cleanup_string(ctx->arguments->output);
                    cleanup_context(ctx);
                    usage("No input string");
                }
                break;
            }
            default:
            {
                cleanup_context(ctx);
                usage("Invalid option");
            }
        }
    }
    if(ctx->arguments->input_length == 0)
    {
        cleanup_context(ctx);
        usage("No input string");
    }
    if(ctx->arguments->modification_option == invalid)
    {
        cleanup_context(ctx);
        usage("No modification option added");
    }
}

size_t write_to_server(struct context *ctx)
{
    // ssize_t bytes_written;
    size_t  nwrote;
    ssize_t int_wrote;
    ssize_t option_written;
    int     fd = open(ctx->fifo_in, O_WRONLY | O_CLOEXEC);

    if(fd == -1)
    {
        usage("Could not open file");
    }
    // printf("%zu\n", ctx->arguments->input_length);
    int_wrote = write(fd, &ctx->arguments->input_length, sizeof(size_t));
    if(int_wrote == -1)
    {
        close(fd);
        cleanup_context(ctx);
        exit(EXIT_FAILURE);
        // usage("Could not write to file");
    }

    option_written = write(fd, &ctx->arguments->option_char, sizeof(char));
    if(option_written == -1)
    {
        // cpp-check surpress doubleFree
        close(fd);
        cleanup_context(ctx);
        usage("Could not write to file");
    }

    nwrote = 0;
    do
    {
        ssize_t temp_wrote;
        size_t  remaining;

        remaining  = (ctx->arguments->input_length - nwrote);
        temp_wrote = write(fd, &ctx->arguments->input[nwrote], remaining);

        if(temp_wrote < 0)
        {
            cleanup_file(fd);
            cleanup_context(ctx);
            usage("Could not write to file");
        }

        nwrote += (size_t)temp_wrote;
    } while(nwrote != ctx->arguments->input_length);

    // if(bytes_written == -1)
    // {
    //     cleanup_file(fd);
    //     usage("Could not write to file");
    // }

    close(fd);

    // printf("Wrote %zu bytes\n", nwrote);
    return nwrote;
}

ssize_t await_server_response(struct context *ctx)
{
    char   *printer;
    ssize_t bytes_read = 0;
    ssize_t remaining  = (ssize_t)ctx->arguments->input_length;
    int     fd         = open(ctx->fifo_out, O_RDONLY | O_CLOEXEC);

    // printf("Awaiting server response...\n");

    if(fd == -1)
    {
        // free(output);
        cleanup_context(ctx);
        usage("Could not open file");
    }

    // temp_out = (char*) malloc(sizeof(char) * output_length);

    do
    {
        bytes_read += read(fd, &ctx->arguments->output[bytes_read], ctx->arguments->input_length);
        if(bytes_read == -1)
        {
            // free(output);
            cleanup_file(fd);
            cleanup_context(ctx);
            usage("Could not read from file");
        }
        remaining -= bytes_read;
    } while(remaining != 0);

    close(fd);
    printer = (char *)malloc(sizeof(char) * ctx->arguments->input_length + sizeof(char));
    if(printer == NULL)
    {
        cleanup_context(ctx);
        usage("Could not allocate memory for printer");
    }
    strlcpy(printer, &ctx->arguments->output[0], ctx->arguments->input_length + 1);
    write(STDOUT_FILENO, printer, ctx->arguments->input_length);
    printf("\n");
    free(printer);

    return bytes_read;
}

void get_modification_char(struct context *ctx)
{
    if(ctx == NULL)
    {
        exit(1);
    }
    if(ctx->arguments->modification_option == lower)
    {
        ctx->arguments->option_char = 'l';
        return;
    }
    if(ctx->arguments->modification_option == upper)
    {
        ctx->arguments->option_char = 'u';
        return;
    }
    if(ctx->arguments->modification_option == none)
    {
        ctx->arguments->option_char = 'n';
        return;
    }
    if(ctx->arguments->modification_option == invalid)
    {
        cleanup_context(ctx);
        usage("Invalid mod option");
    }
    cleanup_context(ctx);
    exit(1);
}

void modify_text_input(struct context *ctx)
{
    // char option;
    // printf("writing to server\n");

    get_modification_char(ctx);
    write_to_server(ctx);

    ctx->arguments->output = (char *)malloc(sizeof(char) * ctx->arguments->input_length + 1);
    if(ctx->arguments->output == NULL)
    {
        cleanup_context(ctx);
        usage("Could not allocate memory");
    }

    // printf("we good\n");

    await_server_response(ctx);

    // write(1, ctx->arguments->output, ctx->arguments->input_length);

    free(ctx->arguments->output);
    ctx->arguments->output = NULL;
}

void test_fifo(const char *filename, const char *msg, size_t msg_size)
{
    ssize_t bytes_written;
    int     fd = open(filename, O_WRONLY | O_TRUNC | O_CLOEXEC);

    if(fd == -1)
    {
        exit(1);
    }

    bytes_written = write(fd, msg, msg_size);
    if(bytes_written == -1)
    {
        cleanup_file(fd);
        usage("Could not write to fifo");
    }

    // fprintf(stdout, "bytes_written: %ld\n", bytes_written);
    close(fd);
}

int main(int argc, char **argv)
{
    const size_t    in_size  = 16;
    const size_t    out_size = 19;
    struct context *ctx      = (struct context *)malloc(sizeof(struct context));

    // printf("starting client...\n");

    if(ctx == NULL)
    {
        usage("Could not allocate memory for context");
    }
    ctx->arguments = NULL;
    ctx->fifo_in   = (char *)malloc(sizeof(char) * FIFONAMEMAXLENGTH);
    if(ctx->fifo_in == NULL)
    {
        ctx->fifo_out = NULL;
        cleanup_context(ctx);
        usage("Could not allocate memory for writing fifo name");
    }
    ctx->fifo_out = (char *)malloc(sizeof(char) * FIFONAMEMAXLENGTH);
    if(ctx->fifo_out == NULL)
    {
        cleanup_context(ctx);
        usage("Could not allocate memory for receiving fifo name");
    }

    strlcpy(ctx->fifo_in, "/tmp/write_fifo", in_size);
    strlcpy(ctx->fifo_out, "/tmp/response_fifo", out_size);
    parse_arguments(argc, argv, ctx);
    // test_fifo(ctx->fifo_in, ctx->arguments->input, ctx->arguments->input_length);
    modify_text_input(ctx);

    cleanup_context(ctx);
    // test_fifo("/tmp/myfifo", msg, msg_len);
}
