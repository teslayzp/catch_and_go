#include<stdio.h>
#include<unistd.h>
#include<curses.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<signal.h>
#include "highscore.h"
#include "statistics.h"

// ANSI color codes for terminal output
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define cyan "\033[36m"       
#define BLUE   "\033[34m"
#define RESET  "\033[0m"

// Signal flags - MUST be volatile sig_atomic_t for signal safety
volatile sig_atomic_t pause_request = 0;  // Set when Ctrl+Z is pressed
volatile sig_atomic_t quit_request = 0;   // Set when Ctrl+C is pressed

// Animation state for moss (seaweed)
static time_t last_moss_update = 0;
static int moss_reversed = 0;
static int wave_offset = 0;

// Game state variables
int score = 0;
int lives = 3;
int fish_caught_total = 0;
int hooks_missed_total = 0;
int paused = 0;  // Regular int, not volatile - only modified in main loop
int time_limit = 30;
time_t start_time;
time_t pause_start = 0;
time_t total_pause_time = 0;
char player_name[20] = "Player";

// Global color pair IDs for ncurses
int COLOR_RED_PAIR = 1;
int COLOR_GREEN_PAIR = 2;
int COLOR_YELLOW_PAIR = 3;
int COLOR_BLUE_PAIR = 4;
int COLOR_MAGENTA_PAIR = 5;
int COLOR_CYAN_PAIR = 6;

/**
 * Calculate remaining game time accounting for pauses
 * Returns: seconds remaining (0 if time is up)
 */
int get_remaining_time(){
    time_t current = time(NULL);
    int elapsed = (int)difftime(current, start_time) - total_pause_time;

    // If currently paused, don't count current pause duration yet
    if(paused && pause_start > 0){
        elapsed -= (int)difftime(current, pause_start);
    }
    int remaining = time_limit - elapsed;
    return remaining > 0 ? remaining : 0;
}

/**
 * Signal handler for SIGTSTP (Ctrl+Z)
 * IMPORTANT: We intercept Ctrl+Z to pause game instead of suspending process
 * This prevents "suspended jobs" issues in the terminal
 */
void handle_sigtstp(int sig) {
    pause_request = 1;  // Just set flag, return immediately
    // Note: By handling this signal, we prevent default suspend behavior
}

/**
 * Signal handler for SIGINT (Ctrl+C)
 * IMPORTANT: Signal handlers must be minimal - only set flags!
 */
void handle_sigint(int sig) {
    quit_request = 1;  // Just set flag, return immediately
}

/**
 * Toggle game pause state
 * Handles pause timer tracking and screen refresh
 */
void toggle_pause(){
    time_t current = time(NULL);
    
    if(paused){
        // Resuming from pause
        if(pause_start > 0){
            total_pause_time += (int)difftime(current, pause_start);
            pause_start = 0;
        }
        paused = 0;
        
        // Restore ncurses screen state after resume
        refresh();
        clear();
    } else {
        // Entering pause state
        pause_start = current;
        paused = 1;
    }
}

/**
 * Fish structure - represents a single fish in the pond
 */
typedef struct {
    int pos;            // Horizontal position
    int row;            // Vertical position (row)
    int dir;            // Direction: -1 (left) or 1 (right)
    int width;          // Width of fish ASCII art
    int framesPerStep;  // Speed control - frames before moving
    int frameCounter;   // Current frame count
} Fish;

/**
 * Draw animated border with waves, moss, and castle
 * Creates the game environment visualization
 */
