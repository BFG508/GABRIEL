// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef MISSION_RINGS_H
#define MISSION_RINGS_H

// We include the main Raylib library so the compiler knows what 'Vector3' is.
// We also need 'player.h' because the race system needs to know where the player is to check collisions.
#include "raylib.h"
#include "player.h"


// --- CONSTANTS ---
// Defining the number of rings here makes it easy to add more later without changing the logic.
#define MAX_RINGS 50


// --- DATA STRUCTURES ---
// A 'struct' groups related variables into a single package.

// Represents a single floating checkpoint in the sky.
typedef struct Ring {
    Vector3 position; // Where the ring is located in the 3D world.
    float radius;     // How big the ring is (collision size).
    float pitch;      // Rotation on the Z axis (in degrees) to face different directions.
    float yaw;        // Rotation on the Y axis (in degrees) to face different directions.
    float roll;       // Rotation on the X axis (in degrees) to face different directions.
    bool active;      // True if the player still needs to fly through this ring.
} Ring;


// --- FORWARD DECLARATION ---
// We tell the compiler that the "RaceSystem" struct exists somewhere else (in race.h).
// This prevents a circular dependency loop between the header files so they don't block each other.
typedef struct RaceSystem RaceSystem;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in race.c before defining what they actually do.

// Updates the ring mission logic (collisions and target progression).
// VERY IMPORTANT: We pass POINTERS to both the race and the player.
// We need the race pointer to update the target ring and mark rings as inactive.
// We need the player pointer (read-only in this case) to check their exact 3D coordinates.
void UpdateMissionRings(RaceSystem *race, Player *player);

// Draws the 3D models of the rings and the navigation arrow.
// We pass POINTERS to avoid copying the whole array of rings into memory 60 times per second.
void DrawMissionRings3D(RaceSystem *race, Player *player);

// Draws the specific UI for the rings mission (remaining rings text, etc.).
void DrawMissionRingsUI(RaceSystem *race);

#endif // Ends the include guard