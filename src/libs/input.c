#include <tvi.h>

static HANDLE hStdIn;
static DWORD original_mode;

void init_input() {
    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error getting input handle\n");
        exit(1);
    }

    if (!GetConsoleMode(hStdIn, &original_mode)) {
        fprintf(stderr, "Error getting console mode\n");
        exit(1);
    }

    DWORD new_mode = ENABLE_EXTENDED_FLAGS;
    new_mode |= (original_mode & ~ENABLE_ECHO_INPUT);  
    new_mode &= ~ENABLE_LINE_INPUT;
    
    if (!SetConsoleMode(hStdIn, new_mode)) {
        fprintf(stderr, "Error setting console mode\n");
        exit(1);
    }
}

// 恢复原始控制台模式
void restore_input_mode() {
    SetConsoleMode(hStdIn, original_mode);
}

// Process colon commands
int process_command(EditorState* state, const char* cmd) {
    if (strcmp(cmd, "q") == 0) {
        return 1; // Signal to exit
    } else if (strcmp(cmd, "w") == 0) {
        if (save_file(state)) {}
    } else if (strcmp(cmd, "wq") == 0) {
        // Save and quit
        if (save_file(state)) {
            return 1;
        }
    } else if (strcmp(cmd, "set number") == 0) {
        // Toggle line numbers on
        state->show_numbers = 1;
    } else if (strcmp(cmd, "set nonumber") == 0) {
        // Toggle line numbers off
        state->show_numbers = 0;
    }
    
    // Clear command buffer
    state->command[0] = '\0';
    state->mode = 0; // Return to normal mode
    return 0;
}

// Handle cursor movement
void move_cursor_up(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_row > 0) {
        state->cursor_row--;
        // Adjust column if new row is shorter
        if (state->cursor_col >= state->lines[state->cursor_row].length) {
            state->cursor_col = state->lines[state->cursor_row].length;
        }
    }
}

void move_cursor_down(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_row < state->num_lines - 1) {
        state->cursor_row++;
        // Adjust column if new row is shorter
        if (state->cursor_col >= state->lines[state->cursor_row].length) {
            state->cursor_col = state->lines[state->cursor_row].length;
        }
    }
}

void move_cursor_left(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_col > 0) {
        state->cursor_col--;
    } else if (state->cursor_row > 0) {
        // Move to end of previous line
        state->cursor_row--;
        state->cursor_col = state->lines[state->cursor_row].length;
    }
}

void move_cursor_right(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_col < state->lines[state->cursor_row].length) {
        state->cursor_col++;
    } else if (state->cursor_row < state->num_lines - 1) {
        // Move to start of next line
        state->cursor_row++;
        state->cursor_col = 0;
    }
}

// Handle keyboard input
void handle_input(EditorState* state) {
    INPUT_RECORD ir;
    DWORD num_read;
    KEY_EVENT_RECORD key_event;
    
    // Wait for input with timeout to allow for window resize detection
    if (!WaitForSingleObject(hStdIn, 100)) {
        // Read input if available
        if (!ReadConsoleInput(hStdIn, &ir, 1, &num_read) || num_read == 0) {
            return;
        }
    } else {
        // No input, check for window resize
        int old_rows = state->screen_rows;
        int old_cols = state->screen_cols;
        get_terminal_size(state);
        
        if (old_rows != state->screen_rows || old_cols != state->screen_cols) {
            refresh_screen(state);
        }
        return;
    }
    
    if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) {
        return;
    }
    
    key_event = ir.Event.KeyEvent;
    
    // Handle escape key (return to normal mode)
    if (key_event.wVirtualKeyCode == VK_ESCAPE) {
        state->mode = 0;
        state->command[0] = '\0';
        return;
    }
    
    switch (state->mode) {
        case 0: // Normal mode
            // Handle normal mode commands
            if (key_event.wVirtualKeyCode == VK_UP || 
                (key_event.uChar.AsciiChar == 'k' && !key_event.dwControlKeyState)) {
                move_cursor_up(state);
            } else if (key_event.wVirtualKeyCode == VK_DOWN || 
                     (key_event.uChar.AsciiChar == 'j' && !key_event.dwControlKeyState)) {
                move_cursor_down(state);
            } else if (key_event.wVirtualKeyCode == VK_LEFT || 
                     (key_event.uChar.AsciiChar == 'h' && !key_event.dwControlKeyState)) {
                move_cursor_left(state);
            } else if (key_event.wVirtualKeyCode == VK_RIGHT || 
                     (key_event.uChar.AsciiChar == 'l' && !key_event.dwControlKeyState)) {
                move_cursor_right(state);
            } else if (key_event.uChar.AsciiChar == 'i') {
                // Enter insert mode
                state->mode = 1;
            } else if (key_event.uChar.AsciiChar == ':') {
                // Enter command mode
                state->mode = 2;
                state->command[0] = '\0';
            } else if (key_event.uChar.AsciiChar == 'x') {
                // Delete character
                delete_char(state);
            }
            break;
            
        case 1: // Insert mode
            // Handle insert mode input
            if (key_event.wVirtualKeyCode == VK_BACK) {
                // Backspace
                delete_char(state);
            } else if (key_event.wVirtualKeyCode == VK_RETURN) {
                // Enter key - new line
                insert_newline(state);
            } else if (key_event.wVirtualKeyCode == VK_LEFT) {
                move_cursor_left(state);
            } else if (key_event.wVirtualKeyCode == VK_RIGHT) {
                move_cursor_right(state);
            } else if (key_event.wVirtualKeyCode == VK_UP) {
                move_cursor_up(state);
            } else if (key_event.wVirtualKeyCode == VK_DOWN) {
                move_cursor_down(state);
            } else if (key_event.uChar.AsciiChar >= 32 && key_event.uChar.AsciiChar <= 126) {
                // Printable characters
                insert_char(state, key_event.uChar.AsciiChar);
            }
            break;
            
        case 2: // Command mode
            // Handle command input
            if (key_event.wVirtualKeyCode == VK_BACK) {
                // Backspace in command
                int len = strlen(state->command);
                if (len > 0) {
                    state->command[len - 1] = '\0';
                }
            } else if (key_event.wVirtualKeyCode == VK_RETURN) {
                // Execute command
                if (process_command(state, state->command)) {
                    // Exit if command says so
                    cleanup_screen();
                    free_lines(state);
                    if (state->filename) free(state->filename);
                    restore_input_mode();
                    exit(0);
                }
            } else if (key_event.uChar.AsciiChar >= 32 && key_event.uChar.AsciiChar <= 126) {
                // Add character to command
                int len = strlen(state->command);
                if (len < 255) {
                    state->command[len] = key_event.uChar.AsciiChar;
                    state->command[len + 1] = '\0';
                }
            }
            break;
    }
}
