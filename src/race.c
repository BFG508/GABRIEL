// Include stdio library to use file input/output operations.
#include <stdio.h>

// Include string library to use string manipulation functions.
#include <string.h>

// Include math library to use advanced mathematical functions.
#include <math.h>


// We include our own header file.
// We also need resource_manager.h so this .c file knows what a 'ringModel' is.
// We also need raymath.h to calculate the 3D distances between the player and the rings.
#include "race.h"
#include "resource_manager.h"
#include "raymath.h"


// --- OUTLINED TEXT ---
// Draws text with a solid border by rendering it 8 times around the center point.
static void DrawTextOutlined(const char *text, int posX, int posY, int fontSize, Color color, int outlineSize) {
    DrawText(text, posX - outlineSize, posY, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY, fontSize, BLACK);
    DrawText(text, posX, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX - outlineSize, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY - outlineSize, fontSize, BLACK);
    DrawText(text, posX - outlineSize, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX + outlineSize, posY + outlineSize, fontSize, BLACK);
    DrawText(text, posX, posY, fontSize, color);
}


// --- FACTORY FUNCTION ---
// This acts as the "Track Designer". Instead of hardcoding the rings in C,
// it dynamically reads a text file from the hard drive based on the levelID.
RaceSystem InitRace(int levelID) {
    RaceSystem race = { 0 };
    
    // 1. Setup the Referee rules (Default starting state).
    race.targetRing = 0;       
    race.timer = 0.0f;         
    race.finishedTimer = 0.0f; 
    race.isRaceActive = true;  
    race.isFinished = false;   

    // 2. Build the filename string dynamically (e.g., "levels/lvl1.txt").
    const char *filename = TextFormat("levels/lvl%d.txt", levelID);
    
    // 3. Open the file in Read ("r") mode.
    FILE *file = fopen(filename, "r");
    
    if (file != NULL) {
        // Read and discard the first line (the name) because the track builder,
        // only cares about the numbers below it.
        char dummyName[50];
        fgets(dummyName, 50, file); 
        
        // Read the 4 floats for player spawn (X, Y, Z, Yaw).
        fscanf(file, "%f %f %f %f", &race.startPos.x, &race.startPos.y, &race.startPos.z, &race.startYaw);

        int ringCount = 0;
        
        // Read the first line: How many rings are in this level?
        if (fscanf(file, "%d", &ringCount) == 1) {
            
            // Safety Check: Prevent buffer overflow if the file has too many rings.
            if (ringCount > MAX_RINGS) {
                ringCount = MAX_RINGS;
            }
            race.totalRings = ringCount;

            // Loop through the file and read the properties for each ring.
            for (int i = 0; i < ringCount; i++) {
                // fscanf reads the 7 floats separated by spaces.
                fscanf(file, "%f %f %f %f %f %f %f", 
                    &race.rings[i].position.x,
                    &race.rings[i].position.y,
                    &race.rings[i].position.z,
                    &race.rings[i].radius,
                    &race.rings[i].pitch,
                    &race.rings[i].yaw,
                    &race.rings[i].roll);
                
                race.rings[i].active = true;
            }
        }
        
        // Always close the file when you are done to free OS resources.
        fclose(file);
        
    }

    return race;
}


// --- UPDATE LOOP ---
// This function acts as the Referee. It checks the stopwatch and sees if the 
// player has crossed the current target ring, or crashed into its physical frame.
void UpdateRace(RaceSystem *race, Player *player) {
    
    // If the race is already finished, update the post-race timer and stop.
    if (race->isFinished) {
        race->finishedTimer += GetFrameTime();
        return; 
    }

    if (!race->isRaceActive) return;

    // --- 1. UPDATE THE STOPWATCH ---
    race->timer += GetFrameTime();


    // --- 2. OMNIDIRECTIONAL MATH COLLISION (SCORING AND CRASHING) ---
    // We loop through all active rings to check for physical crashes, 
    // and check the target ring for scoring.
    for (int i = 0; i < MAX_RINGS; i++) {
        if (!race->rings[i].active) {
            continue;
        }

        Ring *ring = &race->rings[i];

        // Calculate the vector pointing from the ring to the player
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
        // We only score points for the current target ring
        if (i == race->targetRing) {
            
            // We restrict the valid scoring zone to only the inner 15% of the ring
            // for visual immersion.
            float validHoleRadius = ring->radius * 0.10f;

            if (distance2D <= validHoleRadius && depthDistance < 1.0f) {
                ring->active = false;
                race->targetRing++;

                if (race->targetRing >= race->totalRings) {
                    race->isFinished = true;
                    race->isRaceActive = false; 
                }
            }
        }
    }
}


