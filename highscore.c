#include "highscore.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Load high scores from file
// System calls used: open(), read(), close(), stat()
int load_highscores(HighScore scores[], int max_scores) {
    int fd;
    struct stat file_stat;
    
    // Check if file exists using stat()
    if (stat(HIGHSCORE_FILE, &file_stat) == -1) {
        // File doesn't exist, return 0 scores
        return 0;
    }
    
    // Open file for reading
    fd = open(HIGHSCORE_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening highscore file");
        return 0;
    }
    
    // Read high scores
    int count = 0;
    ssize_t bytes_read;
    
    while (count < max_scores) {
        bytes_read = read(fd, &scores[count], sizeof(HighScore));
        if (bytes_read == 0) {
            break; // End of file
        }
        if (bytes_read == -1) {
            perror("Error reading highscore file");
            close(fd);
            return count;
        }
        if (bytes_read == sizeof(HighScore)) {
            count++;
        }
    }
    
    close(fd);
    return count;
}

// Save high scores to file
// System calls used: open(), write(), close()
int save_highscores(HighScore scores[], int count) {
    int fd;
    
    // Open file for writing (create if doesn't exist, truncate if exists)
    fd = open(HIGHSCORE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening highscore file for writing");
        return -1;
    }
    
    // Write all scores
    for (int i = 0; i < count; i++) {
        ssize_t bytes_written = write(fd, &scores[i], sizeof(HighScore));
        if (bytes_written != sizeof(HighScore)) {
            perror("Error writing highscore");
            close(fd);
            return -1;
        }
    }
    
    close(fd);
    return 0;
}

// Add a new high score
int add_highscore(const char* name, int score, int speed_level) {
    HighScore scores[MAX_HIGHSCORES];
    int count = load_highscores(scores, MAX_HIGHSCORES);
    
    // Create new score entry
    HighScore new_score;
    strncpy(new_score.name, name, MAX_NAME_LENGTH - 1);
    new_score.name[MAX_NAME_LENGTH - 1] = '\0';
    new_score.score = score;
    new_score.speed_level = speed_level;
    new_score.date = time(NULL);
    
    // Find insertion position
    int insert_pos = count;
    for (int i = 0; i < count; i++) {
        if (score > scores[i].score) {
            insert_pos = i;
            break;
        }
    }
    
    // If not in top 10, don't add
    if (insert_pos >= MAX_HIGHSCORES) {
        return 0;
    }
    
    // Shift scores down
    for (int i = MAX_HIGHSCORES - 1; i > insert_pos; i--) {
        if (i - 1 < count) {
            scores[i] = scores[i - 1];
        }
    }
    
    // Insert new score
    scores[insert_pos] = new_score;
    
    // Update count
    if (count < MAX_HIGHSCORES) {
        count++;
    }
    
    // Save updated scores
    return save_highscores(scores, count);
}

// Check if score qualifies as high score
int is_highscore(int score) {
    HighScore scores[MAX_HIGHSCORES];
    int count = load_highscores(scores, MAX_HIGHSCORES);
    
    if (count < MAX_HIGHSCORES) {
        return 1; // Less than 10 scores, any score qualifies
    }
    
    // Check if better than lowest high score
    return score > scores[count - 1].score;
}

// Display high scores (for terminal output after game)
#include <string.h>

// Display high scores (for terminal output after game)
void display_highscores() {
    HighScore scores[MAX_HIGHSCORES];
    int count = load_highscores(scores, MAX_HIGHSCORES);

    char printed_names[MAX_HIGHSCORES][32];  // store unique names
    int printed_count = 0;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║                    HIGH SCORES                        ║\n");
    printf("╠════╦══════════════════╦═══════╦═══════╦═══════════════╣\n");
    printf("║ #  ║ Name             ║ Score ║ Speed ║ Date          ║\n");
    printf("╠════╬══════════════════╬═══════╬═══════╬═══════════════╣\n");

    if (count == 0) {
        printf("║              No high scores yet!                       ║\n");
    } else {
        int rank = 1;

        for (int i = 0; i < count; i++) {

            // 1. Check if this name is already printed
            int is_duplicate = 0;
            for (int j = 0; j < printed_count; j++) {
                if (strcmp(scores[i].name, printed_names[j]) == 0) {
                    is_duplicate = 1;
                    break;
                }
            }

            if (is_duplicate)
                continue;  // skip duplicated name

            // 2. Mark this name as printed
            strcpy(printed_names[printed_count++], scores[i].name);

            // 3. Print the line
            char date_str[20];
            struct tm* tm_info = localtime(&scores[i].date);
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);

            printf("║ %-2d ║ %-16s ║ %5d ║   %d   ║ %-13s ║\n",
                   rank++,
                   scores[i].name,
                   scores[i].score,
                   scores[i].speed_level,
                   date_str);
        }
    }

    printf("╚════╩══════════════════╩═══════╩═══════╩═══════════════╝\n");
}
