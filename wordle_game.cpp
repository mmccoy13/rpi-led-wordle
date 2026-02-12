#include "led-matrix.h"
#include "graphics.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <cmath>
#include <chrono> 
#include <signal.h> 

using namespace rgb_matrix;

// --- Global Matrix Pointer for Signal Handling ---
RGBMatrix *global_matrix = NULL;

// --- Colors ---
const Color COLOR_GREEN(106, 170, 100);
const Color COLOR_YELLOW(201, 180, 88);
const Color COLOR_NOT_IN_WORD(20, 20, 20); 
const Color COLOR_OUTLINE(58, 58, 60);      
const Color COLOR_WHITE(255, 255, 255);
const Color COLOR_BLACK(0, 0, 0);
const Color COLOR_RED(200, 50, 50);
const Color COLOR_KEY_DEFAULT(70, 70, 70); 
const Color COLOR_GOLD(255, 215, 0);

struct Tile {
    char letter = ' ';
    Color color = COLOR_NOT_IN_WORD;
    bool revealed = false; 
};

struct GameStats {
    int played = 0;
    int wins = 0;
    int currentStreak = 0;
    int maxStreak = 0;
};

std::atomic<bool> running(true);

// --- Signal Handler ---
void InterruptHandler(int signo) {
    (void)signo; 
    if (global_matrix) {
        global_matrix->Clear(); 
        delete global_matrix;
        global_matrix = NULL;   
    }
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
    exit(0);
}

class WordleGame {
public:
    std::string targetWord = "PIZZA";
    std::unordered_set<std::string> dictionary;
    Tile grid[6][5];
    int currentRow = 0, currentCol = 0;
    
    bool gameOver = false;
    bool gameWon = false;
    std::chrono::steady_clock::time_point endTime; 
    
    GameStats stats;
    bool statsSaved = false;

    int keyStates[26] = {0}; 
    int pendingKeyStates[5] = {0}; 
    int shakeFrames = 0;
    int flipFrames = 0;
    int flipTileIndex = -1;
    int animCounter = 0; 
    
    // --- Animation State ---
    int popFrames = 0;
    int popCol = -1;
    const int MAX_POP_FRAMES = 8; 

    // --- Save Stats ---
    void saveStats() {
        std::ofstream statsFile("stats.txt");
        if (statsFile.is_open()) {
            statsFile << stats.played << " " << stats.wins << " " << stats.currentStreak << " " << stats.maxStreak;
            statsFile.close();
        }
    }

    void loadFiles() {
        std::ifstream dictFile("python/valid_guesses.txt");
        std::string w;
        while (dictFile >> w) {
            std::transform(w.begin(), w.end(), w.begin(), ::toupper);
            dictionary.insert(w);
        }
        std::ifstream ansFile("python/answer.txt");
        if (ansFile >> targetWord) {
            std::transform(targetWord.begin(), targetWord.end(), targetWord.begin(), ::toupper);
        }

        // --- Stats Logic ---
        std::ifstream statsFile("stats.txt");
        if (statsFile.is_open()) {
            statsFile >> stats.played >> stats.wins >> stats.currentStreak >> stats.maxStreak;
        } else {
            saveStats(); // Create file silently
        }
    }

    void submitGuess() {
        if (currentCol != 5 || gameOver || flipFrames > 0) return;
        std::string guess = "";
        for (int i = 0; i < 5; i++) guess += grid[currentRow][i].letter;
        
        if (!dictionary.empty() && dictionary.find(guess) == dictionary.end()) {
            shakeFrames = 30; 
            return;
        }

        std::string tempTarget = targetWord;
        std::vector<bool> targetUsed(5, false);
        std::vector<bool> guessMatched(5, false);

        for (int i = 0; i < 5; i++) {
            if (guess[i] == tempTarget[i]) {
                grid[currentRow][i].color = COLOR_GREEN;
                targetUsed[i] = true;
                guessMatched[i] = true;
                pendingKeyStates[i] = 3; 
            }
        }
        for (int i = 0; i < 5; i++) {
            if (guessMatched[i]) continue;
            bool found = false;
            for (int j = 0; j < 5; j++) {
                if (!targetUsed[j] && guess[i] == tempTarget[j]) {
                    grid[currentRow][i].color = COLOR_YELLOW;
                    targetUsed[j] = true;
                    found = true;
                    pendingKeyStates[i] = 2;
                    break;
                }
            }
            if (!found) {
                grid[currentRow][i].color = COLOR_NOT_IN_WORD;
                pendingKeyStates[i] = 1;
            }
        }
        flipFrames = 100;
        flipTileIndex = 0;
    }
};

