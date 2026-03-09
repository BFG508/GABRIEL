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
#include "rlgl.h"

// Notice we use quotes "" for our own files, and angle brackets <> for system libraries.
#include "player.h"
#include "resource_manager.h"
#include "race.h"
#include "leaderboard.h"
#include "ui.h"


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


// -- MAIN FUNCTION --
int main(void) {
    // --- 1. INITIALIZATION (SETUP) ---
    
    // Allow the user to resize the window.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    // Open a window with a temporary size so Raylib can connect to the OS.
    InitWindow(800, 600, "Simple Flight Simulator");

    // Push the far clipping plane from the default 1000.0f out to 5000.0f units.
    // This stops distant rings, mountains, and helipads from suddenly popping into existence.
    rlSetClipPlanes(0.1f, 5000.0f);
    
    // Ask the OS for the current monitor's dimensions.
    int monitor = GetCurrentMonitor();
    int displayWidth = GetMonitorWidth(monitor);
    int displayHeight = GetMonitorHeight(monitor);
    
    // Calculate the maximum 4:3 resolution. 
    // We subtract 100 pixels from the height so the taskbar doesn't overlap the game.
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

    
    // Leaderboard & text input setup.
    // We leave the leaderboard struct empty for now. 
    // It will be dynamically loaded when the player finishes a specific level.
    Leaderboard leaderboard = { 0 };

    char playerName[MAX_NAME_LENGTH + 1] = "\0"; 
    int letterCount = 0;                         

    // Variables for the gamepad virtual keyboard (Arcade style input).
    // This allows players without a physical keyboard to enter their names.
    const char virtualKeyboard[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    int virtualKeyboardLen = 37;
    int virtualKeyIndex = 0;

    // Timer for continuous scrolling.
    float stickScrollTimer = 0.0f;

    // Timers and latches for continuous deleting (Gamepad).
    float deleteTimer = 0.0f;
    bool deleteFirstPress = false;


    // Analog Stick Latches.
    // These prevent the grid selection from flying at 60 slots per second 
    // when the player holds the analog stick in a direction.
    bool stickMovedX = false;
    bool stickMovedY = false;

    // Timer for mouse double-clicks.
    double lastClickTime = 0.0;


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
            if (fabsf(leftStickX) < 0.2f) {
                stickMovedX = false;
            }
            if (fabsf(leftStickY) < 0.2f) {
                stickMovedY = false;
            }

            // Grid navigation logic (5x5) with strict row/column wrapping.
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
            
            // Mouse selection logic.
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                
                // We need to fetch the screen size here to recreate the grid math.
                int screenWidth = GetScreenWidth();
                int screenHeight = GetScreenHeight();
                
                int gridCols = 5;
                float slotSize = 80.0f;
                float padding = 20.0f;
                float startX = (screenWidth - (gridCols * slotSize + (gridCols - 1) * padding)) / 2;
                float startY = screenHeight * 0.3f;

                // Loop through all 25 theoretical slots to check for a collision with the mouse.
                for (int i = 0; i < 25; i++) {
                    int row = i / gridCols;
                    int col = i % gridCols;
                    int levelNum = i + 1;

                    // Recreate the exact bounding box of this specific level slot.
                    Rectangle slotRect = { startX + col * (slotSize + padding), startY + row * (slotSize + padding), slotSize, slotSize };

                    // If the mouse is inside the box and the level actually exists...
                    if (CheckCollisionPointRec(mousePos, slotRect) && levelNum <= MAX_LEVELS) {
                        if (currentLevel == levelNum) {
                            // The level was already selected. Check for double-click (0.4 seconds window).
                            if ((GetTime() - lastClickTime) < 0.4) {
                                currentState = STATE_VEHICLE_SELECT; // Enter the level!
                            } else {
                                lastClickTime = GetTime(); // Too slow, reset the timer.
                            }
                        } else {
                            // Single click on a new level: Select it.
                            currentLevel = levelNum;
                            lastClickTime = GetTime();
                        }
                        
                        // Stop checking the rest of the grid since we already found the clicked slot.
                        break; 
                    }
                }
            }

            // Confirm level selection.
            if (IsKeyPressed(KEY_ENTER) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                if (currentLevel <= MAX_LEVELS) {
                    currentState = STATE_VEHICLE_SELECT;
                }
            }
            
        } else if (currentState == STATE_VEHICLE_SELECT) {
            UpdateMusicStream(menuMusic);
            if (!IsMusicStreamPlaying(menuMusic)) {
                PlayMusicStream(menuMusic);
            }
            
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

            // Update the camera (1st/3rd person logic and orbital math).
            UpdateDynamicCamera(&camera, &player);

            // Dynamic audio logic.
            // We only play engine sounds if the vehicle is still intact!
            if (race.missionFailed) {
                // Force complete silence on crash
                if (IsSoundPlaying(planeSound)) StopSound(planeSound);
                if (IsSoundPlaying(helicopterSound)) StopSound(helicopterSound);
            }
            else {
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
            UpdateMusicStream(endingMusic);
            if (!IsMusicStreamPlaying(endingMusic)) {
                PlayMusicStream(endingMusic);
            }

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
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
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
                if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    if (!deleteFirstPress) {
                        // 1. Initial immediate delete (The first 'click').
                        letterCount--;
                        if (letterCount < 0) letterCount = 0;
                        playerName[letterCount] = '\0';
                        
                        deleteFirstPress = true;
                        deleteTimer = -0.4f; // Initial delay before rapid-fire starts (0.4 seconds).
                    } else {
                        // 2. Continuous delete while held down.
                        deleteTimer += GetFrameTime();
                        
                        // Speed of the deletion (0.08f = extremely fast rapid fire).
                        if (deleteTimer > 0.08f) {
                            letterCount--;
                            if (letterCount < 0) letterCount = 0;
                            playerName[letterCount] = '\0';
                            
                            deleteTimer = 0.0f; // Reset timer for the next letter deletion.
                        }
                    }
                } else {
                    // Button released, reset the latch.
                    deleteFirstPress = false;
                }
            }
            
            // Submit name and save (ENTER or START).
            if ((IsKeyPressed(KEY_ENTER) || 
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))) && letterCount > 0) {
                // 1. Generate a dynamic filename based on the level just played.
                const char *filename = TextFormat("data/times_lvl%d.txt", currentLevel);
                
                // 2. Load, update, and save the specific leaderboard.
                leaderboard = LoadLeaderboard(filename);
                AddLeaderboardEntry(&leaderboard, playerName, race.timer, player.type);
                SaveLeaderboard(&leaderboard, filename);
                
                // 3. Proceed to the viewing screen.
                currentState = STATE_LEADERBOARD;
            }
            
        } else if (currentState == STATE_LEADERBOARD) {
            UpdateMusicStream(endingMusic);
            if (!IsMusicStreamPlaying(endingMusic)) {
                PlayMusicStream(endingMusic);
            }

            // Wait strictly for ENTER (Keyboard) or 'B' (Gamepad) to return to the main menu.
            if (IsKeyPressed(KEY_ENTER) ||
               (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                currentState = STATE_LEVEL_SELECT;
            }
        }

        
        // --- B) DRAW PHASE (RENDERING) ---
        // Now that all the math is done, we paint the results onto the screen.
        BeginDrawing();
        
        // Wipe the previous frame clean with a sky blue color.
        // We don't clear the background for the leaderboard because it has its own DARKBLUE background in ui.c.
        if (currentState != STATE_LEADERBOARD) {
            ClearBackground(SKYBLUE);
        }

        // Global dynamic resolution
        // We calculate the screen dimensions once per frame, and share them across all states.
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        // A clean switch statement using ui.c.
        switch (currentState) {
            case STATE_MENU:
                DrawMainMenu(screenWidth, screenHeight);
                break;
                
            case STATE_LEVEL_SELECT:
                DrawLevelSelectScreen(currentLevel, MAX_LEVELS, screenWidth, screenHeight);
                break;
                
            case STATE_VEHICLE_SELECT:
                DrawVehicleSelectScreen(screenWidth, screenHeight);
                break;
                
            case STATE_PLAYING:
                // --- 3D WORLD RENDERING ---
                // Switch Raylib into 3D rendering mode using our camera.
                BeginMode3D(camera);
                    // 1. Draw the skybox exactly where the camera is. 
                    DrawModel(skyboxModel, camera.position, 3.0f, WHITE);

                    // 2. Infinite green grid trick.
                    DrawPlane((Vector3){ player.position.x, 0.0f, player.position.z }, (Vector2){ 10000.0f, 10000.0f }, DARKGREEN);

                    float spacing = 50.0f; 
                    int slices = 60;       

                    float snapX = (int)(player.position.x / spacing) * spacing;
                    float snapZ = (int)(player.position.z / spacing) * spacing;

                    for (int i = -slices; i <= slices; i++) {
                        float offset = i * spacing;
                        float extent = slices * spacing;
                        
                        DrawLine3D((Vector3){ snapX + offset, 0.05f, snapZ - extent }, (Vector3){ snapX + offset, 0.05f, snapZ + extent }, LIME);
                        DrawLine3D((Vector3){ snapX - extent, 0.05f, snapZ + offset }, (Vector3){ snapX + extent, 0.05f, snapZ + offset }, LIME);
                    }

                    // 3. Draw the floating 3D rings/helipads and the navigation arrow for the race.
                    DrawRace3D(&race, &player);

                    // 4. Draw the physical aircraft if we are in 3rd person (orbit) view.
                    if (!player.isFirstPerson) {
                        Model *currentModel;
                        if (player.type == VEHICLE_PLANE) {
                            currentModel = &planeModel;
                        } else {
                            currentModel = &helicopterModel;
                        }
                        
                        Matrix baseTransform = currentModel->transform;
                        Matrix matRoll  = MatrixRotateZ(player.rotation.z);
                        Matrix matPitch = MatrixRotateX(player.rotation.x);
                        Matrix matYaw   = MatrixRotateY(player.rotation.y);
                        Matrix dynamicRotation = MatrixMultiply(MatrixMultiply(matRoll, matPitch), matYaw);
                        
                        currentModel->transform = MatrixMultiply(baseTransform, dynamicRotation);
                        if (player.type == VEHICLE_PLANE) {
                            DrawModel(*currentModel, player.position, 0.08f, WHITE); 
                        } else if (player.type == VEHICLE_HELICOPTER) {
                            DrawModel(*currentModel, player.position, 0.8f, WHITE);
                        }
                        currentModel->transform = baseTransform;
                    }

                    // 5. Draw smoke particles.
                    for (int i = 0; i < MAX_PARTICLES; i++) {
                        if (player.smoke[i].active) {
                            Color smokeColor = Fade(WHITE, player.smoke[i].life * 0.6f);
                            float size = 0.1f + ((1.0f - player.smoke[i].life) * 0.7f);
                            DrawSphere(player.smoke[i].position, size, smokeColor);
                        }
                    }
                    
                EndMode3D(); // Switch back to 2D rendering mode.

                // --- 2D HUD RENDERING ---
                // Call our unified HUD drawer from the UI module!
                DrawHUD(&player, &race, showControls, screenWidth, screenHeight);
                break;
                
            case STATE_NAME_INPUT:
                // Pass the current virtual key character (using the index).
                DrawNameInputScreen(playerName, virtualKeyboard[virtualKeyIndex], screenWidth, screenHeight);
                break;
                
            case STATE_LEADERBOARD:
                DrawLeaderboardScreen(&leaderboard, screenWidth, screenHeight);
                break;
        }

        EndDrawing(); // Tell Raylib we are done painting this frame, display it!
    }


    // --- 3. TEARDOWN (CLEANUP) ---
    // The loop is over (User closed the game). Time to clean up.
    UnloadGameResources(); // Our custom function to free RAM.
    CloseAudioDevice();    // Close audio device after unloading resources.
    CloseWindow();         // Raylib's function to close the OS window safely.
    return 0;              // Tell Windows the program finished successfully.
}