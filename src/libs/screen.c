#include <tvi.h>

static HANDLE hStdOut;
static CHAR_INFO* buffer;
static COORD buffer_size;
static SMALL_RECT write_region;

#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)

// initial
void init_screen(EditorState* state) {
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error getting output handle\n");
        exit(1);
    }

    update_terminal_size(state);
    
    buffer_size.X = state->screen_cols;
    buffer_size.Y = state->screen_rows;
    buffer = malloc(buffer_size.X * buffer_size.Y * sizeof(CHAR_INFO));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for buffer\n");
        exit(1);
    }
    
    // write
    write_region.Left = 0;
    write_region.Top = 0;
    write_region.Right = buffer_size.X - 1;
    write_region.Bottom = buffer_size.Y - 1;

    // hide cursor
    CONSOLE_CURSOR_INFO cursor_info = {0};
    cursor_info.dwSize = 1;
    cursor_info.bVisible = 0;
    SetConsoleCursorInfo(hStdOut, &cursor_info);
    
    SetConsoleOutputCP(CP_UTF8);
}

// cls
void cleanup_screen() {
    CONSOLE_CURSOR_INFO cursor_info = {0};
    cursor_info.dwSize = 1;
    cursor_info.bVisible = 1;
    SetConsoleCursorInfo(hStdOut, &cursor_info);
    
    free(buffer);
    
    SetConsoleOutputCP(CP_OEMCP);
}

// update
void update_terminal_size(EditorState* state) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        state->screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        state->screen_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        
        if (buffer_size.X != state->screen_cols || buffer_size.Y != state->screen_rows) {
            buffer_size.X = state->screen_cols;
            buffer_size.Y = state->screen_rows;
            buffer = realloc(buffer, buffer_size.X * buffer_size.Y * sizeof(CHAR_INFO));
            
            write_region.Right = buffer_size.X - 1;
            write_region.Bottom = buffer_size.Y - 1;
        }
    }
}

void clear_buffer(WORD attr) {
    for (int y = 0; y < buffer_size.Y; y++) {
        for (int x = 0; x < buffer_size.X; x++) {
            buffer[y * buffer_size.X + x].Char.AsciiChar = ' ';
            buffer[y * buffer_size.X + x].Attributes = attr;
        }
    }
}

// char
void buffer_putchar(int x, int y, char c, WORD attr) {
    if (x >= 0 && x < buffer_size.X && y >= 0 && y < buffer_size.Y) {
        buffer[y * buffer_size.X + x].Char.AsciiChar = c;
        buffer[y * buffer_size.X + x].Attributes = attr;
    }
}

// string
void buffer_puts(int x, int y, const char* str, WORD attr) {
    int i = 0;
    while (str[i] && x + i < buffer_size.X) {
        buffer_putchar(x + i, y, str[i], attr);
        i++;
    }
}

// refresh
void flush_buffer() {
    COORD buffer_coord = {0, 0};
    WriteConsoleOutputA(hStdOut, buffer, buffer_size, buffer_coord, &write_region);
}

void get_terminal_size(EditorState* state) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        state->screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        state->screen_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        state->screen_cols = 80;
        state->screen_rows = 24;
    }
}


void draw_border(EditorState* state) {}

void draw_lines(EditorState* state) {
    WORD text_attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;  // 白色文本
    WORD mode_attr = FOREGROUND_YELLOW;
    
    // welcome
    if (state->welcome_screen) {
        int mid_row = state->screen_rows / 2;
        int mid_col = state->screen_cols / 2;
        
        buffer_puts(mid_col - 10, mid_row - 3, "Tiny VI Editor (tvi)", text_attr);
        buffer_puts(mid_col - 15, mid_row - 1, "Commands: :w (save), :q (quit), :wq (save & quit)", text_attr);
        buffer_puts(mid_col - 15, mid_row, "Insert mode: press 'i', exit with ESC", text_attr);
        buffer_puts(mid_col - 15, mid_row + 1, "Navigation: arrow keys or h/j/k/l", text_attr);
        buffer_puts(mid_col - 12, mid_row + 3, "Press any key to start editing", text_attr);
        return;
    }

    int display_row = 0; 
    for (int line_num = 0; line_num < state->num_lines && display_row < state->screen_rows; line_num++) {
        int col = 0;
        
        // line number
        if (state->show_numbers) {
            char num_str[12];
            snprintf(num_str, sizeof(num_str), "%6d ", line_num + 1);
            buffer_puts(col, display_row, num_str, mode_attr);
            col += 7;
        }
        
        int max_col = state->screen_cols - (state->show_numbers ? 7 : 0);  // 调整最大列数（无边界时）
        if (state->lines[line_num].length <= max_col) {
            buffer_puts(col, display_row, state->lines[line_num].data, text_attr);
        } else {
            for (int i = 0; i < max_col; i++) {
                buffer_putchar(col + i, display_row, state->lines[line_num].data[i], text_attr);
            }
        }
        
        display_row++;
    }

    // mode info
    const char* mode_str;
    switch (state->mode) {
        case 0: mode_str = "NORMAL MODE"; break;
        case 1: mode_str = "INSERT MODE"; break;
        case 2: 
            char cmd_str[256];
            size_t max_cmd_len = sizeof(cmd_str) - 10;
            if (strlen(state->command) > max_cmd_len) {
                char truncated[max_cmd_len + 1];
                strncpy(truncated, state->command, max_cmd_len);
                truncated[max_cmd_len] = '\0';
                snprintf(cmd_str, sizeof(cmd_str), "COMMAND: %s", truncated);
            } else {
                snprintf(cmd_str, sizeof(cmd_str), "COMMAND: %s", state->command);
            }
            mode_str = cmd_str;
            break;
        default: mode_str = "";
    }
    buffer_puts(0, state->screen_rows - 1, mode_str, mode_attr); // mode

    int cursor_col = state->cursor_col + (state->show_numbers ? 7 : 0);
    COORD coord = { (SHORT)cursor_col, (SHORT)state->cursor_row };
    SetConsoleCursorPosition(hStdOut, coord);
}

void refresh_screen(EditorState* state) {
    clear_buffer(0);
    
    draw_border(state);
    draw_lines(state);
    
    flush_buffer();
}
