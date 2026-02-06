
# CANgaroo

### English | [中文](documents/Readme-CN.md)

An open source can bus analyzer with support for transmit/receive of standard and FD frames and DBC decoding of incoming frames

**Supported interfaces:**

* [CANable](http://canable.io) SLCAN interfaces on Windows and Linux
* [CANable 2](http://canable.io) SLCAN interfaces on Windows and Linux with FD support
* Candlelight interfaces on Windows
* Socketcan interfaces on Linux
* [CANblaster](https://github.com/normaldotcom/canblaster) socketCAN over UDP server with auto-discovery
* GrIP Driver

![demo1](https://user-images.githubusercontent.com/2422337/179544017-0deb66ab-e81d-4e6c-9d99-4059a14921c0.gif)


Written by Hubert Denkmair <hubert@denkmair.de>

Further development by:
* Ethan Zonca <e@ethanzonca.com>
* WeAct Studio
* Schildkroet (https://github.com/Schildkroet/CANgaroo)
* Wikilift (https://github.com/wikilift/CANgaroo)


## Building on Linux
* Install all required packages in a vanilla Ubuntu 24.04 (Qt6):
  * sudo apt install -y build-essential git cmake qt6-base-dev qt6-base-private-dev qt6-tools-dev libqt6serialport6-dev libqt6charts6-dev libnl-3-dev libnl-route-3-dev libgl1-mesa-dev
* build with:
  * qmake6
  * make
  * make install
* For Debian/Ubuntu users, can go to Releases and download .deb*

## Building on Windows
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
  for a normal release build, these are: Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Xml.dll


## [Changelog](documents/Changelog.md)
## [To-Do](documents/Todo.md)