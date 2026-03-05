// Include stdbool library to use booleans
#include <stdbool.h>

// Include Raylib's libraries
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
    STATE_PLAYING,
    STATE_NAME_INPUT,
    STATE_LEADERBOARD
} GameState;


int main(void) {
    // --- 1. INITIALIZATION (SETUP) ---
    
    // Allow the user to resize the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    // Open a window with a temporary size so Raylib can connect to the OS
    InitWindow(800, 600, "Simple Flight Simulator");
    
    // Ask the OS for the current monitor's dimensions
    int monitor = GetCurrentMonitor();
    int displayWidth = GetMonitorWidth(monitor);
    int displayHeight = GetMonitorHeight(monitor);
    
    // Calculate the maximum 4:3 resolution. 
    // We subtract 100 pixels from the height so the Windows Taskbar doesn't overlap the game.
    int targetHeight = displayHeight - 100;
    int targetWidth = (targetHeight * 4) / 3;
    
    // Safety check: just in case the user has a vertical monitor (width is smaller than height)
    if (targetWidth > displayWidth) {
        targetWidth = displayWidth - 100;
        targetHeight = (targetWidth * 3) / 4;
    }
    
    // Apply the calculated 4:3 size and center the window on the screen perfectly
    SetWindowSize(targetWidth, targetHeight);
    SetWindowPosition((displayWidth - targetWidth) / 2, (displayHeight - targetHeight) / 2);


    InitAudioDevice(); // Initialize audio device before loading resources.

    SetTargetFPS(60); // Force the game to run at a stable 60 frames per second.

    // Call our custom module to load heavy files into RAM.
    LoadGameResources(); 

    // Set the initial game state to show the menu first.
    GameState currentState = STATE_MENU;
    
    // Create an empty player. It will be properly initialized when the user selects a vehicle.
    Player player = { 0 };
    
    // Setup the 3D camera.
    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 20.0f, 0.0f }; // Defines which way is "up" (Y axis).
    camera.fovy = 60.0f;                        // Field of View (zoom level).
    camera.projection = CAMERA_PERSPECTIVE;     // Gives a realistic 3D depth effect.

    // First person camera toggle.
    bool isFirstPerson = false;
    
    // Toggle to hide/show the controls UI.
    bool showControls = true;

    // Initialize the Race System (The track and the referee).
    RaceSystem race = InitRace();

    // Load existing records from the hard drive into RAM.
    Leaderboard leaderboard = LoadLeaderboard("data/times.txt");
    
    // Variables to handle the keyboard input for the player's name.
    char playerName[MAX_NAME_LENGTH + 1] = "\0"; // Empty string to start.
    int letterCount = 0;                         // Tracks how many letters have been typed.


    // --- 2. THE MAIN GAME LOOP ---
    // This loop runs 60 times per second until the user clicks the X or presses ESC.
    while (!WindowShouldClose()) {
        
        // Exit with gamepad.
        // If the left middle button (View/Back) is pressed, break the loop to close the game.
        if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
            break;
        }

        // --- A) UPDATE PHASE (LOGIC & MATH) ---
        // Here we process input and move things, but we do not draw anything yet.
        if (currentState == STATE_MENU) {
            
            // UpdateMusicStream must be called every single frame to keep the buffer flowing.
            UpdateMusicStream(menuMusic);
            
            // If the music isn't playing yet, start it.
            if (!IsMusicStreamPlaying(menuMusic)) {
                PlayMusicStream(menuMusic);
            }
            
            // Wait for the user to press 1 (Keyboard) or X (Gamepad) to select the Plane
            if (IsKeyPressed(KEY_ONE) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) {
                StopMusicStream(menuMusic);         
                player = InitPlayer(VEHICLE_PLANE); 
                race = InitRace();
                currentState = STATE_PLAYING;       
            } 
            // Wait for the user to press 2 (Keyboard) or Y (Gamepad) to select the Helicopter
            else if (IsKeyPressed(KEY_TWO) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
                StopMusicStream(menuMusic);              
                player = InitPlayer(VEHICLE_HELICOPTER);
                race = InitRace();
                currentState = STATE_PLAYING;            
            }
        } else if (currentState == STATE_PLAYING) {

            // Mid-flight vehicle switching
            if (IsKeyPressed(KEY_ONE) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) player.type = VEHICLE_PLANE;
            if (IsKeyPressed(KEY_TWO) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) player.type = VEHICLE_HELICOPTER;

            // Quick restart.
            // If the player makes a mistake, press R to restart the race instantly.
            if (IsKeyPressed(KEY_R) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                race = InitRace();                // Resets all rings and stopwatch.
                player = InitPlayer(player.type); // Teleports player back to origin.
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
            
            // Dynamic logic camera
            // Toggle camera mode when pressing 'C' (Keyboard) or 'A' (Gamepad)
            if (IsKeyPressed(KEY_C) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) isFirstPerson = !isFirstPerson;

            if (isFirstPerson) {
                // 1st person (Cockpit).
                // Place camera exactly at player's position, slightly elevated for eye level
                camera.position = (Vector3){ player.position.x, player.position.y + 0.5f, player.position.z };
                
                // Calculate exactly where the nose of the vehicle is pointing using trigonometry.
                // (We use negative sine/cosine because moving forward means moving into -Z).
                camera.target.x = camera.position.x - (sinf(player.rotation.y) * cosf(player.rotation.x));
                camera.target.y = camera.position.y +  sinf(player.rotation.x); 
                camera.target.z = camera.position.z - (cosf(player.rotation.y) * cosf(player.rotation.x));
                
            } else {
                // 3rd person (Chase).
                camera.target = player.position;
                float cameraDistance = 4.0f;
                float cameraHeight = 1.5f; 

                camera.position.x = player.position.x + (sinf(player.rotation.y) * cameraDistance);
                camera.position.y = player.position.y + cameraHeight;
                camera.position.z = player.position.z + (cosf(player.rotation.y) * cameraDistance);
            }

            // Check if the race is over and the 5-second victory screen has passed.
            if (race.isFinished && race.finishedTimer > 5.0f) {
                currentState = STATE_NAME_INPUT;
                
                StopSound(planeSound);
                StopSound(helicopterSound);

                // Reset the typing variables for a fresh start.
                playerName[0] = '\0';
                letterCount = 0;
            }
            
            // Toggle controls visibility when pressing 'H' (Keyboard) or 'Menu/Start' (Gamepad).
            if (IsKeyPressed(KEY_H) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))) showControls = !showControls;
        
        } else if (currentState == STATE_NAME_INPUT) {
            
            // --- TEXT INPUT LOGIC ---
            // GetCharPressed() reads the keyboard queue character by character.
            int key = GetCharPressed();
            
            // While there are keys in the queue, process them.
            while (key > 0) {
                // Only allow standard printable characters (ASCII 32 to 125) and respect the limit.
                if ((key >= 32) && (key <= 125) && (letterCount < MAX_NAME_LENGTH)) {
                    playerName[letterCount] = (char)key;
                    playerName[letterCount + 1] = '\0'; // Always keep the string null-terminated.
                    letterCount++;
                }
                key = GetCharPressed(); // Check if there's another key pressed very quickly.
            }
            
            // Handle BACKSPACE to delete characters.
            if (IsKeyPressed(KEY_BACKSPACE)) {
                letterCount--;
                if (letterCount < 0) letterCount = 0;
                playerName[letterCount] = '\0';
            }
            
            // Handle ENTER to submit the name.
            if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                // 1. Add the new champion to the RAM array.
                AddLeaderboardEntry(&leaderboard, playerName, race.timer, player.type);
                // 2. Save the RAM array to the Hard Drive.
                SaveLeaderboard(&leaderboard, "data/times.txt");
                // 3. Move to the Leaderboard screen.
                currentState = STATE_LEADERBOARD;
            }
            
        } else if (currentState == STATE_LEADERBOARD) {
            
            // Wait strictly for ENTER (Keyboard) or 'B' (Gamepad) to return to the main menu.
            if (IsKeyPressed(KEY_ENTER) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                currentState = STATE_MENU;
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
            
            // Main menu.
            const char *title = "SIMPLE FLIGHT SIMULATOR";
            int titleWidth = MeasureText(title, 50); 
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 50, DARKBLUE);

            const char *author = "Author: Benito Fernandez";
            int authorWidth = MeasureText(author, 35); 
            DrawText(author, (screenWidth - authorWidth) / 2, screenHeight * 0.33f, 35, WHITE);
            
            // NEW: DYNAMIC MENU TEXT BASED ON GAMEPAD CONNECTION
            const char *opt1;
            const char *opt2;

            if (IsGamepadAvailable(0)) {
                opt1 = "Press [X] to fly the SR-71 Blackbird";
                opt2 = "Press [Y] to fly the AH-64 Apache";
            } else {
                opt1 = "Press [1] to fly the SR-71 Blackbird";
                opt2 = "Press [2] to fly the AH-64 Apache";
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
                // The size is 5000x5000 units, which covers the entire visible horizon.
                DrawPlane((Vector3){ player.position.x, 0.0f, player.position.z }, (Vector2){ 5000.0f, 5000.0f }, DARKGREEN);

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

            // UI controls (Toggleable)
            if (showControls) {
                if (IsGamepadAvailable(0)) {
                    DrawText("LT/RT: Throttle | Stick: Move", 20, 20, 20, DARKGRAY);
                } else {
                    DrawText("W/S: Throttle | A/D: Yaw/Roll | SPACE/SHIFT: Pitch", 20, 20, 20, DARKGRAY);
                }
            } else {
                if (IsGamepadAvailable(0)) {
                    DrawText("Press [Menu] to show controls", 20, 20, 10, GRAY);
                } else {
                    DrawText("Press [H] to show controls", 20, 20, 10, GRAY);
                }
            }

            // Draw the race stopwatch and remaining rings on the screen (Centered).
            DrawRaceUI(&race);

            // Aeronautical HUD.
            DrawText(TextFormat("ALTITUDE: %.0f ft", player.position.y * 10.0f), 20, screenHeight - 100, 20, LIME);

            float maxThrottle;
            if (player.type == VEHICLE_PLANE) {
                maxThrottle = 0.8f;
            } else {
                maxThrottle = 0.4f;
            }

            int powerPercentage = (int)((-player.throttle / maxThrottle) * 100.0f);
            
            DrawText(TextFormat("POWER: %d %%", powerPercentage), 20, screenHeight - 60, 20, LIME);

        }  else if (currentState == STATE_NAME_INPUT) {
            
            // Name Input Screen
            const char *title = "NEW FLIGHT RECORD!";
            int titleWidth = MeasureText(title, 40);
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.2f, 40, GOLD);
            
            const char *prompt = "ENTER YOUR CALLSIGN, PILOT:";
            int promptWidth = MeasureText(prompt, 30);
            DrawText(prompt, (screenWidth - promptWidth) / 2, screenHeight * 0.4f, 30, WHITE);
            
            // Draw the actual name being typed inside a classic console-style bracket
            const char *nameDisplay = TextFormat("[ %s_ ]", playerName);
            int nameWidth = MeasureText(nameDisplay, 40);
            DrawText(nameDisplay, (screenWidth - nameWidth) / 2, screenHeight * 0.5f, 40, LIME);
            
            // Blinking instructions
            if ((int)(GetTime() * 2) % 2 == 0) {
                const char *instruction = "PRESS ENTER TO CONFIRM";
                int instWidth = MeasureText(instruction, 20);
                DrawText(instruction, (screenWidth - instWidth) / 2, screenHeight * 0.7f, 20, GRAY);
            }
            
        } else if (currentState == STATE_LEADERBOARD) {
            
            // Leaderboard Screen
            ClearBackground(DARKBLUE); // A different background to make it feel like a computer terminal
            
            const char *title = "--- TOP 10 PILOTS ---";
            int titleWidth = MeasureText(title, 40);
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.1f, 40, GOLD);
            
            // Loop through the records and print them list-style
            int startY = screenHeight * 0.25f;
            int spacing = 35;
            
