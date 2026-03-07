// Include math library to use advanced mathematical functions.
#include <math.h>

// We include our own header file.
// We also need resource_manager.h so this .c file knows about the 'environmentModel'.
#include "player.h"
#include "resource_manager.h"


// --- FACTORY FUNCTION ---
// This function creates a new Player from scratch and hands it back to main.c.
// It returns a full copy of the 'Player' struct.
Player InitPlayer(VehicleType type, Vector3 startPos, float startYaw) {
    // We create a local Player variable and initialize all its memory to 0.
    // { 0 } is a great C trick to ensure no garbage data is left in memory.
    Player p = { 0 };
    
    // --- 1. PHYSICAL PROPERTIES ---
    // Set the starting physical properties based on the level data.
    p.position = startPos;
    p.rotation = (Vector3){ 0.0f, startYaw, 0.0f }; // Set initial yaw (direction the nose is pointing).
    p.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };     // Start completely stationary.

    p.throttle = 0.0f;                              // Engine is at 0% power.
    p.acceleration = 0.010f;                        // Engine power gained per frame when accelerating.
    p.friction = 0.95f;                             // Air resistance/drag (loses 5% of vertical momentum per frame).
    
    p.type = type;                                  // Assign the chosen vehicle model (Plane or Helicopter).
    
    // --- 2. CAMERA STATE ---
    // Initialize the default camera perspective for the new flight.
    p.isFirstPerson = false;                        // Default to the 3rd person orbit camera view.
    p.cameraAngleYaw = 0.0f;                        // Reset the manual horizontal camera rotation.
    p.cameraAnglePitch = 0.0f;                      // Reset the manual vertical camera rotation.

    // Hand the finished package back to whoever called this function (passed by value).
    return p;
}


