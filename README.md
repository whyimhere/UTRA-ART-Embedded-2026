# UTRA-ART Embedded 2026

Welcome to the 2026 repository for the University of Toronto Robotics Association (UTRA) Autonomous Rover Team - Embedded Subteam. 

This repository contains the firmware codes (C/C++/Python/etc.) and hardware designs (PCBs, schematics) for our 2026 autonomous rover systems.

## 🚀 Active Projects

Our current development is divided into three major tasks:

1. **Fault Monitor with LED:** Hardware and firmware for detecting power system faults and triggering physical LED indicators for immediate visual diagnostics.
2. **Speed Signal Translator:** Interface system responsible for reading, translating, and securely transmitting speed and encoder signals across the rover's control architecture.
3. **Battery Amount Indicator:** Power monitoring circuit and firmware to track voltage/current and display real-time battery capacity.

## 📂 Repository Structure

Each project is contained in its own directory:

- `/battery-indicator`
- `/fault-monitor`
- `/speed-translator`
- `/wiring-diagram`
- `/docs` - Shared datasheets, pinout diagrams, and system architecture notes.

Inside each task folder, work is separated by domain to prevent conflicts:

- `/docs` - Component datasheets, IC specifications, and reference diagrams specific to the task.
- `/firmware` - Microcontroller code (C/C++) and Computer code (e.g.python)
- `/hardware` - KiCad schematics, PCB board layouts, and Gerber files.

Feel free to create other folders if you need.

## 🛠️ Getting Started & Workflow

To keep our codebase and hardware designs stable, we use a feature-branch workflow. 

**1. Clone the repository:**
`git clone https://github.com/your-org/UTRA-ART-Embedded-2026.git`

**2. Create a new branch for your task:**
Never work directly on the `main` branch. Create a branch using the format `project/feature`:
`git checkout -b fault-monitor/led-driver-code`

**3. Commit your changes:**
Write clear, descriptive commit messages so the rest of the team knows what was updated.
`git commit -m "Added FreeRTOS task for LED blinking"`

**4. Push and open a Pull Request:**
Push your branch to GitHub and open a PR. Request a review from another subteam member before merging into `main`.
`git push origin fault-monitor/led-driver-code`

## ⚔️ Handling Git Conflicts

When multiple people work on the same file, Git may flag a **merge conflict** when you try to pull or merge. Here is how to resolve them safely:

### For Firmware (Code)
1. Pull the latest `main` branch into your feature branch: `git pull origin main`
2. Open the conflicting files in your editor (like VS Code). Look for the conflict markers (`<<<<<<<`, `=======`, `>>>>>>>`).
3. Manually edit the code to keep the correct changes from both branches, delete the conflict markers, and save the file.
4. Run a test compile to make sure nothing is broken.
5. Add the resolved files (`git add .`) and commit the merge (`git commit -m "Resolved merge conflicts"`).

### ⚠️ For Hardware (Kicad files, etc.)
* **If you get a KiCad conflict:** Coordinate directly with the person who made the other changes on Discord. You will almost always need to choose one person's version to overwrite the other (`Accept Incoming` or `Accept Current`), and the person whose changes were overwritten will need to manually re-apply their work in the KiCad GUI.
* **To avoid this:** Communicate with your team before modifying a schematic or PCB layout that someone else is currently working on.

## Important Notes
* **Hardware Designers:** Please ensure all KiCad backups (`*.bak`, `*-bak.kicad_pcb`) and routing auto-saves are excluded via the `.gitignore` before committing.
* **Firmware Developers:** Do not commit compiled binaries or local build folders.
