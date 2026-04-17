# GhostWire Desktop

![Release](https://img.shields.io/github/v/release/momentics/GhostWire?label=version&color=blue) ![Platforms](https://img.shields.io/badge/platforms-Windows%207%2C8%2C8.1%2C10%2C11%20%7C%20Ubuntu%2022.04%20%7C%20macOS-lightgrey) ![License](https://img.shields.io/badge/license-MIT-green)

| | |
|:---:|---|
| <img src="resources/icons/ghostwire.png" alt="GhostWire Logo" width="512"> | A graphical front-end for the native **GhostWire** library with DPI (OSI L4-L7) protection for **Telegram Desktop**. The app runs in the system tray (user mode) and does not use external services and servers. |

---
1. **Uninterrupted access** GhostWire ensures a reliable connection to the messaging platform, even under severe network restrictions and censorship attempts by internet service providers.
2. **Total connection invisibility** The software securely conceals the true nature of the transmitted information. To monitoring equipment, your correspondence appears as standard web browsing on secure pages using popular applications like Chrome or Safari.
3. **Protection against behavioral recognition** The intentional distortion of data volumes and the introduction of randomized delays prevent surveillance systems from identifying the application's activity through characteristic network rhythms.
4. **Resilience to disconnections** Should the current network node suddenly become blocked or fail, the technology instantly and seamlessly transitions to backup addresses, keeping the conversation uninterrupted.
5. **High-speed file transfer** Thanks to dedicated pathways for large attachments, users can send and receive photographs, videos, and documents without tedious waiting periods.
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
Windows
<p align="center">
  <img src="resources/icons/MenuBar.png" alt="Context menu" width="350">
</p>
Linux
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