void draw_border() {
    // Wave pattern for water surface
    const char* wave = "~~~~    ";

    // Animate moss (seaweed) by toggling pattern every second
    time_t current_time = time(NULL);
    if (current_time != last_moss_update) {
        moss_reversed = !moss_reversed;
        wave_offset = (wave_offset + 1) % 8;
        last_moss_update = current_time;
    }

    // Two alternating moss patterns for animation effect
    const char* moss_normal[] = {"(", " )", "(", " )", "("};
    const char* moss_reverse[] = {" )", "(", " )", "(", " )"};
    const char** moss = moss_reversed ? moss_reverse : moss_normal;
    
    // Draw moss at various positions along the bottom
    int moss_len = 5;
    int start_rowm = LINES - moss_len - 1;
    if (start_rowm < 0) start_rowm = 0;
    
    attron(COLOR_PAIR(COLOR_GREEN_PAIR));
    // Clear old moss positions
    for(int i = 0; i < moss_len; i++){
        mvaddstr(start_rowm + i, 5, "  ");
        mvaddstr(start_rowm + i, 10, "  ");
        mvaddstr(start_rowm + i, 15, "  ");
        mvaddstr(start_rowm + i, 20, "  ");
        mvaddstr(start_rowm + i, COLS / 2, "  ");
        mvaddstr(start_rowm + i, COLS / 2 + 10, "  ");
        mvaddstr(start_rowm + i, COLS / 2 + 15, "  ");
        mvaddstr(start_rowm + i, COLS / 2 + 20, "  ");
        mvaddstr(start_rowm + i, COLS / 2 + 40, "  ");
        mvaddstr(start_rowm + i, COLS / 2 + 45, "  ");
    }
    
    // Draw new moss positions
    for(int i = 0; i < moss_len; i++){
        mvaddstr(start_rowm + i, 5, moss[i]);
        mvaddstr(start_rowm + i, 10, moss[i]);
        mvaddstr(start_rowm + i, 15, moss[i]);
        mvaddstr(start_rowm + i, 20, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2 + 10, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2 + 15, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2 + 20, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2 + 40, moss[i]);
        mvaddstr(start_rowm + i, COLS / 2 + 45, moss[i]);
    }
    attroff(COLOR_PAIR(COLOR_GREEN_PAIR));
    
    // ASCII art castle
    const char* castle[] = {
        "               T~~",
         "               |",
          "              /^\\",
           "             /   \\",
        " _   _   _  /     \\  _   _   _", 
        "[ ]_[ ]_[ ]/ _   _ \\[ ]_[ ]_[ ]",
        "|_=__-_ =_|_[ ]_[ ]_|_=-___-__|", 
        " | _- =  | =_ = _    |= _=   |",
        " |= -[]  |- = _ =    |_-=_[] |", 
        " | =_    |= - ___    | =_ =  |",
        " |=  []- |-  /| |\\   |=_ =[] |",
        " |- =_   |=|       | |- = -  |",
        " |_______|__|_|_|_|__|_______|",
    };
    int castle_len = sizeof(castle)/sizeof(castle[0]);
    int castle_width = 0;
    for (int i = 0; i < castle_len; i++) {
        int len = strlen(castle[i]);
        if (len > castle_width) castle_width = len;
    }

    // Draw animated wave pattern at water surface
    attron(COLOR_PAIR(COLOR_CYAN_PAIR));
    int wave_len = strlen(wave);
    for (int i = 0; i < COLS - 1; i++) {
        mvaddch(LINES / 4, i, '~');
        mvaddch(LINES / 4 + 1, i, wave[(i + wave_offset) % wave_len]);
        mvaddch(LINES / 4 + 2, i, wave[(i + wave_offset + 2) % wave_len]);
    }
    attroff(COLOR_PAIR(COLOR_CYAN_PAIR));

    // Draw castle in bottom-right corner
    attron(COLOR_PAIR(COLOR_BLUE_PAIR));
    int start_col = COLS - castle_width - 1;
    if (start_col < 0) start_col = 0;
    int start_row = LINES - castle_len - 1;
    if (start_row < 0) start_row = 0;
    
    for(int i = 0; i < castle_len; i++){
        mvaddstr(start_row + i, start_col, castle[i]);
    }
    mvaddstr((2 * start_row) + 1, start_col + 12, player_name);
    attroff(COLOR_PAIR(COLOR_BLUE_PAIR));
}

/**
 * Erase boat from previous position
 * Used when boat moves to avoid ghosting
 */
static void erase_boat(int boat_x) {
    int water_y = LINES / 4;
    int boat_y = water_y - 2;
    int max_w = 14;
    
    // Bounds checking
    if (boat_x < 0) boat_x = 0;
    if (boat_x + max_w >= COLS) max_w = COLS - boat_x;
    
    if (max_w > 0 && boat_y >= 0) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < max_w && (boat_x + j) < COLS; j++) {
                mvaddch(boat_y + i, boat_x + j, ' ');
            }
        }
    }
}

