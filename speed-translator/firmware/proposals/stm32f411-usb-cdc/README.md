# STM32F411 USB CDC Speed Signal Translator

## Firmware build

Install these dependencies on your machine:

- **STM32CubeF4 1.28.3** provides CMSIS, HAL, and the STM32 USB Device Library. In [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html), select **Help → Manage embedded software packages** and install the F4 firmware package through version 1.28.3. Use the package folder containing `package.xml`, typically named `STM32Cube_FW_F4_V1.28.3`.
- **Arm GNU Toolchain 15.3.Rel2** is the tested compiler release. Install the `arm-none-eabi` distribution with newlib-nano and put its `bin` directory on `PATH`.
- **CMake 3.22 or newer** and **Ninja** run the build. The build was verified with CMake 4.4.4 and Ninja 1.13.2.

Run the commands from this directory and substitute the path to your local STM32CubeF4 package.

### Windows (PowerShell)

```powershell
cmake --preset Debug "-DSTM32CUBEF4_ROOT=C:/Users/your-name/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3"
cmake --build --preset Debug
```

### Linux and macOS

```sh
cmake --preset Debug "-DSTM32CUBEF4_ROOT=$HOME/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3"
cmake --build --preset Debug
```

The build produces `build/Debug/Speed_Signal_Translator.elf`.

## USB serial controller

Connect the board running the firmware by USB. Install the pinned `pyserial` dependency in a local Python virtual environment. List available ports and replace the example port with the STM32 port shown on your machine.

### Windows (PowerShell)

```powershell
py -3 -m venv .venv
./.venv/Scripts/python.exe -m pip install -r requirements.txt
./.venv/Scripts/python.exe -m serial.tools.list_ports -v
./.venv/Scripts/python.exe mac_pwm.py --port COM3
```

### Linux

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python -m serial.tools.list_ports -v
.venv/bin/python mac_pwm.py --port /dev/ttyACM0
```

If opening the port reports a permission error, check your distribution's serial-device access rules.

### macOS

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python mac_pwm.py
```

On macOS the script can detect a USB modem port automatically. To choose a specific port, list ports with `.venv/bin/python -m serial.tools.list_ports -v` and pass `--port`.
