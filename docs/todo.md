# To-Do List (`todo.md`)

This document is intended to track the development progress of the project, including ongoing tasks, known issues, and future plans.

---

## Recently Completed Tasks

- [x] All main screen buttons and page transition system completed (`ScreenManager`).
- [x] Temperature, conductivity, and pressure data successfully read via RS-485.
- [x] GUI successfully launched in Windows simulation environment.
- [x] Wi-Fi scanning, password entry, and connection functions implemented.
- [x] Settings are now read from and written to `/etc/settings.txt`.
- [x] Background thread structure added to `sensor_recorder.cpp`.
- [x] Charts in the Average Data screen are now generated dynamically.

---

## In Progress

- [ ] SSH toggle (enable/disable) feature is under development.
- [ ] Automatic Wi-Fi reconnection feature is being implemented.
- [ ] Night/Day mode is being developed.

---

## Known Issues

- [ ] When Wi-Fi is manually disconnected, quickly exiting and re-entering the Wi-Fi Settings screen may cause the Wi-Fi service to hang.

---

## Planned Features

- [ ] Server setup for data sharing and sending data from device to server.
- [ ] Brightness adjustment applied directly from GUI to the LCD.
- [ ] Memory saving mode.
- [ ] Power saving mode.
- [ ] Zoom/pan functionality for charts.
- [ ] Screen timeout feature (the screen will dim while the app continues running).

---

This document is continuously updated. New bugs, ideas, and tasks will be added during development.
