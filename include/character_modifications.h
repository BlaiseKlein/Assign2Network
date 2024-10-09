//
// Created by blaise-klein on 9/15/24.
//

#ifndef CHARACTER_MODIFICATION_H
#define CHARACTER_MODIFICATION_H

typedef char (*operation_func)(char);

char text_to_lowercase(char input_char);

char text_to_uppercase(char input_char);

char text_unmodified(char input_char);

#endif    // CHARACTER_MODIFICATION_H
