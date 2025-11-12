#include<stdio.h>
#include<unistd.h>
#include<curses.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<signal.h>
#include "highscore.h"
#include "statistics.h"

volatile sig_atomic_t interrupted = 0;
volatile sig_atomic_t pause_request = 0;
volatile sig_atomic_t paused_flag = 0;
volatile sig_atomic_t game_over = 0;

static time_t last_moss_update = 0;
static int moss_reversed = 0;
static int wave_offset = 0;

int score = 0;
int lives = 3;
int fish_caught_total = 0;
int hooks_missed_total = 0;
int paused = 0;
int time_limit = 30;
time_t start_time;
time_t pause_start = 0;
time_t total_pause_time = 0;
char player_name[20] = "Player";

int get_remaining_time(){
    time_t current = time(NULL);
    int elapsed = (int)difftime(current, start_time) - total_pause_time;

    if(paused && pause_start > 0){
        elapsed -= (int)difftime(current, pause_start);
    }
    int remaining = time_limit - elapsed;
    return remaining > 0 ? remaining : 0;
}

void handle_sigtstp(int sig) {
    pause_request = 1; 
}

void handle_sigint(int sig) {
    interrupted = 1;
}

void toggle_pause(){
    if(paused){
        if(pause_start > 0){
            total_pause_time += (int)difftime(time(NULL), pause_start);
            pause_start = 0;
        }
        paused = 0;
    }else{
        pause_start = time(NULL);
        paused = 1;
    }
}

typedef struct {
    int pos;
    int row;
    int dir;
    int width;
    int framesPerStep;
    int frameCounter;
} Fish;

void draw_border() {
    const char* wave = "~~~~    ";
    
    time_t current_time = time(NULL);
    if (current_time != last_moss_update) {
        moss_reversed = !moss_reversed;
        wave_offset = (wave_offset + 1) % 8;
        last_moss_update = current_time;
    }
    
    const char* moss_normal[] = {"(", " )", "(", " )", "("};
    const char* moss_reverse[] = {" )", "(", " )", "(", " )"};
    const char** moss = moss_reversed ? moss_reverse : moss_normal;
    
    int moss_len = 5;
    int moss_width = 2;
    int start_moss = COLS - moss_width - 1;
    if (start_moss < 0) start_moss = 0;
    int start_rowm = LINES - moss_len - 1;
    if (start_rowm < 0) start_rowm = 0;
    
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
    
    const char* castle[] = {
        "               T~~", "               |", "              /^\\", "             /   \\",
        " _   _   _  /     \\  _   _   _", "[ ]_[ ]_[ ]/ _   _ \\[ ]_[ ]_[ ]",
        "|_=__-_ =_|_[ ]_[ ]_|_=-___-__|", " | _- =  | =_ = _    |= _=   |",
        " |= -[]  |- = _ =    |_-=_[] |", " | =_    |= - ___    | =_ =  |",
        " |=  []- |-  /| |\\   |=_ =[] |", " |- =_   | =|tesla|  |- = -  |",
        " |_______|__|_|_|_|__|_______|",
    };
    int castle_len = sizeof(castle)/sizeof(castle[0]);
    int castle_width = 0;
    for (int i = 0; i < castle_len; i++) {
        int len = strlen(castle[i]);
        if (len > castle_width) castle_width = len;
    }
    
    int wave_len = strlen(wave);
    for (int i = 0; i < COLS - 1; i++) {
        mvaddch(LINES / 4, i, '~');
        mvaddch(LINES / 4 + 1, i, wave[(i + wave_offset) % wave_len]);
        mvaddch(LINES / 4 + 2, i, wave[(i + wave_offset + 2) % wave_len]);
    }
    
    int start_col = COLS - castle_width - 1;
    if (start_col < 0) start_col = 0;
    int start_row = LINES - castle_len - 1;
    if (start_row < 0) start_row = 0;
    
    for(int i = 0; i < castle_len; i++){
        mvaddstr(start_row + i, start_col, castle[i]);
    }
}

