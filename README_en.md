# GhostWire Desktop

![Release](https://img.shields.io/github/v/release/momentics/GhostWire?label=version&color=blue) ![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey) ![License](https://img.shields.io/badge/license-MIT-green)

| | |
|:---:|---|
| <img src="resources/icons/ghostwire.png" alt="GhostWire Logo" width="512"> | A graphical front-end for the native **GhostWire** library with DPI (OSI L4-L7) detection protection for **Telegram Desktop**. The app lives in the system tray. |

---

## What it looks like

When launched, an animated icon appears in the tray. Right-clicking it opens a compact menu.

---

## Screenshots

### Animated tray icon

<p align="center">
  <img src="resources/icons/TaskBar.png" alt="TaskBar icon" width="400">
</p>

### Context menu

<p align="center">
  <img src="resources/icons/MenuBar.png" alt="Context menu" width="350">
</p>
<p align="center">
  <img src="resources/icons/MenuBarUbuntu.png" alt="Context menu" width="350">
</p>

---

## Controls

| Action | Result |
|---|---|
| **Right-click** the icon | Open the context menu |
| **Start** | Launch the proxy and begin accepting connections |
| **Stop** | Stop the proxy and clear the interface |
| **Connect Telegram** | Open the proxy configuration dialog in Telegram Desktop |
| **Check for updates** | Check GitHub for a new version |
| **Exit** | Quit the application |

---

## Launch behavior

- **First launch** — GhostWire starts in the "Stopped" state
- **Subsequent launches** — the app restores the previous state. If the proxy was running when the app was last closed, it starts automatically. If it was stopped, it stays stopped

---

## Using with Telegram

After pressing **Start**, you can select **Connect Telegram**:

<p align="center">
  <img src="resources/icons/TGBar.png" alt="Telegram Proxy Setup" width="350">
</p>

Or configure the proxy manually:

1. Telegram Desktop → Settings → Advanced → Connection type → SOCKS5
2. Host: `127.0.0.1`, Port: `1080`
3. Save

---

## Update checking

On startup, the app automatically checks for new versions on GitHub (no more than once every 24 hours). When an update is found, a tray notification appears with a link to the release page.

Manual check: the **Check for updates** button in the context menu.

---
