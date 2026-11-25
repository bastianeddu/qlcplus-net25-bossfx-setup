#!/usr/bin/env bash
# Legt das dedizierte Netzwerkprofil "ArtNet-Net25" an (Laptop <-> Showtec Net-2/5 direkt).
# Linux Mint 22.x (Ubuntu 24.04 Basis). Mit sudo ausfuehren.
set -euo pipefail

CON="ArtNet-Net25"

if nmcli connection show | grep -q "^${CON}"; then
    echo "Profil ${CON} existiert bereits."
else
    sudo nmcli connection add type ethernet con-name "${CON}" \
        ipv4.method manual ipv4.addresses 2.0.0.100/8 ipv4.never-default yes \
        ipv6.method disabled autoconnect yes connection.autoconnect-priority 10
    echo "Profil ${CON} angelegt."
fi

sudo nmcli connection up "${CON}"

echo "--- Kontrolle ---"
ip -4 addr show | grep -E 'inet 2\.0\.0\.' || echo "FEHLER: keine 2.0.0.x Adresse aktiv!"
ip route | grep default
ping -c 3 2.0.0.1
