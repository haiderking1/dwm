# Personal dwm setup

My dwm 6.8 setup with a Quickshell bar matching my Jellybeans Waybar theme.

- Super-based shortcuts and ten workspaces.
- Fullscreen toggle, flat mouse acceleration, and a 200 ms keyboard repeat delay at about 35 repeats per second.
- Native autostart and background rebuild/reload with window and workspace restoration.
- Quickshell workspace controls, clock, RAM, PipeWire volume, and system tray.
- Native X11 dock support and a bar bridge. The bridge reads the loaded wallpaper for a matching bar background without a compositor.

## Configuration

This is a personal setup, not a portable distribution. Paths currently point to /home/soka.

| Path | Purpose |
| --- | --- |
| config.h | Active dwm configuration |
| config/keys.h | Keyboard shortcuts |
| config/autostart.h | Login applications |
| config/reload.h | Build and reload paths |
| input/settings.c | Mouse and keyboard settings |
| bar/quickshell/ | Quickshell configuration snapshot |
| bar/bridge/ | Native X11 state and wallpaper bridge |
| session/ | SDDM session and installer |

The live Quickshell configuration is ~/.config/quickshell/dwm. This repository contains a source copy under bar/quickshell; copy changes between these directories when updating or restoring the setup. Screenshots and generated artifacts are excluded.

## Build and restore

Requires a C compiler, make, X11/Xft/Xinerama/fontconfig development files, libXi, and Quickshell. The configured apps and JetBrainsMono Nerd Font must also be installed for their shortcuts and icons.

Build and install both dwm and its bar bridge:

~~~sh
make install PREFIX="$HOME/.local"
~~~

To restore the Quickshell configuration, back up any existing configuration first, then copy the contents of bar/quickshell into ~/.config/quickshell/dwm. session/install-sddm.sh registers the SDDM entry as root; its launcher expects ~/.local/bin/dwm-session, copied from session/dwm-session with executable permissions.

Super+Shift+R rebuilds and reloads dwm without logging out. Quickshell watches its live QML files for changes. Autostart runs at login, not on dwm reload.

## Main shortcuts

| Shortcut | Action |
| --- | --- |
| Super+Q / E / X | Alacritty / Nautilus / Zen |
| Super+D | Rofi |
| Super+C | Close window |
| Super+F | Toggle fullscreen |
| Super+1–9 / 0 | Select workspace 1–10 |
| Super+Shift+number | Move window to workspace |
| Super+Shift+R | Rebuild and reload |
| Super+Shift+Q | Log out |

## Source

Based on suckless dwm, originally cloned at 44dbc68. This personal repository starts with fresh Git history. The upstream license and copyright notices remain in LICENSE. README contains the original dwm documentation.
