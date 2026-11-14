# STM32F411 USB CDC Speed Signal Translator

## Firmware build

Install these dependencies on your machine:

- **STM32CubeF4 1.28.3** provides CMSIS, HAL, and the STM32 USB Device Library. In [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html), select **Help → Manage embedded software packages** and install the F4 firmware package through version 1.28.3. Use the package folder containing `package.xml`, typically named `STM32Cube_FW_F4_V1.28.3`.
- **Arm GNU Toolchain 15.3.Rel2** is the tested compiler release. Install the `arm-none-eabi` distribution with newlib-nano and put its `bin` directory on `PATH`.
- **CMake 3.22 or newer** and **Ninja** run the build. The build was verified with CMake 4.4.4 and Ninja 1.13.2.

From this directory, replace the example path with your own STM32CubeF4 package path. On Windows, use a path such as `C:/Users/your-name/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3`.

```sh
cmake --preset Debug "-DSTM32CUBEF4_ROOT=/path/to/STM32Cube_FW_F4_V1.28.3"
cmake --build --preset Debug
```

The build produces `build/Debug/Speed_Signal_Translator.elf`.

## macOS serial controller

`requirements.txt` pins the Python serial library. Create a local virtual environment and install it there:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python mac_pwm.py
```

The controller looks for a macOS USB modem port exposed by the board.
