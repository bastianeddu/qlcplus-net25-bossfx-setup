/*
 * ESP32 DMX-512 Show-Player — autark, startet beim Einschalten automatisch.
 *
 * Hardware: ESP32-WROOM-32 DevKit + MAX3485-Modul (RS-485) + XLR-Buchse.
 * Show wird im Flash (LittleFS, /show.bin) gespeichert und geloopt.
 * Ohne gespeicherte Show laeuft eine Demo-Show (BossFx-1, 28CH @ d001).
 *
 * Bibliothek: Arduino Library Manager -> "ESP_DMX" (someweisguy, v4.x)
 * Board: "ESP32 Dev Module" (WROOM-32! WROVER hat keine GPIO16/17)
 * Partition Scheme: "Huge APP (3MB No OTA/1MB LittleFS)"  (~1 MB = ~2000 Frames = ~50 s Show)
 *
 * Serial-Monitor (115200) Befehle:
 *   r = 10 s Show vom DMX-Bus aufnehmen (ESP32 parallel am Bus anschliessen!)
 *   p = gespeicherte Show abspielen (loop)
 *   d = Demo-Show abspielen
 *   b = Blackout senden
 *   x = aktuelles Play stoppen
 */

#include <Arduino.h>
#include <esp_dmx.h>
#include <FS.h>
#include <LittleFS.h>
#include <vector>
#include <utility>

// ---------- Hardware-Pins ----------
static const int PIN_TX = 17;   // -> MAX3485 DI
static const int PIN_RX = 16;   // <- MAX3485 RO
static const int PIN_EN = 21;   // -> MAX3485 DE + RE (zusammenlegen! Richtungs-Umschaltung)
static dmx_port_t dmxPort = 1;  // UART2. Port 0 ist die USB-Serial-Konsole!

#define SHOW_FILE   "/show.bin"
#define FRAME_BYTES DMX_PACKET_SIZE   // 513 = Startcode + 512 Slots
#define FPS         40

static byte frame[FRAME_BYTES];    // frame[0]=Startcode, frame[1..512]=DMX-Kanaele 1..512
static volatile bool g_stop = false;

// ---------- Demo-Show: BossFx-1 @ d001, 28CH ----------
struct Scene { uint16_t hold_ms; uint16_t fade_ms; byte data[28]; };

static Scene mk(uint16_t hold, uint16_t fade, std::initializer_list<std::pair<int, int>> v) {
  Scene s{hold, fade, {0}};
  for (auto &p : v) s.data[p.first - 1] = (byte)p.second;   // DMX-Kanal 1..28 -> Index 0..27
  return s;
}

static const std::vector<Scene> demo = {
  mk(500,  500,  {}),                                              // Blackout
  mk(3000, 1000, {{6,255},{10,255}}),                              // Spot 1 rot
  mk(3000, 1000, {{7,255},{10,255}}),                              // Spot 1 gruen
  mk(3000, 1000, {{8,255},{10,255},{11,255},{15,255}}),            // Spot 1 blau + Spot 2 rot
  mk(4000, 1000, {{1,255},{2,255},{3,255},{5,200}}),               // Derby 1 weiss, Motor dreht
  mk(4000, 1000, {{16,255},{17,255},{18,255},{20,100}}),           // Derby 2
  mk(3000, 1000, {{24,7},{25,7},{26,7},{27,7},{28,7}}),            // Strobes + UV DAUERHAFT an (Wert 5-9!)
};

// ---------- DMX ----------
static void dmxInit() {
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  if (!dmx_driver_install(dmxPort, &config, NULL, 0)) Serial.println("DMX install FAIL");
  dmx_set_pin(dmxPort, PIN_TX, PIN_RX, PIN_EN);
  memset(frame, 0, sizeof(frame));
}

static void sendFrame(const byte *f) {
  memcpy(frame, f, FRAME_BYTES);
  dmx_write(dmxPort, frame, FRAME_BYTES);
  dmx_send(dmxPort);                          // komplettes 512-Slot-Paket
  dmx_wait_sent(dmxPort, DMX_TIMEOUT_TICK);   // warten bis uebertragen (~23 ms)
  vTaskDelay(pdMS_TO_TICKS(2));               // -> ~40 fps
}

