#include <tvi.h>

HANDLE hStdOut, hStdIn;
CONSOLE_CURSOR_INFO cursor_info;
DWORD original_mode;

// Initialize screen settings
void init_screen(EditorState* state) {
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    
    // Hide cursor
    cursor_info.cbSize = sizeof(CONSOLE_CURSOR_INFO);
    cursor_info.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &cursor_info);
    
    // Set input mode for raw input
    GetConsoleMode(hStdIn, &original_mode);
    SetConsoleMode(hStdIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);
    
    // Get initial terminal size
    get_terminal_size(state);
}

// Restore original screen settings
void cleanup_screen() {
    // Show cursor
    cursor_info.bVisible = TRUE;
    SetConsoleCursorInfo(hStdOut, &cursor_info);
    
    // Restore input mode
    SetConsoleMode(hStdIn, original_mode);
    
    // Clear screen
    system("cls");
}

// Get current terminal dimensions
void get_terminal_size(EditorState* state) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    state->screen_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    state->screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

// Set cursor position
static void set_cursor_pos(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hStdOut, coord);
}

// Draw window borders
void draw_borders(EditorState* state) {
    set_cursor_pos(0, 0);
    
    // Top border
    printf("%c", 201); // Top-left corner
    for (int i = 1; i < state->screen_cols - 1; i++) {
        printf("%c", 205); // Horizontal line
    }
    printf("%c", 187); // Top-right corner
    
    // Side borders
    for (int i = 1; i < state->screen_rows - 2; i++) {
        set_cursor_pos(0, i);
        printf("%c", 186); // Vertical line
        
        set_cursor_pos(state->screen_cols - 1, i);
        printf("%c", 186); // Vertical line
    }
    
    // Status bar separator
    int status_row = state->screen_rows - 2;
    set_cursor_pos(0, status_row);
    printf("%c", 204); // Left T-junction
    for (int i = 1; i < state->screen_cols - 1; i++) {
        printf("%c", 205); // Horizontal line
    }
    printf("%c", 185); // Right T-junction
    
    // Command line (bottom row)
    set_cursor_pos(0, state->screen_rows - 1);
    for (int i = 0; i < state->screen_cols; i++) {
        printf(" ");
    }
}

// Draw text lines with optional line numbers
void draw_lines(EditorState* state) {
    if (state->welcome_screen) {
        // Welcome screen content
        const char* welcome[] = {
            "Tiny Vi (tvi) - A simple vi clone for Windows",
            "",
            "Basic commands:",
            "  :w      - Save file",
            "  :q      - Quit",
            "  :wq     - Save and quit",
            "  :set number - Show line numbers",
            "  i       - Enter insert mode",
            "  ESC     - Enter normal mode",
            "",
            "Cursor movement: Arrow keys or h/j/k/l",
            "",
            "Start with: tvi.exe [filename] to edit a file"
        };
        int num_welcome = sizeof(welcome) / sizeof(welcome[0]);
        int start_row = (state->screen_rows - 4 - num_welcome) / 2;
        
        for (int i = 0; i < num_welcome && i + start_row < state->screen_rows - 3; i++) {
            set_cursor_pos(2, start_row + i);
            printf("%s", welcome[i]);
        }
        return;
    }
    
    // Calculate visible area
    int line_num_width = state->show_numbers ? 6 : 0;
    
    // Draw each visible line
    for (int i = 0; i < state->screen_rows - 3; i++) {
        set_cursor_pos(1 + line_num_width, i + 1);
        
        if (i < state->num_lines) {
            // Print line content, truncated to fit screen
            int max_col = state->screen_cols - 3 - line_num_width;
            if (state->lines[i].length > 0) {
                int print_len = (state->lines[i].length < max_col) ? state->lines[i].length : max_col;
                printf("%.*s", print_len, state->lines[i].data);
            }
        }
        
        // Clear remaining characters in the line
        COORD pos;
        GetConsoleCursorPosition(hStdOut, &pos);
        for (int j = pos.X; j < state->screen_cols - 1; j++) {
            printf(" ");
        }
        
        // Draw line numbers if enabled
        if (state->show_numbers && i < state->num_lines) {
            set_cursor_pos(1, i + 1);
            printf("%4d  ", i + 1);
        }
    }
}

// Draw status bar with current mode and filename
void draw_status_bar(EditorState* state) {
    set_cursor_pos(1, state->screen_rows - 2);
    
    // Show current mode
    const char* mode_str;
    switch (state->mode) {
        case 0: mode_str = "-- NORMAL --"; break;
        case 1: mode_str = "-- INSERT --"; break;
        case 2: mode_str = "-- COMMAND --"; break;
        default: mode_str = "";
    }
    printf("%s", mode_str);
    
    // Show filename
    if (state->filename) {
        set_cursor_pos(state->screen_cols - strlen(state->filename) - 1, state->screen_rows - 2);
        printf("%s", state->filename);
    }
}

// Draw command input line
void draw_command_line(EditorState* state) {
    set_cursor_pos(1, state->screen_rows - 1);
    if (state->mode == 2) {
        printf(":%s", state->command);
    } else {
        printf(" "); // Clear command line when not in command mode
    }
}

// Update cursor position for display
void update_cursor_display(EditorState* state) {
    if (state->welcome_screen) return;
    
    int x = 1 + state->cursor_col;
    int y = 1 + state->cursor_row;
    
    // Adjust for line numbers
    if (state->show_numbers) {
        x += 6;
    }
    
    set_cursor_pos(x, y);
}

// Refresh entire screen
void refresh_screen(EditorState* state) {
    // Clear screen by filling with spaces
    for (int y = 0; y < state->screen_rows; y++) {
        set_cursor_pos(0, y);
        for (int x = 0; x < state->screen_cols; x++) {
            printf(" ");
        }
    }
    
    // Redraw all elements
    draw_borders(state);
    draw_lines(state);
    draw_status_bar(state);
    draw_command_line(state);
    
    // Update cursor position
    update_cursor_display(state);
}
    