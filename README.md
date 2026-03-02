# G.A.B.R.I.E.L. ✈️
**G**eneral **A**viation & **B**asic **R**aylib **I**nteractive **E**ngine **L**ayer

A basic but dynamic 3D flight simulator written purely in C. This project was created to practice low-level C programming concepts, manual memory management, 3D coordinate systems, matrix transformations, and game loop architecture.

## 🚀 Features
* **Time Trial Racing System:** A fully functional 3D checkpoint circuit with strict cylindrical collision detection, an active stopwatch, and a dynamic vectorial navigation arrow.
* **Continuous Throttle Physics:** Smooth acceleration, lift generation, and momentum decay (friction) custom-built for both fixed-wing aircraft and helicopters.
* **Dynamic Visual Tilt & Steering:** The aircraft yaw, rolls, and pitches realistically based on user input and aerospace matrix multiplication.
* **Advanced Collision Detection:** Dual raycasting system to detect mountains directly ahead and precisely calculate 3D ground height beneath the vehicle.
* **State Machine:** Clean architectural separation between the Main Menu and the active Game Loop.

## 🛠️ Technology Stack
This project uses **Raylib**, a highly modular and simple-to-use library to enjoy videogames programming in pure C without the overhead of massive game engines.

## 🎮 Controls
* **W / S:** Throttle Up / Down (Engine Power)
* **A / D:** Yaw Left / Right (Steers the nose, induces roll)
* **SPACE / SHIFT:** Pitch Up / Down (Climb or Dive)
* **1 / 2 / C / H:** Easter Eggs

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
* **Skybox** model by [djvivid](https://sketchfab.com/djvivid)
* **Ring** model by [FishyBusiness](https://sketchfab.com/FishyBusiness)

Also this project uses the following open-source sounds from Freesound:
* **Airplane** sound by [daliacoss](https://freesound.org/people/daliacoss/)
* **Helicopter** sound by [UnderlinedDesigns](https://freesound.org/people/UnderlinedDesigns/)
* **Menu** music by [Vrymaa](https://freesound.org/people/Vrymaa/)