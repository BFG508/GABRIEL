// --- INCLUDE GUARD ---
// Prevents this header file from being included multiple times in the same compilation process.
// If it gets included twice, the compiler would complain about "redefinition" errors.
#ifndef LEADERBOARD_H
#define LEADERBOARD_H

// Include the main Raylib library.
// We also inclue player.h  so the compiler knows what 'VehicleType' is.
// We also include standard C libraries for string manipulation and file Input/Output.
#include "raylib.h"
#include "player.h"
#include <string.h>
#include <stdio.h>


// --- CONSTANTS ---
// Defining the maximum entries and name length here makes it easy to adjust later 
// without digging through the logic code.
#define MAX_LEADERBOARD 10
#define MAX_NAME_LENGTH 15


// --- DATA STRUCTURES ---
// A 'struct' groups related variables into a single package.

// Represents a single player's record in the leaderboard.
typedef struct LeaderboardEntry {
    char name[MAX_NAME_LENGTH + 1]; // +1 to leave room for the invisible null-terminator '\0'.
    float time;                     // The total race time.
    VehicleType vehicle;            // Player's vehicle type  
} LeaderboardEntry;

// The main system that holds the entire Top 10 list.
typedef struct Leaderboard {
    LeaderboardEntry entries[MAX_LEADERBOARD]; // The array holding up to 10 records.
    int count;                                 // How many records are currently stored (0 to 10).
} Leaderboard;


// --- FUNCTION PROTOTYPES ---
// These declarations tell the compiler the names of our functions and what parameters they take,
// so it doesn't panic when we call them in main.c before defining what they actually do.

// Reads the text file from the hard drive and loads the saved times into RAM.
// It returns a full 'Leaderboard' struct (passed by value).
Leaderboard LoadLeaderboard(const char *filename);

// Writes the current leaderboard from RAM into a text file on the hard drive.
// We pass a POINTER (*lb) to avoid copying the entire array just to read it.
void SaveLeaderboard(Leaderboard *lb, const char *filename);

// Evaluates a new time, finds its proper sorted position (lowest to highest), 
// shifts the slower times down, and inserts the new record.
// We pass a POINTER (*lb) because we need to modify the actual leaderboard array.
void AddLeaderboardEntry(Leaderboard *lb, const char *name, float time, VehicleType vehicle);

#endif // Ends the include guard