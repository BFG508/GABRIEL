// We include our own header file.
#include "resource_manager.h"


// --- GLOBAL VARIABLE DEFINITIONS ---
// This is where the compiler actually reserves physical RAM for the external files.
Texture2D mapTexture;

Model mapModel;
Model environmentModel;
Model skyboxModel;

Model planeModel;
Model helicopterModel;

Model ringModel;


Sound planeSound;  
Sound helicopterSound;

Music menuMusic;


// --- LOAD FUNCTION ---
// This function reads the heavy files from the slow Hard Drive 
// and loads them into the fast RAM.
void LoadGameResources(void) {
    // Load the 3D models, the sounds and the music using Raylib's built-in function.
    // NOTE: If you misspell the file name, Raylib won't crash your game; 
    // but it will just print a yellow warning in the console and show nothing on screen.

    // 1. 3D models
    // Raylib automatically reads the geometry and the embedded textures from the .glb files.
    environmentModel = LoadModel("resources/models/terrain.glb");
    skyboxModel = LoadModel("resources/models/skybox.glb");


    ringModel = LoadModel("resources/models/ring.glb");
    ringModel.transform = MatrixMultiply(ringModel.transform, MatrixRotateX(90.0f * DEG2RAD));


    planeModel = LoadModel("resources/models/blackbird.glb");
    planeModel.transform = MatrixMultiply(planeModel.transform, MatrixRotateY(90.0f * DEG2RAD));

    helicopterModel = LoadModel("resources/models/apache.glb");
    helicopterModel.transform = MatrixMultiply(helicopterModel.transform, MatrixRotateY(90.0f * DEG2RAD));

    // 2. Sounds & Music
    planeSound = LoadSound("resources/sounds/plane.wav");
    SetSoundVolume(planeSound, 1.30f);

    helicopterSound = LoadSound("resources/sounds/helicopter.wav");
    SetSoundVolume(helicopterSound, 0.50f);


    menuMusic = LoadMusicStream("resources/sounds/menu.mp3");
    SetMusicVolume(menuMusic, 0.55f);
}


// --- UNLOAD FUNCTION ---
// The Golden Rule of C: If you allocate memory, you MUST free it.
// This function cleans up the RAM. We call it at the very end of main.c, 
// right after the game loop finishes and the user closes the window.
void UnloadGameResources(void) {
    // Destroys the external files and frees the RAM they were taking up.
    // If we didn't do this, we would create a "Memory Leak".

    // 1. 3D models
    UnloadModel(environmentModel);
    UnloadModel(skyboxModel);

    UnloadModel(ringModel);

    UnloadModel(planeModel);
    UnloadModel(helicopterModel);
    
    // 2. Sounds & Music
    UnloadSound(planeSound);
    UnloadSound(helicopterSound);

    UnloadMusicStream(menuMusic);
}