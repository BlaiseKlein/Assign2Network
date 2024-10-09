//
// Created by blaise-klein on 9/14/24.
//

#ifndef FILTERS_H
#define FILTERS_H
#include "character_modifications.h"
#include <stdio.h>

enum Text_Modifications
{
    lower   = 0,
    upper   = 1,
    none    = 2,
    invalid = 3
};

struct arguments
{
    int                     argc;
    char                   *input_file;
    char                   *output_file;
    enum Text_Modifications modification_option;
    char                  **argv;
};

struct settings
{
    unsigned int count;
    const char  *message;
};

struct context
{
    struct arguments *arguments;
    struct settings   settings;
};

void usage(const char *err);

void cleanup_file(int fd);

ssize_t read_input_file(int fd, char *c);

ssize_t write_output_file(int fd, char write_character);

void parse_arguments(int argc, char **argv, struct context *ctx);

void cleanup_context(struct context *ctx);

void cleanup_files(struct context *ctx);

void copy_file_data(struct context *);

#endif    // FILTERS_H
