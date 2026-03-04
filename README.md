# G.A.B.R.I.E.L. ✈️
**G**eneral **A**viation & **B**asic **R**aylib **I**nteractive **E**ngine **L**ayer

A basic but dynamic 3D flight simulator written purely in C. This project was created to practice low-level C programming concepts, manual memory management, 3D coordinate systems, matrix transformations, and game loop architecture.

## 🚀 Features
* **Time Trial Racing System:** A fully functional 3D checkpoint circuit with strict cylindrical collision detection, an active stopwatch, and a dynamic vectorial navigation arrow.
* **Continuous Throttle Physics:** Smooth acceleration, lift generation, and momentum decay (friction) custom-built for both fixed-wing aircraft and helicopters.
* **Dynamic Visual Tilt & Steering:** The aircraft yaw, rolls, and pitches realistically based on user input and aerospace matrix multiplication.
* **Advanced Collision Detection:** Dual raycasting system to detect mountains directly ahead and precisely calculate 3D ground height beneath the vehicle.
* **Adaptive 4:3 Resolution:** Auto-scaling window that detects monitor size to maximize screen real estate while maintaining a retro simulator aspect ratio.
* **Full Gamepad & Steam Deck Support:** Plug-and-play Xbox controller integration with analog precision and real-time dynamic UI text swapping.
* **State Machine:** Clean architectural separation between the Main Menu and the active Game Loop.

## 🛠️ Technology Stack
This project uses **Raylib**, a highly modular and simple-to-use library to enjoy videogames programming in pure C without the overhead of massive game engines.

## 🎮 Controls
| Action | Keyboard | Gamepad (Xbox / Steam Deck) |
| :--- | :--- | :--- |
| **Throttle (Engine Power)** | W / S | LT / RT (Analog Triggers) |
| **Steer (Yaw / Roll)** | A / D | Left Stick (X-Axis) |
| **Pitch (Climb / Dive)** | SPACE / SHIFT | Left Stick (Y-Axis) |
| **Select Plane** | 1 | X Button |
| **Select Helicopter** | 2 | Y Button |
| **Toggle Camera (1st/3rd Person)** | C | A Button |
| **Toggle UI Controls** | H | Menu / Start Button |
| **Quick Restart** | R | B Button |
| **Exit Game** | ESC | View / Back Button |

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
* **Menu** music by [Vrymaa](https://freesound.org/people/Vrymaa/)