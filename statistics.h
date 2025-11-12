#ifndef STATISTICS_H
#define STATISTICS_H

#include <time.h>

#define STATS_FILE "game_stats.log"
#define MAX_LOG_ENTRIES 100

typedef struct {
    time_t timestamp;
    char player_name[20];
    int final_score;
    int fish_caught;
    int hooks_missed;
    int speed_level;
    int lives_remaining;
    int game_duration;
} GameStats;

// Function prototypes
int log_game_stats(GameStats* stats);
int load_game_history(GameStats history[], int max_entries);
void display_game_history();
void display_player_stats(const char* player_name);

#endif
