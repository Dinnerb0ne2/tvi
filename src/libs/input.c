#include <tvi.h>

static HANDLE hStdIn;
static DWORD originalConsoleMode;

/**
 * Initialize console input system with raw mode configuration
 * Disables line buffering and input echo for proper editor behavior
 */
void init_input() {
    // Get standard input handle
    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Failed to get input handle (error %lu)\n", GetLastError());
        exit(1);
    }

    // Save original console mode for later restoration
    if (!GetConsoleMode(hStdIn, &originalConsoleMode)) {
        fprintf(stderr, "Error: Failed to get console mode (error %lu)\n", GetLastError());
        exit(1);
    }

    // Configure optimal input mode for editor
    DWORD rawMode = originalConsoleMode;
    rawMode &= ~ENABLE_ECHO_INPUT;          // Disable input echoing
    rawMode &= ~ENABLE_LINE_INPUT;          // Disable line buffering
    rawMode |= ENABLE_PROCESSED_INPUT;      // Enable basic input processing
    rawMode |= ENABLE_VIRTUAL_TERMINAL_INPUT; // Enable VT100 support

    // Apply input mode with fallback compatibility
    if (!SetConsoleMode(hStdIn, rawMode)) {
        fprintf(stderr, "Warning: Primary input mode failed (error %lu), trying fallback\n", GetLastError());
        
        // Fallback mode for older systems
        rawMode = ENABLE_PROCESSED_INPUT;
        rawMode &= ~ENABLE_ECHO_INPUT;
        rawMode &= ~ENABLE_LINE_INPUT;
        
        if (!SetConsoleMode(hStdIn, rawMode)) {
            fprintf(stderr, "Error: Failed to set fallback input mode (error %lu)\n", GetLastError());
            exit(1);
        }
    }
}

/**
 * Restore original console input mode before program exit
 */
void restore_input_mode() {
    SetConsoleMode(hStdIn, originalConsoleMode);
}

/**
 * Process colon commands entered in command mode
 * @param state Editor state structure
 * @param cmd Command string to process
 * @return 1 if editor should exit, 0 otherwise
 */
int process_command(EditorState* state, const char* cmd) {
    if (strcmp(cmd, "q") == 0) {
        return 1; // Quit without saving
    } else if (strcmp(cmd, "w") == 0) {
        save_file(state); // Save current file
    } else if (strcmp(cmd, "wq") == 0) {
        if (save_file(state)) return 1; // Save and quit
    } else if (strcmp(cmd, "set number") == 0) {
        state->show_numbers = 1; // Enable line numbers
    } else if (strcmp(cmd, "set nonumber") == 0) {
        state->show_numbers = 0; // Disable line numbers
    }
    
    // Reset command state
    state->command[0] = '\0';
    state->mode = 0;
    return 0;
}

/**
 * Move cursor up with boundary checking
 * @param state Editor state structure
 */
void move_cursor_up(EditorState* state) {
    if (state->welcome_screen || state->cursor_row <= 0) return;
    
    state->cursor_row--;
    // Adjust column if new row is shorter than current column position
    if (state->cursor_col > state->lines[state->cursor_row].length) {
        state->cursor_col = state->lines[state->cursor_row].length;
    }
}

/**
 * Move cursor down with boundary checking
 * @param state Editor state structure
 */
void move_cursor_down(EditorState* state) {
    if (state->welcome_screen || state->cursor_row >= state->num_lines - 1) return;
    
    state->cursor_row++;
    // Adjust column if new row is shorter than current column position
    if (state->cursor_col > state->lines[state->cursor_row].length) {
        state->cursor_col = state->lines[state->cursor_row].length;
    }
}

/**
 * Move cursor left with boundary checking and line wrapping
 * @param state Editor state structure
 */
void move_cursor_left(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_col > 0) {
        state->cursor_col--;
    } else if (state->cursor_row > 0) {
        // Wrap to end of previous line
        state->cursor_row--;
        state->cursor_col = state->lines[state->cursor_row].length;
    }
}

/**
 * Move cursor right with boundary checking and line wrapping
 * @param state Editor state structure
 */
void move_cursor_right(EditorState* state) {
    if (state->welcome_screen) return;
    
    if (state->cursor_col < state->lines[state->cursor_row].length) {
        state->cursor_col++;
    } else if (state->cursor_row < state->num_lines - 1) {
        // Wrap to start of next line
        state->cursor_row++;
        state->cursor_col = 0;
    }
}

