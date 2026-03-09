// Include math library to use advanced mathematical functions.
#include <math.h>

// We include our own header files.
// We also need raymath.h to calculate the 3D distances between the player and the base.
#include "race.h"
#include "mission_landing.h"
#include "raymath.h"


// --- UPDATE LOOP (WORKER) ---
// This function acts as the Referee specifically for Precision Landing Missions.
// It checks the player's 3D coordinates, throttle, and kinetic energy to determine a safe touchdown.
void UpdateMissionLanding(RaceSystem *race, Player *player) {
    
    // --- 0. DYNAMIC PAD MOVEMENT ---
    float dt = GetFrameTime();
    
    if (race->padMoveType == 1) {
        // --- LINEAR MOVEMENT ---
        race->currentPadSpeed += race->padAccel * dt;
        Vector3 dir = Vector3Normalize(race->padVelocity);
        
        if (Vector3Length(race->padVelocity) == 0) dir = (Vector3){1.0f, 0.0f, 0.0f}; 
        
        race->landingZone.x += dir.x * race->currentPadSpeed * dt;
        race->landingZone.y += dir.y * race->currentPadSpeed * dt;
        race->landingZone.z += dir.z * race->currentPadSpeed * dt;
        
    } else if (race->padMoveType == 2) {
        // --- CIRCULAR MOVEMENT (ORBIT) ---
        race->currentPadSpeed += race->padAccel * dt;
        
        if (race->padRadius > 0.0f) {
            float angularVel = race->currentPadSpeed / race->padRadius;
            race->currentPadAngle += angularVel * dt;
            
            race->landingZone.x = race->padOrigin.x + cosf(race->currentPadAngle) * race->padRadius;
            race->landingZone.z = race->padOrigin.z + sinf(race->currentPadAngle) * race->padRadius;
            race->landingZone.y = race->padOrigin.y; 
        }
    }


    // --- 1. CALCULATE TELEMETRY DATA (RELATIVE PHYSICS) ---
    // A) Pad's velocity vector.
    Vector3 padVelVector = { 0.0f, 0.0f, 0.0f };
    
    if (race->padMoveType == 1) {
        Vector3 dir = Vector3Normalize(race->padVelocity);
        if (Vector3Length(race->padVelocity) == 0) dir = (Vector3){1.0f, 0.0f, 0.0f};
        padVelVector = Vector3Scale(dir, race->currentPadSpeed);
    } 
    else if (race->padMoveType == 2 && race->padRadius > 0.0f) {
        padVelVector.x = -sinf(race->currentPadAngle) * race->currentPadSpeed;
        padVelVector.z =  cosf(race->currentPadAngle) * race->currentPadSpeed;
    }

    // B) Real world speed conversion (60 frames per second).
    Vector3 playerVelPerSec = Vector3Scale(player->velocity, 60.0f);

    // C) Relative speed separation (CRITICAL FIX).
    // We separate the speed into downward force (Sink Rate) and sliding force (Horizontal).
    Vector3 relativeVelocity = Vector3Subtract(playerVelPerSec, padVelVector);
    float verticalImpact = fabsf(relativeVelocity.y); 
    float horizontalSlip = Vector2Length((Vector2){relativeVelocity.x, relativeVelocity.z});

    // D) Distances
    Vector2 playerPos2D = { player->position.x, player->position.z };
    Vector2 padPos2D = { race->landingZone.x, race->landingZone.z };
    float horizontalDistance = Vector2Distance(playerPos2D, padPos2D);

    // Calculate vertical distance (Altitude directly above the landing pad).
    float altitude = player->position.y - race->landingZone.y;

    bool isTouchingGround = (altitude <= 1.0f && altitude >= -1.0f);
    bool isAbovePad = (horizontalDistance <= race->landingRadius);


    // --- 2. ANTI-CHEAT & EVALUATION (THE BLACK BOX) ---
    // IMMEDIATE RETURN: This completely stops a crashed plane from generating a "Victory" 
    // state while sliding on the ground, preventing the Name Input screen from showing up.
    if (race->missionFailed || race->isFinished) {
        return;
    }

    // We evaluate the exact moment the aircraft's landing gears touch the ground.
    if (isTouchingGround) {
        
        // Failure 1: The "Off-Road" check. Missed the pad entirely.
        if (!isAbovePad) {
            race->missionFailed = true;
        }
        
        // Failure 2: Vertical structural failure (Slammed into the ground too hard).
        // We check 'prevSpeed' which stores the vertical descent rate of the previous frame.
        else if (race->prevSpeed > race->maxLandingSpeed) {
            race->missionFailed = true;
        }
        
        // Failure 3: Landing gear collapse (Sliding horizontally too fast relative to the pad).
        // We give a generous 3.0x multiplier to the limit so airplanes can touch down and brake.
        else if (horizontalSlip > race->maxLandingSpeed * 3.0f) {
            race->missionFailed = true;
        }
        
        // Success: Safe touchdown!
        else {
            race->isFinished = true;
            race->isRaceActive = false;
        }
    }
    
    // Store this frame's vertical sink rate for the next frame's impact evaluation.
    race->prevSpeed = verticalImpact;
}


