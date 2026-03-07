// Include math library to use advanced mathematical functions.
#include <math.h>

// We include our own header files.
// We also need resource_manager.h so this .c file knows what a 'ringModel' is.
// We also need raymath.h to calculate the 3D distances between the player and the rings.
#include "race.h"
#include "mission_rings.h"
#include "resource_manager.h"
#include "raymath.h"


// --- UPDATE LOOP (WORKER) ---
// This function acts as the Referee specifically for Ring Missions.
// It checks if the player has crossed the current target ring, or crashed into its physical frame.
void UpdateMissionRings(RaceSystem *race, Player *player) {
    
    // --- 0) OMNIDIRECTIONAL MATH COLLISION (SCORING AND CRASHING) ---
    // We loop through all active rings to check for physical crashes, 
    // and check the target ring for scoring.
    for (int i = 0; i < MAX_RINGS; i++) {
        if (!race->rings[i].active) {
            continue;
        }

        Ring *ring = &race->rings[i];

        // Calculate the vector pointing from the ring to the player.
        Vector3 diff = Vector3Subtract(player->position, ring->position);

        // Calculate which way the ring's hole is facing based on yaw and pitch.
        Vector3 ringForward = { 
            -sinf(ring->yaw * DEG2RAD) * cosf(ring->pitch * DEG2RAD), 
            sinf(ring->pitch * DEG2RAD), 
            -cosf(ring->yaw * DEG2RAD) * cosf(ring->pitch * DEG2RAD) 
        };

        // Calculate depth (Z) and radial (X-Y) distance to the ring's mathematical center.
        float depthDistance = fabsf(Vector3DotProduct(diff, ringForward));
        float totalDistanceSqr = Vector3LengthSqr(diff);
        float distance2D = sqrtf(fabsf(totalDistanceSqr - (depthDistance * depthDistance)));


        // --- A) HARD COLLISION (MATHEMATICAL TORUS WITH RADIAL DEFLECTION) ---
        // The core of the tube is located at exactly 'ring->radius' on the 2D plane.
        float radialDiff = distance2D - ring->radius;
        float distanceToTubeCore = sqrtf((radialDiff * radialDiff) + (depthDistance * depthDistance));
        
        // The visual tube thickness plus a 2.0f buffer.
        float tubeThickness = (ring->radius * 0.05f) + 2.0f;

        if (distanceToTubeCore < tubeThickness) {
            
            // 1. Find where the player is relative to the flat plane of the ring.
            float dot = Vector3DotProduct(diff, ringForward);
            Vector3 planeProjection = Vector3Subtract(diff, Vector3Scale(ringForward, dot));
            
            // 2. Normalize to get the exact direction from ring center to player on that plane.
            Vector3 planeDir = Vector3Normalize(planeProjection);
            
            // 3. Find the exact coordinate of the solid tube's core nearest to the player.
            Vector3 corePoint = Vector3Add(ring->position, Vector3Scale(planeDir, ring->radius));
            
            // 4. Create a pushback vector pointing strictly outwards from the solid tube.
            Vector3 pushOutward = Vector3Normalize(Vector3Subtract(player->position, corePoint));
            
            // 5. Deflect the player (Slide along the ring instead of a dead stop).
            // We penalize the speed slightly (lose 20% speed) instead of killing the engine.
            player->throttle *= 0.8f; 
            
            // Push the player radially away from the metal frame.
            player->position.x += pushOutward.x * 0.5f;
            player->position.y += pushOutward.y * 0.5f;
            player->position.z += pushOutward.z * 0.5f;
            
            // Add a slight bounce to the velocity to make the impact feel real.
            player->velocity.x += pushOutward.x * 0.05f;
            player->velocity.y += pushOutward.y * 0.05f;
            player->velocity.z += pushOutward.z * 0.05f;
        }


        // --- B) SCORING THE TARGET RING ---
        // We only score points for the current target ring.
        if (i == race->targetRing) {
            
            // We restrict the valid scoring zone to only the inner 15% of the ring
            // for visual immersion.
            float validHoleRadius = ring->radius * 0.10f;

            if (distance2D <= validHoleRadius && depthDistance < 1.0f) {
                ring->active = false;
                race->targetRing++;

                // Check if this was the final ring.
                if (race->targetRing >= race->totalRings) {
                    race->isFinished = true;
                    race->isRaceActive = false; 
                }
            }
        }
    }
}