// --- UPDATE LOOP ---
// This function runs 60 times per second to update the vehicle's physics and visual tilt.
// We pass a POINTER (*player) so we edit the actual player in main.c, not a local copy.
void UpdatePlayer(Player *player) {
    
    // --- 0. DELTA TIME (TIME SCALE) ---
    // GetFrameTime() tells us the real time passed since last frame.
    // By multiplying it by 60.0f, we get a scale factor.
    float dtScale = GetFrameTime() * 60.0f;

    // --- 1. THROTTLE (ENGINE POWER) ---
    // Keyboard inputs.
    if (IsKeyDown(KEY_W)) player->throttle -= player->acceleration; 
    if (IsKeyDown(KEY_S)) player->throttle += player->acceleration;

    // Gamepad inputs (RT to accelerate, LT to brake/reverse).
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) player->throttle -= player->acceleration;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_2))  player->throttle += player->acceleration;
    }

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
    // We create temporary variables to store the angle the player wants to reach this frame.
    float targetRoll = 0.0f;  // Roll: Tilting wings left/right.
    float targetPitch = 0.0f; // Pitch: Pointing nose up/down.

    // Keyboard Yaw (A/D)
    if (IsKeyDown(KEY_A)) {
        player->rotation.y += 0.02f * dtScale; // Rotate nose left.
        targetRoll = 0.4f;                     // Target tilt left.
    }
    if (IsKeyDown(KEY_D)) {
        player->rotation.y -= 0.02f * dtScale; // Rotate nose right.
        targetRoll = -0.4f;                    // Target tilt right.
    }

    // Gamepad yaw/roll (Left stick x-axis).
    if (IsGamepadAvailable(0)) {
        float leftX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        
        // Deadzone check: We ignore inputs smaller than 0.15f to prevent stick drift.
        if (fabsf(leftX) > 0.15f) {
            player->rotation.y -= leftX * 0.02f * dtScale; 
            targetRoll = -leftX * 0.4f;          
        }
    }


    // --- 3. CALCULATE VELOCITY DIRECTION VECTOR (TRIGONOMETRY) ---
    // We use sine and cosine to distribute the engine's power across the X and Z axes
    // based on the direction the vehicle is currently facing (Yaw angle).
    player->velocity.x = player->throttle * sinf(player->rotation.y);
    player->velocity.z = player->throttle * cosf(player->rotation.y);


    // --- 4. PHYSICS: GRAVITY & LIFT ---
    // A constant downward force pulling the vehicle to the ground every frame.
    float gravity = 0.015f;
    player->velocity.y -= gravity; 

    if (player->type == VEHICLE_PLANE) {
        // --- PLANE PHYSICS ---
        // Forward speed is simply the absolute power of the throttle.
        float forwardSpeed = -player->throttle; 
        if (forwardSpeed < 0.0f) {
            forwardSpeed = 0.0f; // No lift when reversing.
        }

        // Generate lift based on speed to counteract gravity.
        float lift = forwardSpeed * 0.030f; 
        player->velocity.y += lift;

        // Pitch up/down only works well if we have forward speed (airflow over the wings).
        // Keyboard pitch.
        if (IsKeyDown(KEY_SPACE)) {
            player->velocity.y += forwardSpeed * 0.05f; // Use speed to climb.
            targetPitch = 0.3f;                         
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            player->velocity.y -= forwardSpeed * 0.05f; // Dive.
            targetPitch = -0.3f;                        
        }

        // Gamepad pitch (Left stick y-axis).
        // In aviation: Pulling stick BACK (positive Y) raises the nose. 
        if (IsGamepadAvailable(0)) {
            float leftY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            if (fabsf(leftY) > 0.15f) {
                player->velocity.y += forwardSpeed * 0.05f * leftY;
                targetPitch = 0.3f * leftY;
            }
        }
    }

    else if (player->type == VEHICLE_HELICOPTER) {
        // --- HELICOPTER PHYSICS ---
        // Helicopters don't need forward speed to fly, they use raw rotor power.
        if (IsKeyDown(KEY_SPACE)) {
            // Rotor thrust must be stronger than gravity to climb.
            player->velocity.y += player->acceleration * 3.0f;
            targetPitch = 0.15f;
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            // Reduce collective (drop faster than normal gravity).
            player->velocity.y -= player->acceleration * 2.0f;
            targetPitch = -0.15f;
        }

        // Gamepad pitch (Left stick y-axis)
        if (IsGamepadAvailable(0)) {
            float leftY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            if (fabsf(leftY) > 0.15f) {
                player->velocity.y += player->acceleration * 3.0f * leftY;
                targetPitch = 0.15f * leftY;
            }
        }
    }


    // --- 5. APPLY FRICTION (MOMENTUM DECAY) ---
    // We only apply friction to the Y axis (vertical). X and Z are engine-driven.
    player->velocity.y *= player->friction;


    // --- 6. ADVANCED COLLISION DETECTION (DUAL RAYCASTING) ---
    // A) Forward Ray (Crashes): Detects mountains in front of the aircraft.
    Ray forwardRay = { 0 };
    forwardRay.position = player->position;
    forwardRay.direction = GetPlayerForwardVector(player);

    // B) Satellite Ray (Ground detection): Shoots from 1000 units directly downwards.
    Ray downRay = { 0 };
    downRay.position = (Vector3){ player->position.x, 1000.0f, player->position.z };
    downRay.direction = (Vector3){ 0.0f, -1.0f, 0.0f };

    // Crash conditions.
    bool hasCrashed = false;
    
    // We set the fallback to absolute zero (0.0f) instead of 0.5f
    // so the mathematical floor is exactly at Y = 0.
    float groundHeight = 0.0f; 

    for (int i = 0; i < environmentModel.meshCount; i++) {
        // Check forward crash.
        RayCollision forwardHit = GetRayCollisionMesh(forwardRay, environmentModel.meshes[i], environmentModel.transform);
        if (forwardHit.hit && forwardHit.distance < 2.0f) {
            hasCrashed = true;
        }

        // Check 3D floor underneath.
        RayCollision groundHit = GetRayCollisionMesh(downRay, environmentModel.meshes[i], environmentModel.transform);
        if (groundHit.hit) {
            // Keep the highest ground point found (in case meshes overlap).
            if (groundHit.point.y > groundHeight) {
                groundHeight = groundHit.point.y;
            }
        }
    }

    // Crash logic: Kill the engine and PUSH BACK.
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
    player->position.x += player->velocity.x * dtScale;
    player->position.y += player->velocity.y * dtScale;
    player->position.z += player->velocity.z * dtScale;


    // --- 8. SMOOTH INTERPOLATION ---
    // Lerp gradually changes the current rotation towards the target rotation over time.
    player->rotation.x = Lerp(player->rotation.x, targetPitch, 0.05f);
    player->rotation.z = Lerp(player->rotation.z, targetRoll, 0.05f);


    // --- 9. DYNAMIC FLOOR COLLISION ---
    // We use the actual 3D ground height from our satellite ray!
    float safeFloor = groundHeight + 0.5f; // +0.5f so the belly rests on the ground.

    if (player->position.y <= safeFloor) {
        player->position.y = safeFloor; 
        
        // Kill downward velocity so it doesn't accumulate while grounded.
        if (player->velocity.y < 0.0f) {
            player->velocity.y = 0.0f;
        }
        
        // When touching the ground, smoothly force the nose back to a level position.
        player->rotation.x = Lerp(player->rotation.x, 0.0f, 0.1f);
        
        // Zero-floor throttle kill.
        // If the player touches the absolute bottom (Y = 0, which means safeFloor is 0.5f).
        // or if they fly off the map where no mesh exists, we kill the engine.
        if (player->position.y <= 0.5f) {
            player->throttle = 0.0f;
        }
    }

    // --- 10. PARTICLE SYSTEM (TWIN SMOKE TRAILS) ---
    if (player->type != VEHICLE_PLANE) {
        // If it's not the plane, instantly kill all particles.
        for (int i = 0; i < MAX_PARTICLES; i++) {
            player->smoke[i].active = false;
        }
        player->smokeDelayTimer = 0.0f; // Reset the timer.
    } else {
        // If it is the plane, advance the timer.
        player->smokeDelayTimer += 1.0f  * dtScale;
        
        // 1. Emit smoke only if accelerating AND 1 second (60 frames) has passed since switching.
        if (player->throttle < -0.1f && player->smokeDelayTimer > 60.0f) {
            
            // Calculate the "Right" vector based on where we are looking (Yaw).
            float rightX =  cosf(player->rotation.y);
            float rightZ = -sinf(player->rotation.y);
            float engineOffset = 0.35f; // Distance from the center to each engine.
            
            int spawned = 0;
            
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (!player->smoke[i].active) {
                    player->smoke[i].active = true;
                    player->smoke[i].life = 1.0f; 
                    
                    // If spawned is 0, right engine (1.0). If 1, left engine (-1.0).
                    float sideDir;
                    
                    if (spawned == 0) {
                        sideDir = 1.0f;
                    } else {
                        sideDir = -1.0f;
                    }
                    
                    player->smoke[i].position = (Vector3){ 
                        player->position.x + (rightX * engineOffset * sideDir), 
                        player->position.y - 0.15f, 
                        player->position.z + (rightZ * engineOffset * sideDir) 
                    };

                    // Generate a tiny random velocity for horizontal spread (turbulence).
                    player->smoke[i].velocity.x = (float)GetRandomValue(-15, 15) / 1000.0f;
                    player->smoke[i].velocity.y = 0.02f; // Upward drift.
                    player->smoke[i].velocity.z = (float)GetRandomValue(-15, 15) / 1000.0f;
                    
                    spawned++;
                    if (spawned >= 2) break; // Exit the loop once both particles have spawned.
                }
            }
        }
    }

    // 2. Update active particles.
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (player->smoke[i].active) {
            player->smoke[i].life -= 0.02f; 
            player->smoke[i].position.y += 0.02f;

            // Apply physical drift.
            player->smoke[i].position.x += player->smoke[i].velocity.x * dtScale;
            player->smoke[i].position.y += player->smoke[i].velocity.y * dtScale;
            player->smoke[i].position.z += player->smoke[i].velocity.z * dtScale;
            
            if (player->smoke[i].life <= 0.0f) {
                player->smoke[i].active = false;
            }
        }
    }
}