WordleGame game;

char getch() {
    char buf = 0;
    struct termios old; 
    if (tcgetattr(0, &old) < 0) perror("tcgetattr()");
    struct termios cur = old;
    cur.c_lflag &= ~ICANON; cur.c_lflag &= ~ECHO;
    if (tcsetattr(0, TCSANOW, &cur) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror ("read()");
    if (tcsetattr(0, TCSANOW, &old) < 0) perror ("tcsetattr ~ICANON");
    return buf;
}

void inputThreadFunc() {
    while (running) {
        char c = getch(); 
        if (c == 3 || c == 27) { 
            running = false; 
            break; 
        }
        c = toupper(c);
        
        if (c == 127 || c == 8) {
            if (game.currentCol > 0 && game.flipFrames == 0 && !game.gameOver) {
                game.grid[game.currentRow][--game.currentCol].letter = ' ';
            }
        } else if (c == '\n' || c == '\r') {
            game.submitGuess(); 
        } else if (c >= 'A' && c <= 'Z') {
            if (game.currentCol < 5 && game.currentRow < 6 && game.flipFrames == 0 && !game.gameOver) {
                game.grid[game.currentRow][game.currentCol].letter = c;
                
                // Pop Animation Trigger
                game.popCol = game.currentCol;
                game.popFrames = game.MAX_POP_FRAMES; 
                
                game.currentCol++;
            }
        }
    }
}

inline void DrawOutline(FrameCanvas *canvas, int x, int y, int size, const Color& c) {
    for (int i = 0; i < size; i++) {
        canvas->SetPixel(x + i, y, c.r, c.g, c.b);            
        canvas->SetPixel(x + i, y + size - 1, c.r, c.g, c.b); 
        canvas->SetPixel(x, y + i, c.r, c.g, c.b);            
        canvas->SetPixel(x + size - 1, y + i, c.r, c.g, c.b); 
    }
}

void DrawPopTile(FrameCanvas *canvas, int x, int y, int size) {
    int offset = (size - 10) / 2;
    DrawOutline(canvas, x - offset, y - offset, size, COLOR_OUTLINE);
}

void DrawVisualKeyboard(FrameCanvas *canvas, int startY, Font &kbdFont) {
    const char* layout[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
    for (int i = 0; i < 3; i++) {
        std::string row = layout[i];
        int rowWidth = (row.length() * 5) + (row.length() - 1);
        int x = (canvas->width() - rowWidth) / 2;
        int y = startY + (i * 10); 
        for (char c : row) {
            int state = game.keyStates[c - 'A'];
            Color kColor = (state == 1) ? COLOR_BLACK : (state == 2) ? COLOR_YELLOW : (state == 3) ? COLOR_GREEN : COLOR_KEY_DEFAULT;
            
            for (int dx = 0; dx < 5; dx++) {
                for (int dy = 0; dy < 5; dy++) {
                    canvas->SetPixel(x + dx, y + dy, kColor.r, kColor.g, kColor.b);
                }
            }
            char str[2] = {c, '\0'};
            rgb_matrix::DrawText(canvas, kbdFont, x + 1, y + 3, COLOR_WHITE, NULL, str); 
            x += 6;
        }
    }
}

void RenderGrid(FrameCanvas *canvas, int startX, int startY, Font &gameFont, int maxRows) {
    int rowOffsetX = 0;
    if (game.shakeFrames > 0) {
        int offsets[] = {0, 4, 0, -4}; 
        rowOffsetX = offsets[(game.shakeFrames / 2) % 4];
    }

    for (int r = 0; r < maxRows; r++) { 
        for (int c = 0; c < 5; c++) {
            int x = startX + (c * 12);
            if (r == game.currentRow) x += rowOffsetX;
            int y = startY + (r * 11);

            int drawH = 10;
            int yOff = 0;
            
            bool isFlipping = (game.flipFrames > 0 && r == game.currentRow && c == game.flipTileIndex);
            bool isPopping = (game.popFrames > 0 && r == game.currentRow && c == game.popCol);

            if (isFlipping) {
                int f = game.flipFrames % 20; 
                drawH = std::abs(10 - f) * 10 / 10;
                yOff = (10 - drawH) / 2;
            }

            if (isPopping) {
                float progress = 1.0f - ((float)game.popFrames / (float)game.MAX_POP_FRAMES);
                int extraSize = (int)(sin(progress * M_PI) * 3.5f);
                DrawPopTile(canvas, x, y, 10 + extraSize);
            }
            else if (!game.grid[r][c].revealed && !isFlipping) {
                DrawOutline(canvas, x, y + yOff, 10, COLOR_OUTLINE);
            } 
            else if (game.grid[r][c].revealed || isFlipping) {
                Color tileCol = game.grid[r][c].revealed ? game.grid[r][c].color : COLOR_KEY_DEFAULT;
                for(int i = 0; i < 10; i++) {
                    for(int j = 0; j < drawH; j++) {
                        canvas->SetPixel(x+i, y+j+yOff, tileCol.r, tileCol.g, tileCol.b);
                    }
                }
            }

            if (game.grid[r][c].letter != ' ' && drawH > 4) {
                char str[2] = { game.grid[r][c].letter, '\0' };
                int textY = y + 8 - (10 - drawH) / 2;
                rgb_matrix::DrawText(canvas, gameFont, x + 2, textY, COLOR_WHITE, NULL, str);
                rgb_matrix::DrawText(canvas, gameFont, x + 3, textY, COLOR_WHITE, NULL, str);
            }
        }
    }
}

void UpdateGame() {
    if (game.gameOver) {
        if (!game.statsSaved) {
            game.stats.played++;
            if (game.gameWon) {
                game.stats.wins++;
                game.stats.currentStreak++;
                if (game.stats.currentStreak > game.stats.maxStreak) game.stats.maxStreak = game.stats.currentStreak;
            } else {
                game.stats.currentStreak = 0;
            }
            game.saveStats();
            game.statsSaved = true;
        }

        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - game.endTime).count();
        if (duration >= 2) {
            InterruptHandler(0); 
        }
    }

    if (game.shakeFrames > 0) game.shakeFrames--;
    if (game.popFrames > 0) game.popFrames--; 

    if (game.flipFrames > 0) {
        game.flipFrames--;
        game.flipTileIndex = 4 - (game.flipFrames / 20);
        int tileFrame = game.flipFrames % 20; 

        if (tileFrame == 10) { 
            game.grid[game.currentRow][game.flipTileIndex].revealed = true;
        }

        if (game.flipFrames == 0) {
            for(int i = 0; i < 5; i++) {
                int keyIdx = game.grid[game.currentRow][i].letter - 'A';
                int newState = game.pendingKeyStates[i];
                if (newState > game.keyStates[keyIdx]) {
                    game.keyStates[keyIdx] = newState;
                }
            }

            std::string guess = "";
            for (int i = 0; i < 5; i++) guess += game.grid[game.currentRow][i].letter;
            
            if (guess == game.targetWord) {
                game.gameOver = true;
                game.gameWon = true;
                game.endTime = std::chrono::steady_clock::now(); 
            } else if (game.currentRow == 5) {
                game.gameOver = true;
                game.gameWon = false;
                game.endTime = std::chrono::steady_clock::now(); 
            }
            
            game.currentRow++; 
            game.currentCol = 0;
        }
    }
    
    if (game.gameOver) game.animCounter++;
}

// --- UPDATED: Padding Added Above Title ---
void DrawWin(FrameCanvas *canvas, Font &victoryFont, Font &gameFont) {
    std::string text = "VICTORY";
    int width = 0; for (char c : text) width += victoryFont.CharacterWidth(c);
    
    // Increased base Y from 16 -> 22 for padding
    int bounceY = 22 + (int)(sin(game.animCounter * 0.1) * 3); 
    rgb_matrix::DrawText(canvas, victoryFont, (canvas->width() - width) / 2, bounceY, COLOR_GREEN, NULL, text.c_str());

    std::string rankText = "GREAT!";
    int rowsUsed = game.currentRow; 
    if (rowsUsed == 1) rankText = "GENIUS!";
    else if (rowsUsed == 2) rankText = "MAGNIFICENT";
    else if (rowsUsed == 3) rankText = "IMPRESSIVE";
    else if (rowsUsed == 4) rankText = "SPLENDID";
    else if (rowsUsed == 5) rankText = "GREAT";
    else rankText = "PHEW!";

    int winPct = (game.stats.wins * 100) / (game.stats.played > 0 ? game.stats.played : 1);
    std::string s1 = "Streak: " + std::to_string(game.stats.currentStreak);
    std::string s2 = "Win%: " + std::to_string(winPct);
    
    int gridHeight = rowsUsed * 11;
    int footerMargin = 5; 
    int gridY = canvas->height() - gridHeight - footerMargin;
    
    // Adjusted space calculation since title is lower
    int spaceAvailable = gridY - 30; // Was 25
    int contentStartY = 30 + (spaceAvailable / 2) - 8; 

    int rw = 0; for(char c : rankText) rw += gameFont.CharacterWidth(c);
    rgb_matrix::DrawText(canvas, gameFont, (canvas->width() - rw)/2, contentStartY, COLOR_GOLD, NULL, rankText.c_str());

    int sw1 = 0; for(char c : s1) sw1 += gameFont.CharacterWidth(c);
    int sw2 = 0; for(char c : s2) sw2 += gameFont.CharacterWidth(c);
    
    int lineSpacing = (spaceAvailable > 40) ? 10 : 7;
    
    rgb_matrix::DrawText(canvas, gameFont, (canvas->width() - sw1)/2, contentStartY + lineSpacing, COLOR_WHITE, NULL, s1.c_str());
    rgb_matrix::DrawText(canvas, gameFont, (canvas->width() - sw2)/2, contentStartY + (lineSpacing * 2), COLOR_WHITE, NULL, s2.c_str());

    RenderGrid(canvas, (canvas->width() - 58) / 2, gridY, gameFont, rowsUsed);
}

// --- UPDATED: Padding Added Above Title ---
void DrawLoss(FrameCanvas *canvas, Font &font, Font &defeatFont, Font &gameFont) {
    std::string text = "DEFEAT";
    int width = 0; for (char c : text) width += defeatFont.CharacterWidth(c);
    
    // Increased Y from 16 -> 22 for padding
    rgb_matrix::DrawText(canvas, defeatFont, (canvas->width() - width) / 2, 22, COLOR_RED, NULL, text.c_str());

    std::string wordMsg = game.targetWord;
    int wW = 0; for(char c : wordMsg) wW += font.CharacterWidth(c);
    Color targetCol = (game.animCounter % 20 < 10) ? COLOR_YELLOW : COLOR_WHITE;
    
    // Shifted word reveal down (26 -> 32)
    rgb_matrix::DrawText(canvas, font, (canvas->width() - wW) / 2, 36, targetCol, NULL, wordMsg.c_str());

    // Shifted grid down (32 -> 38)
    RenderGrid(canvas, (canvas->width() - 58) / 2, 45, gameFont, 6);
}

int main(int argc, char *argv[]) {
    RGBMatrix::Options options;
    RuntimeOptions runtime;
    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv, &options, &runtime);
    if (matrix == NULL) return 1;

    global_matrix = matrix; 
    signal(SIGINT, InterruptHandler);
    
    FrameCanvas *canvas = matrix->CreateFrameCanvas();
    Font gameFont, logoFont, victoryFont, defeatFont, keyboardFont; 
    
    if (!gameFont.LoadFont("fonts/5x6.bdf")) return 1;
    if (!logoFont.LoadFont("fonts/Karnak.bdf")) return 1;
    if (!victoryFont.LoadFont("fonts/victory.bdf")) return 1;
    if (!defeatFont.LoadFont("fonts/defeat.bdf")) return 1;
    if (!keyboardFont.LoadFont("fonts/3x3pixel.bdf")) return 1; 
    
    game.loadFiles();
    std::thread iThread(inputThreadFunc);

    while (running) {
        UpdateGame();
        canvas->Clear();
        
        if (game.gameOver) {
            if (game.gameWon) {
                DrawWin(canvas, victoryFont, gameFont);
            } else {
                DrawLoss(canvas, gameFont, defeatFont, gameFont);
            }
        } else {
            std::string logoText = "Wordle";
            int logoWidth = 0;
            for (char c : logoText) logoWidth += logoFont.CharacterWidth(c);
            rgb_matrix::DrawText(canvas, logoFont, (canvas->width() - logoWidth) / 2, 13, COLOR_WHITE, NULL, logoText.c_str());

            RenderGrid(canvas, (canvas->width() - 58) / 2, 16, gameFont, 6);
            DrawVisualKeyboard(canvas, 95, keyboardFont); 
        }

        canvas = matrix->SwapOnVSync(canvas);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    
    InterruptHandler(0);
    return 0;
}