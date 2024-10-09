//
// Created by blaise-klein on 9/15/24.
//

#include "character_modifications.h"
#include <ctype.h>

char text_to_lowercase(char input_char)
{
    return (char)tolower(input_char);
}

char text_to_uppercase(char input_char)
{
    return (char)toupper(input_char);
}

char text_unmodified(char input_char)
{
    return input_char;
}
