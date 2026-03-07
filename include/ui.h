// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef UI_H
#define UI_H

// Include the main Raylib library so the compiler knows what 'Color' and basic drawing functions are.
#include "raylib.h"

// We include the core game structures because the UI needs to read their data 
// (like altitude, speed, mission timers, and the top 10 records) to display them on screen.
#include "player.h"
#include "race.h"
#include "leaderboard.h"


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take.
// By isolating all 2D drawing functions here, we keep our main game loop clean and focused purely on logic.

// NOTE ON RESPONSIVE DESIGN: 
// We pass 'screenWidth' and 'screenHeight' to almost every function so the UI can dynamically 
// calculate the center of the screen, ensuring it looks perfect on any monitor resolution.

// Draws the initial welcome screen with the blinking "Press Start" message.
void DrawMainMenu(int screenWidth, int screenHeight);

// Draws the 5x5 dynamic mission grid.
// It needs to know the 'currentLevel' to highlight it in GOLD, and 'maxLevels' 
// to know which boxes should be drawn in blue (available) or gray (restricted).
void DrawLevelSelectScreen(int currentLevel, int maxLevels, int screenWidth, int screenHeight);

// Draws the screen where the player chooses between the SR-71 Blackbird and the AH-64 Apache.
void DrawVehicleSelectScreen(int screenWidth, int screenHeight);

// Draws the in-flight Head-Up Display (HUD).
// This includes the altitude, throttle percentage, controls helper, and delegates 
// the mission-specific UI (like rings left or landing warnings) to the race system.
// We pass POINTERS (*player and *race) to read their data without copying massive structs into RAM every frame.
void DrawHUD(Player *player, RaceSystem *race, bool showControls, int screenWidth, int screenHeight);

// Draws the arcade-style naming screen after a victory.
// It needs the 'playerName' array to show what the user has typed so far, 
// and the 'currentVirtualKey' to show which letter the gamepad joystick is currently hovering over.
void DrawNameInputScreen(const char *playerName, char currentVirtualKey, int screenWidth, int screenHeight);

// Draws the classic Top 10 High-Score terminal.
// We pass a POINTER to the leaderboard to read the names, times, and vehicles efficiently.
void DrawLeaderboardScreen(Leaderboard *lb, int screenWidth, int screenHeight);

#endif // Ends the include guard