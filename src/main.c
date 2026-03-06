// Include stdbool library to use booleans.
#include <stdbool.h>

// Include stdio library to use file input/output operations.
#include <stdio.h>

// Include string library to use string manipulation functions.
#include <string.h>

// Include math library to use advanced mathematical functions.
#include <math.h>


// Include Raylib's libraries.
#include "raylib.h"
#include "raymath.h"

// Notice we use quotes "" for our own files, and angle brackets <> for system libraries.
#include "player.h"
#include "resource_manager.h"
#include "race.h"
#include "leaderboard.h"


// --- GAME STATES (STATE MACHINE) ---
// A Finite State Machine (FSM) is a core concept in game dev.
// The game can only be in one state at a time. This prevents the player 
// from moving the plane while they are still in the main menu.
typedef enum GameState {
    STATE_MENU,
    STATE_LEVEL_SELECT,
    STATE_VEHICLE_SELECT,
    STATE_PLAYING,
    STATE_NAME_INPUT,
    STATE_LEADERBOARD
} GameState;


// --- OUTLINED TEXT ---
// Draws text with a solid border by rendering it 8 times around the center point.
static void DrawTextOutlined(const char *text, int posX, int posY, int fontSize, Color color, int outlineSize) {
    DrawText(text, posX - outlineSize, posY, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY, fontSize, BLACK);
    DrawText(text, posX, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX - outlineSize, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX - outlineSize, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX, posY, fontSize, color);
}