/**
 * Draw boat and fishing hook
 * boat_x: horizontal position of boat
 * hook_depth: how deep the hook is lowered
 */
static void draw_boat_and_hook(int boat_x, int hook_depth) {
    // Boat ASCII art
    const char *boat_top = "    __/\\__   ";
    const char *boat_hull = "___/______\\__";
    
    int water_y = LINES / 4;
    int boat_y = water_y - 2;
    int bw = (int)strlen(boat_hull);
    
    // Draw boat with bounds checking
    attron(COLOR_PAIR(COLOR_RED_PAIR));
    if (boat_x < 0) boat_x = 0;
    if (boat_x + bw >= COLS) boat_x = COLS - bw - 1;
    if (boat_y >= 0) mvaddnstr(boat_y, boat_x, boat_top, COLS - boat_x);
    if (boat_y + 1 >= 0) mvaddnstr(boat_y + 1, boat_x, boat_hull, COLS - boat_x);
    attroff(COLOR_PAIR(COLOR_RED_PAIR));

    // Calculate hook position (center of boat)
    int line_x = boat_x + bw / 2;
    int line_start_y = water_y + 1;
    int line_end_y = line_start_y + hook_depth;
    
    // Draw fishing line and hook
    attron(COLOR_PAIR(COLOR_MAGENTA_PAIR));
    if (line_x >= 0 && line_x < COLS) {
        // Clear entire vertical line first
        for (int y = line_start_y; y < LINES; y++) {
            mvaddch(y, line_x, ' ');
        }
        // Draw fishing line
        for (int y = line_start_y; y < line_end_y && y < LINES; y++) {
            mvaddch(y, line_x, '|');
        }
        // Draw hook at end
        if (line_end_y < LINES) mvaddch(line_end_y, line_x, 'J');
    }
    attroff(COLOR_PAIR(COLOR_MAGENTA_PAIR));
}

/**
 * Draw a fish at its current position
 * Uses appropriate left or right facing ASCII art based on direction
 */
void draw_fish(Fish* fish, const char** left_fish, const char** right_fish, int lines) {
    attron(COLOR_PAIR(COLOR_CYAN_PAIR));
    const char** art = (fish->dir == -1) ? left_fish : right_fish;
    
    for (int i = 0; i < lines; i++) {
        const char* s = art[i];
        int len = (int)strlen(s);
        int avail = COLS - fish->pos;
        if (avail > 0) {
            if (len > avail) len = avail;
            mvaddnstr(fish->row + i, fish->pos, s, len);
        }
    }
    attroff(COLOR_PAIR(COLOR_CYAN_PAIR));
}

/**
 * Erase fish from screen at current position
 * Used before moving fish to new position
 */
void erase_fish(Fish* fish, int lines) {
    static const char blanks[] = "                                                                ";
    int erase_len = fish->width;
    if (erase_len > (int)sizeof(blanks) - 1) erase_len = (int)sizeof(blanks) - 1;
    
    for (int i = 0; i < lines; i++) {
        int avail = COLS - fish->pos;
        int len = erase_len;
        if (avail <= 0) continue;
        if (len > avail) len = avail;
        mvaddnstr(fish->row + i, fish->pos, blanks, len);
    }
}

/**
 * Get player name at game start
 * Displays welcome message and prompts for name
 */
void get_player_name() {
    printf(cyan "\n╔════════════════════════════════════════════════╗\n" RESET);
    printf(cyan "║          WELCOME TO FISHING GAME!              ║\n" RESET);
    printf(cyan "╚════════════════════════════════════════════════╝\n" RESET);
    printf(cyan "\nEnter your name (max 19 characters): " RESET);
    fgets(player_name, sizeof(player_name), stdin);
    player_name[strcspn(player_name, "\n")] = 0; // Remove newline
    if (strlen(player_name) == 0) {
        strcpy(player_name, "Player");
    }
}

