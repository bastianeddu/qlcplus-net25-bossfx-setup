# QLC+ + Showtec Net-2/5 + Stairville LED BossFx-1 – Setup (Linux Mint 22.x)

Getestet auf Linux Mint 22.3 (Zena, Ubuntu 24.04 Basis) mit QLC+ 5.3.1.

## Uebersicht / Verkabelung

```
Variante A (direkt):  Laptop --LAN-Kabel--> Net-2/5 (Default-IP 2.0.0.1)
Variante B (Heimnetz, empfohlen): Laptop --WLAN--> Fritzbox --LAN/Switch--> Net-2/5 (IP per DHCP, z. B. 192.168.188.61)
                                        | DMX (Port 01, 5-pol XLR) --5->3-Adapter--> Fixture DMX-IN (3-pol)
```

Protokoll-Kette: **QLC+ Universe 1 -> Art-Net Universe 0 (UDP 6454) -> Net-2/5 Port 01 (Net 0 / Primary 0) -> DMX-512**
Art-Net wird als Broadcast gesendet (z. B. 192.168.188.255:6454); falls der Broadcast nicht durchgereicht wird: Unicast auf die Node-IP einstellen (siehe 3b).

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
- Webinterface antwortet mit **HTTP 401**? Einfach **Admin / Admin** eingeben (Basic-Auth-Popup).
  Browser-Popup kommt nicht? URL direkt: `http://Admin:Admin@2.0.0.1/`
- Ohne Webinterface geht alles auch ueber das **OLED-Display am Geraet** (Tasten MENU/UP/DOWN/ENTER):
  - Status-Seiten (nach Boot, UP/DOWN blättern): **DEVICE INFO** (IP, Netmask) und **PORT STATUS** (Port A/B: In/Out, Protoc, Univer)
  - **MENU -> Hauptmenue**: 2. IP Mode (DHCP IP / Manual IP), 3. Set Netmask, 4. Set DMX Port, 5. Set Display

### 3b. Net-2/5 im Heimnetz betreiben (DHCP ueber Fritzbox) — getestet

Empfohlene Variante: Node steckt an Fritzbox-LAN (oder Switch daran), bekommt per DHCP eine IP.

1. Node zunaechst direkt an Laptop (Profil ArtNet-Net25 aktiv, `ping 2.0.0.1` ok), Webinterface `http://2.0.0.1` (Admin/Admin):
   - Port Settings wie in 3. (Port 01 = Output/ArtNet/Net 0/Primary 0/40Hz, Port 02 = Disable)
   - Device Settings: **DHCP Enable = ON** -> Apply -> Node bootet neu (ggf. Power-Cycle noetig)
2. Node an Fritzbox-LAN/Switch umstecken, 12-V-Netzteil dran. Fritzbox: Heimnetz -> Netzwerk -> Geraet suchen (MAC z. B. A0:AF:BD:...) -> IP notieren (z. B. 192.168.188.61) -> **"Diesem Netzwerkgeraet immer dieselbe IPv4-Adresse zuweisen"** aktivieren.
3. Laptop: Ethernet-Port frei (`sudo nmcli device disconnect enp0s31f6`, falls aktiv), WLAN verbinden:
   ```bash
   ping -c 4 192.168.188.61
   ip neigh show 192.168.188.61     # MAC des Node, z. B. a0:af:bd:6e:43:c1
   ```
4. QLC+ neu starten -> Inputs/Outputs -> Universe 1 -> Output: **"ART-NET (192.168.188.98)"** (die **WLAN-IP** des Laptops!) -> Input None -> Strg+S.
5. Test: Simple Desk-Schieber bewegen -> **ACT-LED am Node blinkt**, blaue DMX-LED an Port 01 leuchtet.

**Fallback Unicast** (wenn Broadcast WLAN->LAN nicht durchgereicht wird, ACT bleibt dunkel obwohl tcpdump Pakete zeigt):
QLC+ Inputs/Outputs -> Zahnrad beim **ArtNet**-Plugin -> Outputs -> Universe 1 -> Spalte **IP-Adresse = 192.168.188.61** (statt Broadcast) -> OK -> Strg+S.

**Hinweise:**
- Im DHCP-Modus ist die Netmask im Display-Menue gesperrt (kommt vom DHCP-Server = 255.255.255.0). Die Anzeige 255.0.0.0 ist nur der gespeicherte statische Restwert.
- Ping/ARP zum Node gehen, aber ACT blinkt nicht? Meist **Stecker/Kabel-Problem** (NO-CARRIER bzw. schlechter Kontakt) oder Switch-Port mit fremdem VLAN — Node testweise direkt an Fritzbox-LAN haengen.
- Webinterface (TCP 80) antwortet nicht, Ping aber schon? Node powercyclen (am besten direkt an der Fritzbox), dann ist das Web-UI wieder da.

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
  Display-Menue (Tasten MODE/UP/DOWN/ENTER): Hauptmenue zeigt u. a. **DNH** (das ist "dMX" = DMX-Modus; das Display sieht aus wie "DNH"), SOU (Sound), AUT (Auto-Shows), COL (Farbe), SLA (Slave), SYS.
  -> **DNH** waehlen -> **`dXXX` = 001**, **`xxCH` = 28CH**.
  DMX-IN ist 3-polig -> am Net-2/5 den 5->3-Pol-Adapter (1:1, 2:2, 3:3) verwenden.
  Letztes Geraet der Kette mit 120-Ohm-Terminator am DMX-OUT abschliessen.
