# G.A.B.R.I.E.L. ✈️
**G**eneral **A**viation & **B**asic **R**aylib **I**nteractive **E**ngine **L**ayer

A basic but dynamic 3D flight simulator written purely in C. This project was created to practice low-level C programming concepts, manual memory management, 3D coordinate systems, matrix transformations, game loop architecture, and finite state machines (state transitions).

## 🚀 Features
* **Continuous Throttle Physics:** Smooth acceleration, lift generation, and momentum decay (friction) custom-built for both fixed-wing aircraft and helicopters.
* **Dynamic Visual Tilt & Steering:** The aircraft yaw, rolls, and pitches realistically based on user input and aerospace matrix multiplication.
* **Modular Mission Architecture:** A clean "Director-Worker" design in C that separates global race logic from specialized mission types, preventing spaghetti code and allowing easy expansion.
* **Data-Driven Level Design:** Dynamically parses external `.txt` files to generate 3D tracks, ring coordinates, mission titles, and player spawn points without recompiling the C code. Includes a scalable 5x5 grid mission selector.
* **Time Trial Racing System:** A fully functional 3D checkpoint circuit with strict cylindrical collision detection, an active stopwatch, and a dynamic vectorial navigation arrow.
* **Precision Landing Operations:** A new mission type requiring pilots to strictly manage their kinetic energy, descent rate, and throttle to execute a safe touchdown on a designated 3D helipad.
* **Advanced Collision Detection:** Dual raycasting system to detect mountains directly ahead and precisely calculate 3D ground height beneath the vehicle.
* **Infinite Horizon Grid:** Utilizes OpenGL matrix transformations (`rlPushMatrix` / `rlPopMatrix`) to dynamically snap a massive grid to the player, creating a boundless, high-performance visual floor without Z-fighting or popping.
* **Smooth 3rd-Person Orbit Camera:** Look around your aircraft dynamically using linear interpolation (Lerp) for cinematic, weight-feeling camera movements, featuring absolute positioning for gamepad thumbsticks.
* **Robust Persistent Leaderboards:** A local file-based high-score system that tracks the fastest pilots per level. Includes strict data sanitization (anti-ghosting) to handle duplicate names seamlessly and an arcade-style virtual wheel for gamepad input.
* **State Machine:** Clean architectural separation between the Main Menu, Level Select, Game Loop, and Leaderboards.
* **Full Mouse, Gamepad & Steam Deck Support:** Seamlessly navigate the UI using a controller, keyboard, or the newly implemented responsive mouse controls (single-click to select, double-click to launch). Plug-and-play Xbox integration with analog precision and real-time dynamic text swapping.
* **Adaptive 4:3 Resolution:** Auto-scaling window that detects monitor size to maximize screen real estate while maintaining a retro simulator aspect ratio.

## 🛠️ Technology Stack
This project uses **Raylib**, a highly modular and simple-to-use library to enjoy videogames programming in pure C without the overhead of massive game engines.

## 🎮 Controls

### ✈️ Flight Controls
| Action | Keyboard | Gamepad (Xbox / Steam Deck) |
| :--- | :--- | :--- |
| **Throttle (Engine Power)** | W / S | LT / RT (Analog Triggers) |
| **Turn (Yaw / Roll)** | A / D | Left Stick (X-Axis) |
| **Pitch (Climb / Dive)** | SPACE / SHIFT | Left Stick (Y-Axis) |
| **Select Plane** | 1 | X Button |
| **Select Helicopter** | 2 | Y Button |
| **Toggle Camera (1st/3rd Person)** | C | A Button |
| **Look Around (3rd Person)** | Arrow Keys | Right Stick |
| **Quick Restart** | R | B Button |
| **Toggle UI Controls** | H | Menu / Start Button |
| **Exit Game** | ESC | View / Back Button |

### 🖥️ Menu & Navigation
| Action | Keyboard / Mouse | Gamepad (Xbox / Steam Deck) |
| :--- | :--- | :--- |
| **Navigate Grid / Letters** | Arrow Keys | D-PAD / Left Stick |
| **Select Level** | Left Click (Mouse) | - |
| **Confirm / Launch Level** | ENTER / Double Click | A Button |
| **Virtual Keyboard: Type** | A-Z Keys | A Button |
| **Virtual Keyboard: Delete** | BACKSPACE | X Button |
| **Virtual Keyboard: Submit** | ENTER | Menu / Start Button |

## ⚙️ Prerequisites & Compilation
This project uses a universal `Makefile`. You must have **Raylib** installed on your system to compile it.

### Windows (MinGW)
1. Download the Raylib Windows Installer from [GitHub](https://github.com/raysan5/raylib/releases).
2. Open your terminal in the project folder and run:
   ```bash
   mingw32-make OBJS="src/*.c"

### Linux
1. Install Raylib and development dependencies (X11, OpenGL, etc.).
2. Run:
   ```bash
   make OBJS="src/*.c"

### macOS
1. Install Raylib via Homebrew: `brew install raylib`.
2. Run:
   ```bash
   make OBJS="src/*.c"

### ⚠️ Note on Compiling
If you get an error stating that the compiler cannot find `raylib.h` or `-lraylib`, you may need to open the `Makefile` in a text editor and adjust the `INCLUDE_PATHS` and `LIBRARY_PATHS` to match exactly where you installed Raylib on your local machine (e.g., `C:/raylib/raylib/src`).

## 🎨 Credits & Assets
This project uses the following open-source 3D models from Sketchfab:
* **SR-71 Blackbird** model by [KOG_THORNS](https://sketchfab.com/ioai25312)
* **AH-64 Apache** model by [Muhamad Mirza Arrafi](https://sketchfab.com/nazidefenseforceofficial)
* **Terrain** model by [EntropyNine](https://sketchfab.com/entropy9ine)
* **Skybox** model by [skybox3dArchitect](https://sketchfab.com/skybox3dArchitect)
* **Ring** model by [FishyBusiness](https://sketchfab.com/FishyBusiness)

Also this project uses the following open-source sounds from Freesound:
* **Airplane** sound by [daliacoss](https://freesound.org/people/daliacoss/)
* **Helicopter** sound by [UnderlinedDesigns](https://freesound.org/people/UnderlinedDesigns/)
* **Initial menu** music by [Vrymaa](https://freesound.org/people/Vrymaa/)
* **Ending** music by [ViraMiller](https://freesound.org/people/ViraMiller/)