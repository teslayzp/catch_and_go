#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <time.h>

#define MAX_HIGHSCORES 10
#define MAX_NAME_LENGTH 20
#define HIGHSCORE_FILE "highscores.dat"

typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
    int speed_level;
    time_t date;
} HighScore;

// Function prototypes
int load_highscores(HighScore scores[], int max_scores);
int save_highscores(HighScore scores[], int count);
int add_highscore(const char* name, int score, int speed_level);
void display_highscores();
int is_highscore(int score);

#endif
