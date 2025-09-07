#include <tvi.h>

// Insert a character at cursor position
void insert_char(EditorState* state, char c) {
    if (state->welcome_screen) {
        // Clear welcome screen when first character is entered
        free_lines(state);
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        if (!state->lines) {
            fprintf(stderr, "Memory allocation failed for new line\n");
            return;
        }
        
        state->lines[0].data = malloc(2);
        if (!state->lines[0].data) {
            fprintf(stderr, "Memory allocation failed for line data\n");
            free(state->lines);
            state->lines = NULL;
            state->num_lines = 0;
            return;
        }
        
        state->lines[0].data[0] = c;
        state->lines[0].data[1] = '\0';
        state->lines[0].length = 1;
        state->welcome_screen = 0;
        state->cursor_col = 1;
        
        // Refresh screen after welcome screen transition
        refresh_screen(state);
        return;
    }
    
    Line* line = &state->lines[state->cursor_row];
    char* new_data = realloc(line->data, line->length + 2);
    if (!new_data) {
        fprintf(stderr, "Memory allocation failed in insert_char\n");
        return;
    }
    line->data = new_data;
    
    // Shift characters to make space
    memmove(&line->data[state->cursor_col + 1], 
           &line->data[state->cursor_col], 
           line->length - state->cursor_col + 1);
    
    // Insert new character
    line->data[state->cursor_col] = c;
    line->length++;
    state->cursor_col++;
    
    // Critical: Refresh screen after insertion
    refresh_screen(state);
}

// Delete character at cursor position
void delete_char(EditorState* state) {
    if (state->welcome_screen || state->num_lines == 0) return;
    
    Line* line = &state->lines[state->cursor_row];
    
    if (state->cursor_col > 0) {
        // Delete character before cursor
        memmove(&line->data[state->cursor_col - 1], 
               &line->data[state->cursor_col], 
               line->length - state->cursor_col + 1);
        line->length--;
        
        // Only realloc if we're reducing the size significantly
        if (line->length + 1 < (size_t)(line->length + 2)) {
            char* new_data = realloc(line->data, line->length + 1);
            if (new_data) {
                line->data = new_data;
            }
        }
        
        state->cursor_col--;
    } else if (state->cursor_row > 0) {
        // Merge with previous line
        Line* prev_line = &state->lines[state->cursor_row - 1];
        size_t new_length = prev_line->length + line->length;
        
        // Allocate space for merged line
        char* new_data = realloc(prev_line->data, new_length + 1);
        if (!new_data) {
            fprintf(stderr, "Memory allocation failed in delete_char\n");
            return;
        }
        prev_line->data = new_data;
        
        // Append current line to previous line
        strcat(prev_line->data, line->data);
        prev_line->length = new_length;
        
        // Move cursor to end of previous line
        state->cursor_col = prev_line->length;
        state->cursor_row--;
        
        // Remove current line
        free(line->data);
        state->num_lines--;
        
        // Shift remaining lines
        if (state->cursor_row + 1 < state->num_lines) {
            memmove(&state->lines[state->cursor_row + 1], 
                   &state->lines[state->cursor_row + 2], 
                   (state->num_lines - state->cursor_row - 1) * sizeof(Line));
        }
        
        // Realloc lines array
        Line* new_lines = realloc(state->lines, state->num_lines * sizeof(Line));
        if (new_lines) {
            state->lines = new_lines;
        }
    }
    
    // Critical: Refresh screen after deletion
    refresh_screen(state);
}

// Insert new line at cursor position
void insert_newline(EditorState* state) {
    if (state->welcome_screen) {
        // Clear welcome screen when first newline is entered
        free_lines(state);
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        if (!state->lines) {
            fprintf(stderr, "Memory allocation failed for new line\n");
            return;
        }
        
        state->lines[0].data = malloc(1);
        if (!state->lines[0].data) {
            fprintf(stderr, "Memory allocation failed for line data\n");
            free(state->lines);
            state->lines = NULL;
            state->num_lines = 0;
            return;
        }
        
        state->lines[0].data[0] = '\0';
        state->lines[0].length = 0;
        state->welcome_screen = 0;
    } else {
        // Current line info
        int current_row = state->cursor_row;
        Line* current_line = &state->lines[current_row];
        
        // Create new line with content after cursor
        Line new_line;
        new_line.length = current_line->length - state->cursor_col;
        new_line.data = malloc(new_line.length + 1);
        if (!new_line.data) {
            fprintf(stderr, "Memory allocation failed for new line\n");
            return;
        }
        
        strncpy(new_line.data, &current_line->data[state->cursor_col], new_line.length);
        new_line.data[new_line.length] = '\0';
        
        // Truncate current line at cursor position
        current_line->length = state->cursor_col;
        char* truncated_data = realloc(current_line->data, current_line->length + 1);
        if (truncated_data) {
            current_line->data = truncated_data;
        }
        current_line->data[current_line->length] = '\0';
        
        // Insert new line into lines array
        state->num_lines++;
        Line* new_lines = realloc(state->lines, state->num_lines * sizeof(Line));
        if (!new_lines) {
            fprintf(stderr, "Memory allocation failed for lines array\n");
            free(new_line.data);
            state->num_lines--;
            return;
        }
        state->lines = new_lines;
        
        // Shift lines to make space
        memmove(&state->lines[current_row + 2], 
               &state->lines[current_row + 1], 
               (state->num_lines - current_row - 2) * sizeof(Line));
        state->lines[current_row + 1] = new_line;
        
        // Move cursor to start of new line
        state->cursor_row++;
    }
    
    state->cursor_col = 0;
    
    // Critical: Refresh screen after new line
    refresh_screen(state);
}

// Show welcome screen when no file is opened
void show_welcome_screen(EditorState* state) {
    // Clear any existing lines
    free_lines(state);
    
    // Welcome screen will be rendered in draw_lines
    state->num_lines = 0;
    state->cursor_row = 0;
    state->cursor_col = 0;
    state->welcome_screen = 1;
    
    // Refresh to show welcome screen
    refresh_screen(state);
}

// Helper function to free all lines
// void free_lines(EditorState* state) {
//     for (int i = 0; i < state->num_lines; i++) {
//         free(state->lines[i].data);
//     }
//     free(state->lines);
//     state->lines = NULL;
//     state->num_lines = 0;
// }