- **beamZ Par64 DMX 151.257** (Display-Variante): Taste **MODE** bis **Addr / A001** erscheint,
  **ENTER**, mit UP/DOWN **A001** einstellen, **ENTER** (= DMX-Modus, Adresse 001).

## 6. Test (getestet: alle BossFx-1-Komponenten OK)

Simple Desk -> Universe 1 -> Kanalansicht "Fixture-Kanaele", GM = 100 %, alles erst auf 0:

| Kanaele | Erwartung |
|---|---|
| ch10=255 + ch6=255 | Spot 1 rot hell |
| ch7=255 (statt ch6) / +ch8=255 | Spot 1 gruen / weiss |
| ch9=255 | Spot 1 Strobe |
| ch15=255 + ch11=255, ch14=255 | Spot 2 rot + Strobe |
| ch1-3=255, ch5=100/200/0, ch4=255 | Derby 1 weiss, Motor CW/CCW/Stopp, Strobe |
| ch16-18=255, ch20=200, ch19=255 | Derby 2 analog |
| ch24-27=**5-9** | weisse Strobe-LEDs **dauerhaft an** (10-255 = blinken!) |
| ch28=**5-9** | **UV dauerhaft an** (10-255 = blinken!) |
| ch21=5, ch22=5, ch23=200 (kurz!, dann 21-23=0) | Laser rot/gruen/Motor — nie in den Strahl schauen! |

- beamZ PAR: Kanal 1 (Master Dimmer) = 255, Kanal 6 (Mode) = 0 (DMX-Modus!), Kanal 2 = 255 -> rot.
- **Wichtig:** UV/Strobe "dauerhaft an" heisst Wert **5-9**, nicht 255 (255 = Strobe-Modus des Geraets).
- Kanalreihenfolge in QLC+ folgt der DMX-Belegung des Geraets (fix: Derby1=1-5, Spot1=6-10, Spot2=11-15, Derby2=16-20, Laser=21-23, Strobes=24-27, UV=28) und laesst sich nicht umsortieren. Die Wunsch-Anordnung (unten Derby/Spots, oben Strobes/UV/Laser) bildest du in der **Virtual Console** mit eigenen Rahmen/Buttons nach.

## 7. Troubleshooting

- `ip link show enp0s31f6` muss `LOWER_UP` + `state UP` zeigen, sonst Kabel/Port pruefen (NO-CARRIER = kein Kabel erkannt).
- `ping 2.0.0.1` muss antworten (direkt) bzw. `ping <Node-IP>` (DHCP-Modus); OLED des Net-2/5 zeigt die IP.
- ACT-LED blinkt nicht, obwohl QLC+ sendet (tcpdump auf WLAN-Interface: `sudo tcpdump -i wlp0s20f3 -n udp port 6454`)?
  -> Kabel/Stecker checken, Node direkt an Fritzbox testen, Port-Status am OLED pruefen, ggf. Art-Net auf Unicast (3b).
- Im DHCP-Modus in QLC+ **immer die WLAN-IP** als ART-NET-Output waehlen, nicht 2.0.0.100.
- Am Net-2/5: blaue DMX-LED an Port 01 = DMX-Ausgang aktiv; ACT-LED blinkt bei Art-Net-Verkehr.
- QLC+ DMX-Monitor (Inputs/Outputs-Tab) zeigt gesendete Werte.
- Universum stimmt nicht? QLC+ Universe 1 = Art-Net Universe 0 = Node Primary 0.
- Fixture taucht nicht in Add auf? Datei direkt in `~/.qlcplus/fixtures/` (kein Unterordner), QLC+ neu starten.
- PAR reagiert nicht: Dimmer-Kanal auf 255, Mode-Kanal im DMX-Bereich (0), richtiger XLR-Eingang (IN, nicht OUT), 5->3-Adapter korrekt verdrahtet, Terminator am Ende.

## Dateien

- `fixtures/Stairville-LED-BossFx-1.qxf` – BossFx-1, 28-Kanal-Modus (Quelle: Thomann-Handbuch 424577, S. 25-27)
- `fixtures/beamZ-PAR64-151257.qxf` – beamZ Par64 DMX 151.257 (Display-Version, 6 Kanäle)
- `setup-artnet-profile.sh` – Netzwerkprofil am Laptop