// --- RENDERING FUNCTION (3D WORLD) ---
// This draws exclusively the landing pad geometry and the navigation arrow.
void DrawMissionLanding3D(RaceSystem *race, Player *player) {
    if (race->isRaceActive || race->missionFailed) { 
        DrawCylinder(race->landingZone, race->landingRadius, race->landingRadius, 0.5f, 32, ORANGE);
        
        // Middle layer: A solid white concrete area so it stands out against dark terrain.
        // We lift it by 0.1f on the Y-axis to prevent "Z-fighting" (flickering textures).
        Vector3 midLayer = { race->landingZone.x, race->landingZone.y + 0.1f, race->landingZone.z };
        DrawCylinder(midLayer, race->landingRadius * 0.9f, race->landingRadius * 0.9f, 0.5f, 32, RAYWHITE);
        
        // Top layer: A red bullseye to mark the exact mathematical center of the landing zone.
        // Lifted by 0.2f to sit perfectly on top of the white layer.
        Vector3 bullseye = { race->landingZone.x, race->landingZone.y + 0.2f, race->landingZone.z };
        DrawCylinder(bullseye, race->landingRadius * 0.2f, race->landingRadius * 0.2f, 0.5f, 16, RED);

        if (!race->missionFailed) {
            DrawNavArrow(player, race->landingZone);
        }
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// This draws exclusively the UI elements for landing (speed warnings, distance, etc.).
void DrawMissionLandingUI(RaceSystem *race) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->missionFailed) {
        // --- MISSION FAILED SCREEN ---
        const char *failText = "CRASH LANDING! MISSION FAILED";
        int failWidth = MeasureText(failText, 40);
        DrawTextOutlined(failText, (screenWidth - failWidth) / 2, screenHeight * 0.4f, 40, RED, 2);
        
        // Dynamic Restart Instructions based on hardware.
        const char *restText;
        if (IsGamepadAvailable(0)) {
            restText = "Press [B] to Restart";
        } else {
            restText = "Press [R] to Restart";
        }
        
        int restWidth = MeasureText(restText, 30);
        DrawTextOutlined(restText, (screenWidth - restWidth) / 2, screenHeight * 0.5f, 30, GRAY, 2);
        
    } else if (race->isRaceActive) {
        // --- NORMAL HUD ---
        const char *timerText = TextFormat("MISSION TIME: %.2f", race->timer);
        int timerWidth = MeasureText(timerText, 30);
        DrawTextOutlined(timerText, (screenWidth - timerWidth) / 2, screenHeight * 0.05f, 30, WHITE, 2);

        const char *objText;
        if (race->padMoveType > 0) {
            objText = "OBJECTIVE: LAND ON MOVING CARRIER";
        } else {
            objText = "OBJECTIVE: SAFE TOUCHDOWN";
        }
        int objWidth = MeasureText(objText, 20);
        DrawTextOutlined(objText, (screenWidth - objWidth) / 2, screenHeight * 0.10f, 20, GOLD, 2);    
        
    } else if (race->isFinished && race->finishedTimer < 3.0f) {
        // --- VICTORY SCREEN ---
        const char *winText = "PERFECT LANDING!";
        int winWidth = MeasureText(winText, 40);
        DrawTextOutlined(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, GOLD, 2);
        
        const char *timeText = TextFormat("FINAL TIME: %.2f SECONDS", race->timer);
        int timeWidth = MeasureText(timeText, 30);
        DrawTextOutlined(timeText, (screenWidth - timeWidth) / 2, screenHeight * 0.5f, 30, WHITE, 2);
    }
}