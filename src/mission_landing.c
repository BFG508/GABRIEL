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
    
    // --- 1. CALCULATE TELEMETRY DATA ---
    
    // Calculate the 2D horizontal distance (ignoring altitude) to see if we are over the pad.
    Vector2 playerPos2D = { player->position.x, player->position.z };
    Vector2 padPos2D = { race->landingZone.x, race->landingZone.z };
    float horizontalDistance = Vector2Distance(playerPos2D, padPos2D);

    // Calculate vertical distance (Altitude directly above the landing pad).
    float altitude = player->position.y - race->landingZone.y;

    // Calculate the true kinetic speed (magnitude of the 3D velocity vector).
    // This accounts for both falling too fast (sink rate) and moving forward too fast.
    float currentSpeed = Vector3Length(player->velocity);


    // --- 2. EVALUATE LANDING CONDITIONS ---
    
    // Condition A: Is the aircraft hovering inside the mathematical radius of the landing zone?
    bool isAbovePad = (horizontalDistance <= race->landingRadius);

    // Condition B: Are the landing gears touching the ground? 
    // We assume the aircraft's center of mass is about 0.5f units above the ground when landed.
    bool isTouchingGround = (altitude <= 1.0f);

    // Condition C: Is the aircraft moving slowly enough to survive the impact?
    bool isSpeedSafe = (currentSpeed <= race->maxLandingSpeed);

    // Condition D: Has the pilot cut the engine power?
    // We require the throttle to be practically zero to consider the flight "concluded".
    bool isEngineIdle = (player->throttle <= 0.05f);


    // --- 3. DECLARE VICTORY ---
    // If all conditions are met simultaneously, the landing is successful!
    if (isAbovePad && isTouchingGround && isSpeedSafe && isEngineIdle) {
        race->isFinished = true;
        race->isRaceActive = false;
    }
}


// --- RENDERING FUNCTION (3D WORLD) ---
// This draws exclusively the landing pad geometry and the navigation arrow.
void DrawMissionLanding3D(RaceSystem *race, Player *player) {
    
    if (race->isRaceActive) {
        
        // --- 1. DRAW A HIGH-CONTRAST LANDING PAD ---
        // Base layer: A bright safety orange outer border to clearly separate it from the ground.
        DrawCylinder(race->landingZone, race->landingRadius, race->landingRadius, 0.5f, 32, ORANGE);
        
        // Middle layer: A solid white concrete area so it stands out against dark terrain.
        // We lift it by 0.1f on the Y-axis to prevent "Z-fighting" (flickering textures).
        Vector3 midLayer = { race->landingZone.x, race->landingZone.y + 0.1f, race->landingZone.z };
        DrawCylinder(midLayer, race->landingRadius * 0.9f, race->landingRadius * 0.9f, 0.5f, 32, RAYWHITE);
        
        // Top layer: A red bullseye to mark the exact mathematical center of the landing zone.
        // Lifted by 0.2f to sit perfectly on top of the white layer.
        Vector3 bullseye = { race->landingZone.x, race->landingZone.y + 0.2f, race->landingZone.z };
        DrawCylinder(bullseye, race->landingRadius * 0.2f, race->landingRadius * 0.2f, 0.5f, 16, RED);

        // --- 2. VECTORIAL HUD ARROW (CHEVRON) ---
        // We reuse the high-tech wireframe arrow, but now it points to the landing zone.
        DrawNavArrow(player, race->landingZone);
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// This draws exclusively the UI elements for landing (speed warnings, distance, etc.).
void DrawMissionLandingUI(RaceSystem *race) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->isRaceActive) {
        // 1. Draw the global stopwatch at 5% from the top.
        const char *timerText = TextFormat("MISSION TIME: %.2f", race->timer);
        int timerWidth = MeasureText(timerText, 30);
        DrawTextOutlined(timerText, (screenWidth - timerWidth) / 2, screenHeight * 0.05f, 30, WHITE, 2);

        // 2. Draw the objective status at 10% from the top.
        const char *objText = "OBJECTIVE: SAFE TOUCHDOWN";
        int objWidth = MeasureText(objText, 20);
        DrawTextOutlined(objText, (screenWidth - objWidth) / 2, screenHeight * 0.10f, 20, GOLD, 2);    
    }
    
    // Only draw the victory message if less than 3 seconds have passed.
    else if (race->isFinished && race->finishedTimer < 3.0f) {
        const char *winText = "PERFECT LANDING!";
        int winWidth = MeasureText(winText, 40);
        DrawTextOutlined(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, GOLD, 2);
        
        const char *timeText = TextFormat("FINAL TIME: %.2f SECONDS", race->timer);
        int timeWidth = MeasureText(timeText, 30);
        DrawTextOutlined(timeText, (screenWidth - timeWidth) / 2, screenHeight * 0.5f, 30, WHITE, 2);
    }
}