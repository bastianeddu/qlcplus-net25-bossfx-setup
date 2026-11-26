# ESP32 DMX-Show-Player (autark, ~5–8 €)

Kleinster autarker DMX-Master: ESP32 + RS-485-Transceiver sendet DMX-512 direkt an die Lampe.
**Kein WLAN, kein Laptop, kein Netzteil noetig** — Strom per USB (Ladegeraet/Powerbank), Show liegt im
Flash und startet **automatisch beim Einschalten**.

Anwendung: Show einmal in QLC+ schreiben -> am DMX-Bus aufnehmen (`r`) -> ESP32 abziehen -> ueberall
plug & play (Strom an = Show laeuft).

## BOM (ca.-Preise)

| Teil | ca. Preis | Bemerkung |
|---|---|---|
| ESP32-WROOM-32 DevKit | 3–5 € | **WROOM-32** nehmen (WROVER hat keine GPIO16/17!) |
| MAX3485-Modul (RS-485, 3,3 V) | 0,50–1 € | MAX3485 bevorzugen (3,3 V-nativ); MAX485 geht meist auch |
| XLR 5-pol Einbaubuchse | ~1 € | oder direkt 3-pol XLR fuer die BossFx-1 |
| 120Ω-Terminator | ~1 € | kommt ans **Lampenseitige** Ende der Kette (nicht am ESP32!) |
| USB-Kabel + altes USB-Ladegeraet/Powerbank | 0 € | Stromversorgung = "Einschalten" |

**Gesamt: ~5–8 €**

## Verkabelung

```
ESP32 GPIO17 (TX)  ---->  MAX3485 DI
ESP32 GPIO16 (RX)  <----  MAX3485 RO
ESP32 GPIO21       ---->  MAX3485 DE + RE   (beide Pins am Modul zusammenlegen!)
ESP32 3.3V / GND   ---->  MAX3485 VCC / GND
MAX3485 A          ---->  XLR Pin 3 (DMX Data+)
MAX3485 B          ---->  XLR Pin 2 (DMX Data-)
GND                ---->  XLR Pin 1 (Schirm)
XLR 5-pol OUT      --5->3-Adapter-->  BossFx-1 DMX-IN (3-pol)
BossFx-1 DMX-OUT   <-- 120Ω-Terminator
```

- Am MAX3485-Modul **keinen** 120Ω-Abschluss-Jumper setzen — terminiert wird am entfernten Ende (Lampe).
- BossFx-1 wie gehabt einstellen: Display `DNH` -> `d001` -> `28CH`, Laserschluessel stecken.
- **Laser-Sicherheit:** Kanäle 21/22 in aufgenommenen Shows auf 0 (Blackout) lassen, nie in den Strahl schauen!

## Firmware flashen (Arduino IDE)

1. Arduino IDE installieren, Board-Paket **"esp32" by Espressif Systems** (2.x/3.x) installieren.
2. Library Manager: **"ESP_DMX"** von someweisguy (v4.x) installieren.
3. Board: **"ESP32 Dev Module"**, Port = COM-Port des ESP32.
4. **Tools -> Partition Scheme: "Huge APP (3MB No OTA/1MB LittleFS)"** (LittleFS ~1 MB = ~2000 Frames ≈ 50 s Show bei 40 fps).
5. Sketch `esp32-dmx-player.ino` oeffnen, **Upload**. Serial Monitor (115200) oeffnen.

## Bedienung

| Befehl (Serial Monitor) | Funktion |
|---|---|
| *(nichts)* | **Autostart**: `/show.bin` abspielen (loop), falls vorhanden, sonst Demo-Show |
| `r` | 10 s Show **vom DMX-Bus aufnehmen** und in `/show.bin` speichern |
| `p` | gespeicherte Show abspielen (loop) |
| `d` | eingebaute Demo-Show (BossFx-1) abspielen |
| `b` | Blackout (alles 0) senden |
| `x` | stoppen |

### Show aufnehmen (einmalig)

1. ESP32 **parallel** auf den DMX-Bus haengen (Y-Kabel/T-Stueck) — z. B. am DMX-OUT des Net-2/5,
   waehrend der Laptop/QLC+ die Show einmal komplett abfaehrt.
2. Im Serial Monitor `r` druecken, Show in QLC+ starten, 10 s laufen lassen.
3. Fertig: `/show.bin` liegt im Flash. Laptop/QLC+ kann abgezogen werden.
4. ESP32 nur noch mit USB-Strom versorgen -> Show startet automatisch und loopt.

### Demo-Show (laeuft ohne Aufnahme)

Blackout -> Spot 1 rot -> Spot 1 gruen -> Spot 1 blau + Spot 2 rot -> Derby 1 drehend -> Derby 2 ->
Strobes + UV dauerhaft an (Wert 7 = 5–9-Bereich "dauerhaft an"!). Mit Fades dazwischen.
Eigene Szenen einfach in der `demo`-Tabelle ergaenzen (Kanal 1..28 -> Wert 0..255).

## Hinweise

- DMX-Timing (Break/MAB/250 kBaud) erzeugt die Bibliothek per UART2 + Timer — am ESP32 laeuft das stabil.
- Die Demo-/Aufnahme-Kanaele beziehen sich auf die BossFx-1 im 28CH-Modus an Adresse 001
  (siehe `fixtures/Stairville-LED-BossFx-1.qxf`).
- Flash-Format: pro Frame 513 Byte (Startcode + 512 Slots). Datei loeschen: Serial Monitor `x`, dann
  Datei manuell loeschen oder Partition "Erase Flash" im Arduino-IDE (Tools).
