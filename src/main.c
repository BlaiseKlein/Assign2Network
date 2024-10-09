#include "filters.h"
#include <stdio.h>
#include <stdlib.h>

// const operation_func char_mods[] = {text_to_lowercase, text_to_uppercase, text_unmodified};

int main(int argc, char **argv)
{
    struct context *ctx = (struct context *)malloc(sizeof(struct context));
    parse_arguments(argc, argv, ctx);
    if(ctx == NULL)
    {
        perror("Failed to parse arguments");
    }
    copy_file_data(ctx);

    cleanup_context(ctx);
}
