#ifndef TVI_H
#define TVI_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to represent a single line of text
typedef struct {
    char* data;
    int length;
} Line;

// Structure to hold the entire editor state
typedef struct {
    Line* lines;          // Array of lines
    int num_lines;        // Number of lines
    int cursor_row;       // Current cursor row position
    int cursor_col;       // Current cursor column position
    int screen_rows;      // Number of rows in the terminal
    int screen_cols;      // Number of columns in the terminal
    char* filename;       // Current filename
    int mode;             // 0: normal, 1: insert, 2: command
    char command[256];    // Buffer for command input
    int show_numbers;     // Flag for line numbers (0: off, 1: on)
    int welcome_screen;   // Flag for welcome screen display
} EditorState;

// Screen handling functions
void init_screen(EditorState* state);
void cleanup_screen();
void get_terminal_size(EditorState* state);
void refresh_screen(EditorState* state);
void draw_borders(EditorState* state);
void draw_lines(EditorState* state);
void draw_status_bar(EditorState* state);
void draw_command_line(EditorState* state);

// Editor initialization and cleanup
void init_editor(EditorState* state);
void free_lines(EditorState* state);

// File operations
int load_file(EditorState* state, const char* filename);
int save_file(EditorState* state);

// Input handling
void handle_input(EditorState* state);
int process_command(EditorState* state, const char* cmd);

// Welcome screen
void show_welcome_screen(EditorState* state);

// Cursor movement
void move_cursor_up(EditorState* state);
void move_cursor_down(EditorState* state);
void move_cursor_left(EditorState* state);
void move_cursor_right(EditorState* state);

// Editing functions
void insert_char(EditorState* state, char c);
void delete_char(EditorState* state);
void insert_newline(EditorState* state);

#endif // TVI_H