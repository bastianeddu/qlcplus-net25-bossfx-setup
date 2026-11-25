# QLC+ + Showtec Net-2/5 + Stairville LED BossFx-1 – Setup (Linux Mint 22.x)

Getestet auf Linux Mint 22.3 (Zena, Ubuntu 24.04 Basis) mit QLC+ 5.3.1.

## Uebersicht / Verkabelung

```
Laptop (Mint, QLC+) --LAN-Kabel--> Showtec Net-2/5 Pocket (50808, Default-IP 2.0.0.1)
                                        | DMX (Port A, 5-pol XLR) --5->3-Adapter--> Fixture DMX-IN (3-pol)
```

Protokoll-Kette: **QLC+ Universe 1 -> Art-Net Universe 0 (UDP 6454 Broadcast) -> Net-2/5 Port 01 (Net 0 / Primary 0) -> DMX-512**

Hinweis Universe-Nummerierung: QLC+ zaehlt ab 1, Art-Net ab 0. QLC+ Universe 1 landet automatisch auf Art-Net Universe 0.

## 1. QLC+ installieren (Mint 22.x)

Offizielles OBS-Repo (Paket `qlcplus5`), NICHT `apt install qlcplus` (veraltet):

```bash
echo 'deb http://download.opensuse.org/repositories/home:/mcallegari79/xUbuntu_24.04/ /' | sudo tee /etc/apt/sources.list.d/home:mcallegari79.list
curl -fsSL https://download.opensuse.org/repositories/home:/mcallegari79/xUbuntu_24.04/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home:mcallegari79.gpg > /dev/null
sudo apt update
sudo apt install qlcplus5
```

Falls die QML-Oberflaeche nicht startet ("module QtQuick.Controls is not installed"):

```bash
sudo apt install qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
  qml6-module-qtquick-window qml6-module-qtquick-dialogs qml6-module-qtquick-templates \
  qml6-module-qtmultimedia qml6-module-qt-labs-platform \
  qml6-module-qt3d-core qml6-module-qt3d-render qml6-module-qt3d-input \
  qml6-module-qt3d-extras qml6-module-qt3d-animation qml6-module-qt3d-logic qml6-module-qtquick-scene3d
```

Start: `qlcplus5` (oder Menue "QLC+ 5").

## 2. Netzwerkprofil fuer Art-Net am Laptop

Skript ausfuehren: `sudo ./setup-artnet-profile.sh`
(legt Profil "ArtNet-Net25" an: 2.0.0.100/8, nie Default-Route, autoconnect; aktiviert es und pingt 2.0.0.1).

Oder von Hand (GUI): Systemeinstellungen -> Netzwerk -> neue Ethernet-Verbindung "ArtNet-Net25",
IPv4 manuell 2.0.0.100 / Praefix 8 (Netzmaske 255.0.0.0), Gateway leer,
"nur fuer Ressourcen in diesem Netzwerk" aktivieren, Autoconnect an, IPv6 aus.

## 3. Showtec Net-2/5 Pocket konfigurieren

- Node mit 12-V-Netzteil versorgen (Laptop-Port liefert kein PoE!), per Patchkabel direkt an Laptop.
- Webinterface: http://2.0.0.1 im Browser, Login **Admin / Admin**.
- Tab **Port Settings**:
  - **DMX Port 01**: DMX Status = **Output**, DMX Mode = **Single**, Protocol = **ArtNet**,
    Frame Rate = **40Hz**, RDM = Enable, Network = **0**, Primary = **0**  -> **Apply**
  - **DMX Port 02**: DMX Status = **Disable** -> **Apply**
- Device Settings pruefen: Manual IP **2.0.0.1**, Net Mask **255.0.0.0**.
- Firmware-Updates: Produktseite https://www.showtec-lights.com/en/50808-net-2-5-pocket.html
  -> Downloads -> neueste `50808_firmware Vx-x.zip` -> entpacken (.upf) -> Webinterface
  "Firmware Upgrade" -> Choose File -> **Upload File** (nicht Apply!). Danach "Set Default".
  (Achtung: nur 50808-Dateien, nicht 50807/50809 = Net-2/3!)

