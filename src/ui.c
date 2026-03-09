// Include string library to use string manipulation functions.
#include <string.h>

// We include our own header files.
// We need  race.h to use the shared DrawTextOutlined and GetLevelName functions.
#include "ui.h"
#include "race.h"


// --- 1. MAIN MENU SCREEN ---
void DrawMainMenu(int screenWidth, int screenHeight) {
    // 1. Draw the main title.
    // We use MeasureText to find exactly how many pixels wide the text is.
    // Subtracting this from the screenWidth and dividing by 2 guarantees perfect centering on any monitor.
    const char *title = "SIMPLE FLIGHT SIMULATOR";
    int titleWidth = MeasureText(title, 50); 
    DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 50, DARKBLUE);

    // 2. Draw the author/credits.
    const char *author = "by Benito Fernandez";
    int authorWidth = MeasureText(author, 35);
    DrawText(author, (screenWidth - authorWidth) / 2, screenHeight * 0.35f, 35, WHITE);

    // 3. Draw the blinking prompt based on the connected hardware.
    const char *msg;
    if (IsGamepadAvailable(0)) {
        msg = "PRESS [START] TO BEGIN";
    } else {
        msg = "PRESS [ENTER] TO BEGIN";
    }
    
    int msgWidth = MeasureText(msg, 30);
    
    // Blinking logic: GetTime() returns seconds. Multiplying by 2 and using modulo 2 
    // creates an alternating 0 and 1 every half second.
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawText(msg, (screenWidth - msgWidth) / 2, screenHeight * 0.6f, 30, GRAY);
    }
}


// --- 2. LEVEL SELECT SCREEN ---
void DrawLevelSelectScreen(int currentLevel, int maxLevels, int screenWidth, int screenHeight) {
    // 1. Main screen title.
    const char *title = "SELECT CIRCUIT";
    int titleWidth = MeasureText(title, 40); 
    DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.1f, 40, DARKBLUE);
    
    // 2. Fetch and draw the specific mission name dynamically.
    char currentName[50] = { 0 }; 
    GetLevelName(currentLevel, currentName); // Modifies our string buffer.
    
    const char *lvlText = TextFormat("< LEVEL %d: %s >", currentLevel, currentName);
    int lvlWidth = MeasureText(lvlText, 30);
    
    // Color logic: Gold if the file exists (playable), Gray if it doesn't ("RESTRICTED AREA").
    Color titleColor;
    if (strcmp(currentName, "RESTRICTED AREA") != 0) {
        titleColor = GOLD;
    } else {
        titleColor = GRAY;
    }
    DrawText(lvlText, (screenWidth - lvlWidth) / 2, screenHeight * 0.18f, 30, titleColor);
    
    // 3. Draw the 5x5 Level Grid.
    int gridCols = 5;
    float slotSize = 80.0f;
    float padding = 20.0f;
    
    // Calculate starting X and Y to center the entire block of 25 squares.
    float startX = (screenWidth - (gridCols * slotSize + (gridCols - 1) * padding)) / 2;
    float startY = screenHeight * 0.3f;

    for (int i = 0; i < 25; i++) { 
        int row = i / gridCols; // Integer division drops the decimal (e.g., 7 / 5 = 1st row).
        int col = i % gridCols; // Modulo gets the remainder (e.g., 7 % 5 = 2nd column).
        int levelNum = i + 1;

        // Build the mathematical rectangle for this specific box.
        Rectangle slotRect = { startX + col * (slotSize + padding), startY + row * (slotSize + padding), slotSize, slotSize };
        
        // Dynamic box coloring.
        Color boxColor;
        if (levelNum == currentLevel) {
            boxColor = GOLD;      // Currently selected by the user.
        } else if (levelNum <= maxLevels) {
            boxColor = DARKBLUE;  // Playable level.
        } else {
            boxColor = LIGHTGRAY; // Future/unlocked level.
        }
        
        DrawRectangleRec(slotRect, boxColor);
        DrawRectangleLinesEx(slotRect, 2, WHITE);

        // Only draw the number inside if the level actually exists.
        if (levelNum <= maxLevels) {
            DrawText(TextFormat("%d", levelNum), slotRect.x + 30, slotRect.y + 25, 30, WHITE);
        }
    }

    // 4. Instructions.
    const char *msg;
    if (IsGamepadAvailable(0)) {
        msg = "D-PAD/LEFT STICK to navigate, [A] to confirm";
    } else {
        msg = "ARROWS to navigate, [ENTER] to confirm";
    }
    int msgWidth = MeasureText(msg, 20);
    DrawText(msg, (screenWidth - msgWidth) / 2, screenHeight * 0.85f, 20, GRAY);
}


