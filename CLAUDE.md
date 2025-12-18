# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **warp-gui**, an unofficial Qt-based GUI frontend for Cloudflare's `warp-cli` command-line tool. It provides a system tray application with a custom popup interface for managing WARP VPN connections on Linux with Wayland support.

## Build System

The project uses CMake with Qt6 and requires the following dependencies:
- Qt6 (Core, Gui, Widgets, WaylandClient)
- KF6WindowSystem
- LayerShellQt

### Build Commands

```bash
# Configure and build
cmake -B build
cmake --build build

# Run the application
./build/warp-gui
```

## Architecture

### Core Components

The application follows a Qt-based architecture with these main components:

1. **TrayApp** (`src/tray_app.{h,cpp}`) - Central controller class
   - Manages the system tray icon and context menu
   - Owns and coordinates all other UI components (popup, settings menu)
   - Handles warp-cli command execution via WarpCli wrapper
   - Polls warp-cli status every 5 seconds and updates UI state
   - Processes JSON status responses and plain-text settings output

2. **WarpCli** (`src/warp_cli.{h,cpp}`) - Process wrapper for warp-cli
   - Executes warp-cli commands asynchronously using QProcess
   - Supports both regular output and JSON output modes
   - Uses request IDs to track concurrent commands
   - Emits `finished(requestId, WarpResult)` signal with stdout/stderr/exit code

