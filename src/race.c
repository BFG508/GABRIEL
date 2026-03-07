// Include stdio library to use file input/output operations.
#include <stdio.h>

// Include string library to use string manipulation functions.
#include <string.h>

// We include our own header files.
// This file acts as the "Director", so it needs to know the definitions of all the "Workers".
#include "race.h"
#include "mission_rings.h"
#include "mission_landing.h"


// --- OUTLINED TEXT ---
void DrawTextOutlined(const char *text, int posX, int posY, int fontSize, Color color, int outlineSize) {
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

// --- NAVIGATION ARROW ---
void DrawNavArrow(Player *player, Vector3 targetPos) {
    // 1. Get the direction the player is looking using our new centralized function.
    Vector3 forwardVec = GetPlayerForwardVector(player);
    
    // 2. Position the hologram slightly above and in front of the cockpit.
    float bounceOffset = sinf(GetTime() * 5.0f) * 0.1f; 
    Vector3 arrowCenter = { 
        player->position.x + (forwardVec.x * 2.0f), 
        player->position.y + 1.2f + bounceOffset + (forwardVec.y * 2.0f), 
        player->position.z + (forwardVec.z * 2.0f) 
    };
    
    // 3. Mathematical calculation of the pointing vector and wings.
    Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, arrowCenter));
    Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
    Vector3 rightDir = Vector3Normalize(Vector3CrossProduct(dir, worldUp));
    
    Vector3 arrowTail = Vector3Subtract(arrowCenter, Vector3Scale(dir, 0.5f));
    Vector3 arrowTip = Vector3Add(arrowCenter, Vector3Scale(dir, 0.5f));
    Vector3 headBase = Vector3Subtract(arrowTip, Vector3Scale(dir, 0.4f)); 
    
    Vector3 leftWing = Vector3Add(headBase, Vector3Scale(rightDir, 0.25f));
    Vector3 rightWing = Vector3Subtract(headBase, Vector3Scale(rightDir, 0.25f));
    
    // 4. Draw the 3D model.
    DrawCylinderEx(arrowTail, arrowTip, 0.03f, 0.03f, 6, RED); 
    DrawCylinderEx(leftWing, arrowTip, 0.03f, 0.03f, 6, RED);  
    DrawCylinderEx(rightWing, arrowTip, 0.03f, 0.03f, 6, RED); 
}


// --- FACTORY FUNCTION (THE MISSION BUILDER) ---
// This acts as the "Track Designer". Instead of hardcoding the missions in C,
// it dynamically reads a text file from the hard drive based on the levelID.
RaceSystem InitRace(int levelID) {
    RaceSystem race = { 0 };
    
    // 1. Setup the global Referee rules (Default starting state).
    race.timer = 0.0f;         
    race.finishedTimer = 0.0f; 
    race.isRaceActive = true;  
    race.isFinished = false;   

    // 2. Build the filename string dynamically (e.g., "levels/lvl1.txt").
    const char *filename = TextFormat("levels/lvl%d.txt", levelID);
    
    // 3. Open the file in Read ("r") mode.
    FILE *file = fopen(filename, "r");
    
    if (file != NULL) {
        // Read and discard the first line (the title) because the mission builder
        // only cares about the numbers below it.
        char dummyName[50];
        fgets(dummyName, 50, file); 
        
        // Read the Mission Type identifier (0 = Rings, 1 = Landing, etc.).
        fscanf(file, "%d", &race.missionType);

        // Read the 4 floats for the player's spawn coordinates (X, Y, Z, yaw).
        fscanf(file, "%f %f %f %f", &race.startPos.x, &race.startPos.y, &race.startPos.z, &race.startYaw);


        // --- DYNAMIC PARSING BASED ON MISSION TYPE ---
        
        // BRANCH 0: RING RACE PARSING
        if (race.missionType == 0) {
            race.targetRing = 0; // Initialize the target ring counter.
            int ringCount = 0;
            
            // Read how many rings are in this level.
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
                    
                    race.rings[i].active = true; // Mark the ring as ready to be crossed.
                }
            }
        }
        
        // BRANCH 1: PRECISION LANDING PARSING
        else if (race.missionType == 1) {
            
            // Read the exact 3D coordinates of the helipad's center (X, Y, Z).
            fscanf(file, "%f %f %f", &race.landingZone.x, &race.landingZone.y, &race.landingZone.z);
            
            // Read the physical radius of the safe zone and the maximum safe speed allowed for touchdown.
            fscanf(file, "%f %f", &race.landingRadius, &race.maxLandingSpeed);
        }
        
        // Always close the file when you are done to free Operating System memory resources.
        fclose(file);
    }

    // Return the fully built mission package (passed by value) back to main.c.
    return race;
}


// --- UPDATE LOOP (THE GLOBAL REFEREE) ---
// This function updates the global stopwatch and then DELEGATES the physical 
// collision checks and logic to the specific mission workers.
void UpdateRace(RaceSystem *race, Player *player) {
    
    // If the mission is completely finished, just update the post-mission timer and stop.
    if (race->isFinished) {
        race->finishedTimer += GetFrameTime();
        return; 
    }

    // If the mission hasn't started or is paused, do nothing.
    if (!race->isRaceActive) {
        return;
    }

    // --- 1. UPDATE THE GLOBAL STOPWATCH ---
    race->timer += GetFrameTime();

    // --- 2. DELEGATE LOGIC TO SPECIALIZED MODULES ---
    switch (race->missionType) {
        case 0:
            // Hand over control to the Rings module.
            // We pass the exact same pointers so the worker can modify the real data.
            UpdateMissionRings(race, player);
            break;
            
        case 1:
            // Hand over control to the Landing module.
            UpdateMissionLanding(race, player);
            break;
            
        default:
            // Unknown mission type, do nothing.
            break;
    }
}


// --- RENDERING FUNCTION (3D WORLD) ---
// This must be called inside BeginMode3D() in main.c.
// It acts as a switchboard, routing the drawing commands to the correct worker.
void DrawRace3D(RaceSystem *race, Player *player) {
    
    switch (race->missionType) {
        case 0:
            DrawMissionRings3D(race, player);
            break;
            
        case 1:
            DrawMissionLanding3D(race, player);
            break;
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// This must be called outside BeginMode3D() in main.c, right next to your telemetry.
// It routes the UI drawing commands to the correct worker.
void DrawRaceUI(RaceSystem *race) {
    
    switch (race->missionType) {
        case 0:
            DrawMissionRingsUI(race);
            break;
            
        case 1:
            DrawMissionLandingUI(race);
            break;
    }
}


// --- GET LEVEL NAME ---
// Opens the text file just to read the very first line (the mission title).
// Used by the Menu Screen to display the names without loading the entire 3D track.
void GetLevelName(int levelID, char *outName) {
    const char *filename = TextFormat("levels/lvl%d.txt", levelID);
    FILE *file = fopen(filename, "r");
    
    if (file != NULL) {
        // Read the first line (up to 50 characters).
        if (fgets(outName, 50, file) != NULL) {
            // 'fgets' grabs the invisible 'newline' character at the end of the text line. 
            // We must remove it using 'strcspn' so it prints cleanly on the screen.
            outName[strcspn(outName, "\r\n")] = 0; 
        } else {
            strcpy(outName, "UNKNOWN");
        }
        fclose(file);
    } else {
        // If the file doesn't exist (e.g., trying to load level 15 when we only have 10),
        // we show this placeholder text in the menu.
        strcpy(outName, "RESTRICTED AREA"); 
    }
}