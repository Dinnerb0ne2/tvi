#include <tvi.h>

// display: command line arguments
static void print_help() {
    printf("Tiny VI Editor (tvi) - Minimal vi-like editor\n");
    printf("Usage:\n");
    printf("  tvi [file]         Edit specified file\n");
    printf("  tvi -h, --help     Show this help message\n");
    printf("  tvi -usage         Show internal editor commands\n");
    printf("  tvi -info          Show program information\n");
}

// display: internal editor commands
static void print_usage() {
    printf("Internal Commands:\n");
    printf("Normal Mode:\n");
    printf("  i          Enter insert mode\n");
    printf("  :          Enter command mode\n");
    printf("  h/j/k/l    Move cursor (left/down/up/right)\n");
    printf("  x          Delete character at cursor\n");
    printf("  ESC        Return to normal mode\n");
    printf("\nCommand Mode:\n");
    printf("  :w         Save current file\n");
    printf("  :q         Quit editor\n");
    printf("  :wq        Save and quit\n");
    printf("  :set number   Show line numbers\n");
    printf("  :set nonumber Hide line numbers\n");
}

// display: program information
static void print_info() {
    printf("Tiny VI Editor (tvi) v1.0\n");
    printf("A minimal vi-like text editor for Windows\n");
    printf("Supports cmd and PowerShell\n");
    printf("Copyright (C) 2023\n");
}

static int parse_arguments(int argc, char* argv[], char** filename) {
    if (argc == 1) {
        *filename = NULL;
        return 0;
    }
    
    if (argc == 2) {
        // Check for help/info flags
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
            return 1; // Exit after displaying
        } else if (strcmp(argv[1], "-usage") == 0) {
            print_usage();
            return 1; // Exit after displaying
        } else if (strcmp(argv[1], "-info") == 0) {
            print_info();
            return 1; // Exit after displaying
        } else {
            // Treat as filename
            *filename = argv[1];
            return 0;
        }
    }
    
    // Too many arguments
    fprintf(stderr, "Error: Too many arguments\n");
    fprintf(stderr, "Use 'tvi -h' for help\n");
    return 1;
}

// Main entry point of the Tiny VI editor
int main(int argc, char* argv[]) {
    EditorState state;
    char* filename = NULL;
    
    // Parse command line arguments
    if (parse_arguments(argc, argv, &filename) != 0) {
        return 0; // Exit if we displayed info/help
    }
    
    // Initialize core editor state and screen system
    init_editor(&state);
    init_screen(&state);
    
    // Handle file loading if filename provided
    if (filename) {
        state.filename = _strdup(filename);
        load_file(&state, filename);
        state.welcome_screen = 0; // Disable welcome screen
    } else {
        // No file provided - display welcome screen
        show_welcome_screen(&state);
        state.welcome_screen = 1;
    }
    
    // Main editor loop - runs until user exits
    while (1) {
        // Track current terminal dimensions
        int old_rows = state.screen_rows;
        int old_cols = state.screen_cols;
        
        // Update terminal size information
        update_terminal_size(&state);
        
        // Refresh screen if terminal size changed
        if (old_rows != state.screen_rows || old_cols != state.screen_cols) {
            refresh_screen(&state);
        }
        
        // Process user input events
        handle_input(&state);
        
        // Redraw screen with current state
        refresh_screen(&state);
    }
    
    // Cleanup resources (theoretical reach - loop runs indefinitely)
    free_lines(&state);       // Free allocated text lines
    free(state.filename);     // Free stored filename
    cleanup_screen();         // Restore terminal to original state
    
    return 0;
}
    