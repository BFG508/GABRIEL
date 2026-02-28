// We include the header file so the compiler checks that our functions 
// here perfectly match the prototypes we promised in resource_manager.h.
#include "resource_manager.h"


// --- GLOBAL VARIABLE DEFINITIONS ---
// This is where the compiler actually reserves physical RAM for the plane and 
// the helicopter models.
Model planeModel;
Model helicopterModel;

// --- LOAD FUNCTION ---
// This function reads the heavy files from the slow Hard Drive 
// and loads them into the fast RAM.
void LoadGameResources(void) {
    // Load the 3D models using Raylib's built-in function.
    // It automatically reads the geometry and the embedded textures from the .glb files.
    // NOTE: If you misspell the file name, Raylib won't crash your game; 
    // it will just print a yellow WARNING in the console and show nothing on screen.
    planeModel = LoadModel("resources/models/blackbird.glb");
    planeModel.transform = MatrixMultiply(planeModel.transform, MatrixRotateY(90.0f * DEG2RAD));

    helicopterModel = LoadModel("resources/models/apache.glb");
    helicopterModel.transform = MatrixMultiply(helicopterModel.transform, MatrixRotateY(90.0f * DEG2RAD));
}

// --- UNLOAD FUNCTION ---
// The Golden Rule of C: If you allocate memory, you MUST free it.
// This function cleans up the RAM. We call it at the very end of main.c, 
// right after the game loop finishes and the user closes the window.
void UnloadGameResources(void) {
    // Destroys the models and frees the RAM they were taking up.
    // If we didn't do this, we would create a "Memory Leak".
    UnloadModel(planeModel);
    UnloadModel(helicopterModel);
}