// Include stdbool library to use booleans
#include <stdbool.h>

// Include the main Raylib libraries
#include "raylib.h"
#include "raymath.h"

// Notice we use quotes "" for our own files, and angle brackets <> for system libraries.
#include "player.h"
#include "resource_manager.h"
#include "race.h"


// --- GAME STATES (STATE MACHINE) ---
// A Finite State Machine (FSM) is a core concept in game dev.
// The game can only be in one state at a time. This prevents the player 
// from moving the plane while they are still in the main menu.
typedef enum GameState {
    STATE_MENU,
    STATE_PLAYING
} GameState;

int main(void) {
    // --- 1. INITIALIZATION (SETUP) ---
    // Open the window and set the title
    InitWindow(1200, 900, "Simple Flight Simulator");

    InitAudioDevice(); // Initialize audio device BEFORE loading resources

    SetTargetFPS(60); // Force the game to run at a stable 60 Frames Per Second

    // Call our custom module to load heavy files (models/textures) into RAM
    LoadGameResources(); 

    // Set the initial game state to show the menu first
    GameState currentState = STATE_MENU;
    
    // Create an empty player. It will be properly initialized when the user selects a vehicle.
    Player player = { 0 };
    
    // Setup the 3D camera
    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 20.0f, 0.0f }; // Defines which way is "up" (Y axis)
    camera.fovy = 60.0f;                       // Field of View (zoom level)
    camera.projection = CAMERA_PERSPECTIVE;    // Gives a realistic 3D depth effect

    // First person camera toggle
    bool isFirstPerson = false;
    
    // Toggle to hide/show the controls UI
    bool showControls = true;

    // Initialize the Race System (The Track and the Referee)
    RaceSystem race = InitRace();


    // --- 2. THE MAIN GAME LOOP ---
    // This loop runs 60 times per second until the user clicks the X or presses ESC.
    while (!WindowShouldClose()) {
        
        // --- A) UPDATE PHASE (LOGIC & MATH) ---
        // Here we process input and move things, but we DO NOT draw anything yet.
        if (currentState == STATE_MENU) {
            
            // --- MENU AUDIO LOGIC ---
            // UpdateMusicStream must be called every single frame to keep the buffer flowing
            UpdateMusicStream(menuMusic);
            
            // If the music isn't playing yet, start it
            if (!IsMusicStreamPlaying(menuMusic)) {
                PlayMusicStream(menuMusic);
            }
            
            // If we are in the menu, wait for the user to press 1 or 2
            if (IsKeyPressed(KEY_ONE)) {
                StopMusicStream(menuMusic);         // Stop the chill music!
                player = InitPlayer(VEHICLE_PLANE); // Initialize as a plane
                currentState = STATE_PLAYING;       // Change the state to start the game!
            } else if (IsKeyPressed(KEY_TWO)) {
                StopMusicStream(menuMusic);              // Stop the chill music!
                player = InitPlayer(VEHICLE_HELICOPTER); // Initialize as a helicopter
                currentState = STATE_PLAYING;            // Change the state to start the game!
            }
            
        } else if (currentState == STATE_PLAYING) {
            
            // --- MID-FLIGHT VEHICLE SWITCHING ---
            if (IsKeyPressed(KEY_ONE)) player.type = VEHICLE_PLANE;
            if (IsKeyPressed(KEY_TWO)) player.type = VEHICLE_HELICOPTER;

            // --- QUICK RESTART ---
            // If the player makes a mistake, press R to restart the race instantly
            if (IsKeyPressed(KEY_R)) {
                race = InitRace();                // Resets all rings and stopwatch
                player = InitPlayer(player.type); // Teleports player back to origin
            }

            // If we are playing, update the vehicle's physics.
            UpdatePlayer(&player);

            // If we are playing, update the vehicle's physics.
            // Notice the '&' (address-of operator). We are passing a POINTER to our player
            // so the UpdatePlayer function can modify the real data, not a copy.
            UpdatePlayer(&player);

            // Update the race logic (Stopwatch and ring collisions)
            // We pass pointers to both the race and the player so the referee can check distances.
            UpdateRace(&race, &player);

            // --- DYNAMIC AUDIO LOGIC ---
            if (player.type == VEHICLE_PLANE) {
                // Mute helicopter if it was playing
                if (IsSoundPlaying(helicopterSound)) StopSound(helicopterSound);
                
                // Ensure the plane engine sound is looping
                if (!IsSoundPlaying(planeSound)) PlaySound(planeSound);
                
                // Modify the pitch based on the throttle.
                // Throttle is negative when moving forward, so we invert it.
                float pitch = 1.0f + (-player.throttle * 0.8f);
                SetSoundPitch(planeSound, pitch);
                
            } else if (player.type == VEHICLE_HELICOPTER) {
                // Mute plane if it was playing
                if (IsSoundPlaying(planeSound)) StopSound(planeSound);
                
                // Ensure the helicopter rotor sound is looping
                if (!IsSoundPlaying(helicopterSound)) PlaySound(helicopterSound);
                
                // The helicopter changes pitch slightly when ascending or descending
                float pitch = 1.0f + (player.velocity.y * 0.2f);
                SetSoundPitch(helicopterSound, pitch);
            }
            
            // --- DYNAMIC CAMERA LOGIC ---
            // Toggle camera mode when pressing 'C'
            if (IsKeyPressed(KEY_C)) isFirstPerson = !isFirstPerson;
            
            // NEW: Toggle controls visibility when pressing 'H'
            if (IsKeyPressed(KEY_H)) showControls = !showControls;

            if (isFirstPerson) {
                // --- 1st PERSON (COCKPIT) ---
                // Place camera exactly at player's position, slightly elevated for eye level
                camera.position = (Vector3){ player.position.x, player.position.y + 0.5f, player.position.z };
                
                // Calculate exactly where the nose of the vehicle is pointing using Trigonometry
                // (We use negative Sine/Cosine because moving forward means moving into -Z)
                camera.target.x = camera.position.x - (sinf(player.rotation.y) * cosf(player.rotation.x));
                camera.target.y = camera.position.y +  sinf(player.rotation.x); 
                camera.target.z = camera.position.z - (cosf(player.rotation.y) * cosf(player.rotation.x));
                
            } else {
                // --- 3rd PERSON (CHASE) ---
                camera.target = player.position;
                float cameraDistance = 4.0f; 
                float cameraHeight = 1.5f;   

                camera.position.x = player.position.x + (sinf(player.rotation.y) * cameraDistance);
                camera.position.y = player.position.y + cameraHeight;
                camera.position.z = player.position.z + (cosf(player.rotation.y) * cameraDistance);
            }
        }

        // --- B) DRAW PHASE (RENDERING) ---
        // Now that all the math is done, we paint the results onto the screen.
        BeginDrawing();
        
        // Wipe the previous frame clean with a sky blue color.
        // If we don't do this, the screen will smear like a broken Windows XP window.
        ClearBackground(SKYBLUE);

        if (currentState == STATE_MENU) {
            
            // --- MAIN MENU (DYNAMIC RESOLUTION) ---
            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();
            
            const char *title = "SIMPLE FLIGHT SIMULATOR";
            int titleWidth = MeasureText(title, 50); 
            // Placed at 25% of the screen height
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight * 0.25f, 50, DARKBLUE);

            const char *author = "Author: Benito Fernandez";
            int authorWidth = MeasureText(author, 35); 
            // Placed slightly below the title (approx 33% height)
            DrawText(author, (screenWidth - authorWidth) / 2, screenHeight * 0.33f, 35, WHITE);
            
            const char *opt1 = "Press [1] to fly the SR-71 Blackbird";
            int opt1Width = MeasureText(opt1, 25); 
            // Placed at 45% of the screen height
            DrawText(opt1, (screenWidth - opt1Width) / 2, screenHeight * 0.45f, 25, DARKGRAY);
            
            const char *opt2 = "Press [2] to fly the AH-64 Apache";
            int opt2Width = MeasureText(opt2, 25);
            // Placed at 52% of the screen height
            DrawText(opt2, (screenWidth - opt2Width) / 2, screenHeight * 0.52f, 25, DARKGRAY);
            
        } else if (currentState == STATE_PLAYING) {
            
            // Switch Raylib into 3D rendering mode using our camera
            BeginMode3D(camera);
                
                // We draw the skybox exactly where the camera is. 
                // This creates the optical illusion that the sky is infinitely far away.
                DrawModel(skyboxModel, camera.position, 1.0f, WHITE);

                // Draw terrain
                DrawModel(environmentModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

                // Draw the floating 3D rings and the navigation arrow for the race
                DrawRace3D(&race, &player);


                // Decide which model to use
                Model *currentModel;
                if (player.type == VEHICLE_PLANE) {
                    currentModel = &planeModel;
                } else {
                    currentModel = &helicopterModel;
                }
                
                // 1. Save the model's base matrix
                Matrix baseTransform = currentModel->transform;
                
                // 2. Generate a dynamic rotation matrix (STRICT AEROSPACE ORDER)
                // By multiplying matrices individually (Roll -> Pitch -> Yaw), 
                // we prevent the axes from mixing up when pressing multiple keys (Gimbal Lock).
                Matrix matRoll = MatrixRotateZ(player.rotation.z);
                Matrix matPitch = MatrixRotateX(player.rotation.x);
                Matrix matYaw = MatrixRotateY(player.rotation.y);
                
                // Multiply Roll and Pitch first, then apply Yaw
                Matrix dynamicRotation = MatrixMultiply(matRoll, matPitch);
                dynamicRotation = MatrixMultiply(dynamicRotation, matYaw);
                
                // 3. Combine them and apply temporarily
                currentModel->transform = MatrixMultiply(baseTransform, dynamicRotation);
                
                // 4. Draw the 3D model
                if (!isFirstPerson) {
                    if (player.type == VEHICLE_PLANE) {
                        DrawModel(*currentModel, player.position, 0.08f, WHITE); 
                    } else if (player.type == VEHICLE_HELICOPTER) {
                        DrawModel(*currentModel, player.position, 0.8f, WHITE);
                    }
                }

                // 5. Restore the base matrix
                currentModel->transform = baseTransform;

                // 6. Draw smoke particles
                // We loop through the pool and draw a cube for every active particle
                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (player.smoke[i].active) {
                        
                        // White with transparency (Max alpha of 0.6f so it's not fully opaque)
                        Color smokeColor = Fade(WHITE, player.smoke[i].life * 0.6f);
                        
                        // Smaller size: Starts at 0.1 and grows up to 0.8
                        float size = 0.1f + ((1.0f - player.smoke[i].life) * 0.7f);
                        
                        DrawSphere(player.smoke[i].position, size, smokeColor);
                    }
                }
                
            EndMode3D(); // Switch back to 2D rendering mode

            // --- UI CONTROLS (TOGGLEABLE) ---
            if (showControls) {
                // Main controls help
                DrawText("W/S: Throttle | A/D: Yaw/Roll | SPACE/SHIFT: Pitch", 20, 20, 20, DARKGRAY);
            } else {
                // A tiny, non-intrusive reminder when hidden
                DrawText("Press [H] to show controls", 20, 20, 10, GRAY);
            }

            // Draw the Race Stopwatch and remaining rings on the screen (Centered)
            DrawRaceUI(&race);


            // --- AERONAUTICAL HUD (DYNAMIC RESOLUTION) ---
            int screenHeight = GetScreenHeight();
            
            // Anchored 100 pixels above the bottom edge
            DrawText(TextFormat("ALTITUDE: %.0f ft", player.position.y * 10.0f), 20, screenHeight - 100, 20, LIME);

            // Calculate max throttle depending on the current vehicle
            float maxThrottle;
            if (player.type == VEHICLE_PLANE) {
                maxThrottle = 0.8f;
            } else {
                maxThrottle = 0.4f;
            }

            // Calculate true percentage (Current / Max * 100)
            int powerPercentage = (int)((-player.throttle / maxThrottle) * 100.0f);
            
            // Draw the exact power percentage
            DrawText(TextFormat("POWER: %d %%", powerPercentage), 20, 840, 20, LIME);
        }

        EndDrawing(); // Tell Raylib we are done painting this frame, display it!
    }

    // --- 3. TEARDOWN (CLEANUP) ---
    // The loop is over (user closed the game). Time to clean up.
    UnloadGameResources(); // Our custom function to free RAM
    CloseAudioDevice();    // Close audio device AFTER unloading resources
    CloseWindow();         // Raylib's function to close the OS window safely
    return 0;              // Tell Windows the program finished successfully
}