// --- DYNAMIC CAMERA ---
// Handles the mathematics for spherical orbit (3rd person) and cockpit view (1st person).
void UpdateDynamicCamera(Camera3D *camera, Player *player) {
    
    // Toggle camera mode when pressing 'C' (Keyboard) or 'A' (Gamepad).
    if (IsKeyPressed(KEY_C) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
        player->isFirstPerson = !player->isFirstPerson;
    }

    if (!player->isFirstPerson) {
        // --- 3RD PERSON ORBIT LOGIC ---
        
        if (IsGamepadAvailable(0)) {
            // Gamepad: Absolute positioning based on right thumbstick tilt.
            float smoothFactor = 0.1f;

            float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

            // Deadzone check.
            if (fabsf(rightStickX) < 0.15f) rightStickX = 0.0f;
            if (fabsf(rightStickY) < 0.15f) rightStickY = 0.0f;

            float targetYaw = -rightStickX * 2.5f; 
            float targetPitch = rightStickY * 1.5f; 

            player->cameraAngleYaw = Lerp(player->cameraAngleYaw, targetYaw, smoothFactor);
            player->cameraAnglePitch = Lerp(player->cameraAnglePitch, targetPitch, smoothFactor);
            
        } else {
            // Keyboard: Relative positioning using arrow keys.
            float smoothFactor = 0.3f; 
            
            float targetYaw = player->cameraAngleYaw;
            float targetPitch = player->cameraAnglePitch;

            if (IsKeyDown(KEY_RIGHT)) targetYaw -= 0.08f;
            if (IsKeyDown(KEY_LEFT))  targetYaw += 0.08f;
            if (IsKeyDown(KEY_UP))    targetPitch -= 0.08f;
            if (IsKeyDown(KEY_DOWN))  targetPitch += 0.08f;

            // Auto-centering mechanism.
            if (!IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) targetYaw *= 0.90f;
            if (!IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN))    targetPitch *= 0.90f;

            // Clamp the target angles to prevent clipping.
            if (targetPitch > 1.5f)  targetPitch = 1.5f;
            if (targetPitch < -0.5f) targetPitch = -0.5f;
            if (targetYaw > 2.5f)    targetYaw = 2.5f;
            if (targetYaw < -2.5f)   targetYaw = -2.5f;

            player->cameraAngleYaw = Lerp(player->cameraAngleYaw, targetYaw, smoothFactor);
            player->cameraAnglePitch = Lerp(player->cameraAnglePitch, targetPitch, smoothFactor);
        }
    } else {
        // Reset manual orbit angles when switching to First Person view for a clean transition.
        player->cameraAngleYaw = player->rotation.y; 
        player->cameraAnglePitch = 0.0f;
    }

    // --- APPLY CAMERA TRANSFORMATIONS ---
    if (player->isFirstPerson) {
        // 1st person (Cockpit).
        camera->position = (Vector3){ player->position.x, player->position.y + 0.5f, player->position.z };
        
        // Calculate exactly where the nose of the vehicle is pointing using trigonometry.
        camera->target.x = camera->position.x - (sinf(player->rotation.y) * cosf(player->rotation.x));
        camera->target.y = camera->position.y +  sinf(player->rotation.x); 
        camera->target.z = camera->position.z - (cosf(player->rotation.y) * cosf(player->rotation.x));
        
    } else {
        // 3rd person (Orbit chase camera).
        camera->target = player->position;
        float cameraDistance = 4.0f;
        float cameraHeight = 1.5f; 

        // Combine player rotation with manual orbit input.
        float totalYaw = player->rotation.y + player->cameraAngleYaw;
        float totalPitch = player->cameraAnglePitch;

        // Spherical coordinates calculation.
        camera->position.x = player->position.x + (sinf(totalYaw) * cosf(totalPitch) * cameraDistance);
        camera->position.y = player->position.y + (sinf(totalPitch) * cameraDistance) + cameraHeight;
        camera->position.z = player->position.z + (cosf(totalYaw) * cosf(totalPitch) * cameraDistance);
    }
}


// --- FORWARD VECTOR ---
Vector3 GetPlayerForwardVector(Player *player) {
    return (Vector3){ 
        -sinf(player->rotation.y) * cosf(player->rotation.x), 
         sinf(player->rotation.x), 
        -cosf(player->rotation.y) * cosf(player->rotation.x) 
    };
}