// We include our own header file.
#include "leaderboard.h"

// Standard C doesn't have a built-in function to create folders, 
// so we ask the Operating System (Windows or Linux/Mac) to do it.
#ifdef _WIN32
    #include <direct.h>
    #define MAKE_DIR(name) _mkdir(name)
#else
    #include <sys/stat.h>
    #define MAKE_DIR(name) mkdir(name, 0777)
#endif


// --- LOAD FUNCTION ---
// Reads the saved records from a text file when the game starts.
Leaderboard LoadLeaderboard(const char *filename) {
    // We create a local Leaderboard variable and initialize all its memory to 0.
    // { 0 } is a great C trick to ensure no garbage data is left in memory.
    Leaderboard lb = { 0 };
    
    // Open the file in "r" (Read) mode.
    FILE *file = fopen(filename, "r");
    
    // If the file exists, we read it. If it doesn't (first time playing), 
    // we just return the empty leaderboard.
    if (file != NULL) {
        // Read the first line: how many records are saved?
        if (fscanf(file, "%d", &lb.count) == 1) {
            
            // Loop through the file and read each name and time.
            for (int i = 0; i < lb.count; i++) {
                // fscanf reads string (%s) and float (%f) separated by a space.
                int vType;
                fscanf(file, "%s %f %d", lb.entries[i].name, &lb.entries[i].time, &vType);
                lb.entries[i].vehicle = (VehicleType)vType;
                fscanf(file, "%s %f", lb.entries[i].name, &lb.entries[i].time);
            }
        }
        
        // The Golden Rule of File I/O: Always close the file when done!
        fclose(file);
    }
    
    // Hand the loaded package back to main.c.
    return lb;
}


// --- SAVE FUNCTION ---
// Writes the current records into a text file so they survive when the game closes.
void SaveLeaderboard(Leaderboard *lb, const char *filename) {
    // Create the directory safely before trying to open the file.
    // If the folder already exists, this function will just silently fail and continue.
    MAKE_DIR("data");
    
    // Open the file in "w" (Write) mode. This will create the file if it doesn't exist,
    // or completely overwrite it if it does.
    FILE *file = fopen(filename, "w");
    
    // Safety check: if the OS denies permission to create the file, we abort.
    if (file == NULL) return;

    // Write the total number of records at the very top.
    fprintf(file, "%d\n", lb->count);
    
    // Loop through our array and write each entry line by line.
    for (int i = 0; i < lb->count; i++) {
        fprintf(file, "%s %f %d\n", lb->entries[i].name, lb->entries[i].time, lb->entries[i].vehicle);
    }
    
    fclose(file);
}


// --- ADD ENTRY FUNCTION ---
// This is the core logic. It sorts the new time, drops the 11th place, and keeps the top 10.
void AddLeaderboardEntry(Leaderboard *lb, const char *name, float time, VehicleType vehicle) {
    
    // 1. Filtering.
    // If the board is already full (10 entries) AND this new time is slower (greater) 
    // than our absolute worst time (the one at index 9), we don't even bother.
    if (lb->count == MAX_LEADERBOARD && time >= lb->entries[MAX_LEADERBOARD - 1].time) {
        return; 
    }

    // 2. Finding the right spot.
    // We search from the top (index 0) downwards to find where this new time belongs.
    int targetIndex = 0;
    while (targetIndex < lb->count && lb->entries[targetIndex].time < time) {
        targetIndex++;
    }

    // 3. Shifting losers down.
    // We must move everyone below 'targetIndex' down by one slot to make room.
    // We start from the bottom (either the last item, or the 9th slot to drop the 10th).
    int startShiftIndex = (lb->count < MAX_LEADERBOARD) ? lb->count : MAX_LEADERBOARD - 1;
    
    for (int i = startShiftIndex; i > targetIndex; i--) {
        lb->entries[i] = lb->entries[i - 1]; // Copy the entry above into this slot.
    }

    // 4. Inserting the champion.
    // We copy the string safely to avoid buffer overflows (hackers trying to write beyond memory).
    strncpy(lb->entries[targetIndex].name, name, MAX_NAME_LENGTH);
    lb->entries[targetIndex].name[MAX_NAME_LENGTH] = '\0'; // Force the null-terminator just in case.
    
    lb->entries[targetIndex].time = time;

    strncpy(lb->entries[targetIndex].name, name, MAX_NAME_LENGTH);
    lb->entries[targetIndex].name[MAX_NAME_LENGTH] = '\0'; 
    
    lb->entries[targetIndex].time = time;
    lb->entries[targetIndex].vehicle = vehicle;

    // 5. Update count.
    // If the board wasn't full yet, we now have one more entry.
    if (lb->count < MAX_LEADERBOARD) {
        lb->count++;
    }
}