// Headers for the columns
            DrawText("PILOT", screenWidth * 0.2f, startY - 40, 20, GRAY);
            DrawText("TIME", screenWidth * 0.5f, startY - 40, 20, GRAY);
            DrawText("VEHICLE", screenWidth * 0.75f, startY - 40, 20, GRAY);

            for (int i = 0; i < leaderboard.count; i++) {
                
                // Format the strings
                const char *recordStr = TextFormat("%d. %s", i + 1, leaderboard.entries[i].name);
                const char *timeStr = TextFormat("%.2f s", leaderboard.entries[i].time);
                
                // Determine the vehicle name
                const char *vehStr = "Unknown";
                if (leaderboard.entries[i].vehicle == VEHICLE_PLANE) {
                    vehStr = "Airplane";
                } else if (leaderboard.entries[i].vehicle == VEHICLE_HELICOPTER) {
                    vehStr = "Helicopter";
                }
                
                // Draw Name (Left column)
                DrawText(recordStr, screenWidth * 0.2f, startY + (i * spacing), 30, WHITE);
                
                // Draw Time (Center column)
                DrawText(timeStr, screenWidth * 0.5f, startY + (i * spacing), 30, LIME);

                // Draw Vehicle (Right column)
                DrawText(vehStr, screenWidth * 0.75f, startY + (i * spacing), 30, SKYBLUE);
            }
            
            // Dynamic instructions to exit based on connected hardware
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