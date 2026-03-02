// We include our own header file so this .c file knows what a 'RaceSystem' is.
// We also need resource_manager.h so this .c file knows what a 'ringModel' is.
// We also need raymath.h to calculate the 3D distances between the player and the rings.
#include "race.h"
#include "resource_manager.h"
#include "raymath.h"

// --- FACTORY FUNCTION ---
// This function creates a new Race setup from scratch and hands it back to main.c.
// It acts as the "Track Designer", placing the rings in the 3D world.
RaceSystem InitRace(void) {
    // Initialize all memory to 0 to prevent garbage data
    RaceSystem race = { 0 };
    
    // 1. Setup the track layout (Waypoints)
    // We space them out so the player has time to maneuver.
    // Ring 0: Straight ahead and slightly up
    race.rings[0].position = (Vector3){ 0.0f, 20.0f, -100.0f };
    race.rings[0].radius = 60.0f;
    race.rings[0].active = true;

    // Ring 1: Further ahead, turning right
    race.rings[1].position = (Vector3){ 50.0f, 30.0f, -250.0f };
    race.rings[1].radius = 60.0f;
    race.rings[1].active = true;

    // Ring 2: Dropping altitude
    race.rings[2].position = (Vector3){ 100.0f, 10.0f, -400.0f };
    race.rings[2].radius = 60.0f;
    race.rings[2].active = true;

    // Ring 3: Sharp left turn
    race.rings[3].position = (Vector3){ -50.0f, 40.0f, -500.0f };
    race.rings[3].radius = 60.0f;
    race.rings[3].yaw = 60.0f; // Rotate 60 degrees to face the incoming player
    race.rings[3].active = true;

    // Ring 4: The Finish Line (Lower and smaller for extra difficulty)
    race.rings[4].position = (Vector3){ -50.0f, 15.0f, -700.0f };
    race.rings[4].radius = 50.0f;
    race.rings[4].active = true;


    // 2. Setup the Referee rules
    race.targetRing = 0;       // The player must cross ring index 0 first
    race.timer = 0.0f;         // Clock starts at zero
    race.finishedTimer = 0.0f; // Finish line timer starts at zero
    race.isRaceActive = true;  // The race begins immediately
    race.isFinished = false;   // Not finished yet

    // Hand the finished race package back to main.c
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

    // 1. UPDATE THE STOPWATCH
    race->timer += GetFrameTime();

    // 2. OMNIDIRECTIONAL MATH COLLISION (SCORING AND CRASHING)
    // We loop through ALL active rings to check for physical crashes, 
    // and check the target ring for scoring.
    for (int i = 0; i < MAX_RINGS; i++) {
        if (!race->rings[i].active) continue;

        Ring *ring = &race->rings[i];

        // Calculate the vector pointing from the ring to the player
        Vector3 diff = Vector3Subtract(player->position, ring->position);

        // Calculate which way the ring's hole is facing based on its Yaw angle
        Vector3 ringForward = (Vector3){
            sinf(ring->yaw * DEG2RAD),
            0.0f,
            cosf(ring->yaw * DEG2RAD)
        };

        // Calculate depth (Z) and radial (X-Y) distance to the ring's mathematical center
        float depthDistance = fabsf(Vector3DotProduct(diff, ringForward));
        float totalDistanceSqr = Vector3LengthSqr(diff);
        float distance2D = sqrtf(fabsf(totalDistanceSqr - (depthDistance * depthDistance)));

        // --- A) HARD COLLISION (MATHEMATICAL TORUS WITH RADIAL DEFLECTION) ---
        // The core of the tube is located at exactly 'ring->radius' on the 2D plane.
        float radialDiff = distance2D - ring->radius;
        float distanceToTubeCore = sqrtf((radialDiff * radialDiff) + (depthDistance * depthDistance));
        
        // The visual tube thickness plus a 2.0f buffer
        float tubeThickness = (ring->radius * 0.05f) + 2.0f;

        if (distanceToTubeCore < tubeThickness) {
            
            // 1. Find where the player is relative to the flat plane of the ring
            float dot = Vector3DotProduct(diff, ringForward);
            Vector3 planeProjection = Vector3Subtract(diff, Vector3Scale(ringForward, dot));
            
            // 2. Normalize to get the exact direction from ring center to player on that plane
            Vector3 planeDir = Vector3Normalize(planeProjection);
            
            // 3. Find the exact coordinate of the solid tube's core nearest to the player
            Vector3 corePoint = Vector3Add(ring->position, Vector3Scale(planeDir, ring->radius));
            
            // 4. Create a pushback vector pointing strictly OUTWARDS from the solid tube
            Vector3 pushOutward = Vector3Normalize(Vector3Subtract(player->position, corePoint));
            
            // 5. DEFLECT THE PLANE! (Slide along the ring instead of a dead stop)
            // We penalize the speed slightly (lose 20% speed) instead of killing the engine
            player->throttle *= 0.8f; 
            
            // Push the plane radially away from the metal frame
            player->position.x += pushOutward.x * 0.5f;
            player->position.y += pushOutward.y * 0.5f;
            player->position.z += pushOutward.z * 0.5f;
            
            // Add a slight bounce to the velocity to make the impact feel real
            player->velocity.x += pushOutward.x * 0.05f;
            player->velocity.y += pushOutward.y * 0.05f;
            player->velocity.z += pushOutward.z * 0.05f;
        }
        // --- B) SCORING THE TARGET RING ---
        // We only score points for the current target ring
        if (i == race->targetRing) {
            
            // We restrict the valid scoring zone to only the inner 65% of the ring
            float validHoleRadius = ring->radius * 0.15f;

            if (distance2D <= validHoleRadius && depthDistance < 1.0f) {
                ring->active = false;
                race->targetRing++;

                if (race->targetRing >= MAX_RINGS) {
                    race->isFinished = true;
                    race->isRaceActive = false; 
                }
            }
        }
    }
}

