
# <img src="src/assets/cangaroo.png" width="48" height="48"> Cangaroo
**Open-source CAN bus analyzer for Linux 🐧 / Windows 🪟**

**Supported interfaces:**

* [CANable](http://canable.io) SLCAN interfaces on Windows and Linux
* [CANable 2](http://canable.io) SLCAN interfaces on Windows and Linux with FD support
* Candlelight interfaces on Windows
* Socketcan interfaces on Linux
* [CANblaster](https://github.com/normaldotcom/canblaster) socketCAN over UDP server with auto-discovery
* GrIP Driver

![demo1](https://user-images.githubusercontent.com/2422337/179544017-0deb66ab-e81d-4e6c-9d99-4059a14921c0.gif)


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

## TODO

### backend
* support non-message frames in traces (e.g. markers)
* implement plugin API
* embed python for scripting

### can drivers
* allow socketcan interface config through suid binary
* socketcan: use hardware timestamps (SIOCSHWTSTAMP) if possible
* cannelloni support
* windows vector driver

### import / export
* export to other file formats (e.g. Vector ASC, BLF, MDF)    (no changes)
* import CAN-Traces    ✔ done in v0.3.1: full .ctrace import/export with colors, comments and aliases

### general ui
* give some style to dock windows
* load/save docks from/to config
* ✔ done in v0.3.1: runtime language switching (new menu, .qm loader)

### log window
* filter log messages by level

### can status window
* display #warnings, #passive, #busoff, #restarts of socketcan devices

### trace window
* assign colors to can interfaces / messages    ✔ done in v0.3.1: posible asignation colors by id 
* limit displayed number of messages
* show error frames and other non-message frames
* sort signals by startbit, name or position in candb
* ✔ done in v0.3.1: per-message comments + persistence
* ✔ done in v0.3.1: per-ID aliases + persistence

### CanDB based generator
* generate can messages from candbs
* set signals according to value tables etc.
* provide generator functions for signal values
* allow scripting of signal values

### replay window
* replay can traces
* map interfaces in traces to available networks

### graph window
* test QCustomPlot
* allow for graphing of interface stats, message stats and signals

### packaging / deployment
* provide clean debian package   ✔ done in v0.3.1: go to releases
* Flatpak
* provide static linked binary
* add windows installer
