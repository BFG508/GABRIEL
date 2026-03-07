// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef MISSION_LANDING_H
#define MISSION_LANDING_H

// Include the main Raylib library so the compiler knows what 'Vector3' is.
// We also need 'player.h' because the mission system needs to know where the player is to check collisions and speed.
#include "raylib.h"
#include "player.h"


// --- FORWARD DECLARATION ---
// We tell the compiler that the "RaceSystem" struct exists somewhere else (in race.h).
// This prevents a circular dependency loop between the header files so they don't block each other.
typedef struct RaceSystem RaceSystem;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in race.c before defining what they actually do.

// Updates the precision landing mission logic (distance to pad, speed checks, and touchdown).
// VERY IMPORTANT: We pass POINTERS to both the race and the player.
// We need the race pointer to update the mission status (success/failure) and timers.
// We need the player pointer to physically check their exact 3D coordinates, velocity, and tilt.
void UpdateMissionLanding(RaceSystem *race, Player *player);

// Draws the 3D models for the landing sequence (helipads, runway lights, or approach path).
// We pass POINTERS to avoid copying large structures into memory 60 times per second.
void DrawMissionLanding3D(RaceSystem *race, Player *player);

// Draws the specific UI for the landing mission (altitude warnings, speed indicators, distance to target).
void DrawMissionLandingUI(RaceSystem *race);

#endif // Ends the include guard