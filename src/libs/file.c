#include <tvi.h>

// Initialize editor state
void init_editor(EditorState* state) {
    state->lines = NULL;
    state->num_lines = 0;
    state->cursor_row = 0;
    state->cursor_col = 0;
    state->screen_rows = 24;
    state->screen_cols = 80;
    state->filename = NULL;
    state->mode = 0; // Normal mode
    state->command[0] = '\0';
    state->show_numbers = 0; // Line numbers off by default
    state->welcome_screen = 0;
}

// Free allocated lines
void free_lines(EditorState* state) {
    for (int i = 0; i < state->num_lines; i++) {
        free(state->lines[i].data);
    }
    free(state->lines);
    state->lines = NULL;
    state->num_lines = 0;
}

// Load file into editor
int load_file(EditorState* state, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        // If file doesn't exist, create empty document
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        state->lines[0].data = malloc(1);
        state->lines[0].data[0] = '\0';
        state->lines[0].length = 0;
        return 0;
    }
    
    // Count lines first
    int line_count = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }
    rewind(file);
    
    // Allocate lines array
    free_lines(state);
    state->lines = malloc(sizeof(Line) * line_count);
    state->num_lines = line_count;
    
    // Read lines into array
    for (int i = 0; i < line_count; i++) {
        if (fgets(buffer, sizeof(buffer), file)) {
            // Remove newline character
            int len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                len--;
            }
            
            state->lines[i].data = _strdup(buffer);
            state->lines[i].length = len;
        } else {
            // Handle read error
            state->lines[i].data = malloc(1);
            state->lines[i].data[0] = '\0';
            state->lines[i].length = 0;
        }
    }
    
    fclose(file);
    return 1;
}

// Save file from editor
int save_file(EditorState* state) {
    if (!state->filename) {
        // No filename specified - could add prompt here in future
        return 0;
    }
    
    FILE* file = fopen(state->filename, "w");
    if (!file) {
        return 0;
    }
    
    // Write lines to file
    for (int i = 0; i < state->num_lines; i++) {
        fputs(state->lines[i].data, file);
        fputc('\n', file); // Add newline after each line
    }
    
    fclose(file);
    return 1;
}
    