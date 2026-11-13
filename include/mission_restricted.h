// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would panic and throw "redefinition" errors.
#ifndef MISSION_RESTRICTED_H
#define MISSION_RESTRICTED_H

// Include the main Raylib library so the compiler knows what 'Vector3' is.
// We also need 'player.h' because the mission system needs to know where the player is to check collisions and speed.
#include "raylib.h"
#include "player.h"


// --- CONSTANTS ---
// Defining the number of rings here makes it easy to add more later without changing the logic.
#define MAX_PATH_POINTS 50


// --- FORWARD DECLARATION ---
// We tell the compiler that the "RaceSystem" struct exists somewhere else (in race.h).
// This prevents a circular dependency loop between the header files so they don't block each other.
typedef struct RaceSystem RaceSystem;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in race.c before defining what they actually do.

// Updates the environmental physics (wind displacement), checks the radar ceiling (max altitude),
// and calculates the complex 2D geometry to ensure the player stays inside the corridor boundaries.
// We pass POINTERS (*race and *player) so this function can modify the live data of the game directly.
void UpdateMissionRestricted(RaceSystem *race, Player *player);

// Draws the visual elements of the mission inside the 3D world space.
// This includes the holographic waypoint path projected onto the ground, the translucent red
// radar ceiling in the sky, and the dynamic navigation arrow pointing to the next node.
void DrawMissionRestricted3D(RaceSystem *race, Player *player);

// Draws the flat 2D user interface on top of the screen specifically for this mission type.
// Displays the global survival countdown timer, the crosswind speed warnings, 
// and the specific mission failure/success end screens.
void DrawMissionRestrictedUI(RaceSystem *race);

#endif // Ends the include guard for MISSION_RESTRICTED_H