## Changelog

### v0.3.1 (2025-02)
This release introduces major improvements to the trace window, user interface, 
and import/export capabilities. 

#### New: Full trace import/export (.ctrace)
* Added new JSON-based file format `.ctrace`.
* Export now includes:
  - timestamps  
  - raw ID, ID string, DLC and payload  
  - RX/TX direction  
  - per-message comments  
  - per-ID colors  
  - per-ID aliases
* Import fully reconstructs the trace model, restoring all colors, aliases and comments.

#### New: Per-message comments
* Added editable comments in Linear Trace View (separate for each message).
* Comments are persisted in `.ctrace`.

#### New: ID aliases
* Aliases may now be assigned to CAN IDs.
* Quoted in the UI and exported/imported in `.ctrace`.

#### New: ID color assignment
* Color assignation per ID.
* Custom color override via color picker.
* Colors persisted in `.ctrace`.

#### New: Add language options menu and Spanish translate
* Added `i18n_es_ES`
* Full UI retranslation on `LanguageChange`.
* Menu entry for language selection.


### v0.3.0
* Migrate to Qt6
* Added GrIP driver

### v0.2.4.1
* General bugfixes
* Add WeAct Studio Support
* Initial Translation support

### v0.2.4
* Add initial support for CANFD
* Add support for SLCAN interfaces on Windows and Linux (CANable, CANable 2.0) including FD support
* Add support for [CANblaster](https://github.com/normaldotcom/canblaster) socketCAN over UDP server with auto-discovery
* Add live filtering of CAN messages in trace view

### v0.2.1
* make logging easier
* refactorings
* scroll trace view per pixel, not per item (always show last message when autoscroll is on)

### v0.2.0 released 2016-01-24
* docking windows system instead of MDI interface
* windows build
* windows PCAN-basic driver
* handle muxed signals in backend and trace window
* do not try to extract signals from messages when DLC too short
* can status window
* bugfixes in setup dialog
* show timestamps, log level etc. in log window

### v0.1.3 released 2016-01-16
* new can interface configuration GUI (missing a suid binary to actually set the config)
* use libnl-route-3 for socketcan device config read
* query socketcan interfaces for supported config options
* new logging subsystem, do not use QDebug any more
* some performance improvements when receiving lots of messages 
* bugfix with time-delta view: timestamps not shown when no previous message avail

### v0.1.2 released 2016-01-12
* fix device re-scan ("could not bind" console message)
* fix some dbc parsing issues (signed signals, ...)
* implement big endian signals

### v0.1.1 released 2016-01-11
* change source structure to better fit debian packaging
* add debian packaging info

### v0.1 released 2016-01-10
initial release \o/