/**
 * Main input handling function - processes all user input events
 * @param state Editor state structure
 */
void handle_input(EditorState* state) {
    INPUT_RECORD inputRecord;
    DWORD eventsRead;
    KEY_EVENT_RECORD keyEvent;
    DWORD waitResult;

    // Wait indefinitely for input event
    waitResult = WaitForSingleObject(hStdIn, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
        return;
    }

    // Read input event from console
    if (!ReadConsoleInput(hStdIn, &inputRecord, 1, &eventsRead) || eventsRead == 0) {
        fprintf(stderr, "Input read error: %lu\n", GetLastError());
        return;
    }

    // Process only key down events
    if (inputRecord.EventType != KEY_EVENT || !inputRecord.Event.KeyEvent.bKeyDown) {
        return;
    }

    keyEvent = inputRecord.Event.KeyEvent;

    // Debug: Uncomment to show key codes
    // printf("Key: %c (VK: %d)\n", keyEvent.uChar.AsciiChar, keyEvent.wVirtualKeyCode);

    // Handle welcome screen - any key enters editor
    if (state->welcome_screen) {
        free_lines(state);
        state->num_lines = 1;
        state->lines = malloc(sizeof(Line));
        state->lines[0].data = calloc(1, 1);
        state->lines[0].length = 0;
        state->welcome_screen = 0;
        state->cursor_row = 0;
        state->cursor_col = 0;
        return;
    }

    // Escape key returns to normal mode from any state
    if (keyEvent.wVirtualKeyCode == VK_ESCAPE) {
        state->mode = 0;
        state->command[0] = '\0';
        return;
    }

    // Process input based on current editor mode
    switch (state->mode) {
        case 0:  // Normal mode
            if (keyEvent.wVirtualKeyCode == VK_UP || keyEvent.uChar.AsciiChar == 'k') {
                move_cursor_up(state);
            } else if (keyEvent.wVirtualKeyCode == VK_DOWN || keyEvent.uChar.AsciiChar == 'j') {
                move_cursor_down(state);
            } else if (keyEvent.wVirtualKeyCode == VK_LEFT || keyEvent.uChar.AsciiChar == 'h') {
                move_cursor_left(state);
            } else if (keyEvent.wVirtualKeyCode == VK_RIGHT || keyEvent.uChar.AsciiChar == 'l') {
                move_cursor_right(state);
            } else if (keyEvent.uChar.AsciiChar == 'i') {
                state->mode = 1;  // Enter insert mode
            } else if (keyEvent.uChar.AsciiChar == ':') {
                state->mode = 2;  // Enter command mode
                state->command[0] = '\0';
            } else if (keyEvent.uChar.AsciiChar == 'x') {
                delete_char(state);  // Delete character
            }
            break;
            
        case 1:  // Insert mode
            if (keyEvent.wVirtualKeyCode == VK_BACK) {
                delete_char(state);  // Backspace
            } else if (keyEvent.wVirtualKeyCode == VK_RETURN) {
                insert_newline(state);  // Enter key
            } else if (keyEvent.uChar.AsciiChar >= 32 && keyEvent.uChar.AsciiChar <= 126) {
                insert_char(state, keyEvent.uChar.AsciiChar);  // Printable characters
            }
            break;
            
        case 2:  // Command mode
            if (keyEvent.wVirtualKeyCode == VK_BACK) {
                // Handle backspace in command
                size_t cmdLen = strlen(state->command);
                if (cmdLen > 0) state->command[cmdLen - 1] = '\0';
            } else if (keyEvent.wVirtualKeyCode == VK_RETURN) {
                // Execute command
                if (process_command(state, state->command)) {
                    cleanup_screen();
                    free_lines(state);
                    if (state->filename) free(state->filename);
                    restore_input_mode();
                    exit(0);
                }
            } else if (keyEvent.uChar.AsciiChar >= 32 && keyEvent.uChar.AsciiChar <= 126) {
                // Add character to command buffer
                size_t cmdLen = strlen(state->command);
                if (cmdLen < 255) {
                    state->command[cmdLen] = keyEvent.uChar.AsciiChar;
                    state->command[cmdLen + 1] = '\0';
                }
            }
            break;
    }
}
