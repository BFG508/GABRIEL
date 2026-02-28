# G.A.B.R.I.E.L. ✈️
**G**eneral **A**viation & **B**asic **R**aylib **I**nteractive **E**ngine **L**ayer

A basic but dynamic 3D flight simulator written purely in C. This project was created to practice low-level C programming concepts, manual memory management, 3D coordinate systems, and game loop architecture.

## 🚀 Features
* **Continuous Throttle Physics:** Smooth acceleration and momentum decay.
* **Dynamic Visual Tilt:** The aircraft rolls and pitches realistically based on user input.
* **State Machine:** Clean separation between the Main Menu and the active Game Loop.
* **Mid-Flight Switching:** Change between aircraft models instantly without losing momentum.

## 🛠️ Technology Stack
This project uses **Raylib**, a highly modular and simple-to-use library to enjoy videogames programming without the overhead of massive game engines.

## 🎮 Controls
* **[ 1 ] / [ 2 ]:** Select Aircraft (SR-71 Blackbird / AH-64 Apache)
* **W / S:** Throttle Up / Throttle Down (Engine Power)
* **A / D:** Roll Left / Roll Right (Sideways movement)
* **SPACE:** Pitch Up (Gain altitude)
* **SHIFT:** Pitch Down (Lose altitude)

## ⚙️ Prerequisites & Compilation
This project uses a universal `Makefile`. You must have **Raylib** installed on your system to compile it.

### Windows (MinGW)
1. Download the Raylib Windows Installer from [GitHub](https://github.com/raysan5/raylib/releases).
2. Open your terminal in the project folder and run:
   ```bash
   mingw32-make OBJS="*.c"

### Linux
1. Install Raylib and development dependencies (X11, OpenGL, etc.).
2. Run:
   ```bash
   make OBJS="*.c"

### macOS
1. Install Raylib via Homebrew: `brew install raylib`.
2. Run:
   ```bash
   make OBJS="*.c"

## 🎨 Credits & Assets
This project uses the following open-source 3D models from Sketchfab:
* **SR-71 Blackbird** model by [KOG_THORNS](https://sketchfab.com/KOG_THORNS)
* **AH-64 Apache** model by [Muhamad Mirza Arrafi](https://sketchfab.com/mirza.arrafi)