static void erase_boat(int boat_x) {
    int water_y = LINES / 4;
    int boat_y = water_y - 2;
    int max_w = 14;
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

static void draw_boat_and_hook(int boat_x, int hook_depth) {
    const char *boat_top = "    __/\\__   ";
    const char *boat_hull = "___/______\\__";
    int water_y = LINES / 4;
    int boat_y = water_y - 2;
    int bw = (int)strlen(boat_hull);
    if (boat_x < 0) boat_x = 0;
    if (boat_x + bw >= COLS) boat_x = COLS - bw - 1;
    if (boat_y >= 0) mvaddnstr(boat_y, boat_x, boat_top, COLS - boat_x);
    if (boat_y + 1 >= 0) mvaddnstr(boat_y + 1, boat_x, boat_hull, COLS - boat_x);

    int line_x = boat_x + bw / 2;
    int line_start_y = water_y + 1;
    int line_end_y = line_start_y + hook_depth;
    if (line_x >= 0 && line_x < COLS) {
        for (int y = line_start_y; y < LINES; y++) {
            mvaddch(y, line_x, ' ');
        }
        for (int y = line_start_y; y < line_end_y && y < LINES; y++) {
            mvaddch(y, line_x, '|');
        }
        if (line_end_y < LINES) mvaddch(line_end_y, line_x, 'J');
    }
}

void draw_fish(Fish* fish, const char** left_fish, const char** right_fish, int lines) {
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
}

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

void get_player_name() {
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║          WELCOME TO FISHING GAME!              ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\nEnter your name (max 19 characters): ");
    fgets(player_name, sizeof(player_name), stdin);
    player_name[strcspn(player_name, "\n")] = 0; // Remove newline
    if (strlen(player_name) == 0) {
        strcpy(player_name, "Player");
    }
}

void show_main_menu() {
    int choice;
    
    while(1) {
        printf("\n╔════════════════════════════════════════════════╗\n");
        printf("║              FISHING GAME MENU                 ║\n");
        printf("╠════════════════════════════════════════════════╣\n");
        printf("║  1. Start New Game                             ║\n");
        printf("║  2. View High Scores                           ║\n");
        printf("║  3. View Game History                          ║\n");
        printf("║  4. View Player Statistics                     ║\n");
        printf("║  5. Exit                                       ║\n");
        printf("╚════════════════════════════════════════════════╝\n");
        printf("\nEnter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n'); // Clear input buffer
            printf("Invalid input! Please enter a number.\n");
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
                    printf("Enter player name: ");
                    fgets(search_name, sizeof(search_name), stdin);
                    search_name[strcspn(search_name, "\n")] = 0;
                    display_player_stats(search_name);
                }
                break;
            case 5:
                printf("\nThank you for playing! Goodbye!\n");
                exit(0);
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}

int main(){
    int quit_requested = 0;
    srand(time(NULL));
    
    // Get player name and show menu
    get_player_name();
    show_main_menu();

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(0);

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    start_time = time(NULL);

    const char* left_fish[] = {" /,", "<')=<", " \\`"};
    const char* right_fish[] = {" ,'", "=>('>", " '/"};
    
    int fish_lines = 3;
    int fish_width = 5;
    
    Fish fishes[10];
    int pond_top = LINES / 4 + 3;
    int pond_bottom = LINES - 1;
    int water_span = (pond_bottom - pond_top - fish_lines);
    if (water_span < 3) water_span = 3;
    int band = water_span / 3;
    int top_start = pond_top;
    int top_end = pond_top + band;
    int mid_start = pond_top + band;
    int mid_end = pond_top + 2 * band;
    int bot_start = pond_top + 2 * band;
    int bot_end = pond_bottom - fish_lines;
    if (top_end <= top_start) top_end = top_start + 1;
    if (mid_end <= mid_start) mid_end = mid_start + 1;
    if (bot_end <= bot_start) bot_end = bot_start + 1;

    for (int i = 0; i < 10; i++) {
        fishes[i].pos = (COLS > fish_width) ? rand() % (COLS - fish_width) : 0;
        if (i < 4) {
            int span = mid_end - mid_start;
            fishes[i].row = mid_start + (span > 0 ? rand() % span : 0);
        } else if (i < 6) {
            int span = bot_end - bot_start;
            fishes[i].row = bot_start + (span > 0 ? rand() % span : 0);
        } else {
            int span = top_end - top_start;
            fishes[i].row = top_start + (span > 0 ? rand() % span : 0);
        }
        fishes[i].dir = (rand() % 2) * 2 - 1;
        fishes[i].width = fish_width;
        fishes[i].framesPerStep = 1 + rand() % 6;
        fishes[i].frameCounter = 0;
    }

    int speed = 3;
    int boat_x = COLS / 4;
    int hook_depth = 0;
    int hook_lowering = 0;
    int max_hook_depth = (LINES - (LINES/4) - 4);
    if (max_hook_depth < 0) max_hook_depth = 0;
    static int prev_boat_x = -1;
    int hook_miss_penalized = 0;
    int fish_caught_this_attempt = 0;

    while(!game_over){
        draw_border();

        if (interrupted && !quit_requested) {
            quit_requested = 1;
            interrupted = 0;
            mvprintw(2, (COLS - 30) / 2, "Are you sure you want to quit? (y/n)");
            refresh();
        }

        int ch = getch();
        if (quit_requested && ch != ERR) {
            if (ch == 'y' || ch == 'Y') {
                break;
            } else if (ch == 'n' || ch == 'N') {
                move(LINES / 2, (COLS - 30) / 2);
                clrtoeol();
                refresh();
                quit_requested = 0;
                clear();
            }
        }

        if (!quit_requested) {
            mvprintw(1, 2, "Player: %s | Press Ctrl+C to quit, Ctrl+Z to pause", player_name);
        }

        if (pause_request) {
            toggle_pause();
            pause_request = 0;
            paused_flag = !paused_flag;
            if (paused_flag) {
                mvprintw(LINES / 2, 5,"***GAME PAUSED ***");
                mvprintw((LINES / 2) + 1,5, "Press 'p' or Ctrl+Z to resume");
            } else {
                move(LINES / 2, (COLS - 30) / 2);
                clrtoeol();
                refresh();
            }
        }

        if (paused_flag) {
            if (ch == 'p' || ch == 'P') {
                toggle_pause();
                paused_flag = 0;
                move(LINES / 2, (COLS - 30) / 2);
                clrtoeol();
                refresh();
                clear();
            }
            refresh();
            usleep(50000);
            continue;
        }
        
        if (prev_boat_x >= 0 && prev_boat_x != boat_x) {
            erase_boat(prev_boat_x);
        }
        prev_boat_x = boat_x;
        
        for (int i = 0; i < 10; i++) {
            draw_fish(&fishes[i], left_fish, right_fish, fish_lines);
        }

        draw_boat_and_hook(boat_x, hook_depth);
        refresh();
        usleep(speed * 10000);

        for (int i = 0; i < 10; i++) {
            erase_fish(&fishes[i], fish_lines);
        }

        for (int i = 0; i < 10; i++) {
            fishes[i].frameCounter++;
            if (fishes[i].frameCounter >= fishes[i].framesPerStep) {
                fishes[i].frameCounter = 0;
                fishes[i].pos += fishes[i].dir;
                if (fishes[i].dir == 1 && fishes[i].pos + fishes[i].width >= COLS) {
                    fishes[i].pos = 0;
                } else if (fishes[i].dir == -1 && fishes[i].pos <= 0) {
                    int max_start = (COLS > fishes[i].width) ? (COLS - fishes[i].width) : 0;
                    fishes[i].pos = max_start;
                }
            }
        }

        if (hook_lowering == 1 && hook_depth == 0) {
            hook_miss_penalized = 0;
            fish_caught_this_attempt = 0;
        }

        if (hook_lowering == 1 && hook_depth < max_hook_depth) {
            hook_depth++;
        } else if (hook_lowering == -1 && hook_depth > 0) {
            hook_depth--;
        }
        
        if (hook_depth >= max_hook_depth && hook_lowering == 1) {
            hook_lowering = -1;
        }
        
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

        int water_y = LINES / 4;
        int hook_x = boat_x + (int)strlen("___/______\\__") / 2;
        int hook_y = water_y + 1 + hook_depth;
        
        for (int i = 0; i < 10; i++) {
            if (hook_depth > 0 && hook_y >= fishes[i].row && hook_y < fishes[i].row + fish_lines) {
                if (hook_x >= fishes[i].pos && hook_x < fishes[i].pos + fishes[i].width) {
                    int points = (3 - speed) + 1;
                    score += points;
                    fish_caught_total++;
                    fishes[i].pos = rand() % (COLS - fishes[i].width);
                    int span = (mid_start - top_start);
                    fishes[i].row = top_start + (span > 0 ? rand() % span : 0);
                    fishes[i].dir = (rand() % 2) * 2 - 1;
                    fish_caught_this_attempt = 1;
                    hook_lowering = -1;
                    break;
                }
            }
        }

        int time_left = get_remaining_time();
        if(time_left <= 0 && !paused){
            game_over = 1;
            break;
        }
        
        char lives_display[20];
        strcpy(lives_display, "Lives: ");
        for (int i = 0; i < lives; i++) {
            strcat(lives_display, "* ");
        }
        char speed_display[30];
        sprintf(speed_display, "Speed: %d (%dx points)", speed, (3 - speed) + 1);
        
        if(paused){
            mvprintw(0, 2, "[PAUSED] | %s | %s | score:%d | time:%2ds", lives_display, speed_display, score, time_left);
        }else{
            mvprintw(0, 2, "a:left d:right h:hook s:slower f:faster | %s | %s | score:%d | time:%2ds", lives_display, speed_display, score, time_left);
        }

        if(!paused){
            if(ch != ERR){
                if(ch =='q' || ch =='Q'){
                    break;
                }else if (ch == 'a') {
                    if (boat_x > 0) boat_x--;
                }else if (ch == 'd') {
                    if (boat_x < COLS - 12) boat_x++;
                }else if (ch == 'h' || ch == 'H') {
                    if (hook_lowering == 0) hook_lowering = 1;
                }else if(ch == ' '){
                    for (int i = 0; i < 10; i++) {
                        fishes[i].dir = -fishes[i].dir;
                    }
                }else if(ch == 's'){
                    if(speed < 10){
                        speed += 1;
                    }
                }else if(ch == 'f'){
                    if(speed > 0){
                        speed -= 1;
                    }
                }
            }
        }
    }
    
    endwin();
    
    // Save game statistics
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
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║           GAME OVER - FINAL RESULTS            ║\n");
    printf("╠════════════════════════════════════════════════╣\n");
    printf("║ Player: %-38s ║\n", player_name);
    printf("║ Final Score: %-33d ║\n", score);
    printf("║ Fish Caught: %-33d ║\n", fish_caught_total);
    printf("║ Hooks Missed: %-32d ║\n", hooks_missed_total);
    printf("║ Lives Remaining: %-29d ║\n", lives);
    printf("║ Final Speed Level: %-27d ║\n", speed);
    printf("╚════════════════════════════════════════════════╝\n");
    
    // Check and save high score
    if (is_highscore(score)) {
        printf("\n🎉 CONGRATULATIONS! You achieved a HIGH SCORE! 🎉\n");
        if (add_highscore(player_name, score, speed) == 0) {
            printf("Your score has been saved to the high score table!\n");
        }
    }
    
    display_highscores();
    
    return 0;
}