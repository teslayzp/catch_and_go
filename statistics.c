#include "statistics.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define green "\033[32m"
#define reset "\033[0m"
#define red   "\033[31m"
#define blue  "\033[34m"


// Log game statistics to file
// System calls used: open(), write(), close(), lseek(), stat()
int log_game_stats(GameStats* stats) {
    int fd;
    struct stat file_stat;
    
    // Open file for appending (create if doesn't exist)
    fd = open(STATS_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening stats file");
        return -1;
    }
    
    // Get current file size using stat
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file stats");
        close(fd);
        return -1;
    }
    
    // Write stats to file
    ssize_t bytes_written = write(fd, stats, sizeof(GameStats));
    if (bytes_written != sizeof(GameStats)) {
        perror("Error writing stats");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// Load game history from file
// System calls used: open(), read(), close()
int load_game_history(GameStats history[], int max_entries) {
    int fd;
    
    // Open file for reading
    fd = open(STATS_FILE, O_RDONLY);
    if (fd == -1) {
        // File doesn't exist yet
        return 0;
    }
    
    int count = 0;
    ssize_t bytes_read;
    
    // Read all entries
    while (count < max_entries) {
        bytes_read = read(fd, &history[count], sizeof(GameStats));
        if (bytes_read == 0) {
            break; // End of file
        }
        if (bytes_read == -1) {
            perror("Error reading stats file");
            close(fd);
            return count;
        }
        if (bytes_read == sizeof(GameStats)) {
            count++;
        }
    }
    
    close(fd);
    return count;
}

// Display complete game history
void display_game_history() {
    GameStats history[MAX_LOG_ENTRIES];
    int count = load_game_history(history, MAX_LOG_ENTRIES);
    
    printf("\n");
    printf(blue "╔═════════════════════════════════════════════════════════════════════╗\n" reset);
    printf(blue "║                         GAME HISTORY                                ║\n" reset);
    printf(blue "╠════╦══════════════╦══════════════╦═══════╦═══════╦═══════╦═══════╦══╣\n" reset);
    printf(blue "║ #  ║ Date         ║ Player       ║ Score ║ Catch ║ Miss  ║ Speed ║ L║\n" reset);
    printf(blue "╠════╬══════════════╬══════════════╬═══════╬═══════╬═══════╬═══════╬══╣\n" reset);
    
    if (count == 0) {
        printf(blue "║                    No game history available                         ║\n" reset);
    } else {
        // Display most recent games first
        for (int i = count - 1; i >= 0 && i >= count - 20; i--) {
            char date_str[12];
            struct tm* tm_info = localtime(&history[i].timestamp);
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
            
            printf(blue "║ %-2d ║ %-12s ║ %-12s ║ %5d ║ %5d ║ %5d ║   %d   ║ %d║\n" reset,
                   count - i,
                   date_str,
                   history[i].player_name,
                   history[i].final_score,
                   history[i].fish_caught,
                   history[i].hooks_missed,
                   history[i].speed_level,
                   history[i].lives_remaining);
        }
    }
    
    printf(blue "╚════╩══════════════╩══════════════╩═══════╩═══════╩═══════╩═══════╩══╝\n" reset);
    printf(red "L = Lives Remaining\n" reset);
}

// Display statistics for specific player
void display_player_stats(const char* player_name) {
    GameStats history[MAX_LOG_ENTRIES];
    int count = load_game_history(history, MAX_LOG_ENTRIES);
    
    int total_games = 0;
    int total_score = 0;
    int total_caught = 0;
    int total_missed = 0;
    int best_score = 0;
    
    // Calculate player statistics
    for (int i = 0; i < count; i++) {
        if (strcmp(history[i].player_name, player_name) == 0) {
            total_games++;
            total_score += history[i].final_score;
            total_caught += history[i].fish_caught;
            total_missed += history[i].hooks_missed;
            if (history[i].final_score > best_score) {
                best_score = history[i].final_score;
            }
        }
    }
    
    if (total_games == 0) {
        printf(green "\nNo statistics found for player: %s\n" reset, player_name);
        return;
    }
    
    printf("\n");
    printf(green "╔════════════════════════════════════════════════╗\n" reset);
    printf(green "║         PLAYER STATISTICS: %-16s    ║\n" reset, player_name);
    printf(green "╠════════════════════════════════════════════════╣\n" reset);
    printf(green "║ Total Games Played:        %4d                ║\n" reset, total_games);
    printf(green "║ Best Score:                %4d                ║\n" reset, best_score);
    printf(green "║ Average Score:             %4d                ║\n" reset, total_games > 0 ? total_score / total_games : 0);
    printf(green "║ Total Fish Caught:         %4d                ║\n" reset, total_caught);
    printf(green "║ Total Hooks Missed:        %4d                ║\n" reset, total_missed);
    printf(green "║ Catch Rate:                %3d%%                ║\n" reset, 
           (total_caught + total_missed) > 0 ? (total_caught * 100) / (total_caught + total_missed) : 0);
    printf(green "╚════════════════════════════════════════════════╝\n" reset);
}