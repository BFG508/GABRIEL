// Include the main Raylib libraries
#include "raylib.h"
#include "raymath.h"

// Notice we use quotes "" for our own files, and angle brackets <> for system libraries.
#include "player.h"
#include "resource_manager.h"

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
    InitWindow(800, 600, "Simple Flight Simulator");
    SetTargetFPS(60); // Force the game to run at a stable 60 Frames Per Second

    // Call our custom module to load heavy files (models/textures) into RAM
    LoadGameResources(); 

    // Set the initial game state to show the menu first
    GameState currentState = STATE_MENU;
    
    // Create an empty player. It will be properly initialized when the user selects a vehicle.
    Player player = { 0 };
    
    // Setup the 3D camera
    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Defines which way is "up" (Y axis)
    camera.fovy = 60.0f;                       // Field of View (zoom level)
    camera.projection = CAMERA_PERSPECTIVE;    // Gives a realistic 3D depth effect

    // --- 2. THE MAIN GAME LOOP ---
    // This loop runs 60 times per second until the user clicks the X or presses ESC.
    while (!WindowShouldClose()) {
        
        // --- A) UPDATE PHASE (LOGIC & MATH) ---
        // Here we process input and move things, but we DO NOT draw anything yet.
        if (currentState == STATE_MENU) {
            
            // If we are in the menu, wait for the user to press 1 or 2
            if (IsKeyPressed(KEY_ONE)) {
                player = InitPlayer(VEHICLE_PLANE); // Initialize as a plane
                currentState = STATE_PLAYING;       // Change the state to start the game!
            } else if (IsKeyPressed(KEY_TWO)) {
                player = InitPlayer(VEHICLE_HELICOPTER); // Initialize as a helicopter
                currentState = STATE_PLAYING;            // Change the state to start the game!
            }
            
        } else if (currentState == STATE_PLAYING) {
            
            // --- MID-FLIGHT VEHICLE SWITCHING ---
            // We just change the 'type' variable. We DON'T call InitPlayer() again
            // because that would reset our position and velocity back to zero!
            if (IsKeyPressed(KEY_ONE)) player.type = VEHICLE_PLANE;
            if (IsKeyPressed(KEY_TWO)) player.type = VEHICLE_HELICOPTER;

            // If we are playing, update the vehicle's physics.
            // Notice the '&' (address-of operator). We are passing a POINTER to our player
            // so the UpdatePlayer function can modify the real data, not a copy.
            UpdatePlayer(&player);
            
            // Make the camera follow the player's new position
            camera.target = player.position;
            camera.position = (Vector3){ player.position.x, player.position.y + 1.5f, player.position.z + 4.0f };
        }

        // --- B) DRAW PHASE (RENDERING) ---
        // Now that all the math is done, we paint the results onto the screen.
        BeginDrawing();
        
        // Wipe the previous frame clean with a sky blue color.
        // If we don't do this, the screen will smear like a broken Windows XP window.
        ClearBackground(SKYBLUE);

        if (currentState == STATE_MENU) {
            
            // Draw 2D text for the Main Menu
            DrawText("FLIGHT SIMULATOR", 220, 200, 30, DARKBLUE);
            DrawText("Press [1] to fly the SR-71 Blackbird", 200, 300, 20, DARKGRAY);
            DrawText("Press [2] to fly the AH-64 Apache", 200, 350, 20, DARKGRAY);
            
        } else if (currentState == STATE_PLAYING) {
            
            // Switch Raylib into 3D rendering mode using our camera
            BeginMode3D(camera);
                
                // Draw a visual reference so we can perceive speed and movement
                DrawGrid(10000, 2.0f);

                // Decide which model to use
                Model *currentModel = (player.type == VEHICLE_PLANE) ? &planeModel : &helicopterModel;
                
                // 1. Save the model's base matrix (which includes our permanent fix for crooked models)
                Matrix baseTransform = currentModel->transform;
                
                // 2. Generate a dynamic rotation matrix based on the player's current Pitch and Roll
                Matrix dynamicRotation = MatrixRotateXYZ((Vector3){ player.rotation.x, player.rotation.y, player.rotation.z });
                
                // 3. Combine them and apply temporarily
                currentModel->transform = MatrixMultiply(baseTransform, dynamicRotation);
                
                // 4. Draw the 3D model based on what the player chose.
                // The float number is the scale multiplier.
                if (player.type == VEHICLE_PLANE) {
                    DrawModel(*currentModel, player.position, 0.08f, WHITE); 
                } else if (player.type == VEHICLE_HELICOPTER) {
                    DrawModel(*currentModel, player.position, 0.8f, WHITE);
                }

                // 5. Restore the base matrix so it doesn't spin uncontrollably next frame!
                currentModel->transform = baseTransform;
                
            EndMode3D(); // Switch back to 2D rendering mode

            // Draw the User Interface (UI) on top of the 3D world
            DrawText("W/S: Throttle | A/D: Roll | SPACE/SHIFT: Pitch", 10, 10, 20, DARKGRAY);
        }

        EndDrawing(); // Tell Raylib we are done painting this frame, display it!
    }

    // --- 3. TEARDOWN (CLEANUP) ---
    // The loop is over (user closed the game). Time to clean up.
    UnloadGameResources(); // Our custom function to free RAM
    CloseWindow();         // Raylib's function to close the OS window safely
    return 0;              // Tell Windows the program finished successfully
}