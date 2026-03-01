// We include our own header file so this .c file knows what a 'Player' is.
// Include the math library for trigonometric functions.
#include "player.h"
#include <math.h>

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
    if (IsKeyDown(KEY_W)) player->throttle -= player->acceleration; 
    if (IsKeyDown(KEY_S)) player->throttle += player->acceleration;

    // We must clamp (limit) the throttle so the vehicle doesn't accelerate to infinity.
    // -0.6f is the maximum forward speed, and 0.1f is the maximum reverse speed.
    if (player->throttle < -0.6f) player->throttle = -0.6f; 
    if (player->throttle > 0.1f) player->throttle = 0.1f;   

    // --- 2. STEERING (YAW) & VISUAL TILT (ROLL/PITCH) ---
    // We create temporary variables to store the angle the player WANTS to reach this frame.
    float targetRoll = 0.0f;  // Roll (Alabeo): Tilting wings left/right
    float targetPitch = 0.0f; // Pitch (Cabeceo): Pointing nose up/down

    // A and D now ROTATE the vehicle (Yaw) instead of just sliding it sideways
    if (IsKeyDown(KEY_A)) {
        player->rotation.y += 0.02f; // Rotate nose left (Yaw)
        targetRoll = 0.4f;           // Target tilt left (positive)
    }
    if (IsKeyDown(KEY_D)) {
        player->rotation.y -= 0.02f; // Rotate nose right (Yaw)
        targetRoll = -0.4f;          // Target tilt right (negative)
    }

    // --- 3. CALCULATE DIRECTION VECTOR (TRIGONOMETRY) ---
    // We use Sine and Cosine to distribute the engine's power across the X and Z axes
    // based on the direction the vehicle is currently facing (Yaw angle).
    // NOTE: Requires #include <math.h> at the top of the file.
    player->velocity.x = player->throttle * sinf(player->rotation.y);
    player->velocity.z = player->throttle * cosf(player->rotation.y);

    // --- 4. PHYSICS: GRAVITY & LIFT ---
    // A constant downward force pulling the vehicle to the ground every frame.
    float gravity = 0.008f;
    player->velocity.y -= gravity; 

    if (player->type == VEHICLE_PLANE) {
        // --- PLANE PHYSICS ---
        // Forward speed is now simply the absolute power of the throttle 
        // (Since throttle is negative when moving forward, we invert it to get a positive speed)
        float forwardSpeed = -player->throttle; 
        if (forwardSpeed < 0.0f) forwardSpeed = 0.0f; // No lift when reversing

        // Generate Lift based on speed. 
        // If throttle is halfway (-0.3f), lift cancels out gravity (0.3 * 0.027 ≈ 0.008)
        float lift = forwardSpeed * 0.027f; 
        player->velocity.y += lift;

        // Pitch up/down only works well if we have forward speed (airflow over the wings)
        if (IsKeyDown(KEY_SPACE)) {
            player->velocity.y += forwardSpeed * 0.05f; // Use speed to climb
            targetPitch = 0.3f;                         // Target nose up
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            player->velocity.y -= forwardSpeed * 0.05f; // Dive
            targetPitch = -0.3f;                        // Target nose down
        }
    } 
    else if (player->type == VEHICLE_HELICOPTER) {
        // --- HELICOPTER PHYSICS ---
        // Helicopters don't need forward speed to fly, they use raw rotor power pushing downwards
        if (IsKeyDown(KEY_SPACE)) {
            // Rotor thrust must be stronger than gravity to climb
            player->velocity.y += player->acceleration * 3.0f; 
            targetPitch = 0.15f; // Nose tips up slightly when ascending
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            // Reduce collective (drop faster than normal gravity)
            player->velocity.y -= player->acceleration * 2.0f;
            targetPitch = -0.15f; // Nose tips down slightly
        }
    }

    // --- 5. APPLY FRICTION (MOMENTUM DECAY) ---
    // We ONLY apply friction to the Y axis (vertical) now. 
    // X and Z DO NOT get friction because they are fully controlled by the engine and steering.
    player->velocity.y *= player->friction;

    // --- 6. UPDATE POSITION ---
    // Update the actual coordinates in the 3D world based on current velocity.
    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    player->position.z += player->velocity.z;

    // --- 7. SMOOTH INTERPOLATION (MAGIC MATH) ---
    // Lerp (Linear Interpolation) gradually changes a value towards a target over time.
    player->rotation.z = Lerp(player->rotation.z, targetRoll, 0.05f);  // Smooth Roll
    player->rotation.x = Lerp(player->rotation.x, targetPitch, 0.05f); // Smooth Pitch

    // --- 8. COLLISION DETECTION (THE FLOOR) ---
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