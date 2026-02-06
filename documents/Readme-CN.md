# CANgaroo

### [English](README.md) | 中文

这是一个开源的 CAN 总线分析仪，支持标准帧和 CAN FD 帧的收发，并具备对接收帧进行 DBC 解码 的功能。

### **支持的接口：**

* **[CANable](http://canable.io)**：Windows 和 Linux 上的 SLCAN 接口
* **[CANable 2](http://canable.io)**：Windows 和 Linux 上支持 FD 的 SLCAN 接口
* **Candlelight**：Windows 接口
* **Socketcan**：Linux 接口
* **[CANblaster](https://github.com/normaldotcom/canblaster)**：支持自动发现的基于 UDP 服务器的 socketCAN
* **GrIP 驱动**

---

![demo1](https://user-images.githubusercontent.com/2422337/179544017-0deb66ab-e81d-4e6c-9d99-4059a14921c0.gif)

**作者：** Hubert Denkmair [hubert@denkmair.de](mailto:hubert@denkmair.de)

**后续开发贡献者：**

* Ethan Zonca [e@ethanzonca.com](mailto:e@ethanzonca.com)
* WeAct Studio
* Schildkroet ([https://github.com/Schildkroet/CANgaroo](https://github.com/Schildkroet/CANgaroo))
* Wikilift ([https://github.com/wikilift/CANgaroo](https://github.com/wikilift/CANgaroo))
* 北京星途千舟科技有限公司

---

## 在 Linux 上构建

* **在原生 Ubuntu 24.04 (Qt6) 中安装所需软件包：**
* `sudo apt install -y build-essential git cmake qt6-base-dev qt6-base-private-dev qt6-tools-dev libqt6serialport6-dev libqt6charts6-dev libnl-3-dev libnl-route-3-dev libgl1-mesa-dev`


* **使用以下命令构建：**
* `qmake6`
* `make`
* `make install`

---

## 在 Windows 上构建

* **环境准备：** 使用 **Qt Creator**（社区版即可），它集成了开发所需的大部分工具。
* **PCAN 库配置（必需）：**
1. 从 [PEAK-System 官网](http://www.peak-system.com/fileadmin/media/files/pcan-basic.zip) 下载库文件。
2. 将其解压到项目目录：`src/driver/PeakCanDriver/pcan-basic-api`。
3. 确保运行 Cangaroo 时能够找到 `PCANBasic.dll`（对于标准的 32 位 Windows 构建，请使用 `pcan-basic-api/Win32` 路径下的文件）。建议将其放置在与生成的 `.exe` 文件相同的文件夹中。


* **禁用 Peak 支持（可选）：**
如果你不需要 Peak 驱动支持，可以从 `src/src.pro` 文件中删除以下行：
`win32:include($$PWD/driver/PeakCanDriver/PeakCanDriver.pri)`
* **应用部署：**
发布 Cangaroo 应用时，请确保包含必要的 Qt 库。对于标准的 Release 构建，通常需要：
`Qt5Core.dll`, `Qt5Gui.dll`, `Qt5Widgets.dll`, `Qt5Xml.dll`