/**
 * Display main menu with game options
 * Loops until player starts game or exits
 */
void show_main_menu() {
    int choice;
    
    while(1) {
        printf(cyan "\n╔════════════════════════════════════════════════╗\n" RESET);
        printf(cyan "║              FISHING GAME MENU                 ║\n" RESET);
        printf(cyan "╠════════════════════════════════════════════════╣\n" RESET);
        printf(cyan "║  1. Start New Game                             ║\n" RESET);
        printf(cyan "║  2. View High Scores                           ║\n" RESET);
        printf(cyan "║  3. View Game History                          ║\n" RESET);
        printf(cyan "║  4. View Player Statistics                     ║\n" RESET);
        printf(cyan "║  5. Exit                                       ║\n" RESET);
        printf(cyan "╚════════════════════════════════════════════════╝\n" RESET);
        printf(cyan "\nEnter your choice: " RESET);
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n'); // Clear input buffer
            printf(RED "Invalid input! Please enter a number.\n" RESET);
            continue;
        }
        while(getchar() != '\n'); // Clear remaining input
        
        switch(choice) {
            case 1:
                return; // Start game
            case 2:
                display_highscores();
                break;
            case 3:
                display_game_history();
                break;
            case 4:
                {
                    char search_name[20];
                    printf(GREEN "Enter player name: " RESET);
                    fgets(search_name, sizeof(search_name), stdin);
                    search_name[strcspn(search_name, "\n")] = 0;
                    display_player_stats(search_name);
                }
                break;
            case 5:
                printf(BLUE "\nThank you for playing! Goodbye!\n" RESET);
                exit(0);
            default:
                printf(RED "Invalid choice! Please try again.\n" RESET);
        }
    }
}

/**
 * Cleanup function called before program exit
 * Restores terminal to normal state
 */
void cleanup_terminal() {
    endwin();  // End ncurses mode
    
    // Restore default signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    
    // Reset terminal attributes
    printf("\033[?25h");  // Show cursor
    printf("\033[0m");    // Reset colors
    fflush(stdout);
}

/**
 * Main game function
 */
