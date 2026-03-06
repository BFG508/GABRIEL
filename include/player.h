// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef PLAYER_H
#define PLAYER_H

// Include the main Raylib library so the compiler knows what 'Vector3' is.
// Include the Raylib math library for Lerp function.
#include "raylib.h"
#include "raymath.h"


// --- CONSTANTS ---
// Defining the maximum number of smoke particles on screen here
// makes it easy to add more later without changing the logic.
#define MAX_PARTICLES 200


// --- ENUMERATIONS (STATES) ---
// An 'enum' is a way to assign names to numbers. 
// Instead of remembering that 1 is a plane and 2 is a helicopter, 
// we use clear, readable names. Under the hood, VEHICLE_NONE = 0, 
// VEHICLE_PLANE = 1, and VEHICLE_HELICOPTER = 2.
typedef enum VehicleType {
    VEHICLE_NONE = 0,       // Default state (e.g., when in the main menu).
    VEHICLE_PLANE,          // Represents the SR-71 Blackbird.
    VEHICLE_HELICOPTER      // Represents the AH-64 Apache.
} VehicleType;


// --- DATA STRUCTURES ---
// A 'struct' groups related variables into a single package.

// Defines the physical properties, movement, lifespan, and active state 
// of a single visual smoke entity within the 3D world.
typedef struct Particle {
    Vector3 position; // Where the smoke is in the 3D world.
    Vector3 velocity; // Controls the chaotic drift.
    float life;       // How much time it has left before disappearing (1.0f to 0.0f).
    bool active;      // Is this particle currently being used?
} Particle;

// A comprehensive container that stores the current state, physics parameters, vehicle type, 
// and visual effects pool of the user's controlled aircraft.
typedef struct Player {
    Vector3 position;              // Current (X, Y, Z) coordinates in the 3D world.
    Vector3 velocity;              // Current speed and direction of movement.
    Vector3 rotation;              // Controls the visual tilt (Pitch, Yaw, Roll).

    float throttle;                // Engine power (Continuous movement).
    float acceleration;            // How quickly the vehicle gains speed when a key is pressed.
    float friction;                // Momentum decay multiplier (slows the vehicle down over time).

    VehicleType type;              // Stores whether the player chose the plane or the helicopter.

    float smokeDelayTimer;         // Timer for vehicle's switching.
    Particle smoke[MAX_PARTICLES]; // Particle pool.
} Player;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in main.c before defining what they actually do.

// --- PLAYER FACTORY FUNCTION ---
// Creates and returns a fully initialized Player structure ready for flight.
// This function configures the unique physical properties of the chosen vehicle type,
// and spawns it precisely at the level's designated coordinates and orientation.
// Notice it returns the entire 'Player' struct by value, creating a fresh 
// copy in RAM that the main game loop will take ownership of.
Player InitPlayer(VehicleType type, Vector3 startPos, float startYaw);

// Updates the physics and reads the input for the player.
// VERY IMPORTANT: Notice the asterisk (*). We are passing a POINTER to the player.
// Why? Because if we just passed 'Player player', C would create a temporary COPY of it, 
// update the copy, and destroy it, leaving our real player untouched.
// By passing the memory address (*player), this function modifies the actual player in main.c.
void UpdatePlayer(Player *player);

#endif // Ends the include guard