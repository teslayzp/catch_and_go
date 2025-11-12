#include "highscore.h"
#include <stdio.h>

int main() {
    // Test adding scores
    add_highscore("Alice", 100, 2);
    add_highscore("Bob", 150, 1);
    add_highscore("Charlie", 80, 3);
    
    // Display results
    display_highscores();
    
    return 0;
}