// --- 3. VEHICLE SELECT SCREEN ---
void DrawVehicleSelectScreen(int screenWidth, int screenHeight) {
    const char *title = "SELECT AIRCRAFT";
    int titleWidth = MeasureText(title, 50); 
    DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 50, DARKBLUE);
    
    const char *opt1;
    const char *opt2;
    if (IsGamepadAvailable(0)) {
        opt1 = "Press [X] for SR-71 Blackbird";
        opt2 = "Press [Y] for AH-64 Apache";
    } else {
        opt1 = "Press [1] for SR-71 Blackbird";
        opt2 = "Press [2] for AH-64 Apache";
    }

    int opt1Width = MeasureText(opt1, 30); 
    DrawText(opt1, (screenWidth - opt1Width) / 2, screenHeight * 0.45f, 30, DARKGRAY);
    
    int opt2Width = MeasureText(opt2, 30);
    DrawText(opt2, (screenWidth - opt2Width) / 2, screenHeight * 0.52f, 30, DARKGRAY);
}


// --- 4. IN-FLIGHT HUD (HEAD-UP DISPLAY) ---
void DrawHUD(Player *player, RaceSystem *race, bool showControls, int screenWidth, int screenHeight) {
    
    // 1. Controls Overlay (Toggleable).
    if (showControls) {
        if (IsGamepadAvailable(0)) {
            DrawTextOutlined("LT/RT: Throttle | Left Stick: Move", 20, 20, 20, LIGHTGRAY, 2);
            DrawTextOutlined("Right Stick: Camera | [B] Restart | [A] POV", screenWidth - 450, 20, 20, LIGHTGRAY, 2);
            DrawTextOutlined("[X]/[Y] Switch Aircraft", screenWidth - 250, screenHeight - 60, 20, ORANGE, 2);
        } else {
            DrawTextOutlined("W/S: Throttle | A/D: Yaw/Roll | SPACE/SHIFT: Pitch", 20, 20, 20, LIGHTGRAY, 2);
            DrawTextOutlined("Arrows: Camera | [R] Restart | [C] POV", screenWidth - 410, 20, 20, LIGHTGRAY, 2);
            DrawTextOutlined("[1]/[2] Switch Aircraft", screenWidth - 250, screenHeight - 55, 20, ORANGE, 2);
        }
    } else {
        const char *msg;
        if (IsGamepadAvailable(0)) {
            msg = "Press [Menu] to show controls";
        } else {
            msg = "Press [H] to show controls";
        }
        DrawTextOutlined(msg, 20, 20, 14, LIGHTGRAY, 1);
    }

    // 2. Delegate the mission-specific UI (Rings left, landing timers) to the Race Manager.
    DrawRaceUI(race);

    // 3. Draw the universal aeronautical telemetry (Altitude and Power).
    DrawTextOutlined(TextFormat("ALTITUDE: %.0f ft", player->position.y * 10.0f), 20, screenHeight - 100, 20, LIME, 2);

    float maxThrottle;
    if (player->type == VEHICLE_PLANE) {
        maxThrottle = 0.8f;
    } else {
        maxThrottle = 0.4f;
    }
    
    // Convert the negative throttle float into a clean positive percentage integer.
    int powerPercentage = (int)((-player->throttle / maxThrottle) * 100.0f);
    DrawTextOutlined(TextFormat("POWER: %d %%", powerPercentage), 20, screenHeight - 60, 20, LIME, 2);
}


