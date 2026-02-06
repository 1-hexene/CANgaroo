
# TODO

## backend
* support non-message frames in traces (e.g. markers)
* implement plugin API
* embed python for scripting

## can drivers
* allow socketcan interface config through suid binary
* socketcan: use hardware timestamps (SIOCSHWTSTAMP) if possible
* cannelloni support
* windows vector driver

## import / export
* export to other file formats (e.g. Vector ASC, BLF, MDF)    (no changes)
* import CAN-Traces    ✔ done in v0.3.1: full .ctrace import/export with colors, comments and aliases

## general ui
* give some style to dock windows
* load/save docks from/to config
* ✔ done in v0.3.1: runtime language switching (new menu, .qm loader)

## log window
* filter log messages by level

## can status window
* display #warnings, #passive, #busoff, #restarts of socketcan devices

## trace window
* assign colors to can interfaces / messages    ✔ done in v0.3.1: posible asignation colors by id 
* limit displayed number of messages
* show error frames and other non-message frames
* sort signals by startbit, name or position in candb
* ✔ done in v0.3.1: per-message comments + persistence
* ✔ done in v0.3.1: per-ID aliases + persistence

## CanDB based generator
* generate can messages from candbs
* set signals according to value tables etc.
* provide generator functions for signal values
* allow scripting of signal values

## replay window
* replay can traces
* map interfaces in traces to available networks

## graph window
* test QCustomPlot
* allow for graphing of interface stats, message stats and signals

## packaging / deployment
* provide clean debian package   ✔ done in v0.3.1: go to releases
* Flatpak
* provide static linked binary
* add windows installer
