# Makefile for Fishing Game Project
# ELEC462 System Programming

CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses
TARGET = catch_and_go
OBJS = catch.o highscore.o statistics.o

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Build successful! Run with: ./$(TARGET)"

# Compile catch.c
main.o: catch.c highscore.h statistics.h
	$(CC) $(CFLAGS) -c main.c

# Compile highscore.c
highscore.o: highscore.c highscore.h
	$(CC) $(CFLAGS) -c highscore.c

# Compile statistics.c
statistics.o: statistics.c statistics.h
	$(CC) $(CFLAGS) -c statistics.c

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned build files"

# Clean build files and data files
cleanall: clean
	rm -f highscores.dat game_stats.log
	@echo "Cleaned all files including data"

# Run the game
run: $(TARGET)
	./$(TARGET)

# Install dependencies (for Ubuntu)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libncurses5-dev libncurses-dev

# Show help
help:
	@echo "Fishing Game - Makefile Commands"
	@echo "================================="
	@echo "make          - Build the project"
	@echo "make run      - Build and run the game"
	@echo "make clean    - Remove object files and executable"
	@echo "make cleanall - Remove all files including saved data"
	@echo "make install-deps - Install required libraries (Ubuntu)"
	@echo "make help     - Show this help message"

.PHONY: all clean cleanall run install-deps help