## 4. QLC+: Art-Net-Ausgang + Fixtures

1. Fixtures installieren: `fixtures/*.qxf` nach `~/.qlcplus/fixtures/` kopieren, QLC+ neu starten.
2. QLC+ oeffnen, Tab **Inputs/Outputs** (unten): **Universe 1** waehlen -> Output-Spalte:
   **"ART-NET (2.0.0.100)"** auswaehlen (das ist die LAN-Schnittstelle mit der ArtNet-IP;
   NICHT 127.0.0.1, NICHT die WLAN-IP). Input-Spalte = **None**.
   Zahnrad (Configure) pruefen: Art-Net Universe = 0 (automatisch), Broadcast.
3. Tab **Fixture Manager** -> **Add**:
   - Stairville -> "LED BossFx-1 Bundle (28ch)" -> **Universe 1, Address 001**
   - (optional) beamZ -> "Par64 DMX 151.257 (177x10mm RGB) 6ch" -> Universe 1, Address 007
4. Projekt speichern: **Strg+S** (z. B. `~/show.qxw`). Fixtures + I/O-Mapping liegen im Projekt.

## 5. Fixtures einstellen

- **Stairville LED BossFx-1 Bundle**: Schluesselschalter stecken (Laser! nie in den Strahl schauen).
  Display-Menue: Betriebsart DMX, **`xxCH` = 28CH**, **`dXXX` = 001**.
  DMX-IN ist 3-polig -> am Net-2/5 den 5->3-Pol-Adapter (1:1, 2:2, 3:3) verwenden.
  Letztes Geraet der Kette mit 120-Ohm-Terminator am DMX-OUT abschliessen.
- **beamZ Par64 DMX 151.257** (Display-Variante): Taste **MODE** bis **Addr / A001** erscheint,
  **ENTER**, mit UP/DOWN **A001** einstellen, **ENTER** (= DMX-Modus, Adresse 001).

## 6. Test

Simple Desk -> Universe 1:
- BossFx-1: Kanal 10 (Spot 1 Dimmer) = 255, Kanal 6 (Spot 1 Red) = 255 -> Spot 1 rot.
  Laser-Kanaele 21/22 erst auf 0 (Blackout) lassen!
- beamZ PAR: Kanal 1 (Master Dimmer) = 255, Kanal 6 (Mode) = 0 (DMX-Modus!), Kanal 2 = 255 -> rot.

## 7. Troubleshooting

- `ip link show enp0s31f6` muss `LOWER_UP` + `state UP` zeigen, sonst Kabel/Port pruefen (NO-CARRIER = kein Kabel erkannt).
- `ping 2.0.0.1` muss antworten; OLED des Net-2/5 zeigt die IP (Default 2.0.0.1).
- Am Net-2/5: blaue DMX-LED an Port 01 = DMX-Ausgang aktiv; ACT-LED blinkt bei Netzwerkverkehr.
- QLC+ DMX-Monitor (Inputs/Outputs-Tab) zeigt gesendete Werte.
- Universum stimmt nicht? QLC+ Universe 1 = Art-Net Universe 0 = Node Primary 0.
- Fixture taucht nicht in Add auf? Datei direkt in `~/.qlcplus/fixtures/` (kein Unterordner), QLC+ neu starten.
- PAR reagiert nicht: Dimmer-Kanal auf 255, Mode-Kanal im DMX-Bereich (0), richtiger XLR-Eingang (IN, nicht OUT), 5->3-Adapter korrekt verdrahtet, Terminator am Ende.

## Dateien

- `fixtures/Stairville-LED-BossFx-1.qxf` – BossFx-1, 28-Kanal-Modus (Quelle: Thomann-Handbuch 424577, S. 25-27)
- `fixtures/beamZ-PAR64-151257.qxf` – beamZ Par64 DMX 151.257 (Display-Version, 6 Kanäle)
- `setup-artnet-profile.sh` – Netzwerkprofil am Laptop
