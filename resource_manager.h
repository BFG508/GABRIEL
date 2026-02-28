// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

// Include the main Raylib library so the compiler knows what 'Texture2D' and 'Model' are.
// Include the main Raylib math library for Matrix math functions and the DEG2RAD macro.
#include "raylib.h"
#include "raymath.h"

// --- GLOBAL ASSETS ---
// The 'extern' keyword tells the compiler: "These variables exist, but they are actually
// created and take up memory in another file (in our case, resource_manager.c)".
// This allows any file (like main.c) that includes this header to access the assets safely.
extern Texture2D worldMapTexture;  // Stores the 2D image for the ground
extern Model mapModel;             // Stores the 3D plane for the ground
extern Model planeModel;           // Stores the 3D data for the SR-71 Blackbird
extern Model helicopterModel;      // Stores the 3D data for the AH-64 Apache


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in main.c before defining what they actually do.

// Loads all textures and models from the hard drive into RAM. Must be called once before the game loop.
void LoadGameResources(void);

// Frees the RAM used by the textures and models. Must be called once right before closing the program.
void UnloadGameResources(void);

#endif // Ends the include guard