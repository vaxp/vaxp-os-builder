#!/bin/bash
set +e

dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP=aether
/usr/lib/xdg-desktop-portal &

export DISPLAY=:0
export WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-wayland-1}
export XDG_CURRENT_DESKTOP=aether
export XDG_SESSION_TYPE=aether

wl-clip-persist --clipboard regular --reconnect-tries 0 &
wl-paste --type text --watch cliphist store &

gsettings set org.gnome.desktop.interface gtk-theme 'WhiteSur-Dark'
gsettings set org.gnome.desktop.interface cursor-theme 'Sunity-cursors'
gsettings set org.gnome.desktop.interface icon-theme 'Nordzy--dark_panel'


