#include <tvi.h>

int main(int argc, char* argv[]) {
    EditorState state;
    init_editor(&state);
    init_screen(&state);
    
    // Check for filename argument
    if (argc > 1) {
        state.filename = _strdup(argv[1]);
        load_file(&state, argv[1]);
        state.welcome_screen = 0;
    } else {
        // No file provided, show welcome screen
        show_welcome_screen(&state);
        state.welcome_screen = 1;
    }
    
    // Main loop
    while (1) {
        // Check if terminal size changed
        int old_rows = state.screen_rows;
        int old_cols = state.screen_cols;
        get_terminal_size(&state);
        
        // Refresh screen if size changed
        if (old_rows != state.screen_rows || old_cols != state.screen_cols) {
            refresh_screen(&state);
        }
        
        // Handle user input
        handle_input(&state);
        
        // Refresh display
        refresh_screen(&state);
    }
    
    // Cleanup resources
    free_lines(&state);
    free(state.filename);
    cleanup_screen();
    return 0;
}
    