int main(){
    while(1){
        srand(time(NULL));
        
        // Get player name and show menu
        get_player_name();
        show_main_menu();

        // Register cleanup function to run on exit
        atexit(cleanup_terminal);

        // Initialize ncurses
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        timeout(0);
        keypad(stdscr, TRUE);  // Enable special keys

        // Initialize color pairs
        start_color();
        use_default_colors();
        init_pair(COLOR_RED_PAIR, COLOR_RED, -1);
        init_pair(COLOR_GREEN_PAIR, COLOR_GREEN, -1);
        init_pair(COLOR_YELLOW_PAIR, COLOR_YELLOW, -1);
        init_pair(COLOR_BLUE_PAIR, COLOR_BLUE, -1);
        init_pair(COLOR_MAGENTA_PAIR, COLOR_MAGENTA, -1);
        init_pair(COLOR_CYAN_PAIR, COLOR_CYAN, -1);

        // Set up signal handlers using sigaction (more portable than signal())
        struct sigaction sa_tstp, sa_int;
        
        sa_tstp.sa_handler = handle_sigtstp;
        sigemptyset(&sa_tstp.sa_mask);
        sa_tstp.sa_flags = 0;
        sigaction(SIGTSTP, &sa_tstp, NULL);
        
        sa_int.sa_handler = handle_sigint;
        sigemptyset(&sa_int.sa_mask);
        sa_int.sa_flags = 0;
        sigaction(SIGINT, &sa_int, NULL);

        start_time = time(NULL);

        // Fish ASCII art (left and right facing)
        const char* left_fish[] = {" /,", "<')=<", " \\`"};
        const char* right_fish[] = {" ,'", "=>('>", " '/"};
        
        int fish_lines = 3;
        int fish_width = 5;
        
        // Initialize fish array
        Fish fishes[10];
        int pond_top = LINES / 4 + 3;
        int pond_bottom = LINES - 1;
        int water_span = (pond_bottom - pond_top - fish_lines);
        if (water_span < 3) water_span = 3;
        
        // Divide pond into three depth zones
        int band = water_span / 3;
        int top_start = pond_top;
        int top_end = pond_top + band;
        int mid_start = pond_top + band;
        int mid_end = pond_top + 2 * band;
        int bot_start = pond_top + 2 * band;
        int bot_end = pond_bottom - fish_lines;
        
        // Ensure valid ranges
        if (top_end <= top_start) top_end = top_start + 1;
        if (mid_end <= mid_start) mid_end = mid_start + 1;
        if (bot_end <= bot_start) bot_end = bot_start + 1;

        // Spawn fish at random positions and depths
        for (int i = 0; i < 10; i++) {
            fishes[i].pos = (COLS > fish_width) ? rand() % (COLS - fish_width) : 0;
            
            // Distribute fish across depth zones
            if (i < 4) {
                // Middle depth
                int span = mid_end - mid_start;
                fishes[i].row = mid_start + (span > 0 ? rand() % span : 0);
            } else if (i < 6) {
                // Deep
                int span = bot_end - bot_start;
                fishes[i].row = bot_start + (span > 0 ? rand() % span : 0);
            } else {
                // Shallow
                int span = top_end - top_start;
                fishes[i].row = top_start + (span > 0 ? rand() % span : 0);
            }
            
            fishes[i].dir = (rand() % 2) * 2 - 1;  // -1 or 1
            fishes[i].width = fish_width;
            fishes[i].framesPerStep = 1 + rand() % 6;  // Random speed
            fishes[i].frameCounter = 0;
        }

        // Game state variables
        int speed = 4;
        int boat_x = COLS / 4;
        int hook_depth = 0;
        int hook_lowering = 0;  // 0=idle, 1=lowering, -1=raising
        int max_hook_depth = (LINES - (LINES/4) - 4);
        if (max_hook_depth < 0) max_hook_depth = 0;
        
        static int prev_boat_x = -1;
        int hook_miss_penalized = 0;
        int fish_caught_this_attempt = 0;
        int game_over = 0;
        int quit_confirmation_mode = 0;  // Track if waiting for quit confirmation

        // Main game loop
        while(!game_over){
            draw_border();

            // Handle quit request (Ctrl+C pressed)
            if (quit_request && !quit_confirmation_mode) {
                if (!paused) {
                    toggle_pause();  // Pause game while confirming
                }
                quit_confirmation_mode = 1;
                attron(COLOR_PAIR(COLOR_RED_PAIR));
                mvprintw(LINES / 2, (COLS - 60) / 2, "Are you sure you want to quit? (y/n)");
                attroff(COLOR_PAIR(COLOR_RED));
                refresh();
            }

            int ch = getch();
            
            // Handle quit confirmation
            if (quit_confirmation_mode && ch != ERR) {
                if (ch == 'y' || ch == 'Y') {
                    game_over = 1;
                    clear();
                    endwin();
                    break;
                } else if (ch == 'n' || ch == 'N') {
                    toggle_pause();  // Unpause game
                    quit_request = 0;
                    quit_confirmation_mode = 0;
                    move(LINES / 2, 0);
                    clrtoeol();
                    clear();
                }
            }

            // Show help text when not in confirmation mode
            if (!quit_confirmation_mode) {
                attron(COLOR_PAIR(COLOR_BLUE));
                mvprintw(1, 2, "Player: %s | Press Ctrl+C to quit, Ctrl+Z to pause", player_name);
                attroff(COLOR_PAIR(COLOR_BLUE));
            }

            // Handle pause request (Ctrl+Z pressed)
            if (pause_request) {
                toggle_pause();
                pause_request = 0;
                
                if (paused) {
                    // Show pause message
                    attron(COLOR_PAIR(COLOR_YELLOW_PAIR));
                    mvprintw((LINES / 2) + 1, (COLS - 40) / 2, "*** GAME PAUSED ***");
                    mvprintw((LINES / 2) + 2, (COLS - 40) / 2, "Press 'p' or Ctrl+Z to resume");
                    attroff(COLOR_PAIR(COLOR_YELLOW_PAIR));
                    refresh();
                }
            }

            // If paused, only handle resume command
            if (paused) {
                // Allow both 'p' and Ctrl+Z to resume
                if (ch == 'p' || ch == 'P' || pause_request) {
                    if (pause_request) pause_request = 0;
                    toggle_pause();
                    // Clear pause message area
                    for (int i = 0; i < 4; i++) {
                        move(LINES / 2 + i, 0);
                        clrtoeol();
                    }
                    clear();
                }
                refresh();
                usleep(50000);
                continue;
            }
            
            // Erase boat at old position if it moved
            if (prev_boat_x >= 0 && prev_boat_x != boat_x) {
                erase_boat(prev_boat_x);
            }
            prev_boat_x = boat_x;
            
            // Draw all fish
            for (int i = 0; i < 10; i++) {
                draw_fish(&fishes[i], left_fish, right_fish, fish_lines);
            }

            // Draw boat and hook
            draw_boat_and_hook(boat_x, hook_depth);
            refresh();
            usleep(speed * 10000);

            // Erase fish before moving them
            for (int i = 0; i < 10; i++) {
                erase_fish(&fishes[i], fish_lines);
            }

            // Update fish positions
            for (int i = 0; i < 10; i++) {
                fishes[i].frameCounter++;
                if (fishes[i].frameCounter >= fishes[i].framesPerStep) {
                    fishes[i].frameCounter = 0;
                    fishes[i].pos += fishes[i].dir;
                    
                    // Wrap around screen edges
                    if (fishes[i].dir == 1 && fishes[i].pos + fishes[i].width >= COLS) {
                        fishes[i].pos = 0;
                    } else if (fishes[i].dir == -1 && fishes[i].pos <= 0) {
                        int max_start = (COLS > fishes[i].width) ? (COLS - fishes[i].width) : 0;
                        fishes[i].pos = max_start;
                    }
                }
            }

            // Reset attempt tracking when hook returns to top
            if (hook_lowering == 1 && hook_depth == 0) {
                hook_miss_penalized = 0;
                fish_caught_this_attempt = 0;
            }

            // Update hook position
            if (hook_lowering == 1 && hook_depth < max_hook_depth) {
                hook_depth++;  // Lower hook
            } else if (hook_lowering == -1 && hook_depth > 0) {
                hook_depth--;  // Raise hook
            }
            
            // Auto-raise when hook reaches bottom
            if (hook_depth >= max_hook_depth && hook_lowering == 1) {
                hook_lowering = -1;
            }
            
            // Penalize for missing fish when hook returns to top
            if (hook_depth <= 0 && hook_lowering == -1) {
                hook_lowering = 0;
                if (!fish_caught_this_attempt && !hook_miss_penalized) {
                    lives--;
                    hooks_missed_total++;
                    hook_miss_penalized = 1;
                    if (lives <= 0) {
                        game_over = 1;
                        break;
                    }
                }
            }

            // Check for fish collision with hook
            int water_y = LINES / 4;
            int hook_x = boat_x + (int)strlen("___/______\\__") / 2;
            int hook_y = water_y + 1 + hook_depth;
            
            for (int i = 0; i < 10; i++) {
                // Check if hook is at fish depth
                if (hook_depth > 0 && hook_y >= fishes[i].row && hook_y < fishes[i].row + fish_lines) {
                    // Check if hook is touching fish horizontally
                    if (hook_x >= fishes[i].pos && hook_x < fishes[i].pos + fishes[i].width) {
                        // Caught a fish!
                        int points = (3 - speed) + 1;  // Faster speed = more points
                        score += points;
                        fish_caught_total++;
                        
                        // Respawn fish at random position
                        fishes[i].pos = rand() % (COLS - fishes[i].width);
                        int span = (mid_end - top_start);
                        fishes[i].row = top_start + (span > 0 ? rand() % span : 0);
                        fishes[i].dir = (rand() % 2) * 2 - 1;
                        
                        fish_caught_this_attempt = 1;
                        hook_lowering = -1;  // Auto-raise hook
                        break;
                    }
                }
            }

            // Check time limit
            int time_left = get_remaining_time();
            if(time_left <= 0 && !paused){
                game_over = 1;
                break;
            }
            

            // Display game status
            char lives_display[20];
            strcpy(lives_display,"Lives: ");
            for (int i = 0; i < lives; i++) {
                strcat(lives_display,"* ");
            }

            char speed_display[35];
            attron(COLOR_PAIR(COLOR_GREEN));
            sprintf(speed_display, "Speed: %d (%dx points)", speed, (3 - speed) + 1);
            
            if(paused){
                mvprintw(0, 2, "[PAUSED] | %s | %s | score:%d | time:%2ds   ", 
                        lives_display, speed_display, score, time_left);
            }else{
                mvprintw(0, 2, "a:left d:right h:hook s:slower f:faster | %s | %s | score:%d | time:%2ds   ", 
                        lives_display, speed_display, score, time_left);
            }
            attroff(COLOR_PAIR(COLOR_GREEN));
            // Handle player input (only when not paused)
            if(!paused && !quit_confirmation_mode){
                if(ch != ERR){
                    if(ch =='q' || ch =='Q'){
                        clear();
                        endwin();
                        break;  // Direct quit with 'q'
                    }else if (ch == 'a' || ch == 'A') {
                        // Move boat left
                        if (boat_x > 0) boat_x--;
                    }else if (ch == 'd' || ch == 'D') {
                        // Move boat right
                        if (boat_x < COLS - 12) boat_x++;
                    }else if (ch == 'h' || ch == 'H') {
                        // Drop hook (only if not already lowering)
                        if (hook_lowering == 0) hook_lowering = 1;
                    }else if(ch == ' '){
                        // Easter egg: space reverses all fish
                        for (int i = 0; i < 10; i++) {
                            fishes[i].dir = -fishes[i].dir;
                        }
                    }else if(ch == 's' || ch == 'S'){
                        // Decrease speed (slower game, fewer points)
                        if(speed < 6){
                            speed += 1;
                        }
                    }else if(ch == 'f' || ch == 'F'){
                        // Increase speed (faster game, more points)
                        if(speed > 1){
                            speed -= 1;
                        }
                    }
                }
            }
        }
        
        // Game ended - cleanup ncurses
        endwin();
        
        // Restore terminal to normal state
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        printf("\033[?25h");  // Show cursor
        printf("\033[0m");    // Reset colors
        fflush(stdout);
        
        // Save game statistics to file
        GameStats stats;
        stats.timestamp = time(NULL);
        strncpy(stats.player_name, player_name, sizeof(stats.player_name) - 1);
        stats.final_score = score;
        stats.fish_caught = fish_caught_total;
        stats.hooks_missed = hooks_missed_total;
        stats.speed_level = speed;
        stats.lives_remaining = lives;
        stats.game_duration = (int)difftime(time(NULL), start_time) - total_pause_time;
        log_game_stats(&stats);
        
        // Display final statistics
        printf("\n\n");
        printf(cyan "╔════════════════════════════════════════════════╗\n" RESET);
        printf(cyan "║           GAME OVER - FINAL RESULTS            ║\n" RESET);
        printf(cyan "╠════════════════════════════════════════════════╣\n" RESET);
        printf(cyan "║ Player: %-38s ║\n" RESET, player_name);
        printf(cyan "║ Final Score: %-33d ║\n" RESET, score);
        printf(cyan "║ Fish Caught: %-33d ║\n" RESET, fish_caught_total);
        printf(cyan "║ Hooks Missed: %-32d ║\n"RESET, hooks_missed_total);
        printf(cyan "║ Lives Remaining: %-29d ║\n" RESET, lives);
        printf(cyan "║ Final Speed Level: %-27d ║\n" RESET, speed);
        printf(cyan "╚════════════════════════════════════════════════╝\n" RESET);
        
        // Check and save high score
        if (is_highscore(score)) {
            printf(GREEN "\n🎉 CONGRATULATIONS! You achieved a HIGH SCORE! 🎉\n" RESET);
            if (add_highscore(player_name, score, speed) == 0) {
                printf(GREEN "Your score has been saved to the high score table!\n" RESET);
            }
        }
        
        display_highscores();
        
    }
    
    return 0;
}