// --- RENDERING FUNCTION (3D WORLD) ---
// This must be called inside BeginMode3D() in main.c.
void DrawRace3D(RaceSystem *race, Player *player) {
    
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
        
        // 1. Calculate vehicle forward vector to place the hologram near the windshield.
        Vector3 forwardVec = { 
            -sinf(player->rotation.y) * cosf(player->rotation.x), 
             sinf(player->rotation.x), 
            -cosf(player->rotation.y) * cosf(player->rotation.x) 
        };
        
        // Hover very close: 1.2 units above, 2.0 units forward. Bouncing smoothed to 0.1f.
        float bounceOffset = sinf(GetTime() * 5.0f) * 0.1f; 
        Vector3 arrowCenter = { 
            player->position.x + (forwardVec.x * 2.0f), 
            player->position.y + 1.2f + bounceOffset + (forwardVec.y * 2.0f), 
            player->position.z + (forwardVec.z * 2.0f) 
        };
        
        // 2. True target direction.
        Vector3 targetPos = race->rings[race->targetRing].position;
        Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, arrowCenter));
        
        // 3. Cross product.
        // To prevent the arrow from becoming a circle, we need it to have horizontal "wings".
        // The cross product of our 'Forward' direction and the 'World Up' axis 
        // mathematically generates a perfect 'Left/Right' vector.
        Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
        Vector3 rightDir = Vector3Normalize(Vector3CrossProduct(dir, worldUp));
        
        // 4. Calculate the 4 points of the vector arrow.
        Vector3 arrowTail = Vector3Subtract(arrowCenter, Vector3Scale(dir, 0.5f));
        Vector3 arrowTip = Vector3Add(arrowCenter, Vector3Scale(dir, 0.5f));
        
        // Go back slightly from the tip to attach the wings.
        Vector3 headBase = Vector3Subtract(arrowTip, Vector3Scale(dir, 0.4f)); 
        
        // Extend wings horizontally using our calculated rightDir.
        Vector3 leftWing = Vector3Add(headBase, Vector3Scale(rightDir, 0.25f));
        Vector3 rightWing = Vector3Subtract(headBase, Vector3Scale(rightDir, 0.25f));
        
        // 5. Draw the 3 thin cylinders to form the --> shape.
        DrawCylinderEx(arrowTail, arrowTip, 0.03f, 0.03f, 6, RED); // Central shaft.
        DrawCylinderEx(leftWing, arrowTip, 0.03f, 0.03f, 6, RED);  // Left wing.
        DrawCylinderEx(rightWing, arrowTip, 0.03f, 0.03f, 6, RED); // Right wing.
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// This must be called outside BeginMode3D() in main.c, right next to your telemetry.
void DrawRaceUI(RaceSystem *race) {
    
    // Get current screen dimensions dynamically.
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->isRaceActive) {
        // 1. Draw the stopwatch at % from the top.
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
        DrawTextOutlined(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, LIME, 2);
        
        // Final time perfectly centered horizontally, at 50% height.
        const char *timeText = TextFormat("FINAL TIME: %.2f SECONDS", race->timer);
        int timeWidth = MeasureText(timeText, 30);
        DrawTextOutlined(timeText, (screenWidth - timeWidth) / 2, screenHeight * 0.5f, 30, WHITE, 2);
    }
}


// GET LEVEL NAME ---
// Opens the text file just to read the very first line (the mission title).
void GetLevelName(int levelID, char *outName) {
    const char *filename = TextFormat("levels/lvl%d.txt", levelID);
    FILE *file = fopen(filename, "r");
    
    if (file != NULL) {
        // Read the first line (up to 50 characters)
        if (fgets(outName, 50, file) != NULL) {
            // fgets grabs the invisible 'newline' character at the end. We must remove it.
            outName[strcspn(outName, "\r\n")] = 0; 
        } else {
            strcpy(outName, "UNKNOWN");
        }
        fclose(file);
    } else {
        // If the file doesn't exist, we show this placeholder.
        strcpy(outName, "RESTRICTED AREA"); 
    }
}