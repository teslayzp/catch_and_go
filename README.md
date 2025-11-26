# catch_and_go
# 🎣 Fishing Game - System Programming Project

Course: ELEC462 System Programming
Team Number: KNU-ELEC462-Team-9
Members: Ye Zaw Phyo, 전민영, 정영재  

## 📖 Project Overview

An interactive terminal-based fishing game built in C using system calls and ncurses library. Players control a fishing boat, catch fish at different speeds for varying points, and compete for high scores while managing limited lives and time.

### Motivation
We chose this project to demonstrate practical system programming concepts through an engaging game that incorporates file I/O, process management, signal handling, and real-time user interaction.

---

## ✨ Core Functionalities

### 1. **Interactive Fishing Game**
- Real-time boat movement and hook control
- Multiple fish with independent AI behavior
- Speed-based scoring system (1x to 4x multipliers)
- Lives system with 3 chances
- 30-second time limit
- Pause/resume functionality

### 2. **High Score System** 📊
- Persistent storage of top 10 scores
- Player name tracking
- Score comparison and ranking
- Date-stamped entries
- Automatic save after each game

### 3. **Game Statistics & History** 📈
- Complete game session logging
- Track fish caught, hooks missed, speed level
- View past game history
- Player-specific statistics
- Performance analytics (catch rate, averages)

---

## 🔧 System Calls Used

| System Call | Usage | File |
|------------|-------|------|
| `open()` | Open score/stats files | highscore.c, statistics.c |
| `read()` | Load high scores and game history | highscore.c, statistics.c |
| `write()` | Save scores and statistics | highscore.c, statistics.c |
| `close()` | Close file descriptors | highscore.c, statistics.c |
| `stat()`/`fstat()` | Check file existence and size | highscore.c, statistics.c |
| `lseek()` | File positioning for appends | statistics.c |
| `signal()` | Handle Ctrl+C and Ctrl+Z | catch.c |
| `time()` | Game timer and timestamps | catch.c, highscore.c, statistics.c |

**Total: 8 different system calls** ✅

---

## 📁 Project Structure

```
catch_and_go/
├── catch.c              # Main game loop and logic
├── highscore.c         # High score file operations
├── highscore.h         # High score interface
├── statistics.c        # Game statistics logging
├── statistics.h        # Statistics interface
├── Makefile           # Build automation
├── README.md          # This file
├── ss.gif             # Game interface
├── highscores.dat     # Generated: High score storage
└── game_stats.log     # Generated: Game history log
```

---

## 🚀 Building and Running

### Prerequisites
- **OS:** Ubuntu 24.04 (or compatible Linux)
- **Compiler:** GCC
- **Libraries:** ncurses

### Installation

1. **Install dependencies:**
```bash
make install-deps
```
Or manually:
```bash
sudo apt-get update
sudo apt-get install -y libncurses5-dev
```

2. **Build the project:**
```bash
make
```

3. **Run the game:**
```bash
make run
```
Or directly:
```bash
./catch_and_go
```

### Makefile Commands

```bash
make               # Build the project
make run           # Build and run
make clean         # Remove build files
make cleanall      # Remove build + data files
make install-deps  # Install required libraries
make help          # Show all commands
```

---

## 🎮 How to Play

### Game Controls

| Key | Action |
|-----|--------|
| `a` | Move boat left |
| `d` | Move boat right |
| `h` | Lower/raise hook |
| `f` | Increase speed (faster game, more points) |
| `s` | Decrease speed (slower game, fewer points) |
| `Space` | Reverse all fish directions |
| `Ctrl+Z` or `p` | Pause/Resume |
| `Ctrl+C` | Quit (with confirmation) |
| `q` | Quick quit |

---

### Game Mechanics

**Objective:** Catch as many fish as possible within 30 seconds!

**Speed System:**
- Speed 3 (default): 1 point per fish
- Speed 2: 2 points per fish
- Speed 1: 3 points per fish
- Speed 0 (max): 4 points per fish

**Lives:** You have 3 lives. Lose a life when hook returns without catching fish.

---

## 📚 References
- Bruce Molay."Understanding Unix/Linux Programming"
- Linux man pages: open(2), read(2), write(2), signal(2)
- ncurses Programming Guide
- ELEC462 Course Materials
---

## 📄 License

This project is created for educational purposes as part of KNU-ELEC462 System Programming course.

---

## 🙋 Support

For questions or issues, contact: shainewaiyar@gmail.com, mym0314@naver.com, jyj0522@kakao.com

**Enjoy fishing! 🎣**
