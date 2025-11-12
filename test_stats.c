#include "statistics.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    GameStats stats;
    stats.timestamp = time(NULL);
    strcpy(stats.player_name, "TestPlayer");
    stats.final_score = 120;
    stats.fish_caught = 15;
    stats.hooks_missed = 2;
    stats.speed_level = 2;
    stats.lives_remaining = 1;
    stats.game_duration = 30;
    
    log_game_stats(&stats);
    display_game_history();
    display_player_stats("TestPlayer");
    
    return 0;
}