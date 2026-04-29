# GhostWire Desktop

![Release](https://img.shields.io/github/v/release/momentics/GhostWire?label=version&color=blue) ![Platforms](https://img.shields.io/badge/platforms-Windows%207%2C8%2C8.1%2C10%2C11%20%7C%20Ubuntu%2022.04%20%7C%20macOS-lightgrey) ![License](https://img.shields.io/badge/license-MIT-green)

| | |
|:---:|---|
| <img src="resources/icons/ghostwire.png" alt="GhostWire Logo" width="512"> | **GhostWire Desktop** helps **Telegram Desktop** work through a protected local proxy. It starts a SOCKS5 proxy on your computer, connects Telegram to it, and forwards Telegram traffic through **HTTPS Tunneling** with DPI protection across **L4-L7**. No system-wide VPN, external proxy service, or third-party account is required. |

---

## Quick Start

1. Launch **GhostWire Desktop**.
2. Click **Start** in the app menu.
3. Open **Telegram Desktop**.
4. Click **Connect Telegram**.
5. Confirm the SOCKS5 proxy configuration in Telegram.

If automatic setup does not open, configure Telegram manually:

1. Telegram Desktop -> Settings -> Advanced -> Connection type -> SOCKS5
2. Server: `127.0.0.1`
3. Port: `1080`
4. Save

---

## How It Works

GhostWire acts as a small local gateway for applications that explicitly connect to it.

1. A local SOCKS5 proxy starts on your computer at `127.0.0.1:1080`.
2. Telegram Desktop sends its connections to that local proxy.
3. GhostWire detects the target Telegram DC and selects an appropriate route.
4. Traffic is then forwarded over `wss` on port 443, so it looks like a regular encrypted HTTPS/WebSocket connection.
5. If a route becomes unavailable, the library can switch to another endpoint for the same destination.

---

## Why It Helps

- **No system-wide VPN** - GhostWire does not change routing for your whole operating system.
- **Telegram Desktop only** - other traffic on your computer does not start using GhostWire automatically.
- **Local connection point** - Telegram connects to `127.0.0.1:1080`, which is GhostWire running on your own machine.
- **HTTPS Tunneling** - outgoing connections use port `443`, the standard port for HTTPS traffic.
- **Endpoint rotation** - if one route has problems, GhostWire can try another available endpoint.
- **Dedicated media routes** - the configuration accounts for Telegram media/file traffic.
- **One-click stop** - **Stop** shuts down the local proxy and clears the current statistics.

---

## Protection Features

GhostWire does not promise a universal bypass for every kind of blocking. Its purpose is to make Telegram traffic harder to classify with DPI and more resilient to route-specific failures.

The current configuration uses:

- **L4-L4 DPI protection**
- **HTTPS Tunneling**
- **SNI for Telegram Web**
- **DC-aware routing** - GhostWire maps Telegram IP ranges to the corresponding datacenters
- **Endpoint rotation** - datacenters have primary and fallback endpoints
- **Media routing** - some destinations have dedicated media endpoints and media hostnames
- **User Mode** - the app runs without kernel drivers and without intercepting all system traffic

---

## Important Notes

- GhostWire Desktop is not a system-wide VPN.
- It helps Telegram Desktop only after Telegram is connected to the local SOCKS5 proxy.
- **Connect Telegram** requires Telegram Desktop to be installed and running.
- GhostWire can check for updates, but it does not install them automatically.

---

## What It Looks Like

When GhostWire starts, its icon appears in the system tray. Left-click or right-click the icon to open a compact menu with status, traffic graph, and controls.

---

## Screenshots

### Tray Icon

<p align="center">
  <img src="resources/icons/TaskBar.png" alt="TaskBar icon" height="60">
</p>

### Context Menu

Windows

<p align="center">
  <img src="resources/icons/MenuBar.png" alt="Context menu" width="350">
</p>

Linux

<p align="center">
  <img src="resources/icons/MenuBarUbuntu.png" alt="Context menu" width="350">
</p>

macOS

<p align="center">
  <img src="resources/icons/MenuBarMacOS.png" alt="Context menu" width="350">
</p>

### Telegram Setup

<p align="center">
  <img src="resources/icons/TGBar.png" alt="Telegram Proxy Setup" width="350">
</p>

---

## Menu Actions

| Button | What it does |
|---|---|
| **Start** | Starts the local SOCKS5 proxy at `127.0.0.1:1080` |
| **Stop** | Stops the proxy and clears the current statistics |
| **Connect Telegram** | Opens Telegram with a ready-to-use SOCKS5 setup |
| **Check for updates** | Checks GitHub for a new version |
| **Exit** | Quits the application |

---

## Menu Statistics

| Field | Meaning |
|---|---|
| **Uptime** | How long the proxy has been running since the last start |
| **Connections** | Active WebSocket connections, with the peak value in parentheses |
| **Rotations** | Successful endpoint rotations, with the total number of attempts in parentheses |
| **Peak** | Maximum receive and send speed during the current run |
| **Total** | Total amount of received and sent traffic |

The tray icon also shows the current state:

- <img src="resources/icons/tray_idle.png" alt="IDLE TaskBar icon" width="16"> - the proxy is stopped.
- <img src="resources/icons/tray_active.png" alt="ACTIVE TaskBar icon" width="16"> - the proxy is running.
- <img src="resources/icons/tray_active_frames/frame_03.png" alt="ACTIVE connection frame" width="16"> - active connections are present.
- <img src="resources/icons/tray_degraded.png" alt="DEGRADED TaskBar icon" width="16"> - the proxy is running, but one route or resource is unavailable.

---

## Startup Behavior

- **First launch** - GhostWire starts in the **Stopped** state.
- **Later launches** - the app restores its previous state. If the proxy was running when GhostWire was closed, it starts again automatically. If it was stopped, it stays stopped.

---

## Update Checking

GhostWire Desktop checks for a new version when it starts and then periodically while it is running. If an update is available, the app notifies you and offers to open the GitHub release page.

Updates are not installed automatically. You choose and install the correct package for your system yourself. You can also run a manual check from **Check for updates** in the app menu.

---

## Support the Author

To the Volga "famine-stricken" gubernia :pray: **OZON Bank: 2204 2402 8673 4225**

---
