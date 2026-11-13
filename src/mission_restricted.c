// Include math library to use advanced mathematical functions.
#include <math.h>

// We include our own header files.
// We also need raymath.h to handle advanced Vector2 and Vector3 operations.
#include "race.h"
#include "mission_rings.h"
#include "raymath.h"


// --- UPDATE LOOP (WORKER) ---
// This function acts as the Referee for Restricted Flight Missions.
// It handles wind displacement, radar ceilings, and ensures the player stays inside the designated corridor.
void UpdateMissionRestricted(RaceSystem *race, Player *player) {
    
    // --- 1. QUICK ESCAPE (ZOMBIE PREVENTION) ---
    // If the mission is already over (success or failure), do not calculate physics.
    // This prevents crashed planes from continuing to trigger game logic while sliding on the ground.
    if (race->missionFailed || race->isFinished) {
        return;
    }

    // Delta time ensures our physics (like wind and timers) run at the same speed 
    // regardless of the computer's frame rate.
    float dt = GetFrameTime();


    // --- A. ENVIRONMENTAL PHYSICS (WIND) ---
    // The wind force is a Vector3 that physically pushes the aircraft through the 3D world.
    // Because this modifies the player's position directly, the pilot will have to constantly
    // roll and yaw (crab angle) to counteract this invisible force and stay on the path.
    player->position.x += race->windForce.x * dt;
    player->position.y += race->windForce.y * dt;
    player->position.z += race->windForce.z * dt;


    // --- B. RADAR CEILING CHECK (MAX ALTITUDE) ---
    // If the aircraft's Y coordinate (altitude) goes above the defined ceiling, 
    // it is "detected by enemy radar" and the mission fails instantly.
    if (player->position.y > race->maxAltitude) {
        race->missionFailed = true;
    }

    // --- C. GROUND COLLISION CHECK ---
    // Unlike the landing missions where touching the ground is the goal, 
    // in restricted flight, touching the ground (Y <= 1.0f) is always a fatal crash.
    if (player->position.y <= 1.0f) {
        race->missionFailed = true;
    }


    // --- D. PATH BOUNDARY CHECK (THE CORRIDOR GEOMETRY) ---
    // We must ensure the player's 2D position (X and Z, ignoring altitude) is always 
    // within a certain distance ('pathWidth') of the polyline created by the waypoints.
    
    // Extract the player's position on the flat 2D ground plane.
    Vector2 p = { player->position.x, player->position.z };
    
    // We start with an absurdly high minimum distance. 
    // We will shrink this number as we check each segment of the path.
    float minDistanceToPath = 999999.0f; 

    // Loop through all geometric segments (Line from point A to point B) of the corridor.
    for (int i = 0; i < race->numPathPoints - 1; i++) {
        // Point A (Start of the segment) and Point B (End of the segment).
        Vector2 a = { race->pathPoints[i].x, race->pathPoints[i].z };
        Vector2 b = { race->pathPoints[i+1].x, race->pathPoints[i+1].z };

        // Vector math to find the closest geometric point on the line segment A-B to the player P.
        // 'ab' is the vector representing the segment itself.
        // 'ap' is the vector from the start of the segment to the player.
        Vector2 ab = { b.x - a.x, b.y - a.y };
        Vector2 ap = { p.x - a.x, p.y - a.y };

        // Calculate the projection scalar 't'.
        // This tells us mathematically where the player is located ALONG the infinite line defined by A and B.
        float dotProduct = (ap.x * ab.x + ap.y * ab.y);
        float abLengthSq = (ab.x * ab.x + ab.y * ab.y);
        float t = dotProduct / abLengthSq;

        // Clamp 't' between 0.0 (Point A) and 1.0 (Point B).
        // If 't' is negative, the player is behind point A. If > 1, they are past point B.
        // Clamping ensures we measure distance to the actual finite segment, not the infinite line.
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Calculate the exact 2D coordinates of the closest point on this segment to the player.
        Vector2 closestPoint = { a.x + t * ab.x, a.y + t * ab.y };
        
        // Measure the distance from the player to that closest point.
        float dist = Vector2Distance(p, closestPoint);
        
        // If this segment is closer than any previous segment we checked, update our minimum distance.
        if (dist < minDistanceToPath) {
            minDistanceToPath = dist;
        }
    }

    // After checking all segments, if the absolute closest point on the entire corridor 
    // is further away than our allowed path width, the player flew out of bounds!
    if (minDistanceToPath > race->pathWidth) {
        race->missionFailed = true;
    }


    // --- E. WAYPOINT NAVIGATION PROGRESSION ---
    // The HUD arrow points to the next waypoint. We need to advance this target 
    // when the player gets reasonably close to it.
    if (race->currentWaypoint < race->numPathPoints) {
        Vector2 target2D = { race->pathPoints[race->currentWaypoint].x, race->pathPoints[race->currentWaypoint].z };
        
        // We use a radius of 1.5x the path width as the "checkpoint" trigger area.
        if (Vector2Distance(p, target2D) < race->pathWidth * 1.5f) {
            // Reached the current waypoint! Aim the arrow at the next one.
            race->currentWaypoint++;
            
            // Prevent the array index from going out of bounds if we reach the end of the line.
            if (race->currentWaypoint >= race->numPathPoints) {
                race->currentWaypoint = race->numPathPoints - 1; 
            }
        }
    }


    // --- F. SURVIVAL TIMER (VICTORY CONDITION) ---
    // If the player survived all constraints (radar, wind, out-of-bounds, floor) this frame...
    if (!race->missionFailed) {
        // Add time to the survival clock.
        race->currentSurvivalTime += dt;
        
        // Did we survive the required amount of time?
        if (race->currentSurvivalTime >= race->targetSurvivalTime) {
            race->isFinished = true;        // Mark as victorious.
            race->isRaceActive = false;     // Stop evaluating physics to lock the victory.
        }
    }


    // --- G. FAILURE TRIGGER (AUDIO AND PHYSICS CUTOFF) ---
    if (race->missionFailed) {
        race->isRaceActive = false; // Stop the global timer immediately.
        race->isFinished = false;   // Block the victory/name input screen.
        
        // Zero out the physics vectors.
        // This stops the aircraft from visually drifting or rolling after failing.
        player->throttle = 0.0f;
        player->velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    }
}


