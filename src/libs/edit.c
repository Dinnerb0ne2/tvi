#include <tvi.h>

// Insert a character at cursor position
void insert_char(EditorState* state, char c) {
    if (state->welcome_screen) {
        // Clear welcome screen when first character is entered
        free_lines(state);
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        state->lines[0].data = malloc(2);
        state->lines[0].data[0] = c;
        state->lines[0].data[1] = '\0';
        state->lines[0].length = 1;
        state->welcome_screen = 0;
        state->cursor_col = 1;
        return;
    }
    
    Line* line = &state->lines[state->cursor_row];
    line->data = realloc(line->data, line->length + 2);
    
    // Shift characters to make space
    memmove(&line->data[state->cursor_col + 1], 
           &line->data[state->cursor_col], 
           line->length - state->cursor_col + 1);
    
    // Insert new character
    line->data[state->cursor_col] = c;
    line->length++;
    state->cursor_col++;
}

// Delete character at cursor position
void delete_char(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->num_lines == 0) return;
    
    Line* line = &state->lines[state->cursor_row];
    
    if (state->cursor_col > 0) {
        // Delete character before cursor
        memmove(&line->data[state->cursor_col - 1], 
               &line->data[state->cursor_col], 
               line->length - state->cursor_col + 1);
        line->length--;
        line->data = realloc(line->data, line->length + 1);
        state->cursor_col--;
    } else if (state->cursor_row > 0) {
        // Merge with previous line
        Line* prev_line = &state->lines[state->cursor_row - 1];
        int new_length = prev_line->length + line->length;
        
        // Allocate space for merged line
        prev_line->data = realloc(prev_line->data, new_length + 1);
        
        // Append current line to previous line
        strcat(prev_line->data, line->data);
        prev_line->length = new_length;
        
        // Move cursor to end of previous line
        state->cursor_col = prev_line->length;
        state->cursor_row--;
        
        // Remove current line
        free(line->data);
        memmove(&state->lines[state->cursor_row + 1], 
               &state->lines[state->cursor_row + 2], 
               (state->num_lines - state->cursor_row - 2) * sizeof(Line));
        state->num_lines--;
        state->lines = realloc(state->lines, state->num_lines * sizeof(Line));
    }
}

// Insert new line at cursor position
void insert_newline(EditorState* state) {
    if (state->welcome_screen) {
        // Clear welcome screen when first newline is entered
        free_lines(state);
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        state->lines[0].data = malloc(1);
        state->lines[0].data[0] = '\0';
        state->lines[0].length = 0;
        state->welcome_screen = 0;
    }
    
    // Current line info
    int current_row = state->cursor_row;
    Line* current_line = &state->lines[current_row];
    
    // Create new line with content after cursor
    Line new_line;
    new_line.length = current_line->length - state->cursor_col;
    new_line.data = malloc(new_line.length + 1);
    strncpy(new_line.data, &current_line->data[state->cursor_col], new_line.length);
    new_line.data[new_line.length] = '\0';
    
    // Truncate current line at cursor position
    current_line->length = state->cursor_col;
    current_line->data = realloc(current_line->data, current_line->length + 1);
    current_line->data[current_line->length] = '\0';
    
    // Insert new line into lines array
    state->num_lines++;
    state->lines = realloc(state->lines, state->num_lines * sizeof(Line));
    memmove(&state->lines[current_row + 2], 
           &state->lines[current_row + 1], 
           (state->num_lines - current_row - 2) * sizeof(Line));
    state->lines[current_row + 1] = new_line;
    
    // Move cursor to start of new line
    state->cursor_row++;
    state->cursor_col = 0;
}

// Show welcome screen when no file is opened
void show_welcome_screen(EditorState* state) {
    // Clear any existing lines
    free_lines(state);
    
    // We'll handle welcome screen rendering separately in draw_lines
    state->num_lines = 0;
    state->cursor_row = 0;
    state->cursor_col = 0;
}
    