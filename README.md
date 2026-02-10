
# <img src="src/assets/cangaroo.png" width="48" height="48"> CANgaroo
**Open-source CAN bus analyzer for Linux 🐧 / Windows 🪟**

** 🔩 Supported Interfaces & Hardware:**

*   **PEAK-System (PCAN)**: 
    *   PCAN-USB, PCAN-USB Pro, PCAN-PCIe, etc. (Native driver: `peak_usb`).
*   **Native USB-CAN Adapters**: 
    *   [CANable](https://canable.io/) (with Candlelight firmware)
    *   Kvaser USB/CAN Leaf
    *   Candlelight compatible devices (e.g., MKS CANable, cantact)
*   **USB SLCAN Adapters**:
    *   CANable (with set-default SLCAN firmware)
    *   Arduino-based CAN shields (running SLCAN sketches)
*   **Industrial / Embedded CAN**:
    *   PCIe/mPCIe CAN cards
    *   Embedded CAN controllers on SoCs (e.g., Raspberry Pi with MCP2515)
*   **Remote / Network CAN**: 
    *   [CANblaster](https://github.com/OpenAutoDiagLabs/CANblaster) (UDP)
    *   tcpcan / candlelight-over-ethernet
*   **GrIP Driver

## ⚙️ Features

*   **Real-time CAN/CAN-FD Decoding**: Support for standard and high-speed flexible data-rate frames.
*   **Wide Hardware Compatibility**: Works with **SocketCAN** (Linux), **CANable** (SLCAN), **Candlelight**, and **CANblaster** (UDP).
*   **DBC Database Support**: Load multiple `.dbc` files to instantly decode frames into human-readable signals.
*   **Powerful Data Visualization**: Integrated Graphing tools supporting Time-series, Scatter charts, Text-based monitoring, and interactive Gauge views with zoom and live tooltips.
*   **Advanced Filtering & Logging**: Isolate critical data with live filters and export captures for offline analysis.
*   **Modern Workspace**: A clean, dockable userinterface optimized for multi-monitor setups.

<br>![Cangaroo Trace View](src/docs/view.png)<br>

## 📜 Credits
Written by Hubert Denkmair <hubert@denkmair.de>

Further development by:
* Ethan Zonca <e@ethanzonca.com>
* WeAct Studio
* Schildkroet (https://github.com/Schildkroet/CANgaroo)
* Wikilift (https://github.com/wikilift/CANgaroo)
* Jayachandran Dharuman (https://github.com/OpenAutoDiagLabs/cangaroo)

## Languages
* 🇩🇪 German
* 🇺🇸 English
* 🇪🇸 Spain
* 🇨🇳 Chinese

## 🛠️ Building
### 🐧 Linux
| Distribution | Command |
| :--- | :--- |
| **Ubuntu / Debian** | `sudo apt install qt6-base-dev qt6-base-private-dev qt6-tools-dev libqt6charts6-dev libqt6serialport6-dev build-essential libnl-3-dev libnl-route-3-dev libgl1-mesa-dev` |
| **Fedora** | `sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel qt6-qtserialport-devel libnl3-devel` |
| **Arch Linux** | `sudo pacman -S qt6-base qt6-charts qt6-serialport libnl` |

#### Build with:
  * qmake6
  * make
  * make install

### 🪟 Windows
* Qt Creator (Community Version is okay) brings everything you need
* except for the PCAN libraries. 
  * Get them from http://www.peak-system.com/fileadmin/media/files/pcan-basic.zip
  * Extract to .zip to src/driver/PeakCanDriver/pcan-basic-api
  * Make sure PCANBasic.dll (the one from pcan-basic-api/Win32 on a "normal" 32bit Windows build)
    is found when running cangaroo, e.g. by putting it in the same folder as the .exe file.
* if you don't want Peak support, you can just disable the driver:
  remove the line "win32:include($$PWD/driver/PeakCanDriver/PeakCanDriver.pri)"
  from src/src.pro
* if you want to deploy the cangaroo app, make sure to also include the needed Qt Libraries.
  for a normal release build, these are: Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll Qt5Xml.dll

### ARXML to DBC Conversion
Cangaroo natively supports DBC. If you have ARXML files, you can convert them using `canconvert`:
```bash
# Install canconvert
pip install canconvert

# Convert ARXML to DBC
canconvert TCU.arxml TCU.dbc
```

## 📥 Download

Download the latest release from the [Releases](https://github.com/Schildkroet/CANgaroo).