// --- RENDERING FUNCTION (3D WORLD) ---
// Draws the visual elements of the restricted zone (Corridor, Radar Ceiling, Arrow).
void DrawMissionRestricted3D(RaceSystem *race, Player *player) {
    if (race->isRaceActive || race->missionFailed) { 
        
        // --- 1. DRAW THE SAFE CORRIDOR ON THE GROUND ---
        // We use 'Fade' to make the path semi-transparent so it looks like a hologram.
        Color pathColor = Fade(LIME, 0.3f);
        if (race->missionFailed) {
            pathColor = Fade(RED, 0.5f); // Turn the entire path red to indicate failure.
        }

        // We iterate through the points and draw thick cylinders laid flat on the ground.
        for (int i = 0; i < race->numPathPoints - 1; i++) {
            
            // Start and End points for this segment.
            // We lift it slightly (Y = 0.1f) to prevent "Z-fighting" with the green grass terrain.
            Vector3 p1 = { race->pathPoints[i].x, 0.1f, race->pathPoints[i].z };
            Vector3 p2 = { race->pathPoints[i+1].x, 0.1f, race->pathPoints[i+1].z };
            
            // Draw a cylindrical tube from p1 to p2. 
            // The radius of the cylinder is exactly the 'pathWidth' allowed in the rules.
            DrawCylinderEx(p1, p2, race->pathWidth, race->pathWidth, 12, pathColor);
            
            // Draw a spherical joint at the connection points (Waypoints).
            // This ensures the corners of the path look rounded and smooth instead of sharp and broken.
            DrawSphere(p1, race->pathWidth, pathColor);
        }
        
        // Cap the final waypoint with a sphere so the end of the road isn't flat.
        if (race->numPathPoints > 0) {
            Vector3 lastP = { race->pathPoints[race->numPathPoints-1].x, 0.1f, race->pathPoints[race->numPathPoints-1].z };
            DrawSphere(lastP, race->pathWidth, pathColor);
        }

        // --- 2. DRAW THE RADAR CEILING (Translucent plane above) ---
        // Helps the player visualize exactly how high they can fly before failing.
        float ceilingGridSize = race->pathWidth * 10.0f; // Make the plane wide enough to easily see.
        Vector3 ceilingCenter = { player->position.x, race->maxAltitude, player->position.z };
        
        // Draw a giant red translucent square floating directly above the player.
        DrawPlane(ceilingCenter, (Vector2){ceilingGridSize, ceilingGridSize}, Fade(RED, 0.2f));

        // --- 3. DRAW NAVIGATION ARROW ---
        // Points holographically towards the next node on the path.
        if (!race->missionFailed && race->numPathPoints > 0) {
            DrawNavArrow(player, race->pathPoints[race->currentWaypoint]);
        }
    }
}


// --- RENDERING FUNCTION (2D HUD) ---
// Draws the specific on-screen text for the restricted mission.
void DrawMissionRestrictedUI(RaceSystem *race) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (race->missionFailed) {
        // --- FAILURE SCREEN ---
        const char *failText = "RESTRICTED AIRSPACE VIOLATED!";
        int failWidth = MeasureText(failText, 40);
        DrawTextOutlined(failText, (screenWidth - failWidth) / 2, screenHeight * 0.4f, 40, RED, 2);
        
        // Dynamic controls instruction.
        const char *restText = IsGamepadAvailable(0) ? "Press [B] to Restart" : "Press [R] to Restart";
        int restWidth = MeasureText(restText, 30);
        DrawTextOutlined(restText, (screenWidth - restWidth) / 2, screenHeight * 0.5f, 30, GRAY, 2);
        
    } else if (race->isRaceActive) {
        // --- NORMAL HUD ---
        
        // 1. Survival Timer (Counts DOWN instead of up to create tension).
        float timeLeft = race->targetSurvivalTime - race->currentSurvivalTime;
        if (timeLeft < 0) timeLeft = 0; // Prevent it from briefly showing negative numbers.
        
        const char *timerText = TextFormat("SURVIVE: %.2f s", timeLeft);
        int timerWidth = MeasureText(timerText, 40);
        DrawTextOutlined(timerText, (screenWidth - timerWidth) / 2, screenHeight * 0.05f, 40, LIME, 2);

        // 2. Wind Warning System.
        // We multiply the raw wind vector by 100 to convert it into a dramatic "KM/H" figure for the UI.
        float windSpeedKmh = Vector3Length(race->windForce) * 100.0f; 
        
        // Only show the warning if there is significant wind.
        if (windSpeedKmh > 10.0f) {
            const char *windText = TextFormat("WARNING: CROSSWIND %.0f KM/H", windSpeedKmh);
            int windWidth = MeasureText(windText, 20);
            DrawTextOutlined(windText, (screenWidth - windWidth) / 2, screenHeight * 0.15f, 20, ORANGE, 2);    
        }
        
    } else if (race->isFinished && race->finishedTimer < 3.0f) {
        // --- VICTORY SCREEN ---
        const char *winText = "EVASION SUCCESSFUL!";
        int winWidth = MeasureText(winText, 40);
        DrawTextOutlined(winText, (screenWidth - winWidth) / 2, screenHeight * 0.4f, 40, GOLD, 2);
    }
}