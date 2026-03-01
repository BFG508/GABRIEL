// We include our own header file so this .c file knows what a 'Player' is.
// We include resource_manager.h so this .c file knows about the 'environmentModel'.
// Include the math library for trigonometric functions.
#include "player.h"
#include "resource_manager.h"
#include <math.h>

// --- FACTORY FUNCTION ---
// This function creates a new Player from scratch and hands it back to main.c.
// It returns a full copy of the 'Player' struct.
Player InitPlayer(VehicleType type) {
    // We create a local Player variable and initialize all its memory to 0.
    // { 0 } is a great C trick to ensure no garbage data is left in memory.
    Player p = { 0 };
    
    // Set the starting physical properties.
    p.position = (Vector3){ 0.0f, 5.0f, 0.0f };   // Start 5 unit above the ground
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

    // Limit the throttle based on the vehicle type so it doesn't accelerate to infinity.
    if (player->type == VEHICLE_PLANE) {
        // The plane cannot go backwards. Max throttle is -0.8f, Min is 0.0f (engine idle).
        if (player->throttle < -0.8f) player->throttle = -0.8f; 
        if (player->throttle > 0.0f) player->throttle = 0.0f;   
    } 
    else if (player->type == VEHICLE_HELICOPTER) {
        // Helicopters are slower, but we allow a slight backward movement.
        if (player->throttle < -0.4f) player->throttle = -0.4f; 
        if (player->throttle > 0.1f) player->throttle = 0.1f;   
    }

    // Apply the current engine power directly to the Z velocity.
    player->velocity.z = player->throttle;

    // --- 2. STEERING (YAW) & VISUAL TILT (ROLL/PITCH) ---
    // We create temporary variables to store the angle the player WANTS to reach this frame.
    float targetRoll = 0.0f;  // Roll: Tilting wings left/right
    float targetPitch = 0.0f; // Pitch: Pointing nose up/down

    // A and D now ROTATE the vehicle (Yaw) instead of just sliding it sideways
    if (IsKeyDown(KEY_A)) {
        player->rotation.y += 0.02f; // Rotate nose left
        targetRoll = 0.4f;           // Target tilt left
    }
    if (IsKeyDown(KEY_D)) {
        player->rotation.y -= 0.02f; // Rotate nose right
        targetRoll = -0.4f;          // Target tilt right
    }

    // --- 3. CALCULATE DIRECTION VECTOR (TRIGONOMETRY) ---
    // We use Sine and Cosine to distribute the engine's power across the X and Z axes
    // based on the direction the vehicle is currently facing (Yaw angle).
    player->velocity.x = player->throttle * sinf(player->rotation.y);
    player->velocity.z = player->throttle * cosf(player->rotation.y);

    // --- 4. PHYSICS: GRAVITY & LIFT ---
    // A constant downward force pulling the vehicle to the ground every frame.
    float gravity = 0.008f;
    player->velocity.y -= gravity; 

    if (player->type == VEHICLE_PLANE) {
        // --- PLANE PHYSICS ---
        // Forward speed is now simply the absolute power of the throttle
        float forwardSpeed = -player->throttle; 
        if (forwardSpeed < 0.0f) forwardSpeed = 0.0f; // No lift when reversing

        // Generate Lift based on speed to counteract gravity.
        float lift = forwardSpeed * 0.027f; 
        player->velocity.y += lift;

        // Pitch up/down only works well if we have forward speed (airflow over the wings)
        if (IsKeyDown(KEY_SPACE)) {
            player->velocity.y += forwardSpeed * 0.05f; // Use speed to climb
            targetPitch = 0.3f;                         
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            player->velocity.y -= forwardSpeed * 0.05f; // Dive
            targetPitch = -0.3f;                        
        }
    } 
    else if (player->type == VEHICLE_HELICOPTER) {
        // --- HELICOPTER PHYSICS ---
        // Helicopters don't need forward speed to fly, they use raw rotor power
        if (IsKeyDown(KEY_SPACE)) {
            // Rotor thrust must be stronger than gravity to climb
            player->velocity.y += player->acceleration * 3.0f; 
            targetPitch = 0.15f; 
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            // Reduce collective (drop faster than normal gravity)
            player->velocity.y -= player->acceleration * 2.0f;
            targetPitch = -0.15f; 
        }
    }

    // --- 5. APPLY FRICTION (MOMENTUM DECAY) ---
    // We ONLY apply friction to the Y axis (vertical). X and Z are engine-driven.
    player->velocity.y *= player->friction;

    // --- 6. ADVANCED COLLISION DETECTION (DUAL RAYCASTING) ---
    // A) Forward Ray (Crashes) - Detects mountains in front of the aircraft
    Ray forwardRay = { 0 };
    forwardRay.position = player->position;
    forwardRay.direction = (Vector3){ 
        sinf(player->rotation.y) * cosf(player->rotation.x), 
        -sinf(player->rotation.x), 
        cosf(player->rotation.y) * cosf(player->rotation.x) 
    };

    // B) Satellite Ray (Ground detection) - Shoots from 1000 units directly downwards
    Ray downRay = { 0 };
    downRay.position = (Vector3){ player->position.x, 1000.0f, player->position.z };
    downRay.direction = (Vector3){ 0.0f, -1.0f, 0.0f };

    bool hasCrashed = false;
    float groundHeight = 0.5f; // Default flat ground fallback

    for (int i = 0; i < environmentModel.meshCount; i++) {
        // Check Forward Crash
        RayCollision forwardHit = GetRayCollisionMesh(forwardRay, environmentModel.meshes[i], environmentModel.transform);
        if (forwardHit.hit && forwardHit.distance < 2.0f) {
            hasCrashed = true;
        }

        // Check 3D Floor underneath
        RayCollision groundHit = GetRayCollisionMesh(downRay, environmentModel.meshes[i], environmentModel.transform);
        if (groundHit.hit) {
            // Keep the highest ground point found (in case meshes overlap)
            if (groundHit.point.y > groundHeight) {
                groundHeight = groundHit.point.y;
            }
        }
    }

    // Crash logic: Kill the engine and PUSH BACK
    if (hasCrashed) {
        player->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        player->throttle = 0.0f;
        
        // Push the plane slightly backward so it escapes the 2.0f trap zone!
        player->position.x -= forwardRay.direction.x * 0.5f;
        player->position.y -= forwardRay.direction.y * 0.5f;
        player->position.z -= forwardRay.direction.z * 0.5f;
    }

    // --- 7. UPDATE POSITION ---
    // Update the actual coordinates in the 3D world based on current velocity.
    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    player->position.z += player->velocity.z;

    // --- 8. SMOOTH INTERPOLATION ---
    // Lerp gradually changes the current rotation towards the target rotation over time.
    player->rotation.z = Lerp(player->rotation.z, targetRoll, 0.05f);  
    player->rotation.x = Lerp(player->rotation.x, targetPitch, 0.05f); 

    // --- 9. DYNAMIC FLOOR COLLISION ---
    // We now use the actual 3D ground height from our satellite ray!
    float safeFloor = groundHeight + 0.5f; // +0.5f so the belly rests on the ground

    if (player->position.y < safeFloor) {
        player->position.y = safeFloor; 
        
        // Kill downward velocity so it doesn't accumulate while grounded
        if (player->velocity.y < 0.0f) {
            player->velocity.y = 0.0f;
        }
        
        // When touching the ground, smoothly force the nose back to a level position
        player->rotation.x = Lerp(player->rotation.x, 0.0f, 0.1f);
    }
}