static void blackout() {
  memset(frame, 0, sizeof(frame));
  sendFrame(frame);
}

// ---------- Play: Datei ----------
static void playFile() {
  File f = LittleFS.open(SHOW_FILE, FILE_READ);
  if (!f) { Serial.println("kein " SHOW_FILE " -> Demo"); playDemo(); return; }
  Serial.println("PLAY " SHOW_FILE " (loop)...");
  byte buf[FRAME_BYTES];
  while (!g_stop) {
    if (f.read(buf, FRAME_BYTES) == FRAME_BYTES) sendFrame(buf);
    else f.seek(0);                            // Ende -> von vorne
    if (Serial.available()) return;            // Befehl -> zurueck zu loop()
  }
  f.close();
}

// ---------- Play: Demo mit Fades ----------
static void playDemo() {
  Serial.println("PLAY Demo (loop)...");
  const size_t n = demo.size();
  size_t idx = 0;
  byte cur[28] = {0};
  while (!g_stop) {
    const Scene &s = demo[idx];
    uint32_t t0 = millis();
    while (millis() - t0 < s.fade_ms && !g_stop) {                 // Fade cur -> s
      float k = (millis() - t0) / (float)s.fade_ms;
      for (int c = 0; c < 28; c++) frame[c + 1] = cur[c] + (s.data[c] - cur[c]) * k;
      sendFrame(frame);
      if (Serial.available()) return;
    }
    memcpy(cur, s.data, 28);
    for (int c = 0; c < 28; c++) frame[c + 1] = cur[c];
    uint32_t h0 = millis();
    while (millis() - h0 < s.hold_ms && !g_stop) {                 // Halten
      sendFrame(frame);
      if (Serial.available()) return;
    }
    idx = (idx + 1) % n;
  }
}

// ---------- Record: Show vom DMX-Bus in den Flash ----------
static void recordShow(uint32_t seconds) {
  Serial.printf("REC %lu s, DMX-Bus muss von QLC+/Net-2/5 getrieben werden...\n", (unsigned long)seconds);
  File f = LittleFS.open(SHOW_FILE, FILE_WRITE);
  if (!f) { Serial.println("FS-Fehler"); return; }
  dmx_packet_t pkt;
  byte buf[FRAME_BYTES];
  uint32_t total = seconds * FPS, got = 0;
  for (uint32_t i = 0; i < total && !g_stop; i++) {
    size_t n = dmx_receive(dmxPort, &pkt, DMX_TIMEOUT_TICK);
    if (n > 0 && pkt.err == DMX_OK) {
      memset(buf, 0, sizeof(buf));
      size_t c = n < FRAME_BYTES ? n : FRAME_BYTES;
      dmx_read(dmxPort, buf, c);
      f.write(buf, FRAME_BYTES);
      got++;
    }
    if ((i % FPS) == 0 && Serial.available()) break;
  }
  f.close();
  Serial.printf("gespeichert: %lu Frames (~%lu s) -> " SHOW_FILE "\n",
                (unsigned long)got, (unsigned long)(got / FPS));
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nESP32 DMX-Player");
  Serial.println("Befehle: r=record 10s | p=play file | d=demo | b=blackout | x=stop");
  if (!LittleFS.begin(true)) Serial.println("LittleFS-Fehler (Partition Scheme pruefen!)");
  dmxInit();

  // Autostart beim Einschalten: gespeicherte Show, sonst Demo
  if (LittleFS.exists(SHOW_FILE)) playFile();
  else playDemo();
}

void loop() {
  if (!Serial.available()) return;
  char c = (char)Serial.read();
  g_stop = true;                 // laufendes Play stoppen
  delay(50);
  switch (c) {
    case 'r': g_stop = false; recordShow(10); g_stop = false; break;
    case 'p': g_stop = false; playFile();   break;
    case 'd': g_stop = false; playDemo();   break;
    case 'b': blackout();                   break;
    default:  break;
  }
  g_stop = false;
}