// -- MAIN FUNCTION --
int main(void) {
    // --- 1. INITIALIZATION (SETUP) ---
    
    // Allow the user to resize the window.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    // Open a window with a temporary size so Raylib can connect to the OS.
    InitWindow(800, 600, "Simple Flight Simulator");
    
    // Ask the OS for the current monitor's dimensions.
    int monitor = GetCurrentMonitor();
    int displayWidth = GetMonitorWidth(monitor);
    int displayHeight = GetMonitorHeight(monitor);
    
    // Calculate the maximum 4:3 resolution. 
    // We subtract 100 pixels from the height so the Windows Taskbar doesn't overlap the game.
    int targetHeight = displayHeight - 100;
    int targetWidth = (targetHeight * 4) / 3;
    
    // Safety check: just in case the user has a vertical monitor (width is smaller than height).
    if (targetWidth > displayWidth) {
        targetWidth = displayWidth - 100;
        targetHeight = (targetWidth * 3) / 4;
    }
    
    // Apply the calculated 4:3 size and center the window on the screen perfectly.
    SetWindowSize(targetWidth, targetHeight);
    SetWindowPosition((displayWidth - targetWidth) / 2, (displayHeight - targetHeight) / 2);

    // Initialize audio device before loading resources.
    InitAudioDevice();

    // Force the game to run at a stable 60 frames per second.
    SetTargetFPS(60);

    // Disable default ESC behavior
    SetExitKey(KEY_NULL);

    // Call our custom module to load heavy files into RAM.
    LoadGameResources(); 

    // Set the initial game state to show the menu first.
    GameState currentState = STATE_MENU;
    
    // Create an empty player. It will be properly initialized when the user selects a vehicle.
    Player player = { 0 };
    
    // Setup the 3D camera.
    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 20.0f, 0.0f }; // Defines which way is "up" (Y-axis).
    camera.fovy = 60.0f;                        // Field of View (zoom level).
    camera.projection = CAMERA_PERSPECTIVE;     // Gives a realistic 3D depth effect.

    // First person camera toggle.
    bool isFirstPerson = false;
    
    // Camera orbit variables.
    float cameraAngleYaw = 0.0f;   // Horizontal rotation (Left/Right).
    float cameraAnglePitch = 0.0f; // Vertical rotation (Up/Down).
    
    // Toggle to hide/show the controls UI.
    bool showControls = true;


    // --- LEVEL & RACE SETUP ---
    // We must declare the level variables before initializing the race, 
    // so we can tell the Track Designer which layout to build.
    int currentLevel = 1;
    
    // Instead of a hardcoded constant, we probe the 'data' folder 
    // to see how many level files actually exist in sequential order.
    int MAX_LEVELS = 0;
    while (true) {
        // Guess the next filename.
        const char *testFilename = TextFormat("levels/lvl%d.txt", MAX_LEVELS + 1);
        
        // Try to open it.
        FILE *testFile = fopen(testFilename, "r");
        
        if (testFile != NULL) {
            // The file exists! Close it immediately and increase our count.
            fclose(testFile);
            MAX_LEVELS++;
        } else {
            // The file doesn't exist. We have reached the end of the available levels.
            break; 
        }
    }

    // Initialize the Race System (The track and the referee) using the default level.
    RaceSystem race = InitRace(currentLevel);

    // --- LEADERBOARD & TEXT INPUT SETUP ---
    // We leave the leaderboard struct empty for now. 
    // It will be dynamically loaded when the player finishes a specific level.
    Leaderboard leaderboard = { 0 };

    char playerName[MAX_NAME_LENGTH + 1] = "\0"; 
    int letterCount = 0;                         

    // Variables for the gamepad virtual keyboard (Arcade style input).
    // This allows players without a physical keyboard to enter their names.
    const char virtualKeyboard[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ ";
    int virtualKeyboardLen = 37;
    int virtualKeyIndex = 0;

    // Timer for continuous scrolling.
    float stickScrollTimer = 0.0f;

    // Analog Stick Latches.
    // These prevent the grid selection from flying at 60 slots per second 
    // when the player holds the analog stick in a direction.
    bool stickMovedX = false;
    bool stickMovedY = false;


    // --- 2. THE MAIN GAME LOOP ---
    // This loop runs 60 times per second until the user clicks the X or presses ESC.
    while (!WindowShouldClose()) {
        
        // --- 0) GLOBAL BACK / EXIT LOGIC ---
        // We handle the ESC key (Keyboard) and the View/Back button (Gamepad).
        if (IsKeyPressed(KEY_ESCAPE) || 
           (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
            if (currentState == STATE_PLAYING || 
                currentState == STATE_VEHICLE_SELECT || 
                currentState == STATE_NAME_INPUT || 
                currentState == STATE_LEADERBOARD) 
            {
                // If flying or choosing vehicle, abort the mission and return to Level Select.
                currentState = STATE_LEVEL_SELECT;
                
                // Mute engines so they don't keep buzzing in the menu.
                StopSound(planeSound);
                StopSound(helicopterSound);
            } else if (currentState == STATE_MENU || currentState == STATE_LEVEL_SELECT) {
                // If in any other menu, close the game completely.
                break; 
            }
        }

        // --- A) UPDATE PHASE ---
        if (currentState == STATE_MENU) {
            UpdateMusicStream(menuMusic);
            if (!IsMusicStreamPlaying(menuMusic)) {
                PlayMusicStream(menuMusic);
            }
            
            // Wait for ENTER (Keyboard) or START (Gamepad) to begin the game.
            if (IsKeyPressed(KEY_ENTER) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))) {
                currentState = STATE_LEVEL_SELECT;
            }
            
        } else if (currentState == STATE_LEVEL_SELECT) {
            UpdateMusicStream(menuMusic);
            if (!IsMusicStreamPlaying(menuMusic)) {
                PlayMusicStream(menuMusic);
            }

            // Analog stick reading.
            float leftStickX = 0.0f;
            float leftStickY = 0.0f;
            if (IsGamepadAvailable(0)) {
                leftStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                leftStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            }

            // Reset the latches if the stick returns to the center (deadzone of 20%).
            if (fabsf(leftStickX) < 0.2f) stickMovedX = false;
            if (fabsf(leftStickY) < 0.2f) stickMovedY = false;

            // Grid navigation logic (5x5) with strict Row/Column wrapping.
            int idx = currentLevel - 1;
            int row = idx / 5;
            int col = idx % 5;

            // Right navigation.
            if (IsKeyPressed(KEY_RIGHT) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) ||
               (leftStickX > 0.5f && !stickMovedX)) {
                col = (col + 1) % 5;
                stickMovedX = true;
            }
            // Left navigation.
            if (IsKeyPressed(KEY_LEFT) ||
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) || 
               (leftStickX < -0.5f && !stickMovedX)) {
                col = (col - 1 + 5) % 5;
                stickMovedX = true; 
            }
            // Down navigation.
            if (IsKeyPressed(KEY_DOWN) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) || 
               (leftStickY > 0.5f && !stickMovedY)) {
                row = (row + 1) % 5;
                stickMovedY = true;
            }
            // Up navigation.
            if (IsKeyPressed(KEY_UP) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) || 
               (leftStickY < -0.5f && !stickMovedY)) {
                row = (row - 1 + 5) % 5;
                stickMovedY = true;
            }
            currentLevel = (row * 5) + col + 1;
            
            // Confirm level selection.
            if (IsKeyPressed(KEY_ENTER) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                if (currentLevel <= MAX_LEVELS) {
                    currentState = STATE_VEHICLE_SELECT;
                }
            }
            
        } else if (currentState == STATE_VEHICLE_SELECT) {
            UpdateMusicStream(menuMusic);
            
            if (IsKeyPressed(KEY_ONE) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) {
                StopMusicStream(menuMusic);
                race = InitRace(currentLevel);
                player = InitPlayer(VEHICLE_PLANE, race.startPos, race.startYaw);
                currentState = STATE_PLAYING;       
            } 
            else if (IsKeyPressed(KEY_TWO) || 
                    (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
                StopMusicStream(menuMusic);      
                race = InitRace(currentLevel);
                player = InitPlayer(VEHICLE_HELICOPTER, race.startPos, race.startYaw);
                currentState = STATE_PLAYING;            
            }
            
        } else if (currentState == STATE_PLAYING) {

            // Mid-flight vehicle switching.
            if (IsKeyPressed(KEY_ONE) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) {
                player.type = VEHICLE_PLANE;
            }
            if (IsKeyPressed(KEY_TWO) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
                player.type = VEHICLE_HELICOPTER;
            }

            // Quick restart.
            // If the player makes a mistake, press R to restart the race instantly.
            if (IsKeyPressed(KEY_R) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                race = InitRace(currentLevel);                                  // Pass the current level.
                player = InitPlayer(player.type, race.startPos, race.startYaw); // Teleports player back to origin.
            }

            // Upate the vehicle's physics.
            // Notice the '&' (address-of operator). We are passing a POINTER to our player
            // so the UpdatePlayer function can modify the real data, not a copy.
            UpdatePlayer(&player);

            // Update the race logic (Stopwatch and ring collisions).
            // We pass pointers to both the race and the player so the referee can check distances.
            UpdateRace(&race, &player);

            // Dynamic audio logic
            if (player.type == VEHICLE_PLANE) {
                // Mute helicopter if it was playing.
                if (IsSoundPlaying(helicopterSound)) {
                    StopSound(helicopterSound);
                }
                
                // Ensure the plane engine sound is looping.
                if (!IsSoundPlaying(planeSound)) {
                    PlaySound(planeSound);
                }
                
                // Modify the pitch based on the throttle.
                // Throttle is negative when moving forward, so we invert it.
                float pitch = 1.0f + (-player.throttle * 0.8f);
                SetSoundPitch(planeSound, pitch);
                
            } else if (player.type == VEHICLE_HELICOPTER) {
                // Mute plane if it was playing.
                if (IsSoundPlaying(planeSound)) {
                    StopSound(planeSound);
                }
                
                // Ensure the helicopter rotor sound is looping.
                if (!IsSoundPlaying(helicopterSound)) {
                    PlaySound(helicopterSound);
                }
                
                // The helicopter changes pitch slightly when ascending or descending.
                float pitch = 1.0f + (player.velocity.y * 0.2f);
                SetSoundPitch(helicopterSound, pitch);
            }
            
            // Dynamic logic camera.
            // Toggle camera mode when pressing 'C' (Keyboard) or 'A' (Gamepad).
            if (IsKeyPressed(KEY_C) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                isFirstPerson = !isFirstPerson;
            }

            if (!isFirstPerson) {
                // Interpolation factor for smooth camera movement. Lower values create a more cinematic, delayed response.

                if (IsGamepadAvailable(0)) {
                    // Gamepad: Absolute positioning. The camera directly mirrors the physical tilt of the right thumbstick.
                    float smoothFactor = 0.1f;

                    float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
                    float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

                    // Deadzone check. Ignores tiny inputs to prevent camera jitter caused by hardware stick drift.
                    if (fabsf(rightStickX) < 0.15f) {
                        rightStickX = 0.0f;
                    }
                    if (fabsf(rightStickY) < 0.15f) {
                        rightStickY = 0.0f;
                    }

                    // Calculate the target angle by multiplying the normalized stick input by the maximum allowed radians.
                    float targetYaw = -rightStickX * 2.5f; 
                    float targetPitch = rightStickY * 1.5f; 

                    // Smoothly transition the current camera angle toward the target angle using Linear Interpolation (Lerp).
                    cameraAngleYaw = Lerp(cameraAngleYaw, targetYaw, smoothFactor);
                    cameraAnglePitch = Lerp(cameraAnglePitch, targetPitch, smoothFactor);
                } else {
                    // Keyboard: Relative positioning. The camera rotates continuously as long as the arrow keys are held down.
                    float smoothFactor = 0.3f; 
                    
                    float targetYaw = cameraAngleYaw;
                    float targetPitch = cameraAnglePitch;

                    if (IsKeyDown(KEY_RIGHT)) {
                        targetYaw -= 0.08f;
                    }
                    if (IsKeyDown(KEY_LEFT)) {
                        targetYaw += 0.08f;
                    }
                    if (IsKeyDown(KEY_UP)) {
                        targetPitch -= 0.08f;
                    }
                    if (IsKeyDown(KEY_DOWN)) {
                        targetPitch += 0.08f;
                    }

                    // Auto-centering mechanism. Smoothly returns the target angle to 0.0 when no keys are pressed.
                    if (!IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) {
                        targetYaw *= 0.90f;
                    }
                    if (!IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN)) {
                        targetPitch *= 0.90f;
                    }

                    // Clamp the target angles to prevent the camera from rotating too far or clipping through the aircraft.
                    if (targetPitch > 1.5f) {
                        targetPitch = 1.5f;
                    }
                    if (targetPitch < -0.5f) {
                        targetPitch = -0.5f;
                    }
                    if (targetYaw > 2.5f) {
                        targetYaw = 2.5f;
                    }
                    if (targetYaw < -2.5f) {
                        targetYaw = -2.5f;
                    }

                    // Apply the same Lerp transition used for the gamepad to maintain a consistent feel across input devices.
                    cameraAngleYaw = Lerp(cameraAngleYaw, targetYaw, smoothFactor);
                    cameraAnglePitch = Lerp(cameraAnglePitch, targetPitch, smoothFactor);
                }
            } else {
                // Reset manual orbit angles when the player switches back to First Person view to ensure a clean transition.
                cameraAngleYaw = player.rotation.y; 
                cameraAnglePitch = 0.0f;
            }

            if (isFirstPerson) {
                // 1st person (Cockpit).
                // Place camera exactly at player's position, slightly elevated for eye level.
                camera.position = (Vector3){ player.position.x, player.position.y + 0.5f, player.position.z };
                
                // Calculate exactly where the nose of the vehicle is pointing using trigonometry.
                // (We use negative sine/cosine because moving forward means moving into -Z).
                camera.target.x = camera.position.x - (sinf(player.rotation.y) * cosf(player.rotation.x));
                camera.target.y = camera.position.y +  sinf(player.rotation.x); 
                camera.target.z = camera.position.z - (cosf(player.rotation.y) * cosf(player.rotation.x));
                
            } else {
                // 3rd person (Orbit chase camera).
                camera.target = player.position;
                float cameraDistance = 4.0f;
                float cameraHeight = 1.5f; 

                // Combine player rotation with manual orbit input.
                float totalYaw = player.rotation.y + cameraAngleYaw;
                float totalPitch = cameraAnglePitch;

                // Spherical coordinates calculation.
                camera.position.x = player.position.x + (sinf(totalYaw) * cosf(totalPitch) * cameraDistance);
                camera.position.y = player.position.y + (sinf(totalPitch) * cameraDistance) + cameraHeight;
                camera.position.z = player.position.z + (cosf(totalYaw) * cosf(totalPitch) * cameraDistance);
            }

            // Check if the race is over and the 3-second victory screen has passed.
            if (race.isFinished && race.finishedTimer > 3.0f) {
                currentState = STATE_NAME_INPUT;
                
                StopSound(planeSound);
                StopSound(helicopterSound);

                // Reset the typing variables for a fresh start.
                playerName[0] = '\0';
                letterCount = 0;
                virtualKeyIndex = 0;
            }
            
            // Toggle controls visibility when pressing 'H' (Keyboard) or 'Menu/Start' (Gamepad).
            if (IsKeyPressed(KEY_H) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))) {
                showControls = !showControls;
            }
        
        } else if (currentState == STATE_NAME_INPUT) {
            
            // Keyboard logic (PC).
            // GetCharPressed() reads the keyboard queue character by character.
            int key = GetCharPressed();
            while (key > 0) {
                // If the player presses SPACE (ASCII 32), we convert it to an Underscore (ASCII 95).
                if (key == 32) {
                    key = 95; 
                }

                // Only allow standard printable characters (ASCII 32 to 125) and respect the limit.
                if ((key >= 33) && (key <= 125) && (letterCount < MAX_NAME_LENGTH)) {
                    // In the ASCII table, lowercase letters are between 97 ('a') and 122 ('z').
                    // Their uppercase counterparts are exactly 32 steps below them.
                    if (key >= 97 && key <= 122) {
                        key -= 32; 
                    }

                    playerName[letterCount] = (char)key;
                    playerName[letterCount + 1] = '\0';
                    letterCount++;
                }
                key = GetCharPressed(); 
            }
            
            // Handle backspace to delete characters.
            if (IsKeyPressed(KEY_BACKSPACE)) {
                letterCount--;
                if (letterCount < 0) letterCount = 0;
                playerName[letterCount] = '\0';
            }

            // Gamepad logic (Virtual keyboard).
            if (IsGamepadAvailable(0)) {
                // Read the left stick X axis.
                float leftStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                
                // Track if we should move this frame.
                bool moveRight = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
                bool moveLeft = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);

                // Continuous analog stick scrolling.
                if (fabsf(leftStickX) > 0.5f) {
                    
                    // 1. Initial immediate movement (The first 'click').
                    if (!stickMovedX) {
                        if (leftStickX > 0.5f) moveRight = true;
                        if (leftStickX < -0.5f) moveLeft = true;
                        stickMovedX = true;
                        stickScrollTimer = -0.3f; // Initial delay before auto-repeating starts (0.3 seconds).
                    } 
                    // 2. Continuous movement while held down.
                    else {
                        stickScrollTimer += GetFrameTime();
                        // Speed of the scroll (0.10f = moves 10 times per second).
                        if (stickScrollTimer > 0.10f) {
                            if (leftStickX > 0.5f) moveRight = true;
                            if (leftStickX < -0.5f) moveLeft = true;
                            stickScrollTimer = 0.0f; // Reset timer for the next letter.
                        }
                    }
                } else {
                    // Stick returned to center (deadzone).
                    stickMovedX = false;
                    stickScrollTimer = 0.0f;
                }

                // Apply the calculated movement.
                if (moveRight) {
                    virtualKeyIndex = (virtualKeyIndex + 1) % virtualKeyboardLen;
                }
                if (moveLeft) {
                    virtualKeyIndex = (virtualKeyIndex - 1 + virtualKeyboardLen) % virtualKeyboardLen;
                }
                
                // Type the selected letter (Button A).
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && letterCount < MAX_NAME_LENGTH) {
                    playerName[letterCount] = virtualKeyboard[virtualKeyIndex];
                    playerName[letterCount + 1] = '\0';
                    letterCount++;
                }
                
                // Delete letter (Button B).
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    letterCount--;
                    if (letterCount < 0) letterCount = 0;
                    playerName[letterCount] = '\0';
                }
            }
            
            // Submit name and save (ENTER or START),
            if ((IsKeyPressed(KEY_ENTER) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))) && letterCount > 0) {
                
                // 1. Generate a dynamic filename based on the level just played,
                const char *filename = TextFormat("data/times_lvl%d.txt", currentLevel);
                
                // 2. Load, update, and save the specific leaderboard,
                leaderboard = LoadLeaderboard(filename);
                AddLeaderboardEntry(&leaderboard, playerName, race.timer, player.type);
                SaveLeaderboard(&leaderboard, filename);
                
                // 3. Proceed to the viewing screen,
                currentState = STATE_LEADERBOARD;
            }
            
        } else if (currentState == STATE_LEADERBOARD) {
            
            // Wait strictly for ENTER (Keyboard) or 'B' (Gamepad) to return to the main menu.
            if (IsKeyPressed(KEY_ENTER) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                currentState = STATE_LEVEL_SELECT;
            }
        }


        
        // --- B) DRAW PHASE (RENDERING) ---
        // Now that all the math is done, we paint the results onto the screen.
        BeginDrawing();
        
        // Wipe the previous frame clean with a sky blue color.
        ClearBackground(SKYBLUE);

        // Global dynamic resolution
        // We calculate the screen dimensions ONCE per frame, and share them across all states.
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (currentState == STATE_MENU) {
            // Main title.
            const char *title = "SIMPLE FLIGHT SIMULATOR";
            int titleWidth = MeasureText(title, 50); 
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 50, DARKBLUE);

            const char *author = "by Benito Fernandez";
            int authorWidth = MeasureText(author, 35);
            DrawText(author, (screenWidth - authorWidth) / 2, screenHeight * 0.35f, 35, WHITE);

            // Blinking start message.
            const char *msg;
            if (IsGamepadAvailable(0)) { 
                msg = "PRESS [START] TO BEGIN";
            } else {
                msg = "PRESS [ENTER] TO BEGIN";
            }
            int msgWidth = MeasureText(msg, 30);
            if ((int)(GetTime() * 2) % 2 == 0) DrawText(msg, (screenWidth - msgWidth) / 2, screenHeight * 0.6f, 30, GRAY);
            
        } else if (currentState == STATE_LEVEL_SELECT) {
            // 1. Main screen title.
            const char *title = "SELECT CIRCUIT";
            int titleWidth = MeasureText(title, 40); 
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.1f, 40, DARKBLUE);
            
            // Create an empty string buffer to hold the name.
            char currentName[50] = { 0 }; 
            
            // Ask race.c to open the file and fetch the name for us.
            GetLevelName(currentLevel, currentName);
            
            // 2. Draw the current level number and its specific title.
            const char *lvlText = TextFormat("< LEVEL %d: %s >", currentLevel, currentName);
            int lvlWidth = MeasureText(lvlText, 30);
            
            // Color logic: Gold if the file exists (not restricted), Gray if it doesn't.
            Color titleColor;
            if (strcmp(currentName, "RESTRICTED AREA") != 0) {
                titleColor = GOLD;
            } else {
                titleColor = GRAY;
            }
            DrawText(lvlText, (screenWidth - lvlWidth) / 2, screenHeight * 0.18f, 30, titleColor);
            
            // Level grid (5x5).
            int gridCols = 5;
            float slotSize = 80.0f;
            float padding = 20.0f;
            
            // Calculate starting position to center the 5x5 grid.
            float startX = (screenWidth - (gridCols * slotSize + (gridCols - 1) * padding)) / 2;
            float startY = screenHeight * 0.3f;

            for (int i = 0; i < 25; i++) { // 5x5 = 25 slots.
                int row = i / gridCols;
                int col = i % gridCols;
                int levelNum = i + 1;

                Rectangle slotRect = { startX + col * (slotSize + padding), startY + row * (slotSize + padding), slotSize, slotSize };
                
                // Color logic: Gold if selected, Dark Blue if available, Gray if empty.
                Color boxColor;
                if (levelNum == currentLevel) {
                    boxColor = GOLD;
                } else if (levelNum <= MAX_LEVELS) {
                    boxColor = DARKBLUE;
                } else {
                    boxColor = LIGHTGRAY;
                }
                
                DrawRectangleRec(slotRect, boxColor);
                DrawRectangleLinesEx(slotRect, 2, WHITE);

                // Only draw number if the level exists.
                if (levelNum <= MAX_LEVELS) {
                    DrawText(TextFormat("%d", levelNum), slotRect.x + 30, slotRect.y + 25, 30, WHITE);
                }
            }

            const char *msg;
            if (IsGamepadAvailable(0)) {
                msg = "D-PAD/LEFT STICK to navigate, [A] to confirm";
            } else {
                msg = "ARROWS to navigate, [ENTER] to confirm";
            }
            int msgWidth = MeasureText(msg, 20);
            DrawText(msg, (screenWidth - msgWidth) / 2, screenHeight * 0.85f, 20, GRAY);
            
        } else if (currentState == STATE_VEHICLE_SELECT) {
            const char *title = "SELECT AIRCRAFT";
            int titleWidth = MeasureText(title, 40); 
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 40, DARKBLUE);
            
            const char *opt1;
            const char *opt2;
            if (IsGamepadAvailable(0)) {
                opt1 = "Press [X] for SR-71 Blackbird";
                opt2 = "Press [Y] for AH-64 Apache";
            } else {
                opt1 = "Press [1] for SR-71 Blackbird";
                opt2 = "Press [2] for AH-64 Apache";
            }

            int opt1Width = MeasureText(opt1, 25); 
            DrawText(opt1, (screenWidth - opt1Width) / 2, screenHeight * 0.45f, 25, DARKGRAY);
            
            int opt2Width = MeasureText(opt2, 25);
            DrawText(opt2, (screenWidth - opt2Width) / 2, screenHeight * 0.52f, 25, DARKGRAY);
            
        } else if (currentState == STATE_PLAYING) {
            
            // Switch Raylib into 3D rendering mode using our camera.
            BeginMode3D(camera);
                
                // We draw the skybox exactly where the camera is. 
                // This creates the optical illusion that the sky is infinitely far away.
                DrawModel(skyboxModel, camera.position, 3.0f, WHITE);

                // Infinite green grid.
                // Draw a massive solid dark green plane to hide the skybox below.
                // We place it exactly at Y = 0.0f. 
                // The size is 10000x10000 units, which covers the entire visible horizon.
                DrawPlane((Vector3){ player.position.x, 0.0f, player.position.z }, (Vector2){ 10000.0f, 10000.0f }, DARKGREEN);

                // Draw the grid lines slightly above the plane (Y = 0.05f).
                // If we draw them at exactly 0.0f, they will overlap with the solid plane 
                // and cause a graphical glitch called "Z-fighting" (flickering).
                float spacing = 50.0f; // Distance between each line in the grid.
                int slices = 60;       // How many lines we draw in each direction.

                // We find the nearest grid "intersection" to the player.
                // By snapping the grid to the player's position, we create the optical illusion
                // that the floor is infinite and smoothly follows the camera.
                float snapX = (int)(player.position.x / spacing) * spacing;
                float snapZ = (int)(player.position.z / spacing) * spacing;

                for (int i = -slices; i <= slices; i++) {
                    float offset = i * spacing;
                    float extent = slices * spacing;
                    
                    // Draw lines along the Z axis (Forward/Backward)
                    DrawLine3D((Vector3){ snapX + offset, 0.05f, snapZ - extent },
                               (Vector3){ snapX + offset, 0.05f, snapZ + extent }, LIME);
                               
                    // Draw lines along the X axis (Left/Right)
                    DrawLine3D((Vector3){ snapX - extent, 0.05f, snapZ + offset },
                               (Vector3){ snapX + extent, 0.05f, snapZ + offset }, LIME);
                }

                // Draw the floating 3D rings and the navigation arrow for the race.
                DrawRace3D(&race, &player);

                // Decide which model to use.
                Model *currentModel;
                if (player.type == VEHICLE_PLANE) {
                    currentModel = &planeModel;
                } else {
                    currentModel = &helicopterModel;
                }
                
                // 1. Save the model's base matrix.
                Matrix baseTransform = currentModel->transform;
                
                // 2. Generate a dynamic rotation matrix (Strict aerospace order).
                // By multiplying matrices individually (Roll -> Pitch -> Yaw), 
                // we prevent the axes from mixing up when pressing multiple keys (Gimbal lock).
                Matrix matRoll  = MatrixRotateZ(player.rotation.z);
                Matrix matPitch = MatrixRotateX(player.rotation.x);
                Matrix matYaw   = MatrixRotateY(player.rotation.y);
                Matrix dynamicRotation = MatrixMultiply(MatrixMultiply(matRoll, matPitch), matYaw);
                
                // 3. Combine them and apply temporarily.
                currentModel->transform = MatrixMultiply(baseTransform, dynamicRotation);
                
                // 4. Draw the 3D model.
                if (!isFirstPerson) {
                    if (player.type == VEHICLE_PLANE) {
                        DrawModel(*currentModel, player.position, 0.08f, WHITE); 
                    } else if (player.type == VEHICLE_HELICOPTER) {
                        DrawModel(*currentModel, player.position, 0.8f, WHITE);
                    }
                }

                // 5. Restore the base matrix.
                currentModel->transform = baseTransform;

                // 6. Draw smoke particles.
                // We loop through the pool and draw a cube for every active particle.
                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (player.smoke[i].active) {
                        
                        // White with transparency (Max alpha of 0.6f so it's not fully opaque).
                        Color smokeColor = Fade(WHITE, player.smoke[i].life * 0.6f);
                        
                        // Smaller size: Starts at 0.1 and grows up to 0.8.
                        float size = 0.1f + ((1.0f - player.smoke[i].life) * 0.7f);
                        
                        DrawSphere(player.smoke[i].position, size, smokeColor);
                    }
                }
                
            EndMode3D(); // Switch back to 2D rendering mode.

            // UI controls (Toggleable).
            if (showControls) {
                // 1. Top-left: Movement & Throttle.
                if (IsGamepadAvailable(0)) {
                    DrawTextOutlined("LT/RT: Throttle | Left Stick: Move", 20, 20, 20, LIGHTGRAY, 2);
                } else {
                    DrawTextOutlined("W/S: Throttle | A/D: Yaw/Roll | SPACE/SHIFT: Pitch", 20, 20, 20, LIGHTGRAY, 2);
                }

                // 2. Top-right: Camera & Restart.
                const char *camRestText;
                if (IsGamepadAvailable(0)) {
                    camRestText = "Right Stick: Camera | [B] Restart | [A] POV ";
                } else {
                    camRestText = "Arrows: Camera | [R] Restart | [C] POV";
                }
                int camRestWidth = MeasureText(camRestText, 20);
                DrawTextOutlined(camRestText, screenWidth - camRestWidth - 20, 20, 20, LIGHTGRAY, 2);

                // 3. Bottom right: Vehicle Switch.
                const char *vehText;
                if (IsGamepadAvailable(0)) {
                    vehText = "[X]/[Y] Switch Aircraft";
                } else {
                    vehText = "[1]/[2] Switch Aircraft";
                }
                int vehWidth = MeasureText(vehText, 20);
                DrawTextOutlined(vehText, screenWidth - vehWidth - 20, screenHeight - 60, 20, ORANGE, 2);
            } else {
                if (IsGamepadAvailable(0)) {
                    DrawTextOutlined("Press [Menu] to show controls", 20, 20, 14, LIGHTGRAY, 1);
                } else {
                    DrawTextOutlined("Press [H] to show controls", 20, 20, 14, LIGHTGRAY, 1);
                }
            }

            // Draw the race stopwatch and remaining rings on the screen (Centered).
            DrawRaceUI(&race);

            // Aeronautical HUD.
            DrawTextOutlined(TextFormat("ALTITUDE: %.0f ft", player.position.y * 10.0f), 20, screenHeight - 100, 20, LIME, 2);

            float maxThrottle;
            if (player.type == VEHICLE_PLANE) {
                maxThrottle = 0.8f;
            } else {
                maxThrottle = 0.4f;
            }
            int powerPercentage = (int)((-player.throttle / maxThrottle) * 100.0f);
            DrawTextOutlined(TextFormat("POWER: %d %%", powerPercentage), 20, screenHeight - 60, 20, LIME, 2);

        }  else if (currentState == STATE_NAME_INPUT) {
            
            // Name input acreen.
            const char *title = "NEW FLIGHT RECORD!";
            int titleWidth = MeasureText(title, 40);
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.15f, 40, GOLD);
            
            const char *prompt = "ENTER YOUR CALLSIGN, PILOT:";
            int promptWidth = MeasureText(prompt, 30);
            DrawText(prompt, (screenWidth - promptWidth) / 2, screenHeight * 0.35f, 30, WHITE);
            
            // Draw the actual name being typed inside a classic console-style bracket.
            const char *nameDisplay = TextFormat("[ %s_ ]", playerName);
            int nameWidth = MeasureText(nameDisplay, 40);
            DrawText(nameDisplay, (screenWidth - nameWidth) / 2, screenHeight * 0.45f, 40, LIME);

            // Input instructions.
            // 1. Draw the virtual keyboard ONLY if the player is using a gamepad.
            if (IsGamepadAvailable(0)) {
                const char *virtualUI = TextFormat("<- [ %c ] ->", virtualKeyboard[virtualKeyIndex]);
                int virtualWidth = MeasureText(virtualUI, 40);
                DrawText(virtualUI, (screenWidth - virtualWidth) / 2, screenHeight * 0.60f, 40, ORANGE);
            }
            
            // 2. Define the texts using a classic if-else based on the controller type.
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

            // 3. Draw the 3 rows ordered vertically.
            int row1Width = MeasureText(row1, 20);
            DrawText(row1, (screenWidth - row1Width) / 2, screenHeight * 0.70f, 20, GRAY);
            
            int row2Width = MeasureText(row2, 20);
            DrawText(row2, (screenWidth - row2Width) / 2, screenHeight * 0.75f, 20, GRAY);
            
            // Classic blinking effect on the third row (Confirm).
            if ((int)(GetTime() * 2) % 2 == 0) {
                int row3Width = MeasureText(row3, 30);
                DrawText(row3, (screenWidth - row3Width) / 2, screenHeight * 0.85f, 30, LIGHTGRAY);
            }
            
        } else if (currentState == STATE_LEADERBOARD) {
            // Leaderboard screen.
            ClearBackground(DARKBLUE); // A different background to make it feel like a computer terminal.
            
            const char *title = "--- TOP 10 PILOTS ---";
            int titleWidth = MeasureText(title, 40);
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.1f, 40, GOLD);
            
            // Loop through the records and print them list-style.
            int startY = screenHeight * 0.25f;
            int spacing = 35;
            
            // Headers for the columns.
            DrawText("PILOT", screenWidth * 0.2f, startY - 40, 20, GRAY);
            DrawText("TIME", screenWidth * 0.5f, startY - 40, 20, GRAY);
            DrawText("VEHICLE", screenWidth * 0.75f, startY - 40, 20, GRAY);

            for (int i = 0; i < leaderboard.count; i++) {
                // Format the strings.
                const char *recordStr = TextFormat("%d. %s", i + 1, leaderboard.entries[i].name);
                const char *timeStr = TextFormat("%.2f s", leaderboard.entries[i].time);
                
                // Determine the vehicle name.
                const char *vehStr = "Unknown";
                if (leaderboard.entries[i].vehicle == VEHICLE_PLANE) {
                    vehStr = "Airplane";
                } else if (leaderboard.entries[i].vehicle == VEHICLE_HELICOPTER) {
                    vehStr = "Helicopter";
                }
                
                // Draw Name (Left column).
                DrawText(recordStr, screenWidth * 0.2f, startY + (i * spacing), 30, WHITE);
                
                // Draw Time (Center column).
                DrawText(timeStr, screenWidth * 0.5f, startY + (i * spacing), 30, LIME);

                // Draw Vehicle (Right column).
                DrawText(vehStr, screenWidth * 0.75f, startY + (i * spacing), 30, SKYBLUE);
            }
            
            // Dynamic instructions to exit based on connected hardware.
            const char *exitText;
            if (IsGamepadAvailable(0)) {
                exitText = "PRESS [B] TO RETURN TO BASE";
            } else {
                exitText = "PRESS [ENTER] TO RETURN TO BASE";
            }
            
            int exitWidth = MeasureText(exitText, 20);
            DrawText(exitText, (screenWidth - exitWidth) / 2, screenHeight * 0.9f, 20, GRAY);
        }

        EndDrawing(); // Tell Raylib we are done painting this frame, display it!
    }


    // --- 3. TEARDOWN (CLEANUP) ---
    // The loop is over (User closed the game). Time to clean up.
    UnloadGameResources(); // Our custom function to free RAM.
    CloseAudioDevice();    // Close audio device AFTER unloading resources.
    CloseWindow();         // Raylib's function to close the OS window safely.
    return 0;              // Tell Windows the program finished successfully.
}