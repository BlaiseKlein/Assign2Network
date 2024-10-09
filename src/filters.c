//
// Created by blaise-klein on 9/14/24.
//

#include "filters.h"
#include "character_modifications.h"
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const operation_func char_mods[] = {text_to_lowercase, text_to_uppercase, text_unmodified};

// struct arguments
// {
//     int         argc;
//     const char *program_name;
//     char       *input_file;
//     char       *output_file;
//     int         modification_option;
//     char      **argv;
// };
//
// struct settings
// {
//     unsigned int count;
//     const char  *message;
// };
//
// struct context
// {
//     struct arguments *arguments;
//     struct settings   settings;
//     int               exit_code;
//     char             *exit_message;
// };

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
    if(ctx != NULL && ctx->arguments != NULL)
    {
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
    fprintf(stderr, "%s\nUsage: main.c -i input_filename -o output_filename -f [lower/upper/null] \n", err);
    fprintf(stderr,
            "-i input_file: Enter the filename to read from\n"
            "-o output_filename: Enter the file to write to. Will create it if it does not exist\n"
            "-f [lower/upper/null]: Enter the character modification option.\n"
            "\tlower: Makes all letters lowercase\n"
            "\tupper: Makes all letters uppercase\n"
            "\tnone: Copies characters unchanged\n");
    exit(EXIT_FAILURE);
}

void parse_arguments(int argc, char **argv, struct context *ctx)
{
    int       opt;
    const int valid_argument_count = 7;
    const int help_argument_count  = 2;

    // struct context* ctx = malloc(sizeof(struct context));
    if(ctx == NULL)
    {
        usage("Called to get context");
    }
    if(argc != valid_argument_count && argc != help_argument_count)
    {
        cleanup_context(ctx);
        usage("Wrong number of arguments");
    }

    ctx->arguments = (struct arguments *)malloc(sizeof(struct arguments));
    if(ctx->arguments == NULL)
    {
        usage("Could not allocate memory for arguments");
    }
    ctx->arguments->modification_option = invalid;    // Used to check if the value is set properly later

    opterr = 0;

    while((opt = getopt(argc, argv, "hi:o:f:")) != -1)
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
                ctx->arguments->input_file = optarg;
                if(ctx->arguments->input_file == NULL)
                {
                    cleanup_context(ctx);
                    usage("Input file evaluation failed");
                }
                // free(input_filename);
                break;
            }
            case 'o':
            {
                // char *output_filename = optarg;
                // ctx->arguments->output_file = (char *)malloc(strlen(output_filename) + 1);
                // ctx->arguments->output_file = strdup(output_filename);
                ctx->arguments->output_file = optarg;
                if(ctx->arguments->output_file == NULL)
                {
                    cleanup_context(ctx);
                    usage("Output file evaluation failed");
                }
                // free(output_filename);
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
                if(strcmp(modification_option, "null") == 0)
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
                    cleanup_context(ctx);
                    usage("No input file");
                }
                else if(optopt == 'o')
                {
                    cleanup_context(ctx);
                    usage("No output file");
                    // usage(argv[0], EXIT_FAILURE, "Option '-o' requires a filename.");
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
    if(ctx->arguments->modification_option == invalid)
    {
        cleanup_context(ctx);
        usage("No modification option added");
    }
}

ssize_t read_input_file(int fd, char *c)
{
    ssize_t len = read(fd, c, sizeof(char));
    return len;
}

ssize_t write_output_file(int fd, char write_character)
{
    ssize_t written = write(fd, &write_character, sizeof(write_character));
    if(written == -1)
    {
        return -1;
    }
    return 0;
}

void copy_file_data(struct context *context)
{
    int input_fd  = 0;
    int output_fd = 0;

    if(context == NULL)
    {
        usage("Failed to get context");
    }

    input_fd = open(context->arguments->input_file, O_RDONLY | O_CLOEXEC);
    if(input_fd == -1)
    {
        cleanup_context(context);
        usage("Cannot open input file");
    }

    output_fd = open(context->arguments->output_file, O_WRONLY | O_TRUNC | O_CLOEXEC | O_CREAT, S_IRUSR | S_IWUSR);
    if(output_fd == -1)
    {
        cleanup_context(context);
        cleanup_file(input_fd);
        usage("Cannot open output file");
    }

    while(true)
    {
        ssize_t valid_read  = 0;
        ssize_t valid_write = 0;
        char    c           = 'a';
        valid_read          = read_input_file(input_fd, &c);

        if(valid_read == -1 || valid_read == 0)
        {
            break;
        }

        c = char_mods[context->arguments->modification_option](c);
        // c = text_to_lowercase(c);

        valid_write = write_output_file(output_fd, c);
        if(valid_write == -1)
        {
            cleanup_context(context);
            cleanup_file(input_fd);
            cleanup_file(output_fd);
            usage("Failed to write");
        }
    }
    cleanup_file(input_fd);
    cleanup_file(output_fd);
}

// int main(){
//     char *input_filename = "../input.txt";
//     char *output_filename = "../output.txt";
//     struct context* ctx = malloc(sizeof(struct context));
//     ctx->arguments = malloc(sizeof(struct arguments));
//
//     struct settings set;
//     set.message = "";
//     set.count = 1;
//     ctx->settings = set;
//     ctx->exit_code = 0;
//     ctx->exit_message = "";
//     ctx->arguments->input_file = malloc(strlen(input_filename) + 1);
//     strcpy(ctx->arguments->input_file, input_filename);
//     ctx->arguments->output_file = malloc(strlen(output_filename) + 1);
//     strcpy(ctx->arguments->output_file, output_filename);
//     ctx->arguments->modification_option = 0;
//
//     copy_file_data(ctx);
//     free(ctx->arguments->input_file);
//     free(ctx->arguments->output_file);
//     free(ctx->arguments);
//     free(ctx);
// }
