// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef RACE_H
#define RACE_H

// Include the main Raylib library so the compiler knows what 'Vector3' is.
// We also need 'player.h' because the mission system needs to know where the player is to check collisions.
#include "raylib.h"
#include "player.h"

// Include our specialized mission modules.
// These act as the "Workers" while race.h acts as the "Director".
#include "mission_rings.h"
#include "mission_landing.h"


// --- DATA STRUCTURES ---
// A 'struct' groups related variables into a single package.

// The main system that acts as the "Referee" and "Data Storage" of the missions.
typedef struct RaceSystem {
    // --- GLOBAL MISSION DATA ---
    int missionType;       // 0 = Rings, 1 = Precision Landing, etc.

    Vector3 startPos;      // Where the player should spawn.
    float startYaw;        // Which way the player should face.

    float timer;           // The current mission time in seconds.
    float finishedTimer;   // Tracks how many seconds have passed since mission completion.
    bool isRaceActive;     // True while the player is flying, False when finished or not started.
    bool isFinished;       // True if the player successfully completed the objective.

    // --- MISSION TYPE 0: RINGS DATA ---
    Ring rings[MAX_RINGS]; // The array (list) containing all the rings in the circuit.
    int totalRings;        // Number of total rings.
    int targetRing;        // The index (0 to MAX_RINGS - 1) of the NEXT ring the player must cross.

    // --- MISSION TYPE 1: LANDING DATA ---
    Vector3 landingZone;   // Coordinates for the center of the landing pad.
    float landingRadius;   // The size of the safe landing area (collision size).
    float maxLandingSpeed; // Maximum vertical/forward speed allowed to not crash on touchdown.
} RaceSystem;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in main.c before defining what they actually do.

// Initializes and returns a brand new Mission package based on the level ID.
// It sets up the starting positions, reads the mission type from the file, and resets the timer.
// Notice it returns a full 'RaceSystem' struct (passed by value), just like InitPlayer.
RaceSystem InitRace(int levelID);

// Updates the global mission logic and delegates work to the specific modules (rings or landing).
// VERY IMPORTANT: We pass POINTERS to both the race and the player.
// We need the race pointer to update the timer and delegate the status.
// We need the player pointer to physically check their exact 3D coordinates.
void UpdateRace(RaceSystem *race, Player *player);

// Draws the 3D models for the current mission (rings, helipads, etc.).
// We pass a POINTER to avoid copying the whole struct into memory 60 times per second.
void DrawRace3D(RaceSystem *race, Player *player);

// Draws the specific UI for the current mission (timer, remaining rings, or landing warnings).
void DrawRaceUI(RaceSystem *race);

// Reads only the first line of the level file to get its name for the menu UI.
// We pass a buffer (outName) where the function will write the text.
void GetLevelName(int levelID, char *outName);


// --- SHARED VISUAL UTILITIES ---
// Draws text with a solid border. Shared across all mission modules and main UI.
void DrawTextOutlined(const char *text, int posX, int posY, int fontSize, Color color, int outlineSize);

// Draws the 3D holographic navigation arrow pointing to a specific target.
void DrawNavArrow(Player *player, Vector3 targetPos);

#endif // Ends the include guard