// --- RENDERING FUNCTION (3D WORLD) ---
// This draws exclusively the ring models and the navigation arrow.
void DrawMissionRings3D(RaceSystem *race, Player *player) {
    
    // --- 1. DRAW ALL ACTIVE RINGS ---
    for (int i = 0; i < MAX_RINGS; i++) {
        if (race->rings[i].active) {
            
            Color ringColor;
            
            if (i == race->targetRing) {
                ringColor = GOLD;
            } else {
                ringColor = Fade(LIGHTGRAY, 0.3f);
            }
            
            // 1. Save the 3D model's original base matrix.
            Matrix baseTransform = ringModel.transform;
            
            // 2. Generate the full rotation matrix.
            Matrix matRoll  = MatrixRotateZ(race->rings[i].roll * DEG2RAD);
            Matrix matPitch = MatrixRotateX(race->rings[i].pitch * DEG2RAD);
            Matrix matYaw   = MatrixRotateY(race->rings[i].yaw * DEG2RAD);
            Matrix dynamicRotation = MatrixMultiply(MatrixMultiply(matRoll, matPitch), matYaw);
            
            // 3. Apply the rotation to the model temporarily.
            ringModel.transform = MatrixMultiply(baseTransform, dynamicRotation);
            
            // 4. Draw the model (We pass 0.0f rotation because it is already embedded in the transform).
            Vector3 scale = { race->rings[i].radius, race->rings[i].radius, race->rings[i].radius };
            DrawModelEx(ringModel, race->rings[i].position, (Vector3){0, 1, 0}, 0.0f, scale, ringColor);
            
            // 5. Restore the original matrix.
            ringModel.transform = baseTransform;
        }
    }

    // --- 2. VECTORIAL HUD ARROW (CHEVRON) ---
    // We build a high-tech wireframe arrow (-->) using 3D lines and cross products.
    if (race->isRaceActive) {
        DrawNavArrow(player, race->rings[race->targetRing].position);
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// This draws exclusively the UI elements for the ring missions (like the target ring count).
void DrawMissionRingsUI(RaceSystem *race) {
    
    // Get current screen dimensions dynamically.
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->isRaceActive) {
        // 1. Draw the stopwatch at 5% from the top.
        const char *timerText = TextFormat("RACE TIME: %.2f", race->timer);
        int timerWidth = MeasureText(timerText, 30);
        DrawTextOutlined(timerText, (screenWidth - timerWidth) / 2, screenHeight * 0.05f, 30, WHITE, 2);

        // 2. Draw progress at 10% from the top.
        const char *ringText = TextFormat("TARGET RING: %d / %d", race->targetRing + 1, race->totalRings);
        int ringWidth = MeasureText(ringText, 20);
        DrawTextOutlined(ringText, (screenWidth - ringWidth) / 2, screenHeight * 0.10f, 20, GOLD, 2);
        
    }
    
    // Only draw the victory message if less than 3 seconds have passed.
    else if (race->isFinished && race->finishedTimer < 3.0f) {
        
        // Victory message perfectly centered horizontally, at 40% height.
        const char *winText = "CIRCUIT COMPLETE!";
        int winWidth = MeasureText(winText, 40);
        DrawTextOutlined(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, GOLD, 2);
        
        // Final time perfectly centered horizontally, at 50% height.
        const char *timeText = TextFormat("FINAL TIME: %.2f SECONDS", race->timer);
        int timeWidth = MeasureText(timeText, 30);
        DrawTextOutlined(timeText, (screenWidth - timeWidth) / 2, screenHeight * 0.5f, 30, WHITE, 2);
    }
}