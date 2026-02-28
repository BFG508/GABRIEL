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
// This function runs 60 times per second.
// We pass a POINTER (*player) so we edit the actual player in main.c, not a local copy.
void UpdatePlayer(Player *player) {
    
    // --- 1. READ INPUT & APPLY ACCELERATION ---
    // Because 'player' is a pointer, we MUST use the arrow operator (->) to access its contents.
    // Using player->velocity is a shortcut for (*player).velocity in C.
    
    // Left/Right (X Axis)
    if (IsKeyDown(KEY_A)) player->velocity.x -= player->acceleration;
    if (IsKeyDown(KEY_D)) player->velocity.x += player->acceleration;
    
    // Up/Down (Y Axis)
    if (IsKeyDown(KEY_SPACE)) player->velocity.y += player->acceleration;
    if (IsKeyDown(KEY_LEFT_SHIFT)) player->velocity.y -= player->acceleration;
    
    // Forward/Backward (Z Axis)
    if (IsKeyDown(KEY_W)) player->velocity.z -= player->acceleration;
    if (IsKeyDown(KEY_S)) player->velocity.z += player->acceleration;

    // --- 2. APPLY FRICTION (MOMENTUM DECAY) ---
    // Multiply the current velocity by 0.95.
    // If we let go of the keys, the velocity gets smaller and smaller until it reaches 0.
    // This creates the smooth "sliding" or "drifting" effect.
    player->velocity.x *= player->friction;
    player->velocity.y *= player->friction;
    player->velocity.z *= player->friction;

    // --- 3. UPDATE POSITION ---
    // The core rule of physics in games: Position = Position + Velocity
    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    player->position.z += player->velocity.z;

    // --- 4. COLLISION DETECTION (THE FLOOR) ---
    // If the vehicle's Y position drops below 0.5 (half the height of our old cube),
    // we force it back to 0.5 so it doesn't sink into the grid.
    if (player->position.y < 0.5f) {
        player->position.y = 0.5f;
        
        // If we hit the floor while moving downwards (negative Y velocity),
        // we kill the downward momentum so we don't build up negative speed while resting on the ground.
        if (player->velocity.y < 0.0f) {
            player->velocity.y = 0.0f;
        }
    }
}