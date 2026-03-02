// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef RACE_H
#define RACE_H

// Include the main Raylib library so the compiler knows what 'Vector3' is.
// We also need 'player.h' because the race system needs to know where the player is to check collisions.
#include "raylib.h"
#include "player.h"

// --- CONSTANTS ---
// Defining the number of rings here makes it easy to add more later without changing the logic.
#define MAX_RINGS 5

// --- DATA STRUCTURES ---
// A 'struct' groups related variables into a single package.

// Represents a single floating checkpoint in the sky
typedef struct Ring {
    Vector3 position; // Where the ring is located in the 3D world
    float radius;     // How big the ring is (collision size)
    float yaw;        // Rotation on the Y axis (in degrees) to face different directions
    bool active;      // True if the player still needs to fly through this ring
} Ring;

// The main system that acts as the "Referee" of the race
typedef struct RaceSystem {
    Ring rings[MAX_RINGS]; // The array (list) containing all the rings in the circuit
    int targetRing;        // The index (0 to MAX_RINGS - 1) of the NEXT ring the player must cross
    float timer;           // The current race time in seconds
    float finishedTimer;   // Tracks how many seconds have passed since crossing the finish line
    bool isRaceActive;     // True while the player is racing, False when finished or not started
    bool isFinished;       // True if the player successfully crossed all rings
} RaceSystem;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in main.c before defining what they actually do.

// Initializes and returns a brand new Race package.
// It sets up the starting positions of all the rings and resets the timer.
// Notice it returns a full 'RaceSystem' struct (passed by value), just like InitPlayer.
RaceSystem InitRace(void);

// Updates the race logic (timer and collisions).
// VERY IMPORTANT: We pass POINTERS to both the race and the player.
// We need the race pointer to update the timer and mark rings as inactive.
// We need the player pointer (read-only in this case) to check their exact 3D coordinates.
void UpdateRace(RaceSystem *race, Player *player);

// Draws the rings and the race UI (timer, remaining rings) on the screen.
// We pass a POINTER to avoid copying the whole array of rings into memory 60 times per second.
void DrawRace3D(RaceSystem *race, Player *player);
void DrawRaceUI(RaceSystem *race);

#endif // Ends the include guard