// --- 5. POST-MISSION NAME INPUT SCREEN ---
void DrawNameInputScreen(const char *playerName, char currentVirtualKey, int screenWidth, int screenHeight) {
    // 1. Titles.
    const char *title = "NEW FLIGHT RECORD!";
    DrawText(title, (screenWidth - MeasureText(title, 40)) / 2, screenHeight * 0.15f, 40, GOLD);
    
    const char *prompt = "ENTER YOUR CALLSIGN, PILOT:";
    DrawText(prompt, (screenWidth - MeasureText(prompt, 30)) / 2, screenHeight * 0.35f, 30, WHITE);
    
    // 2. Draw the actual name being typed inside a classic console-style bracket.
    // The underscore '_' acts as a retro blinking cursor.
    const char *nameDisplay = TextFormat("[ %s_ ]", playerName);
    DrawText(nameDisplay, (screenWidth - MeasureText(nameDisplay, 40)) / 2, screenHeight * 0.45f, 40, LIME);

    // 3. Draw the virtual arcade wheel ONLY if a gamepad is detected.
    if (IsGamepadAvailable(0)) {
        const char *virtualUI = TextFormat("<- [ %c ] ->", currentVirtualKey);
        DrawText(virtualUI, (screenWidth - MeasureText(virtualUI, 40)) / 2, screenHeight * 0.60f, 40, ORANGE);
    }
    
    // 4. Input instructions.
    const char *row1;
    const char *row2;
    const char *row3;

    if (IsGamepadAvailable(0)) {
        row1 = "LEFT STICK to Navigate";
        row2 = "[A] Select Letter  |  [X] Delete";
        row3 = "[START] Confirm";
    } else {
        row1 = "Type using your KEYBOARD";
        row2 = "[BACKSPACE] Delete Letter";
        row3 = "[ENTER] Confirm";
    }

    DrawText(row1, (screenWidth - MeasureText(row1, 20)) / 2, screenHeight * 0.70f, 20, GRAY);
    DrawText(row2, (screenWidth - MeasureText(row2, 20)) / 2, screenHeight * 0.75f, 20, GRAY);
    
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawText(row3, (screenWidth - MeasureText(row3, 30)) / 2, screenHeight * 0.85f, 30, LIGHTGRAY);
    }
}


// --- 6. LEADERBOARD TERMINAL ---
void DrawLeaderboardScreen(Leaderboard *lb, int screenWidth, int screenHeight) {
    // A different background to make it feel like an old computer terminal.
    ClearBackground(DARKBLUE); 
    
    const char *title = "--- TOP 10 PILOTS ---";
    DrawText(title, (screenWidth - MeasureText(title, 40)) / 2, screenHeight * 0.1f, 40, GOLD);
    
    // Base layout coordinates.
    int startY = screenHeight * 0.25f;
    int spacing = 35;
    
    // Column Headers.
    DrawText("PILOT", screenWidth * 0.2f, startY - 40, 20, GRAY);
    DrawText("TIME", screenWidth * 0.5f, startY - 40, 20, GRAY);
    DrawText("VEHICLE", screenWidth * 0.75f, startY - 40, 20, GRAY);

    // Loop through the active records and print them list-style.
    for (int i = 0; i < lb->count; i++) {
        // Format the strings.
        const char *recordStr = TextFormat("%d. %s", i + 1, lb->entries[i].name);
        const char *timeStr = TextFormat("%.2f s", lb->entries[i].time);
        
        const char *vehStr;
        if (lb->entries[i].vehicle == VEHICLE_PLANE) {
            vehStr = "Airplane";
        } else {
            vehStr = "Helicopter";
        }
        
        // Draw the 3 aligned columns.
        DrawText(recordStr, screenWidth * 0.2f, startY + (i * spacing), 30, WHITE);
        DrawText(timeStr,   screenWidth * 0.5f, startY + (i * spacing), 30, LIME);
        DrawText(vehStr,    screenWidth * 0.75f, startY + (i * spacing), 30, SKYBLUE);
    }
    
    // Exit instructions.
    const char *exitText;
    if (IsGamepadAvailable(0)) {
        exitText = "PRESS [B] TO RETURN TO BASE";
    } else {
        exitText = "PRESS [ENTER] TO RETURN TO BASE";
    }
    DrawText(exitText, (screenWidth - MeasureText(exitText, 20)) / 2, screenHeight * 0.9f, 20, GRAY);
}