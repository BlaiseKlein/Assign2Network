//
// Created by blaise-klein on 9/30/24.
//

#ifndef TEXT_MODIFY_SERVER_H
#define TEXT_MODIFY_SERVER_H
#include "character_modifications.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

enum Text_Modifications
{
    lower   = 0,
    upper   = 1,
    none    = 2,
    invalid = 3
};

struct thread_arguments
{
    enum Text_Modifications option;
    char                   *msg;
    char                   *fifo_out;
    size_t                  msg_length;
};

static const operation_func char_mods[] = {text_to_lowercase, text_to_uppercase, text_unmodified};

void handle_signal(int signal);

ssize_t send_modified_text(struct thread_arguments *arguments);

static void *handle_fifo_connection(void *args);

void await_fifo_connection(const char *filename, const char *filename_out, size_t file_name_length);

// void *basic_thread_func(void *c);

enum Text_Modifications option_character(char c);

// void basic_fifo_connection(const char *filename);

int main(void);

#endif    // TEXT_MODIFY_SERVER_H