3. **WarpPopup** (`src/popup_widget.{h,cpp}`) - Main popup UI
   - Custom frameless window that appears when clicking tray icon
   - Contains a ToggleSwitch for connect/disconnect
   - Shows connection status and reason
   - **Dynamic title**: Changes between "WARP", "1.1.1.1", or "Zero Trust" based on mode/enrollment
   - **Dynamic title color**: Orange (#ff6a00) for WARP/DNS-only, Blue (#0A64BC) for Zero Trust
   - **Mode-specific messaging**: "Internet" vs "DNS queries" privacy status
   - Has a bottom bar with branding and settings button
   - **Draggable**: Users can drag popup by title area, position persists via QSettings
   - Hides on focus loss or Escape key

4. **SettingsMenu** (`src/settings_menu.{h,cpp}`) - Settings popup menu
   - Shows mode options (WARP, DNS-only) with checkmarks for current mode
   - **Mode options hidden for Zero Trust users** (organizations control mode through device profiles)
   - Provides Preferences, About, and Exit actions
   - Appears when clicking settings button in WarpPopup

5. **PreferencesDialog** (`src/preferences_dialog.{h,cpp}`) - Comprehensive settings dialog
   - **5 pages with sidebar navigation**: General, Connection, Account, Connectivity, Advanced
   - **General page**: Displays connectivity info (DNS protocol, colocation, public IP, device ID)
   - **Connection page**: Network exclusions, 1.1.1.1 for Families (consumer), Gateway DoH (Zero Trust)
   - **Account page**: Registration, Zero Trust enrollment, license management
   - **Connectivity page**: API/DNS/WARP status checks, colocation center, service interruptions
   - **Advanced page**: Settings, Split Tunnels (excluded hosts/IPs), diagnostics
   - **Zero Trust enrollment**: Accepts ToS in-app, launches browser for OAuth, polls for confirmation
   - Dark theme matching popup aesthetic
   - Real-time status updates from multiple warp-cli commands

6. **WaylandPopupHelper** (`src/wayland_popup_helper.{h,cpp}`) - Wayland integration
   - Uses LayerShellQt to position popups correctly on Wayland
   - Handles layer shell setup for popup windows
   - Provides anchor positioning (bottom/top panel detection)

7. **ToggleSwitch** (`src/toggle_switch.{h,cpp}`) - Custom toggle widget
   - Animated iOS-style toggle switch used in the popup
   - Inherits from QAbstractButton with custom paint implementation

### State Management

- Status polling runs every 5 seconds via QTimer
- `m_busy` flag prevents concurrent operations
- **Zero Trust detection**: Checks `warp-cli registration show` for "Account type: Team" or "Organization:" field
- **Mode detection**: Parses `warp-cli settings` output for mode (warp, doh, dot, etc.)
- **Dynamic branding**: Title and colors update based on mode and enrollment status
- Status updates trigger `applyUiState()` which synchronizes:
  - **Tray icon** with custom badges:
    - Disconnected: Theme icon with no badge
    - Connecting: Theme icon with orange circle + three dots
    - Connected: Theme icon with orange circle + white lock symbol
  - Tray tooltip
  - Menu action enabled states
  - Popup UI state (including title and colors)
  - Settings menu checkmarks and visibility (mode options hidden for Zero Trust)
- **Position persistence**: Popup drag offsets saved to QSettings and restored on next show

### warp-cli Command Mapping

Commands are executed via `WarpCli::run()` or `WarpCli::runJson()`:
- `status` → JSON output parsed for connection state
- `settings` → Text output parsed for current mode
- `connect` / `disconnect` → Connection control
- `mode <mode>` → Change between warp/doh/dot/etc (NOT `set-mode`)
- `registration show` → Check enrollment status and account type
- `registration new` → Register new device
- `registration new <org>` → Enroll in Zero Trust organization (launches browser for OAuth)
- `registration license <key>` → Attach WARP+ license key
- `registration delete` → Delete registration (for re-enrollment)

**Note**: Zero Trust enrollment uses browser-based OAuth. The CLI generates a URL, and the app:
1. Shows ToS acceptance dialog
2. Uses `script` command with piped 'y' to accept ToS in pseudo-TTY
3. Launches browser automatically when URL is detected
4. Keeps process alive with `sleep` for callback (up to 120 seconds)
5. Polls for enrollment confirmation

### Wayland-Specific Considerations

- System tray geometry is unavailable on Wayland
- **Cursor position capture**: When tray icon is clicked, cursor position is captured for popup positioning
- Popup positioning uses panel detection (top/bottom) with fallback to cursor-based positioning
- LayerShellQt is required for proper window layer management
- Native window handles must be created before LayerShell setup
- User can drag popup to preferred position; offset is saved and restored on next show
- **Click-outside detection**: Combined `eventFilter()` + `focusOutEvent()` approach handles both Qt widget clicks and desktop clicks on Wayland

## UI Theme and Branding

### Colors
- **Background**: `#1e1e1e` (dark gray)
- **Sidebar/Bottom bar**: `#2a2a2a` (lighter dark gray)
- **Borders**: `#3a3a3a`
- **WARP orange**: `#ff6a00` (used for WARP and 1.1.1.1 mode titles, and tray icon badges)
- **Zero Trust blue**: `#0A64BC` (used for Zero Trust title when enrolled)
- **Text**: `#ffffff` (white)
- **Muted text**: `#888888`, `#999999`

### Tray Icon Design
- Uses theme icons as base (`network-vpn`, `network-vpn-disconnected`)
- Custom badges painted on top using QPainter:
  - **Connected**: Orange circle with white lock symbol (bottom-right corner)
  - **Connecting**: Orange circle with three white dots (bottom-right corner)
  - **Disconnected**: No badge overlay
- Icons are 64x64 pixels with antialiasing for crisp display

### Dynamic Branding
- **Title text**: Changes based on account status
  - "Zero Trust" when enrolled in Teams organization (highest priority)
  - "1.1.1.1" when in DNS-only mode (doh, dot, dnsoverhttps, dnsoverhuges)
  - "WARP" for all other modes (warp, warp+doh, warp+dot, proxy, tunnel_only)
- **Title color**: Blue for Zero Trust, orange for WARP/1.1.1.1
- **Status messages**:
  - DNS-only: "Your DNS queries are [not] private"
  - WARP/Zero Trust: "Your Internet is [not] private"

## Code Patterns

- All UI components use Qt's signals/slots for communication
- Parent-child relationships manage memory (Qt parent ownership)
- Connect/disconnect operations are asynchronous and update via polling
- Error dialogs only shown for registration/license operations, not connect/disconnect
- Settings are refreshed after mode changes to update checkmarks
- **Signal blocking**: `blockSignals(true/false)` used on toggle switch to prevent feedback loops during programmatic updates
- **Member variable storage**: UI labels stored as members instead of `findChild()` for reliable updates
- **Multi-source data fetching**: Preferences dialog combines data from multiple warp-cli commands (status, registration show, debug network, cloudflare trace)
- **Priority-based branding**: Zero Trust takes priority over mode-based branding (Zero Trust > DNS-only > WARP)
- **Mode-specific text**: Different privacy messages for DNS-only ("DNS queries") vs WARP/Zero Trust ("Internet")
- **Custom icon rendering**: Tray icons use QPainter and QPainterPath to draw badges on theme icons (requires QPainter, QPainterPath, QPixmap includes)
