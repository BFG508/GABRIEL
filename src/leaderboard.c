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
        int expectedCount = 0;
        if (fscanf(file, "%d", &expectedCount) == 1) {
            
            // Safety measure: Never read more than our array can hold.
            if (expectedCount > MAX_LEADERBOARD) {
                expectedCount = MAX_LEADERBOARD;
            }

            int actualValidRecords = 0;
            
            // Loop through the file and read each name, time, and vehicle.
            for (int i = 0; i < expectedCount; i++) {
                int vType;
                
                // We must check that fscanf successfully found exactly 3 items.
                // We use %15s to prevent buffer overflows if a name in the text file is too long.
                // If a line in the .txt is corrupted or empty, this prevents "Ghost users".
                if (fscanf(file, "%15s %f %d", lb.entries[actualValidRecords].name, &lb.entries[actualValidRecords].time, &vType) == 3) {
                    
                    lb.entries[actualValidRecords].vehicle = (VehicleType)vType;
                    actualValidRecords++; // Only increment if the read was 100% successful.
                }
            }
            
            // Overwrite the count with the number of unbroken records we actually managed to load.
            lb.count = actualValidRecords;
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
    
    // Safety check: If the OS denies permission to create the file, we abort.
    if (file == NULL) return;

    // Write the total number of valid records at the very top.
    fprintf(file, "%d\n", lb->count);
    
    // Loop through our array and write each entry line by line.
    for (int i = 0; i < lb->count; i++) {
        fprintf(file, "%s %f %d\n", lb->entries[i].name, lb->entries[i].time, lb->entries[i].vehicle);
    }
    
    fclose(file);
}


// --- ADD ENTRY FUNCTION ---
// Evaluates a new time, finds its proper sorted position (lowest to highest), 
// shifts the slower times down, and inserts the new record. Multiple identical names are allowed.
void AddLeaderboardEntry(Leaderboard *lb, const char *name, float time, VehicleType vehicle) {
    
    // 1. Filtering worst times.
    // If the board is full (10 entries) AND this new time is slower (greater) 
    // than our absolute worst time (the one at index 9), we ignore it completely.
    if (lb->count == MAX_LEADERBOARD && time >= lb->entries[MAX_LEADERBOARD - 1].time) {
        return; 
    }

    // 2. Finding the right spot.
    // We search from the top (1st place) downwards to find where this new time belongs.
    int targetIndex = 0;
    while (targetIndex < lb->count && lb->entries[targetIndex].time < time) {
        targetIndex++;
    }

    // 3. Shifting losers down.
    // We must move everyone below 'targetIndex' down by one slot to make room.
    // We start from the bottom (either the last empty item, or the 9th slot to drop the 10th).
    int startShiftIndex;
    if (lb->count < MAX_LEADERBOARD) {
        startShiftIndex = lb->count;
    } else {
        startShiftIndex = MAX_LEADERBOARD - 1;
    }
    
    for (int i = startShiftIndex; i > targetIndex; i--) {
        lb->entries[i] = lb->entries[i - 1]; // Copy the entry above into this slot.
    }

    // 4. Data sanitization (Anti-ghost protocol).
    // We copy the name into a temporary safe string.
    char safeName[MAX_NAME_LENGTH + 1] = "UNKNOWN"; // Default fallback name.
    
    if (name != NULL && strlen(name) > 0) {
        strncpy(safeName, name, MAX_NAME_LENGTH);
        safeName[MAX_NAME_LENGTH] = '\0'; // Force the null-terminator.
        
        // C's fscanf gets confused by spaces. We must replace spaces with underscores.
        for (int i = 0; i < strlen(safeName); i++) {
            if (safeName[i] == ' ') {
                safeName[i] = '_';
            }
        }
    }

    // 5. Inserting the champion.
    // Now that the data is clean, we assign it to the correct spot in the array.
    strncpy(lb->entries[targetIndex].name, safeName, MAX_NAME_LENGTH + 1);
    lb->entries[targetIndex].time = time;
    lb->entries[targetIndex].vehicle = vehicle;

    // 6. Update count.
    // If the board wasn't full yet, we now have one more official entry.
    if (lb->count < MAX_LEADERBOARD) {
        lb->count++;
    }
}