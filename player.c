// We include our own header file so this .c file knows what a 'Player' is.
#include "player.h"

// --- FACTORY FUNCTION ---
// This function creates a new Player from scratch and hands it back to main.c.
// It returns a full copy of the 'Player' struct.
Player InitPlayer(VehicleType type) {
    // We create a local Player variable and initialize all its memory to 0.
    // { 0 } is a great C trick to ensure no garbage data is left in memory.
    Player p = { 0 };
    
    // Set the starting physical properties.
    p.position = (Vector3){ 0.0f, 1.0f, 0.0f };   // Start 1 unit above the ground
    p.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };   // Start completely stationary
    p.throttle = 0.0f;                            // Engine is at 0%
    p.acceleration = 0.005f;                      // Engine power per frame
    p.friction = 0.95f;                           // Air resistance/drag (loses 5% of speed per frame)
    p.type = type;                                // Assign the chosen vehicle model
    
    // Hand the finished package back to whoever called this function.
    return p;
}

// --- UPDATE LOOP ---
// This function runs 60 times per second to update the vehicle's physics and visual tilt.
// We pass a POINTER (*player) so we edit the actual player in main.c, not a local copy.
void UpdatePlayer(Player *player) {
    
    // --- 1. THROTTLE (ENGINE POWER) ---
    // Instead of moving step-by-step, W and S now control the engine's power.
    // Remember: In Raylib's 3D space, negative Z (-Z) means moving FORWARD into the screen.
    if (IsKeyDown(KEY_W)) player->throttle -= player->acceleration; 
    if (IsKeyDown(KEY_S)) player->throttle += player->acceleration;

    // We must clamp (limit) the throttle so the vehicle doesn't accelerate to infinity.
    // -0.6f is the maximum forward speed, and 0.1f is the maximum reverse speed.
    if (player->throttle < -0.6f) player->throttle = -0.6f; 
    if (player->throttle > 0.1f) player->throttle = 0.1f;   

    // Apply the current engine power directly to the Z velocity.
    // As long as there is throttle, the plane will keep moving forward automatically.
    player->velocity.z = player->throttle;

    // --- 2. TARGET ROTATIONS (VISUAL TILT) ---
    // We create temporary variables to store the angle the player WANTS to reach this frame.
    float targetRoll = 0.0f;  // Roll (Alabeo): Tilting wings left/right
    float targetPitch = 0.0f; // Pitch (Cabeceo): Pointing nose up/down

    // X Axis: Left/Right Movement and Roll
    if (IsKeyDown(KEY_A)) {
        player->velocity.x -= player->acceleration * 2.0f; // Move left
        targetRoll = -0.4f;                                // Target tilt left (in radians)
    }
    if (IsKeyDown(KEY_D)) {
        player->velocity.x += player->acceleration * 2.0f; // Move right
        targetRoll = 0.4f;                                 // Target tilt right
    }

    // Y Axis: Up/Down Movement and Pitch
    if (IsKeyDown(KEY_SPACE)) {
        player->velocity.y += player->acceleration * 4.0f; // Gain altitude
        targetPitch = 0.3f;                                // Target nose up
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        player->velocity.y -= player->acceleration * 4.0f; // Lose altitude
        targetPitch = -0.3f;                               // Target nose down
    }

    // --- 3. APPLY FRICTION (MOMENTUM DECAY) ---
    // We only apply friction to the X (sideways) and Y (vertical) axes.
    // We DO NOT apply friction to Z anymore, because the throttle handles continuous Z movement.
    player->velocity.x *= player->friction;
    player->velocity.y *= player->friction;

    // --- 4. UPDATE POSITION ---
    // Update the actual coordinates in the 3D world based on current velocity.
    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    player->position.z += player->velocity.z;

    // --- 5. SMOOTH INTERPOLATION (MAGIC MATH) ---
    // Lerp (Linear Interpolation) gradually changes a value towards a target over time.
    // Instead of snapping instantly to the target angle, the plane smoothly rotates 
    // 5% (0.05f) closer to the target angle every frame.
    player->rotation.z = Lerp(player->rotation.z, targetRoll, 0.05f);  // Smooth Roll
    player->rotation.x = Lerp(player->rotation.x, targetPitch, 0.05f); // Smooth Pitch

    // --- 6. COLLISION DETECTION (THE FLOOR) ---
    if (player->position.y < 0.5f) {
        player->position.y = 0.5f; // Prevent sinking into the ground
        
        // Kill downward velocity so it doesn't accumulate while grounded
        if (player->velocity.y < 0.0f) {
            player->velocity.y = 0.0f;
        }
        
        // When touching the ground, smoothly force the nose back to a level position (0.0f)
        player->rotation.x = Lerp(player->rotation.x, 0.0f, 0.1f);
    }
}