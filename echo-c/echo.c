
/*
 * MIT License
 *
 * Copyright 2022 Jenna Fligor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

// ANCHOR GLOBAL VARS AND #defines

#define VERSION "2.1.0"
#define AUTHOR "Jenna Fligor <jenna@fligor.net>\n"

const char* HELP_STRING =
"Usage: echo [SHORT-OPTION]... [STRING]..."                                 "\n"
"  or:  echo LONG-OPTION"                                                   "\n"
"Echo the STRING(s) to standard output."                                    "\n"
                                                                            "\n"
"  -n             do not output the trailing newline"                       "\n"
"  -e             enable interpretation of backslash escapes"               "\n"
"  -E             disable interpretation of backslash escapes (default)"    "\n"
"      --help     display this help and exit"                               "\n"
"      --version  output version information and exit"                      "\n"
                                                                            "\n"
"If -e is in effect, the following sequences are recognized:"               "\n"
                                                                            "\n"
"  \\\\      backslash"                                                     "\n"
"  \\a      alert (BEL)"                                                    "\n"
"  \\b      backspace"                                                      "\n"
"  \\c      produce no further output"                                      "\n"
"  \\e      escape"                                                         "\n"
"  \\f      form feed"                                                      "\n"
"  \\n      new line"                                                       "\n"
"  \\r      carriage return"                                                "\n"
"  \\t      horizontal tab"                                                 "\n"
"  \\v      vertical tab"                                                   "\n"
"  \\0NNN   byte with octal value NNN (1 to 3 digits)"                      "\n"
"  \\xHH    byte with hexadecimal value HH (1 to 2 digits)"                 "\n"
                                                                            "\n"
"This (poor) copy of the GNU coreutils command `echo' is made by:"          "\n"
AUTHOR;

const char* VERSION_STRING =
"echo -- " VERSION                                                          "\n"
                                                                            "\n"
"This (poor) copy of the GNU coreutils command `echo' is made by:"          "\n"
AUTHOR;

// macro aliases for non-printable charaters
#define CHAR_NULL         (char)0
#define CHAR_BACKSLASH    (char)92
#define CHAR_BELL         (char)7
#define CHAR_BACKSPACE    (char)8
#define CHAR_ESCAPE       (char)27
#define CHAR_FORM_FEED    (char)12
#define CHAR_LINE_FEED    (char)10
#define CHAR_CARRIAGE_RET (char)13
#define CHAR_HOR_TAB      (char)9
#define CHAR_VER_TAB      (char)11

// human-readable names for flag bitmasks
#define TRAILING_NEWLINE (uint_least8_t)0b001
#define BACKSLASH_EXPAND (uint_least8_t)0b010
#define HALT_OUTPUT      (uint_least8_t)0b100

// define global flags variable, and set default flags
uint_least8_t g_flags = TRAILING_NEWLINE;

// lookup tables for base 8 and and base 16 conversions
const uint_least8_t base8_lookup  [4] = { 0, 1, 8, 64 };
const uint_least8_t base16_lookup [3] = { 0, 1, 16 };

// ANCHOR END GLOBAL VARS AND #defines

// ANCHOR STRBUF TYPE AND FUNCTIONS

// defines a heap allocated string type
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} strbuf;

// creates and returns a strbuf with capacity for n charaters, if it can't
// allocate the needed memory ( n+1 * sizeof(char) ) it will write an error
// message to stderr, and exit with EXIT_FAILURE
strbuf strbuf_new(size_t n) {
    strbuf buff;
    buff.capacity = n;
    buff.size = 0;

    buff.data = (char*)calloc(buff.capacity+1, sizeof(char));
    if (buff.data == NULL) {
        fprintf(stderr, "\nerror: failed to allocate space for %zu chars\n",
                buff.capacity);
        exit(EXIT_FAILURE);
    }

    return buff;
}

// puts a charater on the end of a strbuf, if the pointer to the strbuf or the
// pointer to the data inside the strbuf are null than it does nothing, if the
// strbuf can't hold an additional charater it returns null but the passed in
// strbuf remains unchanged.
strbuf* strbuf_push(strbuf* buff, const char ch) {
    if (buff == NULL || buff->data == NULL ) return buff;
    if (buff->size < buff->capacity) {
        buff->data[buff->size] = ch;
        ++(buff->size);
        return buff;
    } else {
        return NULL;
    }
}

// call this function prior to a strbuf going out of scope or being freed from
// the heap to free its contents
void strbuf_del(strbuf* buff) {
    free(buff->data);
    buff = NULL;
}

// ANCHOR END STRBUF TYPE AND FUNCTIONS

// ANCHOR BACKSLASH EXPANSION

// converts a charater "ch" that is 0-9, a-f, or A-F to it's decimal
// equivalent, f charater is not on of those charater the result is
// implementation defined (the largest value of uint_least8_t for the platform)
// but it will always be >15 (the largest valid return value)
uint_least8_t char_digit_to_uint(const char ch) {
    if ( ch >= '0' && ch <= '9' ) return ch - 48;
    if ( ch >= 'a' && ch <= 'f' ) return ch - 87;
    if ( ch >= 'A' && ch <= 'F' ) return ch - 55;
    else return -1;
}

// takes a pointer to a null terminated string "str" and returns a strbuf
// with the result of performing backslash expansion as defined in echo(1)
// (https://linux.die.net/man/1/echo), if "str" is null then an empty
// strbuf is returned, the strbuf must be freed with strbuf_del() prior to
// going out of scope
strbuf backslash_expand(char* str) {
    if (str == NULL) return strbuf_new(0);

    strbuf buff = strbuf_new(strlen(str));

    for ( size_t i = 0; str[i] != 0 ; ++i ) {
        char working_char = str[i];
        if (working_char == CHAR_BACKSLASH) {
            ++i;
            char following_char = str[i];
            if (following_char == CHAR_NULL) {
                strbuf_push(&buff, CHAR_BACKSLASH);
                break;

            } else if (following_char == CHAR_BACKSLASH) {
                strbuf_push(&buff, CHAR_BACKSLASH);

            } else if (following_char == 'a') {
                strbuf_push(&buff, CHAR_BELL);

            } else if (following_char == 'b') {
                strbuf_push(&buff, CHAR_BACKSPACE);

            } else if (following_char == 'c') {
                g_flags |= HALT_OUTPUT;
                break;

            } else if (following_char == 'e') {
                strbuf_push(&buff,CHAR_ESCAPE);
                strbuf_push(&buff, CHAR_LINE_FEED);

            } else if (following_char == 'r') {
                strbuf_push(&buff, CHAR_CARRIAGE_RET);

            } else if (following_char == 't') {
                strbuf_push(&buff ,CHAR_HOR_TAB);

            } else if (following_char == 'v') {
                strbuf_push(&buff, CHAR_VER_TAB);

            } else if (following_char == '0') {
                strbuf oct_buff = strbuf_new(3);

                for (size_t j = 1; j <= 3; ++j) {
                    char oct_working_char = str[i+1];
                    if (oct_working_char == 0) break;
                    if (oct_working_char >= '0' && oct_working_char <= '7') {
                        strbuf_push(&oct_buff, oct_working_char);
                        ++i;
                    } else {
                        break;
                    }
                }
                if (oct_buff.size == 0) continue;

                size_t lookup_index = oct_buff.size;
                char value = 0;
                for ( size_t j = 0 ; j <= 3; ++j ) {
                    char oct_working_char = oct_buff.data[j];
                    if (oct_working_char == 0) break;
                    value += (char)(char_digit_to_uint(oct_working_char) *
                                    base8_lookup[lookup_index]);
                    --lookup_index;
                }
                strbuf_push(&buff, value);
                strbuf_del(&oct_buff);

            } else if (following_char == 'x') {
                strbuf hex_buff = strbuf_new(2);

                for (size_t j = 1; j <= 2; ++j) {
                    char hex_working_char = str[i+1];
                    if (hex_working_char == 0) break;
                    if ((hex_working_char >= '0' && hex_working_char <= '9') ||
                        (hex_working_char >= 'A' && hex_working_char <= 'F') ||
                        (hex_working_char >= 'a' && hex_working_char <= 'f')) {
                        strbuf_push(&hex_buff, hex_working_char);
                        ++i;
                    } else {
                        break;
                    }
                }
                if (hex_buff.size == 0) continue;

                size_t lookup_index = hex_buff.size;
                char value = 0;
                for ( size_t j = 0 ; j <= 2; ++j ) {
                    char hex_working_char = hex_buff.data[j];
                    if (hex_working_char == 0) break;
                    value += (char)(char_digit_to_uint(hex_working_char) *
                                    base16_lookup[lookup_index]);
                    --lookup_index;
                }
                strbuf_push(&buff, value);
                strbuf_del(&hex_buff);
            }
        } else {
            strbuf_push(&buff, working_char);
        }
    }
    return buff;
}

// ANCHOR END BACKSLASH EXPANSION

// ANCHOR COMMAND ARGUMENT PARSSING

void parse_args(const int argc, char** argv) {

    // if the only flag is one of the two special long flags, perform the
    // appropriate action and then exit with EXIT_SUCCESS
    if (argc == 2) {
        if ( strcmp(argv[1], "--version") == 0 ) {
            printf("%s", VERSION_STRING);
            exit(EXIT_SUCCESS);
        } else if ( strcmp(argv[1], "--help") == 0 ) {
            printf("%s", HELP_STRING);
            exit(EXIT_SUCCESS);
        }
    }

    // iterate over arguments checking to see if any of them are valid flags
    // if they are, set the appropriate flag and remove them from the arguments
    // to be printed out
    for (size_t i = 1 ; i < argc ; ++i ) {
        char* arg = argv[i];
        if (arg[0] == '-') {
            size_t arg_len = strlen(arg);
            for ( size_t j = 1; j < arg_len; ++j) {

                bool valid_flag = false;
                uint_least8_t flags = g_flags;
                char ch = arg[j];

                if (ch == 'n') {
                    flags |= TRAILING_NEWLINE;
                    flags ^= TRAILING_NEWLINE;
                    valid_flag = true;
                }
                if (ch == 'e') {
                    flags |= BACKSLASH_EXPAND;
                    valid_flag = true;
                }
                if (ch == 'E') {
                    flags |= BACKSLASH_EXPAND;
                    flags ^= BACKSLASH_EXPAND;
                    valid_flag = true;
                }

                if (!valid_flag) break;
                else {
                    g_flags = flags;
                    arg[0];
                }
            }
        }
        break;
    }
}

// ANCHOR END COMMAND ARGUMENT PARSSING

// ANCHOR MAIN

int main(int argc, char** argv) {

    // set the global variable g_flags based on arguments
    parse_args(argc, argv);

    // print each argument, performing backslash expanssion if necessary
    for (size_t i = 1 ; i < argc ; ++i ) {
        if (g_flags & HALT_OUTPUT) break;

        char* arg = argv[i];
        if (arg[0] == 0) continue;

        if (g_flags & BACKSLASH_EXPAND) {
            strbuf new_str = backslash_expand(arg);
            if (new_str.data == NULL) continue;
            printf("%s", new_str.data);
            strbuf_del(&new_str);
        } else {
            printf("%s", arg);
        }

        if ( i < argc-1 ) {
            printf(" ");
        }
    }

    if (g_flags & TRAILING_NEWLINE) printf("\n");
    return EXIT_SUCCESS;
}

// ANCHOR END MAIN