// --- RENDERING FUNCTION (3D WORLD) ---
// This must be called INSIDE BeginMode3D() in main.c.
void DrawRace3D(RaceSystem *race, Player *player) {
    
    // 1. Draw all active rings
    for (int i = 0; i < MAX_RINGS; i++) {
        if (race->rings[i].active) {
            
            Color ringColor;
            if (i == race->targetRing) ringColor = GOLD;
            else ringColor = Fade(LIGHTGRAY, 0.3f);
            
            // --- BUG 2 FIXED: DRAW WITH ROTATION ---
            // We use DrawModelEx instead of DrawModel to allow individual rotations (yaw).
            Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f }; // Rotate around the Up (Y) axis
            Vector3 scale = { race->rings[i].radius, race->rings[i].radius, race->rings[i].radius };
            
            DrawModelEx(ringModel, race->rings[i].position, rotationAxis, race->rings[i].yaw, scale, ringColor);
        }
    }

    // --- 2. VECTORIAL HUD ARROW (CHEVRON) ---
    // Instead of a solid cone that disappears due to perspective, we build a 
    // high-tech wireframe arrow (----->) using 3D lines and Cross Products.
    if (race->isRaceActive) {
        
        // 1. Calculate vehicle forward vector to place the hologram near the windshield
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
        
        // 2. TRUE Target Direction (No perspective cheating!)
        Vector3 targetPos = race->rings[race->targetRing].position;
        Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, arrowCenter));
        
        // 3. CROSS PRODUCT MATH
        // To prevent the arrow from becoming a circle, we need it to have horizontal "wings".
        // The Cross Product of our 'Forward' direction and the 'World Up' axis 
        // mathematically generates a perfect 'Left/Right' vector.
        Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
        Vector3 rightDir = Vector3Normalize(Vector3CrossProduct(dir, worldUp));
        
        // 4. Calculate the 4 points of the Vector Arrow
        Vector3 arrowTail = Vector3Subtract(arrowCenter, Vector3Scale(dir, 0.5f));
        Vector3 arrowTip = Vector3Add(arrowCenter, Vector3Scale(dir, 0.5f));
        
        // Go back slightly from the tip to attach the wings
        Vector3 headBase = Vector3Subtract(arrowTip, Vector3Scale(dir, 0.4f)); 
        
        // Extend wings horizontally using our calculated rightDir
        Vector3 leftWing = Vector3Add(headBase, Vector3Scale(rightDir, 0.25f));
        Vector3 rightWing = Vector3Subtract(headBase, Vector3Scale(rightDir, 0.25f));
        
        // 5. Draw the 3 thin cylinders to form the -----> shape
        DrawCylinderEx(arrowTail, arrowTip, 0.03f, 0.03f, 6, RED); // Central shaft
        DrawCylinderEx(leftWing, arrowTip, 0.03f, 0.03f, 6, RED);  // Left wing
        DrawCylinderEx(rightWing, arrowTip, 0.03f, 0.03f, 6, RED); // Right wing
    }
}

// --- RENDERING FUNCTION (2D HUD) ---
// This must be called OUTSIDE BeginMode3D() in main.c, right next to your telemetry.
void DrawRaceUI(RaceSystem *race) {
    
    // Get current screen dimensions dynamically
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->isRaceActive) {
        
        // 1. Draw the Stopwatch at 5% from the top
        const char *timerText = TextFormat("RACE TIME: %.2f", race->timer);
        int timerWidth = MeasureText(timerText, 30);
        DrawText(timerText, (screenWidth - timerWidth) / 2, screenHeight * 0.08f, 30, WHITE);

        // 2. Draw progress at 10% from the top
        const char *ringText = TextFormat("TARGET RING: %d / %d", race->targetRing + 1, MAX_RINGS);
        int ringWidth = MeasureText(ringText, 20);
        DrawText(ringText, (screenWidth - ringWidth) / 2, screenHeight * 0.13f, 20, GOLD);
        
    } 
    // Only draw the victory message if less than 5 seconds have passed
    else if (race->isFinished && race->finishedTimer < 5.0f) {
        
        // Victory message perfectly centered horizontally, at 40% height
        const char *winText = "CIRCUIT COMPLETE!";
        int winWidth = MeasureText(winText, 40);
        DrawText(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, LIME);
        
        // Final time perfectly centered horizontally, at 50% height
        const char *timeText = TextFormat("FINAL TIME: %.2f SECONDS", race->timer);
        int timeWidth = MeasureText(timeText, 30);
        DrawText(timeText, (screenWidth - timeWidth) / 2, screenHeight * 0.5f, 30, WHITE);
    }
}