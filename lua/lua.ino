////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    LUA für Teensy 4.1  für TEENSY 4.1                                                                          //
//                                    for VGA monitor output - Juni 2026                                                                          //
//      mit folgenden Features: Fliesskomma-Arithmetik mit double-Präzision                                                                       //
//                              Editor für Skripte mit :-Farb-Syntax                                                                              //
//                                                      -Suchfunktion                                                                             //
//                                                      -Block-Kopier und Einfügefunktion                                                         //
//                                                      -Block-Löschfunktion                                                                      //
//                              Farb- und Grafikfunktionen                                                                                        //
//                              mathematische Funktionen                                                                                          //
//                              SD-Card-Funktionen                                                                                                //
//                              Flashloader für Hex-Dateien                                                                                       //
//                              GFX-Engine für Spiele                                                                                             //
//                                                                                                                                                //
//      von:Reinhard Zielinski <zille09@gmail.com>                                                                                                //
//                                                                                                                                                //
//      Connections: SD-Card -> Builtin                                                                                                           //
//                   VGA-Beschaltung: R: 3(2k), 4(1k), 33(470) | G:11(2k), 13(1k), 2(470) | B:10(820), 12(390) | HSync:15(82) | VSync:8 (82)      //
//                   PCM5102 : BCK: 21, DIN 7, LCK 20  - Kompatibilität zum MCUME-Projekt                                                         //
//                                                                                                                                                //
//                                                                                                                                                //
//                                                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 25.06.2026                         -für Lua zugewiesene PSRAM Größe auf 1024 kB gesenkt, das scheint mehr als ausreichend zu sein
//                                    -beim Start des Editors immernoch Probleme mit dem Cursor -> scheint jetzt behoben zu sein
//                                    -es wurde ein Dateivergleich (jetzige mit vorheriger) eingebaut um falsche gemerkte Cursorpositionen zu verhindern
//
// 24.06.2026                         -nach der Verarbeitung der Fehlerposition im Editor nun auch die letzte bearbeitete Position als Merker
//                                    -Editor springt an die letzte bearbeitete Position
//
// 23.06.2026                         -GFX-Engine vom VGA_t4-Treiber nutzbar gemacht


#include <Arduino.h>
//#include <sys/types.h>
//#include <cmath>
#include <VGA_t4.h>
#include <USBHost_t36.h> // Offizielle USB-Host Bibliothek für Teensy 4.1
#include <SD.h>
#include <SPI.h>



extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

extern "C" int teensy_sd_file_exists(const char* filename) {
  // Hier befinden wir uns in C++ und dürfen 'SD.exists' ganz normal nutzen!
  if (SD.exists(filename)) {
    return 1; // Datei existiert
  }
  return 0;   // Datei existiert nicht
}

#include "FXUtil.h"     // Für die originale update_firmware() Funktion
extern "C" {
#include "FlashTxx.h"   // Für firmware_buffer_init() und firmware_buffer_free()
}


// SD-Karte des Teensy 4.1
const int chipSelect = BUILTIN_SDCARD;
// Das Arbeitsverzeichnis standardmässig auf /lua/
String currentWorkDir = "/lua/";

// Dummy-Implementierungen für fehlende POSIX-Systemaufrufe (Newlib-Stubs)
extern "C" {
  int _open(const char *path, int flags, ...) {
    return -1;
  }
  int _getpid(void) {
    return 1;
  }
  int _times(void *buf) {
    return -1;
  }
  int _unlink(const char *pathname) {
    return -1;
  }
  int _link(const char *oldpath, const char *newpath) {
    return -1;
  }
  int _kill(int pid, int sig) {
    return -1;
  }
}

#include <time.h>                             //RTC-Funktionen Lua-konform

//***************************************** Tastatur ********************************************

// 1. USB Host und Keyboard Controller initialisieren
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
KeyboardController keyboard1(myusb);
USBHIDParser hid1(myusb);

volatile int lastUsbChar = -1; //Merker für die letzte gedrückte Taste

// Timing-Konstanten
const int REPEAT_DELAY = 300; // Millisekunden bis zur ersten Wiederholung
const int REPEAT_RATE = 80;   // Geschwindigkeit der Wiederholung danach

// Zustandsvariablen
uint8_t currentKey = 0;       // Die aktuell gehaltene Taste
uint8_t currentMod = 0;       // Shift, Ctrl, etc.
elapsedMillis repeatTimer;    // Interner Timer
bool repeatPhase = false;     // Ob wir bereits in der Wiederholungsphase sind
// Globale Variablen für das Gedächtnis des Repeat-Systems
int lastRepeatUnicode = 0;    // Speichert den Buchstaben (z.B. 'A' oder 65)
uint8_t lastRepeatMod = 0;    // Speichert Shift/AltGr Zustand
uint8_t lastRepeatKeycode = 0;// Speichert die physische Taste (z.B. 40 für Enter)
bool break_marker = false;    // ESC Taste

//***************************************** SOUND ***********************************************
static const short square[] EXTMEM={
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
32767,32767,32767,32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,
};

const short  noise[] EXTMEM = {
-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,
-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,32767,-32767,-32767,32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,-32767,32767,32767,-32767,
-32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,-32767,-32767,
32767,-32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,-32767,32767,-32767,32767,32767,32767,-32767,-32767,
32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,-32767,32767,32767,-32767,32767,-32767,32767,-32767,-32767,
32767,32767,-32767,-32767,-32767,32767,-32767,-32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,
32767,-32767,32767,-32767,-32767,32767,32767,32767,32767,32767,-32767,32767,-32767,32767,-32767,-32767,
};

#define NOISEBSIZE 0x100

typedef struct
{
  unsigned int spos;
  unsigned int sinc;
  unsigned int vol;
} Channel;

static Channel chan[6] = {
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0} };
  
static void snd_Reset(void)
{
  chan[0].vol = 0;
  chan[1].vol = 0;
  chan[2].vol = 0;
  chan[3].vol = 0;
  chan[4].vol = 0;
  chan[5].vol = 0;
  chan[0].sinc = 0;
  chan[1].sinc = 0;
  chan[2].sinc = 0;
  chan[3].sinc = 0;
  chan[4].sinc = 0;
  chan[5].sinc = 0;
}

static void snd_Mixer(short * stream, int len )
{
  int i;
  long s;  
  len = len >> 1; 
   
  short v0=chan[0].vol;
  short v1=chan[1].vol;
  short v2=chan[2].vol;
  short v3=chan[3].vol;
  short v4=chan[4].vol;
  short v5=chan[5].vol;
  for (i=0;i<len;i++)
  {
    s =((v0*square[(chan[0].spos>>8)&0x3f])>>11);
    s+=((v1*square[(chan[1].spos>>8)&0x3f])>>11);
    s+=((v2*square[(chan[2].spos>>8)&0x3f])>>11);
    s+=((v3*noise[(chan[3].spos>>8)&(NOISEBSIZE-1)])>>11);
    s+=((v4*noise[(chan[4].spos>>8)&(NOISEBSIZE-1)])>>11);
    s+=((v5*noise[(chan[5].spos>>8)&(NOISEBSIZE-1)])>>11);         
    *stream++ = (short)(s);
    *stream++ = (short)(s);
    chan[0].spos += chan[0].sinc;
    chan[1].spos += chan[1].sinc;
    chan[2].spos += chan[2].sinc;
    chan[3].spos += chan[3].sinc;  
    chan[4].spos += chan[4].sinc;  
    chan[5].spos += chan[5].sinc;  
  }        
}

static void sound(int C, int F, int V) {
  if (C < 6) {
    chan[C].vol = V;
    chan[C].sinc = F>>1; 
  }
}

//***************************************** VGA *************************************************
int cursorX = 0;
int cursorY = 0;
VGA_T4 vga;

lua_State *L;

String inputBuffer = "";
static int fb_width, fb_height;

unsigned long lastCursorBlink = 0;
const int blinkInterval = 500; // Blink-Geschwindigkeit in Millisekunden (250ms an / 250ms aus)
bool cursorVisible = false;


// Terminal-Größe
const int TERM_COLS = 80;
const int TERM_ROWS = 60;
EXTMEM char termBuffer[TERM_ROWS][TERM_COLS];
//int cursorX = 0;
//int cursorY = 0;

// Der nutzbare Textbereich im Terminal liegt zwischen Zeile 1 und 58
#define TEXT_START_ROW 1
#define TEXT_END_ROW   58

#define BLUE       11
#define LIGHT_BLUE 83
#define WHITE      255
#define YELLOW     252
#define MAGENTA    163
#define CYAN       127
#define GREEN      24
#define RED        196
#define GRAY       114
#define ORANGE     240
#define DARKGRAY   73
#define DARKBLUE   1
#define BROWN      104
#define DARKRED    96

int fColor = WHITE;
int bColor = DARKBLUE;

const int MAX_C = 640 / 8;                 //Anzahl Textspalten
const int MAX_R = (480 / 8) - 1;           //Anzahl Textzeilen

char currentTitleText[81] = "F1:Edit  F2:Run  F3:Datei  F4:HexMon  F5:Info      --- Teensy-LUA 1.2 ---";
//char currentTitleText[81] = "F1:Edit  F2:Run  F3:Datei  F4:HexMon  F5:Info      --- Teensy-LUA 1.2 ---";

//***************************************** WINDOW **********************************************
// Struktur für die Fenster-Verwaltung
struct WindowSlot {
  bool aktiv = false;
  int x, y, w, h;
  int fc, bc;
  String titel;
  String inhalt;
  uint16_t titelFarbe;
};

// Maximal 8 Fenster-Slots (Index 0 bis 9)
EXTMEM static WindowSlot windowManager[10];

//***************************************** Sprites *********************************************
bool gfx_mode_active = false;                //Umschalter für Pseudografikzeichen

EXTMEM unsigned char tileLibrary[256 * 256];
EXTMEM unsigned char SpriteLibrary[256 * 256];

uint32_t spritePtr = 0; // Aktueller Schreibzeiger im Grafik-Pool
struct SpriteRecord {
  uint32_t startIdx;
  uint16_t w;
  uint16_t h;
  bool active = false;
};

// Konfiguration für 16x16 Tiles (640x480)
#define TILES_W 16
#define TILES_H 16
#define TILES_COLS 40
#define TILES_ROWS 30

// PSRAM Speicher (für eine lange Map)
EXTMEM unsigned char tilemapL0[30 * 500];  // 500 Spalten breit
unsigned char* mapPtrL0 = tilemapL0;       // Der aktuelle "Lese-Kopf"
int currentHScroll0 = 0;                   // Aktuelle Pixel-Position
int currentVScroll0 = 0;                   // dito für vertikales Scrolling
int lastHitX = 0;
int lastHitY = 0;

struct TileAnim {
  int target;  // Das Tile in der Map
  int source;  // Start-Index im Tilesheet
  int frames;  // Anzahl der Bilder
  bool active = false;
};

TileAnim animList[10]; // Max. 10 verschiedene Animationen gleichzeitig
int animCounter = 0;


//***************************************** EDITOR **********************************************
#define EDITOR_MAX_SIZE (128 * 1024)          // 128 KB im PSRAM reservieren
EXTMEM char editorBuffer[EDITOR_MAX_SIZE];    // Editor-Puffer 

#define CLIPBOARD_SIZE (2 * 1024)             // 2 KB Puffer für kopierte Zeilen reservieren
EXTMEM char editorClipboard[CLIPBOARD_SIZE];  // Editor-Clipboard-Speicher

String letzteDatei = "";  //Dateimerker für F-Tasten Funktionen im Terminal
String tmpDatei = "";     //letzte mit dem Editor geöffnete Datei merken für Rücksetzung der gemerkten Cursorposition
int editorStartZeile = 1; // Merker für die Fehler-Anspring-Funktion
int gemerkte_stelle  = 1; // letzte bearbeitete Position merken, um dort weiter zu machen
int gemerkte_zeile   = 1;
//***********************************************************************************************

// 5. Hilfsfunktion: Macht aus "skript.lua" automatisch "/pfad/skript.lua"
FLASHMEM String resolve_lua_path(String filename) {
  if (filename.startsWith("/")) {
    return filename; // Wenn es absolut mit / beginnt, so lassen
  }
  return currentWorkDir + filename; // Ansonsten das aktuelle Verzeichnis davorhängen
}


//********************************** LUA-TERMINAL ***********************************************

FLASHMEM void drawStatusBar() {
  int yPixel = (TERM_ROWS - 1) * 8; // Zeile 59
  vga.drawRect(0, yPixel, 640, 8, DARKRED);
  time_t rawtime = Teensy3Clock.get();
  //rawtime += 3600;

  struct tm* timeinfo = gmtime(&rawtime);
  int sekunden = timeinfo->tm_sec;
  int minuten  = timeinfo->tm_min;
  int stunden  = timeinfo->tm_hour;
  int tag      = timeinfo->tm_mday;
  int monat    = timeinfo->tm_mon + 1;       // tm_mon zählt von 0 bis 11
  int jahr     = timeinfo->tm_year + 1900;   // tm_year zählt die Jahre seit 1900

  char statusBuf[80];
  snprintf(statusBuf, sizeof(statusBuf),
           " System: OS-Bereit | Datum: %02d.%02d.%04d | Zeit: %02d:%02d:%02d ",
           tag, monat, jahr, stunden, minuten, sekunden);
  vga.drawText(8, yPixel, statusBuf, YELLOW, DARKRED, false);
}

FLASHMEM void drawTitleBar() {
  vga.drawRect(0, 0, 640, 8, DARKRED);
  vga.drawText(8, 0, currentTitleText, YELLOW, DARKRED, false);
}

FLASHMEM void scrollTerminal() {
  memmove(&termBuffer[TEXT_START_ROW][0], &termBuffer[TEXT_START_ROW + 1][0], (TEXT_END_ROW - TEXT_START_ROW) * TERM_COLS);
  // 2. Die freigewordene letzte Textzeile (Zeile 58) auf einmal nullen
  memset(&termBuffer[TEXT_END_ROW][0], '\0', TERM_COLS);

  // 3. Den Grafik-Bildschirm für den Textbereich leeren (Y-Pixel 8 bis 472)
  vga.drawRect(0, TEXT_START_ROW * 8, 640, (TEXT_END_ROW - TEXT_START_ROW + 1) * 8, bColor);

  // 4. Den verschobenen Puffer wieder auf den VGA-Bildschirm zeichnen
  for (int y = TEXT_START_ROW; y <= TEXT_END_ROW; y++) {
    for (int x = 0; x < TERM_COLS; x++) {
      char c = termBuffer[y][x];
      if (c != '\0') {
        char echo[] = { c, '\0' };
        vga.drawText(x * 8, y * 8, echo, fColor, bColor, false);
      }
    }
  }
}


FLASHMEM void vga_print_str(const char* str) {

  while (*str) {
    if (*str == '\n') {
      cursorX = 0;
      cursorY++;
      if (cursorY > TEXT_END_ROW) {
        cursorY = TEXT_END_ROW;
        scrollTerminal();
      }
    } else if (*str == '\r') {
      cursorX = 0;
    } else {
      if (cursorY >= TEXT_START_ROW && cursorY <= TEXT_END_ROW) {             // Sicherheitsprüfung: Falls Cursor außerhalb des gültigen Bereichs
        termBuffer[cursorY][cursorX] = *str;
        char echo[] = { *str, '\0' };
        vga.drawText(cursorX * 8, cursorY * 8, echo, fColor, bColor, false);
      }
      cursorX++;

      if (cursorX >= TERM_COLS) {
        cursorX = 0;
        cursorY++;
        if (cursorY > TEXT_END_ROW) {                                         // Auch beim automatischen Zeilenumbruch am Bildschirmrand scrollen!
          cursorY = TEXT_END_ROW;
          scrollTerminal();
        }
      }
    }
    str++;
  }
}
//***********************************************************************************************

//********************************** Lua-Datei ausführen ****************************************
FLASHMEM void executeLuaFile(const char* filename) {
  // Der Pfad-Resolver darf als temporäres Objekt bleiben, da er sich schnell wieder abbaut
  String vollerPfad = resolve_lua_path(filename);

  // 1. FEHLER: DATEI EXISTIERT NICHT
  if (!SD.exists(vollerPfad.c_str())) {
    EXTMEM static char errBuffer[128];
    snprintf(errBuffer, sizeof(errBuffer), "%s existiert nicht!\n\r", vollerPfad.c_str());
    zeigeFehlerPopup("FEHLER", errBuffer);
    return;
  }

  File luaFile = SD.open(vollerPfad.c_str(), FILE_READ);
  if (luaFile) {
    size_t fileSize = luaFile.size();

    // 2. KORREKTUR: Echte PSRAM-Allokation nutzen (entlastet den internen Heap komplett!)
    char* fileBuffer = (char*)extmem_malloc(fileSize + 1);
    
    if (fileBuffer == NULL) {
      EXTMEM static char errBuffer[128];
      snprintf(errBuffer, sizeof(errBuffer), "Zu wenig RAM zum Laden von %s!\n\r", vollerPfad.c_str());
      zeigeFehlerPopup("FEHLER", errBuffer);
      luaFile.close();
      return;
    }

    // Datei direkt in den PSRAM-Puffer einlesen
    luaFile.readBytes(fileBuffer, fileSize);
    fileBuffer[fileSize] = '\0';
    luaFile.close();

    // Puffer an die Lua-Engine übergeben
    if (luaL_loadbuffer(L, fileBuffer, fileSize, vollerPfad.c_str()) == LUA_OK) {
      if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
        // 3. LAUFZEITFEHLER: Puffer im PSRAM für die Fehlermeldung zusammensetzen
        EXTMEM static char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "Laufzeitfehler in %s: %s\n\r", vollerPfad.c_str(), lua_tostring(L, -1));
        
        //Serial.print(errBuffer);
        zeigeFehlerPopup("FEHLER", errBuffer);
        lua_pop(L, 1);
      }
    } else {
      // 4. SYNTAX/DATEIFEHLER: Puffer im PSRAM für die Fehlermeldung zusammensetzen
      EXTMEM static char errBuffer[256];
      snprintf(errBuffer, sizeof(errBuffer), "Dateifehler in %s: %s\n\r", vollerPfad.c_str(), lua_tostring(L, -1));
      
      Serial.print(errBuffer);
      zeigeFehlerPopup("FEHLER", errBuffer);
      lua_pop(L, 1);
    }

    // 5. ULTIMATIV WICHTIG: Den PSRAM-Speicher absolut sauber freigeben!
    extmem_free(fileBuffer);

  } else {
    // 5. ÖFFNEN-FEHLER
    EXTMEM static char errBuffer[128];
    snprintf(errBuffer, sizeof(errBuffer), "Fehler beim Oeffnen von %s\n\r", vollerPfad.c_str());
    
    //Serial.print(errBuffer);
    zeigeFehlerPopup("FEHLER", errBuffer);
  }
}

// Lua-Befehl: run("name.lua")
FLASHMEM static int lua_dofile(lua_State *L) {
  const char* filename = luaL_checkstring(L, 1);
  executeLuaFile(filename);
  return 0;
}

// ************** Zentrale Funktion zur Verarbeitung aller Zeichen (Egal ob USB oder Serial) ****************
FLASHMEM void handleIncomingChar(int c) {

  //------------------------------------ Funktionstasten abfragen ---------------------------------------------

  // --- F1: DATEI IN DEN EDITOR LADEN ---
  if (c == 194) {
    if (letzteDatei.length() > 0) {
      inputBuffer = "edit(\"" + letzteDatei + "\")";
      vga_print_str(inputBuffer.c_str()); vga_print_str("\n");
      int status = luaL_dostring(L, inputBuffer.c_str());
      if (status != LUA_OK) {
        // KORREKTUR: Fehlermeldung komplett ins PSRAM auslagern
        EXTMEM static char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "%s\n", lua_tostring(L, -1));
        zeigeFehlerPopup("DATEI FEHLER", errBuffer);
        lua_pop(L, 1);
      }
      inputBuffer = "";
    } else {
      zeigeFehlerPopup("FEHLER", "Kein Verlauf!\n>");
    }
    return;
  }

  // --- F2: SKRIPT DIREKT STARTEN (RUN) ---
  if (c == 195) {
    if (letzteDatei.length() > 0) {
      inputBuffer = "run(\"" + letzteDatei + "\")";
      vga_print_str(inputBuffer.c_str()); vga_print_str("\n");
      int status = luaL_dostring(L, inputBuffer.c_str());
      if (status != LUA_OK) {
        EXTMEM static char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "%s\n", lua_tostring(L, -1));
        zeigeFehlerPopup("FEHLER", errBuffer);
        lua_pop(L, 1);
      }
      inputBuffer = "";
      vga_print_str("> ");
    } else {
      zeigeFehlerPopup("FEHLER", "Noch keine Datei im Verlauf!\n> ");
    }
    return;
  }

  // --- F3: Verzeichnis anzeigen (Dateimanager laden) ---
  if (c == 196) {
    inputBuffer = "package.loaded[\"fileman\"] = nil\nrun(\"fileman.lua\")\n";
    vga_print_str(inputBuffer.c_str()); vga_print_str("\n");
    int status = luaL_dostring(L, inputBuffer.c_str());
    if (status != LUA_OK) {
      EXTMEM static char errBuffer[256];
      snprintf(errBuffer, sizeof(errBuffer), "Fehler: %s\n", lua_tostring(L, -1));
      zeigeFehlerPopup("FEHLER", errBuffer);
      lua_pop(L, 1);
    }
    inputBuffer = "";
    vga_print_str("> ");
    return;
  }

  // --- F4: sys.hexmon() ---
  if (c == 197) {
    inputBuffer = "sys.hexmon(0)";
    vga_print_str(inputBuffer.c_str()); vga_print_str("\n");
    int status = luaL_dostring(L, inputBuffer.c_str());
    if (status != LUA_OK) {
      EXTMEM static char errBuffer[256];
      snprintf(errBuffer, sizeof(errBuffer), "Fehler: %s\n", lua_tostring(L, -1));
      zeigeFehlerPopup("FEHLER", errBuffer);
      lua_pop(L, 1);
    }
    inputBuffer = "";
    vga_print_str("> ");
    return;
  }

  // --- F5: sys.info() ---
  if (c == 198) {
    inputBuffer = "sys.info()";
    vga_print_str(inputBuffer.c_str()); vga_print_str("\n");
    int status = luaL_dostring(L, inputBuffer.c_str());
    if (status != LUA_OK) {
      EXTMEM static char errBuffer[256];
      snprintf(errBuffer, sizeof(errBuffer), "Fehler: %s\n", lua_tostring(L, -1));
      zeigeFehlerPopup("FEHLER", errBuffer);
      lua_pop(L, 1);
    }
    inputBuffer = "";
    vga_print_str("> ");
    return;
  }

  //-----------------------------------------------------------------------------------------------------------
  //--------------------------------------AUTOVERVOLLSTÄNDIGUNG------------------------------------------------

  // --- TAB-TASTE (ASCII 9): AUTOVERVOLLSTÄNDIGUNG ---
  if (c == 9) {
    int startIndex = -1;
    if (inputBuffer.indexOf("(\"") != -1) {
      startIndex = inputBuffer.indexOf("(\"") + 2; 
    }

    if (startIndex != -1 && inputBuffer.endsWith("\"") == false) {
      String praefix = inputBuffer.substring(startIndex); 
      String treffer = "";
      String trefferListe = "";

      int anzahl = sucheDateiAufSD(praefix, treffer, trefferListe);

      if (anzahl == 1) {
        int loeschLaenge = praefix.length();
        for (int i = 0; i < loeschLaenge; i++) {
          cursorX--;
          if (cursorX < 0) {
            cursorX = TERM_COLS - 1;
            cursorY--;
          }
          termBuffer[cursorY][cursorX] = '\0';
          vga.drawText(cursorX * 8, cursorY * 8, " ", fColor, bColor, false);
        }
        String ergaenzung = treffer + "\")";
        inputBuffer = inputBuffer.substring(0, startIndex) + ergaenzung;
        vga_print_str(ergaenzung.c_str());

      } else if (anzahl > 1) {
        // --- KORREKTUR: Statusleisten-Text komplett im PSRAM zusammenbauen! ---
        int yPixel = (TERM_ROWS - 1) * 8; 
        vga.drawRect(60, yPixel, 640, 8, YELLOW); 

        EXTMEM static char infoBuf[128];
        snprintf(infoBuf, sizeof(infoBuf), " Treffer: [ %s ] ", trefferListe.c_str());
        
        // Abschneiden (Sicherheit gegen Überlauf auf dem Schirm)
        if (strlen(infoBuf) > 78) {
          infoBuf[75] = '.'; infoBuf[76] = '.'; infoBuf[77] = '.'; infoBuf[78] = '\0';
        }

        vga.drawText(8, yPixel, infoBuf, 0, YELLOW, false);
      }
    }
    return;
  }

  //-----------------------------------------------------------------------------------------------------------

  vga.drawText(cursorX * 8, cursorY * 8, " ", fColor, bColor, false);   
  
  if (c == '\n' || c == '\r') {                                         
    vga_print_str("\n");

    if (cursorY > TEXT_END_ROW) {
      cursorY = TEXT_END_ROW;
      scrollTerminal();
    }

    if (inputBuffer.length() > 0) {
      if (inputBuffer.startsWith("edit(\"") && inputBuffer.endsWith("\")")) {
        letzteDatei = inputBuffer.substring(6, inputBuffer.length() - 2);
      }
      else if (inputBuffer.startsWith("run(\"") && inputBuffer.endsWith("\")")) {
        letzteDatei = inputBuffer.substring(5, inputBuffer.length() - 2);
      }
      int status = luaL_dostring(L, inputBuffer.c_str());

      if (status != LUA_OK) {
        // KORREKTUR: Letzter Fehler-Puffer beim Abschicken eines falschen Terminal-Befehls
        EXTMEM static char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "Fehler: %s\n", lua_tostring(L, -1));
        zeigeFehlerPopup("FEHLER", errBuffer);
        lua_pop(L, 1);
      }
      inputBuffer = "";
    }
    vga_print_str("> ");
  }

  else if (c == 8 || c == 127) {                                        // Backspace
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      if (cursorX > 0) {
        cursorX--;
        termBuffer[cursorY][cursorX] = '\0';
        vga.drawText(cursorX * 8, cursorY * 8, " ", fColor, bColor, false);
      }
    }
  }
  else if (c >= 32 && c <= 126) {
    inputBuffer += (char)c;                                                     
    termBuffer[cursorY][cursorX] = (char)c;

    char echo[] = { (char)c, '\0' };                                            
    vga.drawText(cursorX * 8, cursorY * 8, echo, fColor, bColor, false);

    cursorX++;
    if (cursorX > TERM_COLS) {
      cursorX = 0;
      cursorY++;
      if (cursorY > TEXT_END_ROW) {
        cursorY = TEXT_END_ROW;
        scrollTerminal();
      }
    }
  }
}

FLASHMEM uint16_t wait_key(bool modes) {
  lastUsbChar = -1;  // Restmüll im Tastaturpuffer löschen
  if (modes) {
    vga_print_str(PSTR("\n\r"));
    vga_print_str(PSTR("SPACE<Continue> / ESC <Exit>\n\r"));
  }
  while (1) {
    myusb.Task(); // Hält den USB-Host-Controller für die Tastatur aktiv
    yield();      // Gibt Rechenzeit für Interrupts ab!
    if (lastUsbChar != -1) {
      uint16_t c = lastUsbChar;
      lastUsbChar = -1;
      return c;
    }
  }
}

FLASHMEM int lua_global_waitkey(lua_State* L) {
  // 1. Parameter aus Lua auslesen (Standardmäßig false, wenn nichts übergeben wurde)
  bool modes = false;
  if (lua_isboolean(L, 1)) {
    modes = lua_toboolean(L, 1);
  } else if (lua_isnumber(L, 1)) {
    modes = (lua_tointeger(L, 1) != 0);
  }
  uint16_t gedrueckteTaste = wait_key(modes);
  lua_pushinteger(L, gedrueckteTaste);
  return 1; // 1 Rückgabewert an Lua
}

FLASHMEM void OnPress(int unicode, uint8_t modifier, uint8_t keycode) {

  if (lastRepeatKeycode != 0) {   //verhindert weiterlaufen der Repeatfunktion, wenn sich zwei Tasten überlappen
    lastRepeatKeycode = 0;
    repeatPhase = false;
  }
  // Speichern für Auto-Repeat
  lastRepeatUnicode = unicode;
  lastRepeatMod = modifier;
  lastRepeatKeycode = keycode;
  repeatTimer = 0;
  repeatPhase = false;

  process_keyboard_logic(unicode, modifier, keycode);

}

FLASHMEM void OnRelease(int unicode, uint8_t modifier, uint8_t keycode) {
  lastRepeatKeycode = 0; // Stop Repeat
}

FLASHMEM void handleRepeat() {
  if (lastRepeatKeycode == 0) return;

  if (!repeatPhase) {
    if (repeatTimer >= REPEAT_DELAY) {
      repeatPhase = true;
      repeatTimer = 0;
      process_keyboard_logic(lastRepeatUnicode, lastRepeatMod, lastRepeatKeycode);
    }
  } else {
    if (repeatTimer >= REPEAT_RATE) {
      repeatTimer = 0;
      process_keyboard_logic(lastRepeatUnicode, lastRepeatMod, lastRepeatKeycode);
    }
  }
}

FLASHMEM void process_keyboard_logic(int unicode, uint8_t mod, uint8_t keycode) {
  bool shift = (mod & 0x02) || (mod & 0x20);
  bool altGr = (mod & 0x40);

  if (altGr) {
    switch (keycode) {
      case 36: lastUsbChar = 0x7B; return; // AltGr + 7 -> { (Geschweifte Klammer auf)
      case 37: lastUsbChar = 0x5B; return; // AltGr + 8 -> [ (Eckige Klammer auf)
      case 38: lastUsbChar = 0x5D; return; // AltGr + 9 -> ] (Eckige Klammer zu)
      case 39: lastUsbChar = 0x7D; return; // AltGr + 0 -> } (Geschweifte Klammer zu)
      case 45: lastUsbChar = '\\'; return; // AltGr + ß -> \ (Backslash)
      case 100: lastUsbChar = 0x7C; return; // AltGr + < -> | (Pipe / Senkrechter Strich)
    }
  }

  if (unicode > 31 || unicode == 9) {   //normale Textzeichen oder Tab-Taste
    lastUsbChar = unicode;
    return;
  }

  switch (keycode) {
    case 40: lastUsbChar = 13; return;  //Enter
    case 42: lastUsbChar = 8;  return;  //Backspace
    case 41: lastUsbChar = 27; break_marker = true; return;
    case 50: lastUsbChar = 35; return;  //#
    case 53: lastUsbChar = '^'; return;
    case 100: lastUsbChar = (shift ? '>' : '<'); return;
    // Hier kannst du die Pfeiltasten ergänzen:
    case 82: lastUsbChar = 11; return; // Up
    case 81: lastUsbChar = 10; return; // Down
    case 80: lastUsbChar = 21; return; // Left
    case 79: lastUsbChar = 6;  return; // Right
  }
}

//************************************* Lua-Print *************************************
FLASHMEM static int lua_custom_print(lua_State *L) {
  int n = lua_gettop(L);
  String outStr = "";
  for (int i = 1; i <= n; i++) {
    if (i > 1) outStr += "\t";
    if (lua_isstring(L, i)) outStr += lua_tostring(L, i);
    else if (lua_isboolean(L, i)) outStr += (lua_toboolean(L, i) ? "true" : "false");
    else outStr += lua_typename(L, lua_type(L, i));
  }
  outStr += "\n";
  vga_print_str(outStr.c_str());
  return 0;
}
//************************************* Lua-Write *************************************
FLASHMEM static int lua_custom_write(lua_State *L) {
  int n = lua_gettop(L);
  String outStr = "";
  for (int i = 1; i <= n; i++) {
    if (i > 1) outStr += "\t";
    if (lua_isstring(L, i)) outStr += lua_tostring(L, i);
    else if (lua_isboolean(L, i)) outStr += (lua_toboolean(L, i) ? "true" : "false");
    else outStr += lua_typename(L, lua_type(L, i));
  }
  vga_print_str(outStr.c_str());
  return 0;
}

//************************************* Lua-Delay ***********************************************

FLASHMEM static int lua_delay(lua_State *L) {
  delay(luaL_checkinteger(L, 1));
  return 0;
}

//************************************* Lua-Inkey ***********************************************
// Globale Lua-Funktion: inkey() - Gibt den gedrückten Tastencode zurück oder -1
FLASHMEM int lua_global_inkey(lua_State* L) {
  int taste = inchar();
  lua_pushinteger(L, taste);
  return 1;
}

// ============================================================================
// FIRMWARE-LOADER -> flash("XXX.hex") startet den Flasher und lädt XXX.hex
// ============================================================================
FLASHMEM int lua_load_hex(lua_State *L) { 
  const char* filename = luaL_checkstring(L, 1);
  uint32_t buffer_addr, buffer_size;

  vga_print_str(PSTR("INITIALISIERE MULTIBOOT-PUFFER...\r"));
  delay(500);

  // FlasherX-Speicherpuffer anfordern
  firmware_buffer_init(&buffer_addr, &buffer_size);

  // Wenn die Größe 0 ist, abbrechen!
  if (buffer_size == 0) {
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Fehler: Flash voll! buffer_size ist 0.\r"));
    return 0; 
  }

  File hexFile = SD.open(filename, FILE_READ);
  if (!hexFile) {
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("HEX-Datei nicht gefunden!\r"));
    firmware_buffer_free(buffer_addr, buffer_size);
    return 0;
  }
  
  Serial.printf("Datei Groesse: %d Bytes\n", hexFile.size());

  vga_print_str(PSTR("FIRMWARE-TRANSFER STARTET...\r"));
  delay(200);

  // FlasherX übernimmt: Einlesen, RAM-Kopieren, Reboot
  update_firmware(&hexFile, &Serial, buffer_addr, buffer_size);

  // Fallback: Nur wenn die HEX-Datei fehlerhaft/unvollständig war, läuft der Code hier weiter
  hexFile.close();

  zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Hex-Struktur ungueltig. Reboot...\r"));
  firmware_buffer_free(buffer_addr, buffer_size);
  delay(1000);

  REBOOT; 
  
  return 0; // Formaler Abschluss für die Lua-API
}

// ============================================================================
// LUA - FULLSCREEN-EDITOR
// ============================================================================
FLASHMEM int lua_cmd_edit(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    // KORREKTUR: Beide Parameter müssen einzeln mit PSTR() umschlossen werden!
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Dateiname fehlt! Nutzen Sie: edit(\"skript.lua\")"));
    lua_error(L);
    return 0;
  }
  String rawName = lua_tostring(L, 1);

  // KORREKTUR: Aus "test.lua" wird im Hintergrund "/lua/test.lua"
  String vollerPfad = resolve_lua_path(rawName);

  open_fullscreen_editor(vollerPfad);
  vga.clear(bColor);
  delay(10);

  if (strlen(editorBuffer) > 0) {
    // PSTR() sorgt hier dafür, dass der Kompiler-Text direkt im Flash verbleibt
    vga_print_str(PSTR("Kompiliere und starte Lua-Skript aus PSRAM...\n\r"));

    if (luaL_loadbuffer(L, editorBuffer, strlen(editorBuffer), vollerPfad.c_str()) == LUA_OK) {
      if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {   // Code ausführen

        const char* error_msg = lua_tostring(L, -1);
        zeigeFehlerPopup(PSTR("LUA-LAUFZEITFEHLER"), error_msg);
        lua_pop(L, 1);                                  // Fehler vom Stack entfernen
      }

    } else {
      const char* error_msg = lua_tostring(L, -1);
      zeigeFehlerPopup(PSTR("LUA-SYNTAXFEHLER"), error_msg);
      lua_pop(L, 1);                                    // Fehler vom Stack entfernen
    }
  } else {
    // KORREKTUR: Hier war es bereits perfekt gelöst! Beide sauber getrennt.
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Skript ist leer, keine Ausfuehrung.\n\r"));
  }

  return 0;
}

FLASHMEM static int inchar()
{
  myusb.Task();
  handleRepeat();
  if (lastUsbChar != -1) {
    int c = lastUsbChar;
    lastUsbChar = -1;
    return c;
  }
  delayNanoseconds(500);
  yield();
  return -1; // Sofort zurück
}


FLASHMEM int extrahiereFehlerZeile(const char* errorMsg) {
  if (errorMsg == NULL) return 1;
  int zeile = 1;
  const char* ersterDoppelpunkt = strchr(errorMsg, ':');                              // Suchen nach dem ersten Doppelpunkt im rohen C-String
  if (ersterDoppelpunkt != NULL) {
    if (sscanf(ersterDoppelpunkt + 1, "%d", &zeile) == 1) {
      if (zeile > 0) return zeile;                                                    // Gültige Zeilennummer extrahiert!
    }
  }
  return 1;                                                                           // Standard-Fallback, falls kein Muster gefunden wurde
}
FLASHMEM void open_fullscreen_editor(String filename) {
  int tmp_fcolor = fColor;                                                                    //gesetzte Farben sichern
  int tmp_bcolor = bColor;
  int edit_cur_col = 227;
  fColor = WHITE;                                                                             //Texteditor immer Weiss auf Dunkelblau setzen
  bColor = DARKBLUE;

  int tastenCode = -1;

  extern uint8_t external_psram_size;
  if (external_psram_size == 0) {
    zeigeFehlerPopup("FEHLER", "Fehler: Kein PSRAM verbaut!\r\n");
    delay(2000);
    return;
  }

  memset(editorBuffer, 0, EDITOR_MAX_SIZE);                                                    // Puffer löschen und Datei von SD laden
  if (SD.exists(filename.c_str())) {
    File file = SD.open(filename.c_str(), FILE_READ);
    if (file) {
      size_t bytesRead = file.readBytes(editorBuffer, EDITOR_MAX_SIZE - 1);
      editorBuffer[bytesRead] = '\0';
      file.close();
      String Dateivergleich = filename;
      Dateivergleich  = Dateivergleich.substring(strlen(currentWorkDir.c_str()) , strlen(Dateivergleich.c_str()));  //Dateiname aus dem kompletten Pfad extrahieren
      if(strcmp(tmpDatei.c_str(),Dateivergleich.c_str())){                                    //sind die letzte Datei und die jetzige ungleich, Positionsmerker zurücksetzen
        gemerkte_zeile = 1;                                                                   
        gemerkte_stelle = 1;
      }
      tmpDatei = Dateivergleich;                                                              //Datei merken für nächsten Vergleich
    }
  }

  vga.clear(bColor);
  int textLength = strlen(editorBuffer);

  int initialCursorIdx = 0;
  int initialLogischeZeile = 1;
  int initialStartLine = 1;
  int initialCursorY = 16;
  int cursorIdx = 0;
  
  if (editorStartZeile > 1) {                                                                 //Fehlerstelle im Code berechnen
    int aktuelleZeileInSchleife = 1;
    int idx = 0;
    gemerkte_zeile = 1;                                                                       //Fehler geht vor, gemerkte Stelle zurücksetzen
    gemerkte_stelle = 1;
    
    while (editorBuffer[idx] != '\0' && aktuelleZeileInSchleife < editorStartZeile) {         // Byte-Position des Fehlers im Textpuffer suchen
      if (editorBuffer[idx] == '\n') {
        aktuelleZeileInSchleife++;
      }
      idx++;
    }

    if (aktuelleZeileInSchleife == editorStartZeile) {
      initialCursorIdx = idx;
      initialLogischeZeile = editorStartZeile;

      if (initialLogischeZeile > (MAX_R - 4)) {                                               // Berechnen, wie weit der Editor nach unten gescrollt starten muss
        initialStartLine = initialLogischeZeile - 5;                                          // Zentriert den Fehler im Sichtfenster
        initialCursorY = 16 + ((initialLogischeZeile - initialStartLine) * 8);
      } else {
        initialStartLine = 1;
        initialCursorY = 16 + ((initialLogischeZeile - 1) * 8);
      }
    }
    
  }
  //+++++++ DEBUG +++++++++++++++
  Serial.println(gemerkte_stelle);
  Serial.println(gemerkte_zeile);
  
  if(gemerkte_stelle > 1 || gemerkte_zeile > 1){
    editorStartZeile = gemerkte_zeile;
    gemerkte_zeile = 1;                                                                       //gemerkte Zeile zurücksetzen für nächste Benutzung
    int aktuelleZeileInSchleife = 1;
    int idx = 0;
    
    while (editorBuffer[idx] != '\0' && aktuelleZeileInSchleife < editorStartZeile) {         // Byte-Position des Fehlers im Textpuffer suchen
      if (editorBuffer[idx] == '\n') {
        aktuelleZeileInSchleife++;
      }
      idx++;
    }

    if (aktuelleZeileInSchleife == editorStartZeile) {
      initialCursorIdx = idx;
      initialLogischeZeile = editorStartZeile;

      if (initialLogischeZeile > (MAX_R - 4)) {                                                // Berechnen, wie weit der Editor nach unten gescrollt starten muss
        initialStartLine = initialLogischeZeile - 5;                                           // Zentriert den Fehler im Sichtfenster
        initialCursorY = 16 + ((initialLogischeZeile - initialStartLine) * 8);
      } else {
        initialStartLine = 1;
        initialCursorY = 16 + ((initialLogischeZeile - 1) * 8);
      }
    }
    initialCursorIdx = gemerkte_stelle;                                                       //letzte bearbeitete Stelle
    gemerkte_stelle = 1;                                                                      //gemerkte Position im Text zurücksetzen für nächste Benutzung
  }
  
  editorStartZeile = 1;                                                                       // Merker für den nächsten normalen Aufruf löschen
  cursorIdx = initialCursorIdx;                                                               // Start am Dateianfang
  bool isEditing = true;

  int cursorX = 0;
  int cursorY = initialCursorY;
  int logischeZeile = initialLogischeZeile;//1;
  int logischeSpalte = 1;
  int blockStartIdx = -1;                                                                     // -1 Aktuell kein Textblock markiert


  int startLine = initialStartLine;//1;                                                       // Dimension Textfenster
  int maxSichtbareZeilen = MAX_R - 2;
  int startCol = 1;
  int maxSichtbareSpalten = MAX_C;


  auto getCharColorAt = [&](int targetIdx) -> int {                                           // Syntax-Farbe für Zeichen im Puffer ermitteln
    if (targetIdx < 0 || targetIdx >= textLength) return fColor;

    int startIdx = targetIdx;                                                                 // Anfang der aktuellen Zeile finden, für den Lexer
    while (startIdx > 0 && editorBuffer[startIdx - 1] != '\n') {
      startIdx--;
    }

    int charIdx = startIdx;                                                                   // Zeile bis zum Ziel-Index analysieren
    bool insideString = false;
    bool insideComment = false;
    char quoteChar = '\0';
    int detectedColor = fColor;

    while (charIdx <= targetIdx) {
      char ch = editorBuffer[charIdx];
      detectedColor = fColor;                                                                 // Standard

      if (!insideString && !insideComment && ch == '-' && editorBuffer[charIdx + 1] == '-') { // Kommentare prüfen
        insideComment = true;
      }
      if (insideComment) {
        detectedColor = 24;                                                                   // GREEN(Kommentare)
      }

      else if (!insideComment) {                                                              // Strings prüfen
        if ((ch == '"' || ch == '\'') && !insideString) {
          insideString = true; quoteChar = ch; detectedColor = 240;
        } else if (insideString && ch == quoteChar) {
          insideString = false; detectedColor = 240;
        } else if (insideString) {
          detectedColor = 240;                                                                // ORANGE(Strings)
        }
      }

      if (!insideString && !insideComment) {
        bool wordStart = (charIdx == startIdx || isspace((unsigned char)editorBuffer[charIdx - 1]) || ispunct((unsigned char)editorBuffer[charIdx - 1]));
        if (wordStart) {
          const char* primaryKeywords[] = {"function", "local", "if", "then", "else", "elseif", "end", "for",   // Test auf Lua-Keywords
                                           "while", "do", "return", "break", "true", "false", "nil", "and", "or", "not", "in", "repeat", "until", 
                                           "require","package"
                                          };

          for (const char* kw : primaryKeywords) {
            size_t kwLen = strlen(kw);
            if (strncmp(&editorBuffer[charIdx], kw, kwLen) == 0) {
              char nextChar = editorBuffer[charIdx + kwLen];
              if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                if (targetIdx >= charIdx && targetIdx < (int)(charIdx + kwLen)) {
                  return 196;                                                                 // RED (Lua-Keyword)
                }
              }
            }
          }


          const char* systemKeywords[] = {"sys", "vga", "sd", "math",  "hardware_control", "sprite", "delay", "print", "type",   // Test auf System-Keywords
                                          "pairs", "ipairs", "tostring", "tonumber", "error", "assert", "sqrt", "sin", "cos", "tan", "abs",
                                          "floor", "ceil", "random", "min", "max", "pi", "inkey", "run", "write", "waitkey", "edit", "flash"
                                         };

          for (const char* skw : systemKeywords) {
            size_t skwLen = strlen(skw);
            if (strncmp(&editorBuffer[charIdx], skw, skwLen) == 0) {
              char nextChar = editorBuffer[charIdx + skwLen];
              if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                if (targetIdx >= charIdx && targetIdx < (int)(charIdx + skwLen)) {
                  return 127;                                                                 //CYAN System-Keyword
                }
              }
            }
          }
        }
      }

      charIdx++;
    }
    return detectedColor;
  };


  auto berechneLogischePosition = [&]() {                         //Berechnet die Textposition
    int tempZeile = 1;
    int tempSpalte = 1;
    for (int i = 0; i <= textLength; i++) {
      if (i == cursorIdx) {
        logischeZeile = tempZeile;
        logischeSpalte = tempSpalte;
        break;
      }
      char ch = editorBuffer[i];
      if (ch == '\n') {
        tempZeile++;
        tempSpalte = 1;
      }
      else if (ch != '\0') {
        tempSpalte++;

      }
    }

    if (logischeZeile >= startLine + maxSichtbareZeilen) {        // Vertikales Scrolling
      startLine = logischeZeile - maxSichtbareZeilen + 1;
    }
    if (logischeZeile < startLine) {
      startLine = logischeZeile;
    }

    if (logischeSpalte >= startCol + maxSichtbareSpalten) {       // Horizontales Scrolling
      startCol = logischeSpalte - maxSichtbareSpalten + 1;
    }
    if (logischeSpalte < startCol) {
      startCol = logischeSpalte;
    }
  };


  auto redrawScreen = [&]() {                                      // Textfenster ausgeben
    berechneLogischePosition();

    vga.drawRect(0, 16, MAX_C * 8, (MAX_R - 2) * 8, bColor);       // Textfeld komplett löschen


    char statusBuf[128];                                           // Obere Statuszeile
    float kbSize = (float)textLength / 1024.0f;
    snprintf(statusBuf, sizeof(statusBuf), " LUA-EDIT | Ln: %-3d Col: %-2d | Size: %.1f KB | Datei: %s", logischeZeile, logischeSpalte, kbSize, filename.c_str());
    int len = strlen(statusBuf); while (len < MAX_C) {
      statusBuf[len++] = ' ';
    } statusBuf[len] = '\0';
    vga.drawText(0, 0, statusBuf, bColor, fColor, false);


    char infoBuf[128];                                              // Untere Infoleiste
    snprintf(infoBuf, sizeof(infoBuf), " F1:Save/Exit  F2:Find  F3:Copy  F4:Paste  ESC:Abort ");
    int infoLen = strlen(infoBuf); while (infoLen < MAX_C) {
      infoBuf[infoLen++] = ' ';
    } infoBuf[infoLen] = '\0';
    int infoY = (MAX_R) * 8;
    vga.drawText(0, infoY, infoBuf, bColor, fColor, false);


    int currentY = 16;                                              // Text zeilenweise ausgeben
    int aktuelleDruckZeile = 1;
    char* lineStart = editorBuffer;

    while (*lineStart && currentY < infoY) {
      char* lineEnd = strchr(lineStart, '\n');
      if (lineEnd != NULL) *lineEnd = '\0';                         // Temporär kappen


      if (aktuelleDruckZeile >= startLine && aktuelleDruckZeile < startLine + maxSichtbareZeilen) { // Textzeile im Sichtfenster?
        int zeilenLaenge = strlen(lineStart);


        if (zeilenLaenge >= startCol) {                             // Hat die Zeile genug Zeichen für horizontalen Scroll-Ausschnitt?
          char* sichtbarerText = &lineStart[startCol - 1];

          char tempRowBuf[128];
          strncpy(tempRowBuf, sichtbarerText, maxSichtbareSpalten);
          tempRowBuf[maxSichtbareSpalten] = '\0';


          int printX = 0;                                           // SYNTAX HIGHLIGHTING (Zeichenweise)
          int charIdx = 0;
          bool insideString = false;
          bool insideComment = false;
          char quoteChar = '\0';

          while (tempRowBuf[charIdx] != '\0') {
            char ch = tempRowBuf[charIdx];
            int activeColor = fColor;

            if (!insideString && !insideComment && ch == '-' && tempRowBuf[charIdx + 1] == '-') {
              insideComment = true;
            }
            if (insideComment) {
              activeColor = 24;  // GREEN
            }
            else if (!insideComment) {
              if ((ch == '"' || ch == '\'') && !insideString) {
                insideString = true;
                quoteChar = ch;
                activeColor = 240;
              }
              else if (insideString && ch == quoteChar) {
                insideString = false;
                activeColor = 240;
              }
              else if (insideString) {
                activeColor = 240;  // ORANGE
              }
            }

            if (!insideString && !insideComment) {
              bool wordStart = (charIdx == 0 || isspace((unsigned char)tempRowBuf[charIdx - 1]) || ispunct((unsigned char)tempRowBuf[charIdx - 1]));
              if (wordStart) {
                const char* primaryKeywords[] = {"function", "local", "if", "then", "else", "elseif", "end",
                                                 "for", "while", "do", "return", "break", "true", "false", "nil", "and", "or", "not", "in",
                                                 "repeat", "until", "require","package"
                                                };

                for (const char* kw : primaryKeywords) {
                  size_t kwLen = strlen(kw);
                  if (strncmp(&tempRowBuf[charIdx], kw, kwLen) == 0) {
                    char nextChar = tempRowBuf[charIdx + kwLen];
                    if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                      vga.drawText(printX, currentY, kw, 196, bColor, false); // RED
                      printX += kwLen * 8; charIdx += kwLen - 1; goto char_processed;
                    }
                  }
                }

                const char* systemKeywords[] = {"sys", "vga", "sd", "math", "hardware_control", "sprite", "delay", "print",
                                                "type", "pairs", "ipairs", "tostring", "tonumber", "error", "assert", "sqrt", "sin",
                                                "cos", "tan", "abs", "floor", "ceil", "random", "min", "max", "pi", "inkey", "run", 
                                                "write", "waitkey", "edit", "flash"
                                               };

                for (const char* skw : systemKeywords) {
                  size_t skwLen = strlen(skw);
                  if (strncmp(&tempRowBuf[charIdx], skw, skwLen) == 0) {
                    char nextChar = tempRowBuf[charIdx + skwLen];
                    if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                      vga.drawText(printX, currentY, skw, 127, bColor, false); // CYAN
                      printX += skwLen * 8; charIdx += skwLen - 1; goto char_processed;
                    }
                  }
                }
              }
            }

            {
              char singleCharStr[2] = { ch, '\0' };
              vga.drawText(printX, currentY, singleCharStr, activeColor, bColor, false);
              printX += 8;
            }
char_processed:
            charIdx++;
          }
        }

        if (aktuelleDruckZeile == logischeZeile) {                                      // Cursorkoordinaten relativ zum Scroll-Fenster bestimmen
          cursorX = (logischeSpalte - startCol) * 8;
          cursorY = currentY;
        }
        currentY += 8;
      } else if (aktuelleDruckZeile == logischeZeile) {
        cursorX = (logischeSpalte - startCol) * 8;
        cursorY = currentY;
      }

      if (lineEnd != NULL) *lineEnd = '\n'; // Wiederherstellen
      if (lineEnd == NULL) break;
      lineStart = lineEnd + 1;
      aktuelleDruckZeile++;
    }

    if (textLength == 0) {
      cursorX = 0;
      cursorY = 16;
    }

    if (cursorY >= 16 && cursorY < infoY && cursorX >= 0 && cursorX < (MAX_C * 8)) {    // --- CURSOR Ausgeben ---
      char retroCursor[2] = { 127, '\0' };
      vga.drawText(cursorX, cursorY, retroCursor, edit_cur_col, bColor, false);
    }
  };


  auto updateCursorPositionWithWeiche = [&]() {                                 // Berechnung der Position nach Bewegung, Prüfung auf Scrolling
    int alterStartLine = startLine;
    int alterStartCol = startCol;

    berechneLogischePosition();

    if (startLine != alterStartLine || startCol != alterStartCol) {             // Wenn gescrollt wird,Fenster Neuzeichnen
      redrawScreen();
    } else {

      cursorX = (logischeSpalte - startCol) * 8;                                // Wenn Ausschnitt gleich bleibt, nur den Cursor-Block zeichnen
      cursorY = 16 + (logischeZeile - startLine) * 8;


      char statusBuf[128];                                                      // Update Zahlen in oberer Statusleiste
      float kbSize = (float)textLength / 1024.0f;
      snprintf(statusBuf, sizeof(statusBuf), " LUA-EDIT | Ln: %-3d Col: %-2d | Size: %.1f KB | Datei: %s", logischeZeile, logischeSpalte, kbSize, filename.c_str());
      int len = strlen(statusBuf); while (len < MAX_C) {
        statusBuf[len++] = ' ';
      } statusBuf[len] = '\0';
      vga.drawText(0, 0, statusBuf, bColor, fColor, false);

      int infoY = (MAX_R) * 8;
      if (cursorY >= 16 && cursorY < infoY && cursorX >= 0 && cursorX < (MAX_C * 8)) {
        char retroCursor[2] = { 127, '\0' };
        vga.drawText(cursorX, cursorY, retroCursor, edit_cur_col, bColor, false);
      }
    }
  };


  redrawScreen();                                                               // Bildschirm komplett zeichnen
  lastUsbChar = -1;


  while (isEditing) {                                                           // Editor-Hauptschleife
    tastenCode = inchar();
    if (tastenCode == -1) {
      delay(5);
      continue;
    }

    switch (tastenCode) {

      case 27:  // ESC                                                          //----------- ESC ---------------------
        isEditing = false;
        gemerkte_stelle = cursorIdx;                                            //letzte bearbeitete Position merken
        gemerkte_zeile = logischeZeile;
        //editorStartZeile = 1;                                                   //Fehlerposition zurücksetzen
        break;

      case 194: // F1: Speichern auf SD-Karte                                   //----------- F1 ----------------------
        if (filename.length() > 0) {
          SD.remove(filename.c_str());
          File file = SD.open(filename.c_str(), FILE_WRITE);
          if (file) {
            file.print(editorBuffer); file.close();
            vga.clear(bColor);
            vga_print_str("Datei gespeichert.\r\n");
            delay(800);
          }
        } 
        isEditing = false;
        gemerkte_stelle = cursorIdx;                                            //letzte bearbeitete Position merken
        gemerkte_zeile = logischeZeile;
        //editorStartZeile = 1;                                                   //Fehlerposition zurücksetzen
        break;

      case 195: // --- F2-TASTE: SUCHEN (FIND) ---                              //------------- F2 --------------------
        {
          // 1. Obere Statuszeile für die Eingabe temporär löschen
          vga.drawRect(0, 0, MAX_C * 8, 8, bColor);
          vga.drawText(0, 0, " SUCHEN: ", bColor, fColor, false);

          char suchPuffer[32];
          memset(suchPuffer, 0, sizeof(suchPuffer));
          int sLen = 0;
          bool sEditing = true;

          while (sEditing) {                                                    // Eingabeschleife für Suchbegriff
            int sKey = inchar();
            if (sKey == 13 || sKey == 10) {                                     // ENTER -> Suche starten
              sEditing = false;
            } else if (sKey == 27) {                                            // ESC -> Abbrechen
              suchPuffer[0] = '\0';
              sEditing = false;
            } else if ((sKey == 8 || sKey == 127) && sLen > 0) {                // Backspace
              sLen--;
              suchPuffer[sLen] = '\0';
              vga.drawRect(64, 0, (MAX_C - 8) * 8, 8, bColor);
              vga.drawText(64, 0, suchPuffer, bColor, fColor, false);
            } else if (sKey >= 32 && sKey <= 126 && sLen < 30) {                // normales Zeichen
              suchPuffer[sLen++] = (char)sKey;
              suchPuffer[sLen] = '\0';
              vga.drawText(64, 0, suchPuffer, bColor, fColor, false);
            }
            delay(10);
          }


          if (strlen(suchPuffer) > 0) {                                         // Suche im PSRAM-Puffer

            char* fundStelle = strstr(&editorBuffer[cursorIdx + 1], suchPuffer);// Sucht ab der aktuellen Position nach vorne (Find Next)

            if (fundStelle == NULL) {
              fundStelle = strstr(editorBuffer, suchPuffer);                    // am ende angekommen, dann suche vom Anfang des Skripts
            }

            if (fundStelle != NULL) {

              cursorIdx = (int)(fundStelle - editorBuffer);                     // Speicheradresse in logischen Puffer-Index umrechnen
              blockStartIdx = -1;                                               // Eventuelle Markierung aufheben
            } else {

              int infoY = MAX_R * 8;                                            // Fehler unten rechts ausgeben
              vga.drawText(60 * 8, infoY, "[NICHT GEFUNDEN]", bColor, fColor, false);
              delay(1000);
            }
          }
          redrawScreen();                                                       // Fenster neu zeichnen
        }
        break;

      case 196: // --- F3-TASTE: BLOCK MARKIEREN ODER KOPIEREN ---               //--------------- F3 ---------------------
        {
          int infoY = MAX_R * 8;

          if (blockStartIdx == -1) {
            blockStartIdx = cursorIdx;                                           // Block-Startpunkt auf aktuellen logischen Index setzen

            vga.drawText(60 * 8, infoY, "[MARK: START]", bColor, fColor, false); // Rückmeldung unten rechts
            delay(1000);
          } else {

            int start = (blockStartIdx < cursorIdx) ? blockStartIdx : cursorIdx;  // Endpunkt setzen und kopieren
            int ende = (blockStartIdx < cursorIdx) ? cursorIdx : blockStartIdx;
            int blockLaenge = ende - start;

            if (blockLaenge > 0 && blockLaenge < CLIPBOARD_SIZE - 2) {
              strncpy(editorClipboard, &editorBuffer[start], blockLaenge);
              editorClipboard[blockLaenge] = '\0'; // Sicher terminieren

              vga.drawText(60 * 8, infoY, "[BLOCK KOPIERT]", bColor, fColor, false);
              delay(1000);
            }
          }
          redrawScreen();
        }
        break;

      case 197: // --- F4-TASTE: BLOCK EINFÜGEN (Paste) ---                       //-------------- F4 ---------------------
        {
          int clipLen = strlen(editorClipboard);


          if (clipLen > 0 && (textLength + clipLen) < EDITOR_MAX_SIZE - 2) {      // Nur einfügen, wenn was im Clipboard ist

            memmove(&editorBuffer[cursorIdx + clipLen], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);  // Platz schaffen im Text

            memcpy(&editorBuffer[cursorIdx], editorClipboard, clipLen);           // Daten aus der Zwischenablage einfügen

            cursorIdx += clipLen;                                                 // Index und Länge um die Blockgröße nach vorne schieben
            textLength += clipLen;

            blockStartIdx = -1;                                                   // Markierung aufheben
            redrawScreen();                                                       // Kompletten Bildschirm neu zeichnen
          }
        }
        break;


      case 216:                                                                   //----------- Pfeil links --------------
        if (cursorIdx > 0) {
          int korrekteFarbe = getCharColorAt(cursorIdx);                          // Zeichenfarbe-Farbe für die Wiederherstellung ermitteln
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);
          cursorIdx--;
          updateCursorPositionWithWeiche();
        }
        break;

      case 215:                                                                   //----------- Pfeil rechts -------------
        if (cursorIdx < textLength) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);
          cursorIdx++;
          updateCursorPositionWithWeiche();
        }
        break;

      case 218:                                                                   //----------- Pfeil hoch ---------------
        if (cursorIdx > 0) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          int targetCol = 0; int i = cursorIdx;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            targetCol++;
            i--;
          }
          if (i > 0 && editorBuffer[i - 1] == '\n') i--;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            i--;
          }
          int currentCol = 0;
          while (editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
            currentCol++;
            i++;
          }
          cursorIdx = i;
          updateCursorPositionWithWeiche();
        }
        break;

      case 217:                                                                   //------------ Pfeil runter ---------------
        if (cursorIdx < textLength) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          int targetCol = 0; int i = cursorIdx;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            targetCol++;
            i--;
          }
          i = cursorIdx;
          while (i < textLength && editorBuffer[i] != '\n') i++;
          if (i < textLength && editorBuffer[i] == '\n') i++;
          int currentCol = 0;
          while (i < textLength && editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
            currentCol++;
            i++;
          }

          cursorIdx = i;
          updateCursorPositionWithWeiche();
        }
        break;

      case 210:                                                                     //------------- Home -----------------
        while (cursorIdx > 0 && editorBuffer[cursorIdx - 1] != '\n')
        {
          cursorIdx--;
        }
        updateCursorPositionWithWeiche();
        break;

      case 213:                                                                     //-------------- END -----------------
        while (cursorIdx < textLength && editorBuffer[cursorIdx] != '\n')
        {
          cursorIdx++;
        }
        updateCursorPositionWithWeiche();
        break;

      case 13:                                                                      //-------------- ENTER ---------------
      case 10:
        if (textLength < EDITOR_MAX_SIZE - 2)
        { memmove(&editorBuffer[cursorIdx + 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = '\n';
          cursorIdx++;
          textLength++;
          cursorX = 0;              //+++++ Änderung für letzte Zeile
          logischeZeile += 1;       //+++++ Änderung für letzte Zeile
          updateCursorPositionWithWeiche();
          redrawScreen();
        }
        break;

      case 9: // Fügt 2 Leerzeichen ein                                             //-------------- TAB -----------------
        if (textLength < EDITOR_MAX_SIZE - 3)
        { memmove(&editorBuffer[cursorIdx + 2], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = ' ';
          editorBuffer[cursorIdx + 1] = ' ';
          cursorIdx += 2;
          textLength += 2;
          redrawScreen();
        }
        break;

      case 8:                                                                       //------------ BACKSPACE -------------
      case 127:
        if (cursorIdx > 0)
        { memmove(&editorBuffer[cursorIdx - 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          cursorIdx--;
          textLength--;
          updateCursorPositionWithWeiche();
          redrawScreen();
        }
        break;

      case 212: // --- ENTF / DELETE: Zeichen o. ganzen Block löschen ---           //------------ DELETE ----------------
        {
          if (blockStartIdx == -1) {
            // Herkömmliches Verhalten: Einzelnes Zeichen unter dem Cursor löschen
            if (cursorIdx < textLength) {
              memmove(&editorBuffer[cursorIdx], &editorBuffer[cursorIdx + 1], textLength - cursorIdx);
              textLength--;
              redrawScreen();
            }
          } else {
            int start = (blockStartIdx < cursorIdx) ? blockStartIdx : cursorIdx;   // MARKIERTEN BLOCK LÖSCHEN
            int ende = (blockStartIdx < cursorIdx) ? cursorIdx : blockStartIdx;
            int blockLaenge = ende - start;

            if (blockLaenge > 0) {
              memmove(&editorBuffer[start], &editorBuffer[ende], textLength - ende + 1); // Text im PSRAM zusammenziehen
              textLength -= blockLaenge;
              cursorIdx = start;                                                    // Cursor an die Schnittstelle setzen

              int infoY = MAX_R * 8;
              vga.drawText(60 * 8, infoY, "[BLOCK ENTFERNT]", bColor, fColor, false);
              delay(1000);
            }
            blockStartIdx = -1;                                                     // Markierung zurücksetzen
            redrawScreen();                                                         // Fenster neu aufbauen
          }
        }
        break;

      case 211: // --- PAGE UP / BILD AUF ---                                       //------------ PAGE-UP ----------------
        {
          for (int z = 0; z < maxSichtbareZeilen; z++) {                            // versuchen, den Cursor um 'SichtbareZeilen' nach oben zu bewegen
            int targetCol = 0; int i = cursorIdx;
            while (i > 0 && editorBuffer[i - 1] != '\n') {
              targetCol++;
              i--;
            }
            if (i > 0 && editorBuffer[i - 1] == '\n') i--;
            while (i > 0 && editorBuffer[i - 1] != '\n') i--;
            int currentCol = 0;
            while (editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
              currentCol++;
              i++;
            }
            cursorIdx = i;
          }
          redrawScreen();                                                           // Fenster neu aufbauen
        }
        break;

      case 214: // --- PAGE DOWN / BILD AB ---                                      //-------------- PAGE-DOWN -------------
        {
          for (int z = 0; z < maxSichtbareZeilen; z++) {                            // versuchen, den Cursor um 'SichtbareZeilen' nach unten zu bewegen
            int targetCol = 0; int i = cursorIdx;
            while (i > 0 && editorBuffer[i - 1] != '\n') {
              targetCol++;
              i--;
            }
            i = cursorIdx; while (i < textLength && editorBuffer[i] != '\n') i++;
            if (i < textLength && editorBuffer[i] == '\n') i++;
            int currentCol = 0;
            while (i < textLength && editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
              currentCol++;
              i++;
            }
            cursorIdx = i;
          }
          redrawScreen();                                                           // Fenster neu aufbauen
        }
        break;

      default:                                                                      //------- normale Zeichen ------------
        if (tastenCode >= 32 && tastenCode <= 126 && textLength < EDITOR_MAX_SIZE - 2)
        { memmove(&editorBuffer[cursorIdx + 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = (char)tastenCode;
          cursorIdx++;
          textLength++;
          redrawScreen();
        }
        break;
    }
  }
  fColor = tmp_fcolor;                                                              //Bildschirm und Textfarbe wieder auf Ursprung zurücksetzen
  bColor = tmp_bcolor;

  vga.clear(bColor);
}


//****************************************** LUA-Brückenfunktion **********************************************
// ============================================================================
// IN/OUT/PWM/AIN HARDWARE INTERFACE
// ============================================================================
FLASHMEM int lua_hardware_control(lua_State* L) {
  // 1. Parameter-Prüfung: Wir erwarten 3 Argumente (Zahl, String, String/Zahl)
  if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
    zeigeFehlerPopup("FEHLER", "hardware_control(pin, \"MODUS\", \"WERT\")");
    lua_pushboolean(L, false);
    return 0;
  }

  // 2. Werte vom Lua-Stack abholen
  int pin = (int)lua_tonumber(L, 1);
  String modus = lua_tostring(L, 2);

  // 3. Pin-Modus einstellen
  if (modus == "OUTPUT") {
    pinMode(pin, OUTPUT);
  } else if (modus == "INPUT") {
    pinMode(pin, INPUT);
  } else if (modus == "INPUT_PULLUP") {
    pinMode(pin, INPUT_PULLUP);
  }

  // 4. Wert verarbeiten (falls vorhanden)
  if (lua_gettop(L) >= 3) {
    // Wert kann als String ("HIGH"/"LOW") oder Zahl (0-255 für PWM) kommen
    if (lua_isnumber(L, 3)) {
      int analogWert = (int)lua_tonumber(L, 3);
      analogWrite(pin, analogWert); // Für PWM-Ausgabe (z.B. LEDs dimmen)
    }
    else if (lua_isstring(L, 3)) {
      String wertStr = lua_tostring(L, 3);
      if (wertStr == "HIGH" || wertStr == "1") {
        digitalWrite(pin, HIGH);
      } else if (wertStr == "LOW" || wertStr == "0") {
        digitalWrite(pin, LOW);
      }
    }
  }

  // 5. Wenn der Pin im INPUT-Modus ist, geben wir den aktuellen Zustand an Lua zurück
  if (modus == "INPUT" || modus == "INPUT_PULLUP") {
    int sensorWert = digitalRead(pin);
    lua_pushinteger(L, sensorWert); // Schiebt das Ergebnis (0 oder 1) zurück auf den Lua-Stack
    return 1; // Signalisiert Lua: Es gibt 1 Rückgabewert
  }

  return 0; // Kein Rückgabewert bei OUTPUT
}
//****************************************** LUA-SD - Funktionen **********************************************
// C++ Brücke: Lädt ein Lua-Skript von der SD-Karte direkt in den Interpreter
FLASHMEM int lua_sys_load(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushnil(L);
    lua_pushstring(L, "Dateiname fehlt!");
    return 2;
  }

  String filename = lua_tostring(L, 1);

  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  // Datei auf der SD-Karte prüfen
  if (SD.exists(filename.c_str())) {
    File file = SD.open(filename.c_str(), FILE_READ);
    if (file) {
      size_t fileSize = file.size();

      // Speicher im RAM fuer das Skript reservieren
      char* buffer = (char*)malloc(fileSize + 1);
      if (buffer) {
        file.readBytes(buffer, fileSize);
        buffer[fileSize] = '\0';
        file.close();

        // Den geladenen Text an Lua uebergeben (Erzeugt eine Funktion auf dem Stack)
        int status = luaL_loadbuffer(L, buffer, fileSize, filename.c_str());
        free(buffer); // Puffer sofort freigeben, um RAM zu schonen

        if (status == LUA_OK) {
          lua_pushnil(L);

          return 2; // WICHTIG: Wir geben jetzt 2 Werte an Lua zurueck! (chunk, nil)
        } else {
          // Ein Syntaxfehler liegt vor (z.B. Tippfehler in Ihrer clock.lua)
          const char* err = lua_tostring(L, -1);
          zeigeFehlerPopup(" -> LUA SYNTAXFEHLER IN DATEI: ", err);
          lua_pushnil(L);
          lua_pushstring(L, err);
          return 2;
        }
      }
      file.close();
    }
  }

  // Wenn die Datei physisch nicht existiert oder der Pfad falsch geschrieben ist
  zeigeFehlerPopup("Fehler", "Nicht auf SD-Karte vorhanden!\n\r");
  lua_pushnil(L);
  lua_pushstring(L, "Datei existiert nicht!");
  return 2;
}

// ============================================================================
// WILDCARD HILFSFUNKTION (Für sd.ls-Filterung)
// ============================================================================

FLASHMEM bool wildcard_match(const char* pattern, const char* str) {
  while (*pattern) {
    if (*pattern == '*') {
      if (!*(++pattern)) return true; // Ein einzelner Stern am Ende passt auf alles restliche
      while (*str) {
        if (wildcard_match(pattern, str)) return true;
        str++;
      }
      return false;
    } else if (*pattern == '?') {
      if (!*str) return false;
      pattern++; str++;
    } else {
      if (tolower((unsigned char)*pattern) != tolower((unsigned char)*str)) return false;
      pattern++; str++;
    }
  }
  return !*pattern && !*str;
}

// ============================================================================
// SD-KARTEN INTERFACE (Modul: sd)
// ============================================================================
// 1. Datei mit Wildcard suchen sd.ls("*lua")
FLASHMEM int lua_sd_ls(lua_State* L) {
  String searchPattern = "*"; // Standardmäßig alles anzeigen
  String path = currentWorkDir;

  // 1. Parameter auswerten
  if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
    String arg = lua_tostring(L, 1);

    // Prüfen, ob das Argument ein Wildcard (* oder ?) enthält
    if (arg.indexOf('*') != -1 || arg.indexOf('?') != -1) {
      int lastSlash = arg.lastIndexOf('/');
      if (lastSlash != -1) {
        path = arg.substring(0, lastSlash + 1);
        searchPattern = arg.substring(lastSlash + 1);
      } else {
        path = currentWorkDir;
        searchPattern = arg;
      }
    } else {
      // Wenn ein Ordnerpfad ohne Wildcard übergeben wurde, jagen wir ihn durch den Pfad-Resolver
      path = resolve_lua_path(arg);
      if (!path.endsWith("/")) path += "/";
    }
  }

  File dir = SD.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    lua_pushboolean(L, false);
    return 1;
  }

  // Header ausgeben (zeigt nun auch den aktiven Filter)
  vga_print_str("--- Verzeichnis: ");
  vga_print_str(path.c_str());
  if (searchPattern != "*") {
    vga_print_str(" [Filter: ");
    vga_print_str(searchPattern.c_str());
    vga_print_str("]");
  }
  vga_print_str(" ---\n\r");

  vga_print_str("Name                     Typ      Groesse\n\r");
  vga_print_str("-----------------------------------------------\n\r");

  int zeilenZaehler = 3;

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    const char* rawName = entry.name();

    // 2. SYSTEM-FILTER: Unsichtbare Dateien immer ausblenden
    if (rawName[0] == '.' ||
        strcasecmp(rawName, "System Volume Information") == 0 ||
        strcasecmp(rawName, "FOUND.000") == 0 ||
        strncasecmp(rawName, "._", 2) == 0) {
      entry.close();
      continue;
    }

    // 3. WILDCARD-FILTER: Prüfen, ob die Datei auf das Suchmuster passt
    // (Ordner werden vom Filter ausgenommen und immer angezeigt, um navigieren zu können)
    if (!entry.isDirectory() && !wildcard_match(searchPattern.c_str(), rawName)) {
      entry.close();
      continue; // Passt nicht zum Filter -> Überspringen
    }

    // 4. BEGRENZUNG: Dateinamen formatieren und bei Bedarf kürzen
    char displayName[25];
    size_t nameLen = strlen(rawName);

    if (nameLen > 20) {
      strncpy(displayName, rawName, 18);
      displayName[18] = '.';
      displayName[19] = '.';
      displayName[20] = '\0';
    } else {
      strcpy(displayName, rawName);
    }

    // Zeilenpuffer für das saubere Spaltenlayout
    char spaltenBuf[80];
    if (entry.isDirectory()) {
      snprintf(spaltenBuf, sizeof(spaltenBuf), "%-24s <DIR>    ---", displayName);
    } else {
      char groesseStr[20];
      snprintf(groesseStr, sizeof(groesseStr), "%lu Bytes", (unsigned long)entry.size());
      snprintf(spaltenBuf, sizeof(spaltenBuf), "%-24s FILE     %s", displayName, groesseStr);
    }

    // Zeile auf VGA ausgeben
    vga_print_str(spaltenBuf);
    vga_print_str("\n\r");
    entry.close();

    zeilenZaehler++;

    // Seitenumbruch-Logik (MAX_R - 4)
    if (zeilenZaehler >= MAX_R - 4) {
      vga_print_str("-- WEITER MIT TASTE | ESC ZUM ABBRECHEN --\r");
      int taste = wait_key(1);
      delay(150);
      lastUsbChar = -1;
      vga_print_str("                                          \r");

      if (taste == 27) { // ESC gedrückt
        dir.close();
        lua_pushboolean(L, true);
        return 1;
      }
      zeilenZaehler = 0;
    }
  }

  dir.close();
  vga_print_str("-----------------------------------------------\n\r");

  lua_pushboolean(L, true);
  return 1;
}

// 2. NEU: C++ Funktion für sd.cd("pfad") in Lua
FLASHMEM int lua_sd_cd(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Pfad (String) erwartet! Nutzen Sie: sd.cd(\"/neuer_pfad\")"));
    lua_pushboolean(L, false);
    return 1; // KORREKTUR: Muss 1 sein, da wir oben ein boolean pushen!
  }

  String eingabePfad = lua_tostring(L, 1);
  String zielPfad = currentWorkDir; // Wir starten mit dem aktuellen Pfad als Basis

  // ====================================================================
  // SCENARIO 1: ABSOLUTER PFADWECHSEL (Eingabe startet mit '/')
  // ====================================================================
  if (eingabePfad.startsWith("/")) {
    zielPfad = eingabePfad;
  }
  // ====================================================================
  // SCENARIO 2: EBENE NACH OBEN SPRINGEN ("..")
  // ====================================================================
  else if (eingabePfad == "..") {
    if (zielPfad != "/") {
      // Falls am Ende ein trennender Slash ist, schneiden wir ihn kurz ab
      if (zielPfad.endsWith("/") && zielPfad.length() > 1) {
        zielPfad.remove(zielPfad.length() - 1);
      }
      // Wir suchen den letzten Schrägstrich von hinten
      int letzterSlash = zielPfad.lastIndexOf('/');
      if (letzterSlash > 0) {
        zielPfad = zielPfad.substring(0, letzterSlash); // Letzten Ordner abschneiden
      } else {
        zielPfad = "/"; // Keine Slashes mehr da -> Zurück ins Hauptverzeichnis
      }
    }
  }
  // ====================================================================
  // SCENARIO 3: RELATIVER PFADWECHSEL (In einen Unterordner hineingehen)
  // ====================================================================
  else {
    if (!zielPfad.endsWith("/")) {
      zielPfad += "/";
    }
    zielPfad += eingabePfad;
  }

  // SOWIESO-KORREKTUR: Wir stellen sicher, dass jeder gültige Ordner-Pfad 
  // am Ende sauber mit exakt einem '/' abschließt. Das schützt Ihren Resolver!
  if (!zielPfad.endsWith("/")) {
    zielPfad += "/";
  }

  // Doppelte Schrägstriche im fertigen String eliminieren (Sicherheitsanker)
  while (zielPfad.indexOf("//") != -1) {
    zielPfad.replace("//", "/");
  }

  // ====================================================================
  // HARDWARE-CHECK UND SPEICHERUNG
  // ====================================================================
  // Wir prüfen, ob der komplett neu zusammengesetzte Pfad physisch existiert
  if (SD.exists(zielPfad.c_str())) {
    currentWorkDir = zielPfad; // Erst jetzt schalten wir den Kompass um!
    
    vga_print_str(PSTR("Arbeitsverzeichnis geaendert auf: "));
    vga_print_str(currentWorkDir.c_str());
    vga_print_str(PSTR("\n\r"));
    
    lua_pushboolean(L, true);
  } else {
    zeigeFehlerPopup(PSTR("FEHLER"), PSTR("Verzeichnis existiert nicht!\n\r"));
    lua_pushboolean(L, false);
  }

  return 1;
}

// 3. Datei löschen: sd.remove("datei.lua")
FLASHMEM int lua_sd_remove(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    zeigeFehlerPopup("FEHLER", "Argument muss ein Dateiname (String) sein!");
    lua_pushboolean(L, false);
    return 0;
  }
  String filename = resolve_lua_path(lua_tostring(L, 1));
  bool success = SD.remove(filename.c_str());

  lua_pushboolean(L, success);
  return 1;
}

// 4. Ordner erstellen: sd.mkdir("ordnername")
FLASHMEM int lua_sd_mkdir(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    zeigeFehlerPopup("FEHLER", "Argument muss ein Ordnername (String) sein!");
    lua_pushboolean(L, false);
    return 0;
  }
  String dirname = resolve_lua_path(lua_tostring(L, 1));
  bool success = SD.mkdir(dirname.c_str());

  lua_pushboolean(L, success);
  return 1;
}

// 5. Ordner löschen: sd.rmdir("ordnername")
FLASHMEM int lua_sd_rmdir(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    zeigeFehlerPopup("FEHLER", "Argument muss ein Ordnername (String) sein!");
    lua_pushboolean(L, false);
    return 0;
  }
  String dirname = resolve_lua_path(lua_tostring(L, 1));
  bool success = SD.rmdir(dirname.c_str());

  lua_pushboolean(L, success);
  return 1;
}



// 6. sd.copy("quelle.lua", "ziel.lua") - Kopiert eine Datei im aktuellen Arbeitsverzeichnis
FLASHMEM int lua_sd_copy(lua_State* L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    zeigeFehlerPopup("FEHLER", "Zwei Dateinamen (Strings) erwartet! Nutzen Sie: sd.copy(\"von.lua\", \"nach.lua\")");
    lua_pushboolean(L, false);
    return 0;
  }

  String vonPfad = resolve_lua_path(lua_tostring(L, 1));    // Pfade, Arbeitsverzeichnis automatisch ergänzen
  String nachPfad = resolve_lua_path(lua_tostring(L, 2));

  if (!SD.exists(vonPfad.c_str())) {              // Quelldatei vorhanden?
    zeigeFehlerPopup("FEHLER", "Quelldatei existiert nicht!\n\r");
    lua_pushboolean(L, false);
    return 1;
  }

  // 3. Kopier-Vorgang starten
  File sourceFile = SD.open(vonPfad.c_str(), FILE_READ);
  if (!sourceFile) {
    zeigeFehlerPopup("FEHLER", "Konnte Quelldatei nicht oeffnen!\n\r");
    lua_pushboolean(L, false);
    return 1;
  }

  if (SD.exists(nachPfad.c_str())) {              // Falls die Zieldatei existiert,ueberschreiben
    SD.remove(nachPfad.c_str());
  }

  File destFile = SD.open(nachPfad.c_str(), FILE_WRITE);
  if (!destFile) {
    zeigeFehlerPopup("FEHLER", "Konnte Zieldatei nicht erstellen!\n\r");
    sourceFile.close();
    lua_pushboolean(L, false);
    return 1;
  }

  const size_t bufferSize = 512;                  // Blockweise kopieren
  uint8_t buffer[bufferSize];

  while (sourceFile.available() > 0) {
    size_t bytesRead = sourceFile.read(buffer, bufferSize);
    destFile.write(buffer, bytesRead);
  }
  destFile.close();
  sourceFile.close();

  vga_print_str("Datei erfolgreich kopiert.\n\r");
  lua_pushboolean(L, true);
  return 1;
}
// 7. sd.rename("name.alt",name.neu") - Datei umbenennen
FLASHMEM int lua_sd_rename(lua_State* L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    zeigeFehlerPopup("FEHLER", "Zwei Dateinamen (Strings) erwartet! Nutzen Sie: sd.rename(\"von.lua\", \"nach.lua\")");
    lua_pushboolean(L, false);
    return 0;
  }
  String vonPfad = resolve_lua_path(lua_tostring(L, 1));    // Pfade, Arbeitsverzeichnis automatisch ergänzen
  String nachPfad = resolve_lua_path(lua_tostring(L, 2));

  bool erfolg = SD.rename(vonPfad.c_str(), nachPfad.c_str());
  lua_pushboolean(L, erfolg);                     // Ergebnis (true/false) zurück an Lua geben

  return 1;
}
// 8. sd.exist("dateiname") - prüft,ob eine Datei existiert
FLASHMEM int lua_sd_exists(lua_State* L) {
  String Pfad = resolve_lua_path(lua_tostring(L, 1));
  bool existiert = SD.exists(Pfad.c_str());
  // Ergebnis (true/false) an Lua übergeben
  lua_pushboolean(L, existiert);
  return 1;
}

// 9. Funktion für sd.write(dateiname, text) -> Überschreiben
FLASHMEM int lua_sd_write(lua_State* L) {
  const char* dateiname = luaL_checkstring(L, 1);
  const char* text = luaL_checkstring(L, 2);
  String Pfad = resolve_lua_path(dateiname);

  // FILE_WRITE öffnet standardmäßig im Append-Modus.
  // wir müsen die Datei erst löschen, falls sie existiert.
  if (SD.exists(Pfad.c_str())) {
    SD.remove(Pfad.c_str());
  }

  File datei = SD.open(Pfad.c_str(), FILE_WRITE);
  if (datei) {
    datei.print(text);
    datei.close();
    lua_pushboolean(L, true); 
  } else {
    lua_pushboolean(L, false); // Fehler beim Öffnen
  }
  return 1;
}

// 10. Funktion für sd.append(dateiname, text) -> Anhängen
FLASHMEM int lua_sd_append(lua_State* L) {
  const char* dateiname = luaL_checkstring(L, 1);
  const char* text = luaL_checkstring(L, 2);
  String Pfad = resolve_lua_path(dateiname);

  // FILE_WRITE öffnet die Datei auf dem Teensy automatisch am Ende (Append)
  File datei = SD.open(Pfad.c_str(), FILE_WRITE);
  if (datei) {
    datei.print(text);
    datei.close();
    lua_pushboolean(L, true); // Erfolg an Lua melden
  } else {
    lua_pushboolean(L, false); // Fehler beim Öffnen
  }
  return 1;
}
// 11. Funktion für sd.readline(dateiname) -> Zeilenweise lesen
FLASHMEM int lua_sd_read_lines(lua_State* L) {
  const char* dateiname = luaL_checkstring(L, 1);
  String Pfad = resolve_lua_path(dateiname);

  // Datei zum Lesen öffnen
  File datei = SD.open(Pfad.c_str(), FILE_READ);
  if (!datei) {
    lua_pushnil(L); 
    return 1;
  }
  lua_newtable(L);                            // Eine neue, leere Lua-Tabelle für die Zeilen erstellen
  int zeilen_index = 1;
  char puffer[256];

  while (datei.available() > 0) {
    int geleseneBytes = datei.readBytesUntil('\n', puffer, sizeof(puffer) - 1);
    puffer[geleseneBytes] = '\0';
    String zeile = String(puffer);
    
    if (zeile.endsWith("\r")) {             // Entfernt Carriage Return (\r) am Zeilenende, falls vorhanden
      zeile.remove(zeile.length() - 1);
    }

    // Nur echte Zeilen in die Tabelle einfügen
    if (zeile.length() > 0 || datei.available() > 0) {
      lua_pushinteger(L, zeilen_index);     // Zeilen-Index auf den Stack legen
      lua_pushstring(L, zeile.c_str());     // Den Text der Zeile auf den Stack legen
      lua_settable(L, -3);                  // tabelle[zeilen_index] = zeile
      zeilen_index++;
    }
  }
  datei.close();
  return 1; 
}

// 12. Funktion für sd.unmount() -> SD-Karte abmelden
FLASHMEM int lua_sd_unmount(lua_State* L) {
  int tmp_fcolor = fColor;
  // 1. Alle offenen Datei-Handles schließen und Puffer leeren
  SD.sdfs.end();
  fColor = YELLOW;
  vga_print_str("SD-Card abgemeldet. Du kannst die SD-Karte jetzt entfernen.\n");
  fColor = tmp_fcolor;
  lua_pushboolean(L, true);
  return 1;
}

// 13. Funktion für sd.mount() -> SD-Karte anmelden
FLASHMEM int lua_sd_mount() {
  int tmp_fcolor = fColor;
  if (SD.begin(BUILTIN_SDCARD)) {
    fColor = GREEN;
    vga_print_str("SD-Karte erfolgreich angemeldet.\n");
    fColor = WHITE;
    lua_pushboolean(L, true);
  } else {
    zeigeFehlerPopup("FEHLER", "Keine SD-Karte gefunden.\n");
    lua_pushboolean(L, false);
  }
  return 1;
}

// 14. Funktion für sd.cat("dateiname.txt")
FLASHMEM int lua_sd_cat(lua_State* L) {
  const char* dateiname = luaL_checkstring(L, 1);
  String Pfad = resolve_lua_path(dateiname);

  File datei = SD.open(Pfad.c_str(), FILE_READ);
  if (!datei) {
    zeigeFehlerPopup("FEHLER", "Datei konnte nicht geoeffnet werden.\n");
    lua_pushboolean(L, false);
    return 1;
  }
  int zeilen = 0;
  char puffer[256];
  while (datei.available() > 0) {
    int geleseneBytes = datei.readBytesUntil('\n', puffer, sizeof(puffer) - 1);
    puffer[geleseneBytes] = '\0';
    zeilen++;
    if (zeilen > 40) {
      if (wait_key(1) == 27) break;
      zeilen = 0;
    }
    vga_print_str(puffer);
    vga_print_str("\n");
  }

  datei.close();
  lua_pushboolean(L, true);
  return 1;
}

// 15. erstellt eine Tabelle der Dateien auf der SD-Karte
FLASHMEM int lua_sd_get_file_list(lua_State* L) {
  // 1. Eine große Haupt-Tabelle auf dem Lua-Stack erstellen (wird unser Gesamt-Array)
  lua_newtable(L);

  File root = SD.open(currentWorkDir.c_str());
  if (!root || !root.isDirectory()) {
    return 1;
  }

  int eintragIndex = 1; // Lua-Indizes beginnen immer bei 1!

  while (true) {
    File file = root.openNextFile();
    if (!file) {
      break; // Keine Dateien mehr vorhanden
    }

    // 2. SYSTEM-FILTER: Unsichtbare Dateien immer ausblenden
    if (strcasecmp(file.name(), "System Volume Information") == 0 ||
        strcasecmp(file.name(), "FOUND.000") == 0 ||
        strncasecmp(file.name(), "._", 2) == 0) {
      file.close();
      continue;
    }
    // 2. Für JEDE Datei eine eigene kleine Unter-Tabelle (Zeile) erstellen
    lua_newtable(L);

    // Spalte 1: Dateiname hinzufügen
    lua_pushstring(L, file.name());
    lua_rawseti(L, -2, 1); // Setzt den Namen an Index 1 der Unter-Tabelle

    // Spalte 2: Dateigröße formatieren und hinzufügen
    if (file.isDirectory()) {
      lua_pushstring(L, "---"); // Ordner haben keine klassische Dateigröße
      lua_rawseti(L, -2, 2);    // Index 2

      lua_pushstring(L, "ORDNER"); // Spalte 3: Typ
      lua_rawseti(L, -2, 3);    // Index 3
    } else {
      // Größe lesbar in KB umrechnen
      char sizeBuf[16];
      snprintf(sizeBuf, sizeof(sizeBuf), "%.1f KB", (float)file.size() / 1024.0f);
      lua_pushstring(L, sizeBuf);
      lua_rawseti(L, -2, 2);    // Index 2

      lua_pushstring(L, "DATEI");  // Spalte 3: Typ
      lua_rawseti(L, -2, 3);    // Index 3
    }

    // 3. Die fertige Unter-Tabelle (Zeile) in unsere Haupt-Tabelle einfügen
    lua_rawseti(L, -2, eintragIndex);
    eintragIndex++;
    file.close();
  }

  root.close();

  return 1; 
}

// 16. liest den aktuellen Pfad
FLASHMEM int lua_sd_pwd(lua_State* L) {
  lua_pushstring(L, currentWorkDir.c_str());   
  return 1; 
}

//********************************************** Grafikfunktionen *************************************
// ============================================================================
// VGA GRAPHICS INTERFACE (Modul: vga)
// ============================================================================

// 1. vga.color(vordergrund, hintergrund)
FLASHMEM int lua_vga_color(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    zeigeFehlerPopup("FEHLER", "vga.color(vordergrund, hintergrund) erwartet 2 Zahlen!");
    lua_pushboolean(L, false);
    return 0;
  }
  fColor = (int)lua_tonumber(L, 1);
  bColor = (int)lua_tonumber(L, 2);

  return 0;
}

// 2. vga.cls()
FLASHMEM int lua_vga_cls(lua_State* L) {
  int farbe = bColor; // Fallback auf das aktuelle globale bColor
  if (lua_gettop(L) >= 1 && lua_isnumber(L, 1)) {
    farbe = (int)lua_tonumber(L, 1);
  }
  vga.clear(farbe);
  cursorX = 0;
  cursorY = 0;
  memset(termBuffer, '\0', sizeof(termBuffer));
  vga_print_str("\n");
  return 0;
}

// 3. vga.text([spalte, zeile,] "Text" [, fcolor, bcolor, breit])
// Nutzt logische Textkoordinaten (Spalten/Zeilen) und macht Positionen optional!
FLASHMEM int lua_vga_text(lua_State* L) {
  int argumente = lua_gettop(L);
  int spalte = cursorX;
  int zeile = cursorY;
  const char* txt = NULL;

  int txtFColor = fColor;
  int txtBColor = bColor;
  bool doppelteBreite = false; // Standardwert: normale Breite (false)

  // FALL 1: Es wurden mindestens 3 Argumente uebergeben (spalte, zeile, "text" ...)
  if (argumente >= 3 && lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isstring(L, 3)) {
    spalte = (int)lua_tonumber(L, 1);
    zeile = (int)lua_tonumber(L, 2);
    txt = lua_tostring(L, 3);

    // Optionale Farben auswerten (liegen bei 4 und 5)
    if (argumente >= 4 && lua_isnumber(L, 4)) txtFColor = (int)lua_tonumber(L, 4);
    if (argumente >= 5 && lua_isnumber(L, 5)) txtBColor = (int)lua_tonumber(L, 5);

    //-- NEU: Schriftbreite auswerten. Kann bei Argument 4, 5 oder 6 liegen!
    if (argumente == 4 && lua_isboolean(L, 4)) doppelteBreite = lua_toboolean(L, 4);
    else if (argumente == 5 && lua_isboolean(L, 5)) doppelteBreite = lua_toboolean(L, 5);
    else if (argumente >= 6 && lua_isboolean(L, 6)) doppelteBreite = lua_toboolean(L, 6);
  }
  // FALL 2: Es wurde nur der Text uebergeben (Nutzt aktuelle Cursor-Position)
  else if (argumente >= 1 && lua_isstring(L, 1)) {
    txt = lua_tostring(L, 1);

    //-- Optionale Farben auswerten (liegen hier bei 2 und 3)
    if (argumente >= 2 && lua_isnumber(L, 2)) txtFColor = (int)lua_tonumber(L, 2);
    if (argumente >= 3 && lua_isnumber(L, 3)) txtBColor = (int)lua_tonumber(L, 3);

    //-- NEU: Schriftbreite auswerten. Kann bei Argument 2, 3 oder 4 liegen!
    if (argumente == 2 && lua_isboolean(L, 2)) doppelteBreite = lua_toboolean(L, 2);
    else if (argumente == 3 && lua_isboolean(L, 3)) doppelteBreite = lua_toboolean(L, 3);
    else if (argumente >= 4 && lua_isboolean(L, 4)) doppelteBreite = lua_toboolean(L, 4);
  }
  else {
    zeigeFehlerPopup("FEHLER", "vga.text(x,y,text,[fcol,bcol,breit]) erwartet!");
    lua_pushboolean(L, false);
    return 1; //-- KORREKTUR: 1 zurückgeben, da wir ein 'false' pushen!
  }

  // --- Umrechnung von Spalte/Zeile in Pixel (Faktor 8) ---
  int pixelX = spalte * 8;
  int pixelY = zeile * 8;

  // Text auf die VGA-Karte zeichnen (Nutzt jetzt die Variable 'doppelteBreite')
  vga.drawText(pixelX, pixelY, txt, txtFColor, txtBColor, doppelteBreite);

  // Cursor-Weiterschub anpassen (Doppelte Breite verbraucht 16 statt 8 Pixel pro Zeichen)
  int zeichenBreiteImRaster = doppelteBreite ? 2 : 1;
  cursorX = spalte + (strlen(txt) * zeichenBreiteImRaster);

  if (cursorX >= TERM_COLS) {
    cursorX = 0;
    cursorY++;
  }

  lua_pushboolean(L, true); //-- Erfolgs-Rückgabewert für Lua
  return 1;
}

// 4. Pixel zeichnen: vga.pset(x, y, farbe)
FLASHMEM int lua_vga_pset(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    zeigeFehlerPopup("FEHLER", "vga.pset(x, y, farbe) erwartet 3 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int farbe = (int)lua_tonumber(L, 3);

  vga.drawPixel(x, y, farbe);
  return 0;
}

// 5. Rechteck zeichnen: vga.box(x, y, w, h, farbe)
FLASHMEM int lua_vga_box(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "vga.box(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  int rColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) rColor = (int)lua_tonumber(L, 5);

  // Ruft das gefüllte Rechteck Ihrer VGA_t4-Bibliothek auf
  vga.drawRect(x1, y1, x2, y2, rColor);
  return 0;
}

// 6. LEERES RECHTECK (vga.rect): 4-Linien-Methode
FLASHMEM int lua_vga_rect(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "vga.rect(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  // Standardwert aus globaler Variable laden
  int rColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) rColor = (int)lua_tonumber(L, 5);

  // Zeichnet den leeren Rahmen mit Ihren 4 Linien
  vga.drawline(x1, y1, x2, y1, rColor); // Oben quer
  vga.drawline(x1, y1, x1, y2, rColor); // Links runter
  vga.drawline(x2, y1, x2, y2, rColor); // Rechts runter
  vga.drawline(x1, y2, x2, y2, rColor); // Unten quer
  return 0;
}

// 7. Gefuellte Ellipse: vga.filledellipse(x, y, w, h [, fcolor, bcolor])
FLASHMEM int lua_vga_filledellipse(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "vga.fillellipse(x, y, w, h) erwartet mindestens 4 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int w = (int)lua_tonumber(L, 3);
  int h = (int)lua_tonumber(L, 4);

  // Standardwerte aus globalen Variablen laden
  int ellFColor = fColor;
  int ellBColor = bColor;

  // Optionale Farben überschreiben, falls vom Benutzer in Lua übergeben
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) ellFColor = (int)lua_tonumber(L, 5);
  if (lua_gettop(L) >= 6 && lua_isnumber(L, 6)) ellBColor = (int)lua_tonumber(L, 6);

  vga.drawfilledellipse(x, y, w, h, ellFColor, ellBColor);
  return 0;
}

// 8. Leere Ellipse: vga.ellipse(x, y, w, h [, fcolor])
FLASHMEM int lua_vga_ellipse(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "vga.ellipse(x, y, w, h) erwartet mindestens 4 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int w = (int)lua_tonumber(L, 3);
  int h = (int)lua_tonumber(L, 4);

  // Standardwert aus globaler Variable laden
  int ellFColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) ellFColor = (int)lua_tonumber(L, 5);

  vga.drawellipse(x, y, w, h, ellFColor);
  return 0;
}

// 9. EINE EINZELNE LINIE (vga.line(x1, y1, x2, y2 [, farbe])
FLASHMEM int lua_vga_line(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "vga.line(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  int lColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) lColor = (int)lua_tonumber(L, 5);

  vga.drawline(x1, y1, x2, y2, lColor);
  return 0;
}

//10. vga.pos(spalte, zeile) zum Setzen der Cursor-Position auf der Konsole
FLASHMEM int lua_vga_pos(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    zeigeFehlerPopup("FEHLER", "vga.pos(spalte, zeile) erwartet 2 Zahlen!");
    lua_pushboolean(L, false);
    return 1;
  }

  int spalte = (int)lua_tonumber(L, 1);
  int zeile = (int)lua_tonumber(L, 2);

  // Grenzen absichern, damit der Cursor nicht außerhalb des Bildschirms landet
  if (spalte < 0) spalte = 0;
  if (spalte >= TERM_COLS) spalte = TERM_COLS - 1;
  if (zeile < 0) zeile = 0;
  if (zeile >= TERM_ROWS) zeile = TERM_ROWS - 1;

  cursorX = spalte;
  cursorY = zeile;

  return 0;
}

// vga.print(wert) - Gibt Zahlen oder Strings ohne Zeilenumbruch aus
FLASHMEM int lua_vga_print(lua_State* L) {

  if (lua_gettop(L) < 1) {                          // Prüfen, ob überhaupt ein Argument übergeben wurde
    return 0;
  }

  if (lua_isnumber(L, 1)) {                         // Lua-Typ abfragen und passend konvertieren
    double num = lua_tonumber(L, 1);
    char buf[32];

    if (num == (int)num) {                          // Prüfen, ob es eine Ganzzahl oder Fließkommazahl ist für saubere Optik
      snprintf(buf, sizeof(buf), "%d", (int)num);
    } else {
      snprintf(buf, sizeof(buf), "%f", num);
    }
    vga_print_str(buf);
  }
  else if (lua_isstring(L, 1)) {
    const char* txt = lua_tostring(L, 1);
    vga_print_str(txt);
  }

  return 0;                                         // Kein Rückgabewert an Lua
}

FLASHMEM int lua_vga_get_colors(lua_State* L) {
  // Die Werte auf den Lua-Stack legen
  lua_pushinteger(L, fColor); // Erster Rückgabewert (fColor)
  lua_pushinteger(L, bColor); // Zweiter Rückgabewert (bColor)
  return 2; //2 Werte zurückgeben
}

// Unterfunktion Window zeichnen
// Universelle C++ Funktion zum Zeichnen eines Fensters mit automatischem Textumbruch
FLASHMEM void renderWindow(int x, int y, int w, int h, int fc, int bc, const char* titel, const char* inhalt, uint16_t titelFarbe) {
  vga.drawRect(x, y, w, h, bc);
  vga.drawline(x, y, x + w, y, fc);
  vga.drawline(x, y, x, y + h, fc);
  vga.drawline(x + w, y, x + w, y + h, fc);
  vga.drawline(x, y + h, x + w, y + h, fc);

  vga.drawRect(x + 2, y + 2, w - 4, 14, titelFarbe);
  vga.drawText(x + 10, y + 5, titel, fc, titelFarbe, false);

  int textStartX = x + 10;
  int textStartY = y + 25;
  int aktuelleX = textStartX;
  int aktuelleY = textStartY;

  int zeichenBreite = 8;
  int zeilenHoehe = 10;
  int maxTextBreite = w - 20;

  String textKopie = String(inhalt);
  char* textPtr = const_cast<char*>(textKopie.c_str());
  char* wort = strtok(textPtr, " \t\n\r");

  while (wort != NULL) {
    int wortLaenge = strlen(wort);
    int wortBreitePixel = wortLaenge * zeichenBreite;

    if (aktuelleX + wortBreitePixel > textStartX + maxTextBreite && aktuelleX > textStartX) {
      aktuelleX = textStartX;
      aktuelleY += zeilenHoehe;
    }

    if (aktuelleY + zeilenHoehe > y + h - 5) {
      vga.drawText(aktuelleX, aktuelleY, "...", fc, bc, false);
      break;
    }

    vga.drawText(aktuelleX, aktuelleY, wort, fc, bc, false);
    aktuelleX += wortBreitePixel + zeichenBreite;
    wort = strtok(NULL, " \t\n\r");
  }
}


// 1. Fenster öffnen und im Manager registrieren
FLASHMEM int lua_vga_open_window(lua_State* L) {
  int idx = luaL_checkinteger(L, 1);
  if (idx < 0 || idx > 7) {
    zeigeFehlerPopup("INDEX FEHLER", "Fenster-Index muss zwischen 0 und 7 liegen!");
    return 0;
  }

  // Daten in den Slot schreiben
  windowManager[idx].x = luaL_checkinteger(L, 2);
  windowManager[idx].y = luaL_checkinteger(L, 3);
  windowManager[idx].w = luaL_checkinteger(L, 4);
  windowManager[idx].h = luaL_checkinteger(L, 5);
  windowManager[idx].fc = luaL_checkinteger(L, 6);
  windowManager[idx].bc = luaL_checkinteger(L, 7);
  windowManager[idx].titel = String(luaL_checkstring(L, 8));
  windowManager[idx].inhalt = String(luaL_checkstring(L, 9));
  windowManager[idx].titelFarbe = luaL_checkinteger(L, 10);
  windowManager[idx].aktiv = true;

  // Fenster physisch zeichnen
  renderWindow(windowManager[idx].x, windowManager[idx].y, windowManager[idx].w, windowManager[idx].h,
               windowManager[idx].fc, windowManager[idx].bc, windowManager[idx].titel.c_str(),
               windowManager[idx].inhalt.c_str(), windowManager[idx].titelFarbe);
  return 0;
}

FLASHMEM int lua_vga_update_window(lua_State* L) {
  int idx = luaL_checkinteger(L, 1);
  if (idx < 0 || idx > 7 || !windowManager[idx].aktiv) return 0;

  const char* neuerInhalt = luaL_checkstring(L, 2);
  windowManager[idx].inhalt = String(neuerInhalt);

  // alten Fensterinhalt löschen (Bereich restaurieren)
  restoreTerminalArea(windowManager[idx].x, windowManager[idx].y, windowManager[idx].w, windowManager[idx].h);
  // neuen Inhalt Zeichnen
  renderWindow(windowManager[idx].x, windowManager[idx].y, windowManager[idx].w, windowManager[idx].h,
               windowManager[idx].fc, windowManager[idx].bc, windowManager[idx].titel.c_str(),
               windowManager[idx].inhalt.c_str(), windowManager[idx].titelFarbe);

  return 0;
}

// 3. Fenster schließen über den Index (ersetzt Ihre alte closeWindow Version)
FLASHMEM int lua_vga_close_window(lua_State* L) {
  int idx = luaL_checkinteger(L, 1);
  if (idx < 0 || idx > 7) return 0;

  if (windowManager[idx].aktiv) {
    // Nutzt Ihre korrigierte Off-by-One Funktion zum sauberen Löschen
    restoreTerminalArea(windowManager[idx].x, windowManager[idx].y, windowManager[idx].w, windowManager[idx].h);
    windowManager[idx].aktiv = false; // Slot wieder freigeben
  }

  return 0;
}

// Durchläuft alle Slots und schließt offene Fenster nach einem Skript-Ende oder Absturz
FLASHMEM void cleanupWindows() {
  for (int idx = 0; idx < 8; idx++) {
    if (windowManager[idx].aktiv) {
      // Nutzt Ihre korrigierte Off-by-One Funktion zum sauberen Löschen
      restoreTerminalArea(windowManager[idx].x, windowManager[idx].y, windowManager[idx].w, windowManager[idx].h);
      windowManager[idx].aktiv = false; // Slot wieder freigeben
    }
  }
}


// Hauptfenster - TitelBar zeichnen
FLASHMEM int lua_vga_set_title(lua_State* L) {
  const char* neuerTitel = luaL_checkstring(L, 1);
  strncpy(currentTitleText, neuerTitel, sizeof(currentTitleText) - 1);
  currentTitleText[sizeof(currentTitleText) - 1] = '\0'; // String-Abschluss sichern
  drawTitleBar();

  return 0; // Keine Rückgabewerte an Lua
}


FLASHMEM void restoreTerminalArea(int x, int y, int w, int h) {
  // 1. Den betroffenen Fensterbereich auf dem VGA-Schirm komplett löschen
  //hier müssen die Parameter w und h um einen Pixel erhöht werden ?!
  vga.drawRect(x, y, w + 1, h + 1, bColor);

  // 2. Pixel-Koordinaten in das 8x8 Text-Zeilen- und Spaltenraster umrechnen

  int startXCol = x / 8;
  int endXCol   = (x + w) / 8;
  int startYRow = y / 8;
  int endYRow   = (y + h) / 8;

  // Sicherheits-Sperre: Grenzen des termBuffers (60x80) niemals überschreiten
  if (startXCol < 0) startXCol = 0;
  if (endXCol >= TERM_COLS) endXCol = TERM_COLS - 1;
  if (startYRow < TEXT_START_ROW) startYRow = TEXT_START_ROW;
  if (endYRow > TEXT_END_ROW) endYRow = TEXT_END_ROW;

  // 3. Nur den betroffenen Ausschnitt aus dem RAM-Puffer restaurieren
  for (int row = startYRow; row <= endYRow; row++) {
    for (int col = startXCol; col <= endXCol; col++) {
      char c = termBuffer[row][col];
      if (c != '\0') {
        char echo[] = { c, '\0' };
        vga.drawText(col * 8, row * 8, echo, fColor, bColor, false);
      }
    }
  }
}

// ============================================================================
// TIME - TIMER INTERFACE (Modul: sys)  -- und sonstige Systemdienste
// ============================================================================
//Hexmonitor für den PSRAM sys.hexmon(start[,laenge])
FLASHMEM int lua_sys_hexmon(lua_State* L) {
  int args = lua_gettop(L);

  // 1. Parameter prüfen (Startadresse ist Pflicht)
  if (args < 1 || !lua_isnumber(L, 1)) {
    zeigeFehlerPopup("Fehler", "Startadresse fehlt! Nutzen Sie: sys.hexmem(0x70000000 [, laenge])\n\r");
    return 0;
  }

  // 1. Virtuellen Start-Offset einlesen (Standard: 0x0000)
  uint32_t virtuellerStart = 0;
  if (args >= 1 && lua_isnumber(L, 1)) {
    virtuellerStart = (uint32_t)lua_tonumber(L, 1);
  }

  // 2. Gewünschte Länge einlesen (Standard: 512 Bytes)
  uint32_t laenge = 1024;
  if (args >= 2 && lua_isnumber(L, 2)) {
    laenge = (uint32_t)lua_tonumber(L, 2);
  }

  // Der echte physikalische Startpunkt des PSRAMs im Teensy 4.1
  const uint32_t PSRAM_BASIS = 0x70000000;

  vga_print_str("\n\r--- PSRAM HARDWARE HEX-MONITOR (ESC to Abort) ---\n\r");
  vga_print_str("Offset    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Decoded Text\n\r");
  vga_print_str("---------------------------------------------------------------------------\n\r");

  char outLine[128];
  uint32_t aktuellerOffset = virtuellerStart;
  uint32_t bytesVerarbeitet = 0;
  int zeilenZaehler = 0;
  bool laeuft = true;
  // 3. Speicherbereich in 16-Byte-Schritten durchlaufen
  while (laeuft) { //(bytesVerarbeitet < laenge) {
    uint32_t verbleibend = laenge - bytesVerarbeitet;
    int blockGroesse = 16;//(verbleibend > 16) ? 16 : verbleibend;

    // INTERNER INTERRUPT-CHECK: Nach 20 Zeilen (einer Bildschirmseite) pausieren
    if (zeilenZaehler >= 20) {
      int taste = wait_key(true);

      if (taste == 27) { // 27 entspricht der ESC-Taste
        laeuft = false;
        return 0;
      }

      zeilenZaehler = 0; // Zähler für die nächste Seite zurücksetzen
      vga_print_str("\n\r");
    }

    // A) Virtuellen Offset links anzeigen (beginnt wunschgemäß bei 00000000)
    int pos = snprintf(outLine, sizeof(outLine), "%08X  ", aktuellerOffset);

    // B) INTERNE ADDITION: Physikalische Zieladresse im EXTMEM berechnen!
    uint32_t physikalischeAdresse = PSRAM_BASIS + aktuellerOffset;
    uint8_t* memPtr = (uint8_t*)physikalischeAdresse;

    // C) 16 Bytes als HEX-Paare formatieren
    for (int i = 0; i < 16; i++) {
      pos += snprintf(outLine + pos, sizeof(outLine) - pos, "%02X ", memPtr[i]);
    }

    pos += snprintf(outLine + pos, sizeof(outLine) - pos, " ");

    // D) Rechten ASCII-Text decodieren
    for (int i = 0; i < blockGroesse; i++) {
      char ch = (char)memPtr[i];
      if (ch >= 32 && ch <= 126) {
        pos += snprintf(outLine + pos, sizeof(outLine) - pos, "%c", ch);
      } else {
        pos += snprintf(outLine + pos, sizeof(outLine) - pos, ".");
      }
    }

    snprintf(outLine + pos, sizeof(outLine) - pos, "\n\r");
    vga_print_str(outLine);

    // Werte für das nächste Paket hochzählen
    aktuellerOffset += blockGroesse;
    bytesVerarbeitet += blockGroesse;
    zeilenZaehler++;
  }

  vga_print_str("---------------------------------------------------------------------------\n\r");
  return 0;
}
// funktion sys.info()
FLASHMEM int lua_sys_info(lua_State* L) {
  extern uint8_t external_psram_size;
  
  // 1. Freies internes RAM (Heap) berechnen
  extern unsigned long _heap_end;
  extern char *__brkval;
  long freies_internes_ram = (char *)&_heap_end - __brkval;
  
  int belegt_kb = lua_gc(L, LUA_GCCOUNT, 0);
  int maximal_kb = 1024; // Die 1 MB aus Ihrem setup()
  int frei_kb = maximal_kb - belegt_kb;
  
  // 2. CPU-Geschwindigkeit in MHz auslesen
  uint32_t cpu_mhz = F_CPU_ACTUAL / 1000000;

  // 3. Native Teensy-Hardware-Register abfragen
  tempmon_init();
  float grad_celsius = tempmonGetTemp();

  // 4. DER ENTSCHEIDENDE FIX: Puffer auf 512 Bytes vergrößern!
  char monBuf[512];

  // Wir übergeben sizeof(monBuf) (jetzt 512), damit der Compiler weiß, dass genug Platz da ist
  snprintf_P(monBuf, sizeof(monBuf),
           "\n\r====================================\n\r"
           "         TEENSY SYSTEM MONITOR      \n\r"
           "====================================\n\r"
           " CPU-Taktfrequenz : %u MHz\n\r"
           " CPU-Temperatur   : %.1f *C\n\r"
           " Interner RAM     : %d KB frei\n\r"
           " PSRAM gesamt     : %d KB verbaut\n\r"
           " Lua RAM          : %d KB gesamt\n\r"
           " Lua RAM          : %d KB belegt\n\r"
           " Lua RAM          : %d KB frei\n\r"
           "====================================\n\r",
           cpu_mhz, grad_celsius, freies_internes_ram / 1024, external_psram_size * 1024, maximal_kb, belegt_kb, frei_kb);

  // Text direkt auf die VGA-Konsole ausgeben
  vga_print_str(monBuf);

  return 0;
}


void zeigeFehlerPopup(const __FlashStringHelper* titel, const __FlashStringHelper* meldung) {
  // Wandelt die Flash-Texte im Vorbeiflug in temporäre C-Strings um
  String t(titel);
  String m(meldung);
  
  // Ruft Ihre originale Funktion mit den umgewandelten Strings auf
  zeigeFehlerPopup(t.c_str(), m.c_str());
}
// Unterfunktion Fehlerfenster
FLASHMEM void zeigeFehlerPopup(const char* titel, const char* nachricht) {
  cleanupWindows();
  editorStartZeile = extrahiereFehlerZeile(nachricht);                     //fehlerhafte Zeile merken für Editor
  renderWindow(160, 160, 320, 160, 255, DARKRED, titel, nachricht, RED);   //Fehlerfenster aufbauen
  vga.drawText(220, 300, "Druecke eine Taste...", YELLOW, DARKRED, false); //und Fehlertext anzeigen
  wait_key(false);
  restoreTerminalArea(160, 160, 321, 161);
}

// C++ Funktion für sys.gettime()
FLASHMEM int sys_get_time(lua_State* L) {
  uint32_t unixZeit = Teensy3Clock.get();
  //unixZeit += 3600;     //Winterzeit / Zeitzonen-Anpassung, falls nötig

  // Berechnung der Uhrzeit über reine Mathematik
  int sekunden = unixZeit % 60;
  int minuten  = (unixZeit / 60) % 60;
  int stunden  = (unixZeit / 3600) % 24;

  // NEU: Die drei Werte einzeln als Ganzzahlen auf den Lua-Stack legen
  lua_pushinteger(L, stunden);
  lua_pushinteger(L, minuten);
  lua_pushinteger(L, sekunden);

  return 3; // 3 Rückgabewerte an Lua (Stunden, Minuten, Sekunden)
}

FLASHMEM int sys_get_date(lua_State* L) {
  time_t rawtime = Teensy3Clock.get();
  rawtime += 3600;                          // Zeitzonenausgleich (+1 Stunde für Deutschland-Winterzeit)
  struct tm* timeinfo = gmtime(&rawtime);   // gmtime nutzt native C-Bibliothek des Teensy-Compilers

  int tag   = timeinfo->tm_mday;
  int monat = timeinfo->tm_mon + 1;       // tm_mon zählt von 0 bis 11 -> korrigieren auf 1-12
  int jahr  = timeinfo->tm_year + 1900;   // tm_year zählt seit 1900 -> auf echtes Jahr korrigieren

  // Die drei Werte einzeln als Ganzzahlen auf den Lua-Stack legen
  lua_pushinteger(L, tag);
  lua_pushinteger(L, monat);
  lua_pushinteger(L, jahr);

  return 3; // 3 Rückgabewerte an Lua (Tag, Monat, Jahr)
}
// C++ Funktion für system.millis()
FLASHMEM int sys_timer(lua_State* L) {
  lua_pushinteger(L, millis());
  return 1; // 1 Rückgabewert an Lua geliefert
}

FLASHMEM time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

// Hilfsfunktion: für Autovervollständigung - Sucht auf der SD-Karte nach Dateien, die mit 'praefix' beginnen.
FLASHMEM int sucheDateiAufSD(const String& praefix, String& treffer, String& trefferListe) {
  File root = SD.open(currentWorkDir.c_str());
  if (!root) return 0;

  root.rewindDirectory();

  int trefferAnzahl = 0;
  treffer = "";
  trefferListe = "";

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break; // Keine weiteren Dateien vorhanden

    if (!entry.isDirectory()) {
      String name = String(entry.name());
      name.toLowerCase();

      String suche = praefix;
      suche.toLowerCase();

      if (name.startsWith(suche)) {
        trefferAnzahl++;
        if (trefferAnzahl == 1) {
          treffer = String(entry.name());
        }
        if (trefferListe.length() > 0) trefferListe += "  ";
        trefferListe += String(entry.name());
      }
    }
    entry.close();
  }
  root.rewindDirectory();
  root.close();
  return trefferAnzahl;
}

FLASHMEM int lua_sys_get_hardware_data(lua_State* L) {
  // 1. Native Teensy-Hardware-Register für CPU-Temperatur abfragen
  tempmon_init(); 
  float grad_celsius = tempmonGetTemp();

  // 2. KORREKTUR: Den echten, aktuellen Lua-Speicherverbrauch in KB abfragen
  // lua_gc liefert den aktuellen RAM-Verbrauch der Lua-VM
  int lua_speicher_kb = lua_gc(L, LUA_GCCOUNT, 0); 
  
  // Die maximal zugewiesene Poolgröße (aus Ihrem setup(): 1024 KB)
  float maximaler_pool_kb = 1024.0f;
  
  // Berechnen, wie viel von den 1 Megabyte im PSRAM noch absolut frei ist!
  float freier_psram_heap_kb = maximaler_pool_kb - (float)lua_speicher_kb;

  // 3. Beide Werte nacheinander auf den Lua-Stack legen
  lua_pushnumber(L, grad_celsius);         // 1. Rückgabewert für Tacho 1 (Links)
  lua_pushnumber(L, freier_psram_heap_kb); // 2. Rückgabewert für Tacho 2 (Rechts)

  return 2; // 2 Rückgabewerte an Lua übergeben
}
// ============================================================================
// SPRITE - SPRITE und GFX-Engine (Modul: sprite)  ----------------------------
// ============================================================================
//--------------------- sprite.get(x,y,Num) -----------------------------------
FLASHMEM int lua_sprite_get(lua_State* L) {
  // Parameter-Prüfung: Wir erwarten 3 Zahlen von sprite.get (x, y, sNum)
  if (lua_gettop(L) < 3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    zeigeFehlerPopup("FEHLER", "erwartet 3 Zahlen sprite.get(x, y, sNum)");
    lua_pushboolean(L, false);
    return 1;
  }

  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int sNum = (int)lua_tonumber(L, 3);

  const int TILE_SIZE = 16; // 16x16 Pixel vom Bildschirm grabben
  const int totalPixels = TILE_SIZE * TILE_SIZE;
  // Im PSRAM (oder statisch im RAM) Platz für das Sprite reservieren
  static vga_pixel tempTile[totalPixels];

  int p = 0;
  for (int y = 0; y < TILE_SIZE; y++) {
    for (int x = 0; x < TILE_SIZE; x++) {
      // Nutzt Ihre native VGA-Funktion, um das Pixel zu lesen
      tempTile[p++] = vga.getPixel(x1 + x, y1 + y);
    }
  }
  // Das Sprite im Grafik-Controller unter der Nummer registrieren
  vga.sprite_data(sNum, tempTile, totalPixels);

  lua_pushboolean(L, true);
  return 1;
}

//--------------------- sprite.draw(x,y,Num) -----------------------------------
FLASHMEM int lua_sprite_draw(lua_State* L) {
  int top = lua_gettop(L);
  if (top < 3) {
    lua_pushboolean(L, false);
    return 1;
  }

  // Fall 1: 4 Parameter übergeben -> sprite.draw(kanal, x, y, grafik_slot)
  if (top >= 4) {
    int kanal = (int)lua_tonumber(L, 1);
    int x     = (int)lua_tonumber(L, 2);
    int y     = (int)lua_tonumber(L, 3);
    int sNum  = (int)lua_tonumber(L, 4);

    if (kanal >= 0 && kanal < 64 && sNum >= 0 && sNum < 64) {
      vga.sprite(kanal, x, y, sNum); // Kanal und Grafik-Muster getrennt!
      lua_pushboolean(L, true);
    } else {
      lua_pushboolean(L, false);
    }
  } 
  // Fall 2: 3 Parameter (Alte Logik für Abwärtskompatibilität) -> sprite.draw(x, y, sNum)
  else {
    int x    = (int)lua_tonumber(L, 1);
    int y    = (int)lua_tonumber(L, 2);
    int sNum = (int)lua_tonumber(L, 3);

    if (sNum >= 0 && sNum < 64) {
      vga.sprite(sNum, x, y, sNum); // Kanal = Grafikslot
      lua_pushboolean(L, true);
    } else {
      lua_pushboolean(L, false);
    }
  }

  return 1;
}

//--------------------- sprite.update() -----------------------------------
FLASHMEM int lua_sprite_update(lua_State* L) {
  run_tile_animations();
  vga.run_gfxengine();
  return 0; 
}
//--------------------- sprite.hide(id) -----------------------------------
FLASHMEM int lua_sprite_hide(lua_State* L) {
  // Parameter-Prüfung: Wir erwarten genau 1 Zahl von Lua (die Sprite-ID)
  if (lua_gettop(L) < 1 || !lua_isnumber(L, 1)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.hide(id)");
    lua_pushboolean(L, false);
    return 1;
  }
  int spr_id = (int)lua_tonumber(L, 1);
  // Sicherheitsprüfung analog zu Ihrer Hardware (Slots 0 bis 63)
  if (spr_id >= 0 && spr_id < 64) {
    vga.sprite_hide(spr_id);
    lua_pushboolean(L, true);
  } else {
    lua_pushboolean(L, false);
  }
  return 1; // Gibt true/false an Lua zurück
}
//--------------------- sprite.cls(color) -----------------------------------
//------- Unterfunktion für sprite.cls() ---------------
FLASHMEM void set_background_color_correct(uint8_t vgaColor) {
  const int tileSize = TILES_W * TILES_H;
  uint8_t tempTile[tileSize];
  for (int i = 0; i < tileSize; i++) {
    tempTile[i] = vgaColor;
  }
  vga.tile_data(255, (vga_pixel*)tempTile, tileSize);
  unsigned char fillRow[TILES_COLS];
  for (int i = 0; i < TILES_COLS; i++) fillRow[i] = 255;

  for (int r = 0; r < TILES_ROWS; r++) {
    vga.tile_draw_row(0, 0, r, fillRow, TILES_COLS);
  }
}

FLASHMEM int lua_sprite_cls(lua_State* L) {
  int vgaColor = 1; 
  if (lua_gettop(L) >= 1 && lua_isnumber(L, 1)) {
    vgaColor = (int)lua_tonumber(L, 1);
  }
  set_background_color_correct(vgaColor);
  return 0; 
}

//--------------------- sprite.animate(tile, start, bilder) -----------------------------------
FLASHMEM void run_tile_animations() {
  static unsigned long lastAnim = 0;
  static int frameTick = 0;
  if (millis() - lastAnim < 150) return;
  lastAnim = millis();
  frameTick++;

  for (int i = 0; i < animCounter; i++) {
    if (animList[i].active) {
      int currentFrame = frameTick % animList[i].frames;
      int srcIdx = animList[i].source + currentFrame;                           // Bild im PSRAM (tileLibrary) ermitteln
      int dstIdx = animList[i].target;                                          // Welches Tile auf dem Schirm soll ersetzt werden?
      vga.tile_data(dstIdx, (vga_pixel*)&tileLibrary[srcIdx * 256], 256);       // Kopiert die 256 Pixel (16x16) direkt in die VGA-Engine
    }
  }
}

FLASHMEM int lua_sprite_animate(lua_State* L) {
  if (lua_gettop(L) < 3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.animate(tile,start,frames)");
    lua_pushboolean(L, false);
    return 1;
  }
  int t = (int)lua_tonumber(L, 1); // Das Tile in der Map
  int s = (int)lua_tonumber(L, 2); // Start-Index im Tilesheet
  int f = (int)lua_tonumber(L, 3); // Anzahl der Bilder

  if (animCounter < 10) {
    animList[animCounter].target = t;
    animList[animCounter].source = s;
    animList[animCounter].frames = f;
    animList[animCounter].active = true;
    animCounter++;
    lua_pushboolean(L, true);
  } else {
    lua_pushboolean(L, false); // Alle 10 Animationsslots belegt
  }
  return 1;
}

//--------------------- sprite.loadmap(filename) -----------------------------------
//----------- Unterfunktion Map laden -------------
FLASHMEM void load_map(const char* filename) {
  File f = SD.open(filename, FILE_READ);
  if (f) {
    // 1. Gesamte Map ins PSRAM lesen
    f.read(tilemapL0, sizeof(tilemapL0));
    f.close();

    currentHScroll0 = 0;
    for (int i = 0; i < TILES_COLS; i++) {
      unsigned char* columnData = tilemapL0 + (i * TILES_ROWS);
      vga.tile_draw_col(0, i, 0, columnData, TILES_ROWS);
    }
    mapPtrL0 = tilemapL0 + (TILES_COLS * TILES_ROWS);
  }
}

//--------------------- sprite.loadmap(filename) -----------------------------------
FLASHMEM int lua_sprite_mapload(lua_State* L) {
  if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.mapload(filename");
    lua_pushboolean(L, false);
    return 1;
  }
  String datei = currentWorkDir + lua_tostring(L, 1);
  const char* filename = datei.c_str();
  load_map(filename);

  lua_pushboolean(L, true);
  return 1; 
}

//--------------------- sprite.tsheet(filename) -----------------------------------
FLASHMEM int lua_sprite_tsheet(lua_State* L) {
  if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.tsheet(filename");
    lua_pushboolean(L, false);
    return 1;
  }
  String datei = currentWorkDir + lua_tostring(L, 1);
  const char* fname = datei.c_str();

  File bmpFile = SD.open(fname);
  if (!bmpFile) {
    lua_pushboolean(L, false);
    return 1;
  }

  bmpFile.seek(18);                         // Header-Check (Breite und Höhe prüfen, Offset 18 und 22)
  uint32_t width, height;
  bmpFile.read((uint8_t*)&width, 4);
  bmpFile.read((uint8_t*)&height, 4);

  if (width != 256 || height != 256) {
    bmpFile.close();
    lua_pushboolean(L, false);
    return 1;
  }
  
  bmpFile.seek(54);                         //Header überspringen und BMP-Pixeldaten einlesen und in Kacheln (Tiles) umsortieren
  for (int ty = 15; ty >= 0; ty--) {        // Kachel-Reihe (16 Reihen)
    for (int py = 15; py >= 0; py--) {      // Pixel-Zeile innerhalb der Kachel (16 Zeilen)
      for (int tx = 0; tx < 16; tx++) {     // Kachel-Spalte (16 Spalten)
        int slot = ((15 - ty) * 16) + tx;
        int startIdx = slot * 256 + (py * 16);

        for (int px = 0; px < 16; px++) {
          uint8_t b = bmpFile.read();
          uint8_t g = bmpFile.read();
          uint8_t r = bmpFile.read();

          // Konvertierung in dein natives VGA-Farbformat
          uint8_t vgaColor = VGA_RGB(r, g, b);
          tileLibrary[startIdx + px] = vgaColor;
        }
      }
    }
  }
  bmpFile.close();

  for (int i = 0; i < 256; i++) {                                      // Alle 256 Kacheln in den Grafik-Controller laden (+16 Offset)
    vga.tile_data(i + 16, (vga_pixel*)&tileLibrary[i * 256], 256);
  }
  lua_pushboolean(L, true);
  return 1; 
}

//--------------------- sprite.hscroll(direction) -----------------------------------
//------------ Unterfunktion hscroll ------------------------
FLASHMEM void scroll_map_h(int direction) {
  currentHScroll0 += direction;

  // Überprüfen, ob wir eine neue Tile-Grenze (16 Pixel) überschritten haben
  if ((currentHScroll0 & 0x0F) == 0) {
    int tileColIndex = currentHScroll0 / TILES_W;
    int vgaTargetCol;

    if (direction > 0) {
      vgaTargetCol = (tileColIndex + (TILES_COLS - 1)) % TILES_COLS;
      mapPtrL0 = &tilemapL0[(tileColIndex + (TILES_COLS - 1)) * TILES_ROWS];
    } else {
      vgaTargetCol = tileColIndex % TILES_COLS;
      mapPtrL0 = &tilemapL0[tileColIndex * TILES_ROWS];
    }
    unsigned char tempColumn[TILES_ROWS];
    for (int i = 0; i < TILES_ROWS; i++) {
      tempColumn[i] = mapPtrL0[i] + 16;
    }
    int mapColToLoad = (tileColIndex + TILES_COLS - 1) ;//% 960; // Springt nach 999 zurück auf 0
    if (mapColToLoad > 798) {
      currentHScroll0 = 0;
      mapColToLoad = 0;
      mapPtrL0 = 0;
    }
    unsigned char* dataPtr = &tilemapL0[mapColToLoad * 30];
    vga.tile_draw_col(0, vgaTargetCol, 0, dataPtr, TILES_ROWS);
  }
  vga.hscroll(0, currentHScroll0);
}

//--------------------- sprite.vscroll(direction) -----------------------------------
FLASHMEM void scroll_map_v(int direction) {
  currentVScroll0 += direction;

  if (currentVScroll0 < 0) {                                          // 1. Kartengrenzen-Schutz oben
    currentVScroll0 = 0;
    return;
  }
  
  if ((currentVScroll0 & 0x0F) == 0) {                                // Überprüfen, ob eine neue Kachel-Grenze (16 Pixel in der Höhe) überschritten wurde
    int tileRowIndex = currentVScroll0 / 16; 
    int vgaTargetRow;
    int mapRowToLoad;

    if (direction > 0) {
      //scrolle runter
      vgaTargetRow = (tileRowIndex + (TILES_ROWS - 1)) % TILES_ROWS;
      mapRowToLoad = tileRowIndex + TILES_ROWS - 1;
    } else {
      // HOCH-SCROLLEN (Kamera wandert nach oben): 
      vgaTargetRow = tileRowIndex % TILES_ROWS;
      mapRowToLoad = tileRowIndex;
    }

    if (mapRowToLoad > 500) {                                     // Beispiel: Ihre Map ist maximal 500 Kacheln hoch
      currentVScroll0 = (500 - TILES_ROWS) * 16;
      return;
    }
   
    unsigned char tempRow[TILES_COLS];                            // 4. Temporäres Array im RAM vorbereiten, um die horizontale Kachelzeile aufzubauen
    for (int c = 0; c < TILES_COLS; c++) {
      
      tempRow[c] = tilemapL0[c * 30 + mapRowToLoad];              // Holt aus jeder Spalte (c * 30) die Kachel für die gewünschte Zeile (mapRowToLoad)
    }
    
    vga.tile_draw_row(0, 0, vgaTargetRow, tempRow, TILES_COLS);
  }
  vga.vscroll(0, currentVScroll0);
}

//-------------------- sprite.scroll(x,y) x=1=rechts x=-1=links, y=1=runter y=-1=hoch ----------- 
FLASHMEM int lua_sprite_scroll(lua_State* L) {
  if (lua_gettop(L) < 2 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.scroll(h,r)");
    lua_pushboolean(L, false);
    return 1;
  }
  int args = lua_gettop(L);
  
  int pixelX = 0;
  int pixelY = 0;

  if (args >= 1 && lua_isnumber(L, 1)) pixelX = (int)lua_tonumber(L, 1);
  if (args >= 2 && lua_isnumber(L, 2)) pixelY = (int)lua_tonumber(L, 2);

  if (pixelX != 0) {
    int dx = (pixelX > 0) ? 1 : -1;
    int absX = abs(pixelX);
    for (int i = 0; i < absX; i++) {
      scroll_map_h(dx);                                     // Ruft Horizontal-Engine auf
    }
  }

  if (pixelY != 0) {
    int dy = (pixelY > 0) ? 1 : -1;
    int absY = abs(pixelY);
    for (int i = 0; i < absY; i++) {
      scroll_map_v(dy);                                     // Ruft Vertikal-Engine auf
    }
  }

  lua_pushboolean(L, true);
  return 1;
}

//-------------------- sprite.load(filename) --------------------------------------------------- 
FLASHMEM void load_sprite_to_library(const char* filename, int slot) {
  if (slot < 0 || slot > 255) return;   //Fehler slot
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    zeigeFehlerPopup("FEHLER", "BMP nicht gefunden!");
    return;
  }
  bmpFile.seek(54);                    //Header überpringen
  for (int y = 15; y >= 0; y--) {      // BMPs sind "Bottom-Up" gespeichert (letzte Zeile zuerst)
    for (int x = 0; x < 16; x++) {
      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();
      uint8_t vgaColor = VGA_RGB(r, g, b);//(r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6);
      SpriteLibrary[slot * 256 + (y * 16 + x)] = vgaColor;
    }
  }
  bmpFile.close();
  vga.sprite_data(slot, (vga_pixel*)&SpriteLibrary[slot * 256], 256);
}

FLASHMEM int lua_sprite_load_single(lua_State* L) {
  if (lua_gettop(L) < 2 || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.load(filename,id)");
    lua_pushboolean(L, false);
    return 1;
  }
  String datei = currentWorkDir + lua_tostring(L, 1);
  const char* filename = datei.c_str();
  
  int slot = (int)lua_tonumber(L, 2);
  if (slot >= 0 && slot <= 255) {                           // Sicherheitsprüfung analog zu deiner Hardware (Slots 0 bis 255)
    load_sprite_to_library(filename, slot);
    lua_pushboolean(L, true);
  } else {
    lua_pushboolean(L, false);                              // Ungültiger Sprite-Slot
  }
  return 1; 
}

//-------------------- sprite.tile(layer,x,y,id) 0=Hintergrund ------------------------------------- 
FLASHMEM int lua_sprite_tile_draw(lua_State* L) {
  // Parameter-Prüfung: Wir erwarten layer, x, y und die Kachel-ID (tileIndex)
  if (lua_gettop(L) < 4 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.tile(layer,x,y,id)");
    lua_pushboolean(L, false);
    return 1;
  }

  int layer     = (int)lua_tonumber(L, 1);
  int x         = (int)lua_tonumber(L, 2);
  int y         = (int)lua_tonumber(L, 3);
  int tileIndex = (int)lua_tonumber(L, 4);

  // Rufe deine originale, hardwarenahe Kachel-Zeichenfunktion auf!
  vga.tile_draw(layer, x, y, (unsigned char)tileIndex);

  lua_pushboolean(L, true);
  return 1; 
}

//-------------------- sprite.get_tile(layer,x,y,id) 0=Hintergrund ------------------------------------- 
FLASHMEM int get_tile_raw(int world_x, int world_y) {
  int tx = world_x >> 4; // Pixel zu Tile-X
  int ty = world_y >> 4; // Pixel zu Tile-Y
  if (tx < 0 || tx >= 500 || ty < 0 || ty >= 30) return 0;
  return tilemapL0[tx * 30 + ty];
}

FLASHMEM int lua_sprite_get_tile(lua_State* L) {
  // Parameter-Prüfung: Wir erwarten die Schirm-Koordinaten x und y von Lua
  if (lua_gettop(L) < 2 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.get_tile(x,y)");
    lua_pushnumber(L, 0); // Fehlerfall: 0 zurückgeben
    return 1;
  }

  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);

  // Rufe deine originale, clevere Ecken-Abfrage auf!
  int tileID = get_tile_raw(x, y);

  // Gib die gefundene Kachel-ID (0 = Luft, >0 = Solid/Block) an Lua zurück
  lua_pushnumber(L, tileID);
  return 1;
}
//-------------------- sprite.item(x,y,tile,ersatz) --------------------------------------------------- 
FLASHMEM int cmd_item(int x, int y, int tile, int m) {
  int value;
  int wx = x + currentHScroll0;
  int w = 16; int h = 16; // Sprite-Größe

  // Wir prüfen 4 Punkte (Ecken)
  int pointsX[4] = {wx, wx + w - 1, wx, wx + w - 1};
  int pointsY[4] = {y, y, y + h - 1, y + h - 1};

  value = 0;

  for (int i = 0; i < 4; i++) {
    int tx = pointsX[i] >> 4;
    int ty = pointsY[i] >> 4;

    if (tilemapL0[tx * 30 + ty] == tile) {
      // GEFUNDEN! 
      //------------------ nur löschen, wenn gewünscht ----------------------
      if (m == 1) {
        tilemapL0[tx * 30 + ty] = 0; // Im PSRAM löschen
        // Sofort vom Bildschirm löschen (+16 Offset beachten)
        int vgaX = tx % 40;
        unsigned char zeroTile = 0 ;//+ 16;
        vga.tile_draw_col(0, vgaX, ty, &zeroTile, 1);
      }
      //------------------ nur löschen, wenn gewünscht ----------------------
      value = 1; // "Erfolg" 
      break;
    }
  }
  return value;
}

FLASHMEM int lua_sprite_item(lua_State* L) {
  // Parameter-Prüfung: Wir erwarten x, y, tile_id und das loeschen-Flag (0 oder 1)
  if (lua_gettop(L) < 4 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    zeigeFehlerPopup("FEHLER", "erwartet sprite.item(x,y,id,erase)");
    lua_pushnumber(L, 0);
    return 1;
  }

  int x    = (int)lua_tonumber(L, 1);
  int y    = (int)lua_tonumber(L, 2);
  int tile = (int)lua_tonumber(L, 3);
  int m    = (int)lua_tonumber(L, 4);
  int result = cmd_item(x, y, tile, m);

  
  lua_pushnumber(L, result);                          // Ergebnis 1 = Eingesammelt, 0 = Nichts
  return 1;
}

//######################################################## SOUND #######################################################
FLASHMEM int lua_native_sound_play(lua_State* L) {
  if (lua_gettop(L) < 3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    lua_pushboolean(L, false);
    return 1;
  }

  int kanal      = (int)lua_tonumber(L, 1);
  int frequenz   = (int)lua_tonumber(L, 2);
  int lautstaerke = (int)lua_tonumber(L, 3);

  if (kanal >= 0 && kanal < 6) {
    sound(kanal, frequenz, lautstaerke);
    lua_pushboolean(L, true);
  } else {
    lua_pushboolean(L, false);
  }

  return 1;
}

// Lua-Befehl: sound.stop(optionaler_kanal)
FLASHMEM int lua_native_sound_stop(lua_State* L) {
  if (lua_gettop(L) >= 1 && lua_isnumber(L, 1)) {
    int kanal = (int)lua_tonumber(L, 1);
    if (kanal >= 0 && kanal < 6) {
      sound(kanal, 0, 0); // Schaltet diesen einen Kanal stumm
    }
  } else {
    snd_Reset(); // Schaltet alle 6 Kanäle auf einmal stumm
  }
  return 0;
}

//################################################### Lua-RAM-Zuweisung ###############################################
void* lua_psram_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
  (void)ud; 
  (void)osize;

  // 1. Speicher freigeben
  if (nsize == 0) {
    if (ptr) extmem_free(ptr); // Nutzt das offizielle PSRAM-Free des Cores
    return NULL;
  }

  // 2. Speicher neu anfordern oder verändern
  if (ptr == NULL) {
    // Jedes Lua-Objekt (egal wie klein) landet direkt im 8MB PSRAM!
    return extmem_malloc(nsize); 
  } else {
    // Sicheres, bus-konformes Vergrößern/Schrumpfen im PSRAM über den Core
    return extmem_realloc(ptr, nsize);
  }
}


//######################################################## SETUP #######################################################
FLASHMEM void setup() {
  Serial.begin(9600);
  delay(200);
  myusb.begin();
  keyboard1.attachPress(OnPress);
  keyboard1.attachRelease(OnRelease); // Stoppt das Repeat-System

  // VGA Setup
  vga_error_t err = vga.begin(VGA_MODE_640x480);//352x240);
  vga.get_frame_buffer_size(&fb_width, &fb_height);
  vga.begin_gfxengine(1, 256, 256);
  
  memset(termBuffer, '\0', sizeof(termBuffer));
  vga.clear(bColor);

  // SD-Karte initialisieren
  if (!SD.begin(chipSelect)) {
    vga_print_str("SD-Karte: FEHLGESCHLAGEN\n");
  }
  

  unsigned int initialSize = 1048576;  // 1MB scheint mehr als ausreichend zu sein

  // 1. ZÜNDUNG: Wir nutzen lua_newstate mit unserem sauberen extmem-Allocator
  L = lua_newstate(lua_psram_allocator, NULL, initialSize);
  
  if (L == NULL) {
    vga_print_str("Lua-PSRAM-Initialisierung: FEHLGESCHLAGEN\n");
    while(1); // Stop falls das PSRAM physisch nicht erkannt wird
  }

  // ------------------------ Lua Setup -----------------------------
  //L = luaL_newstate();
  luaL_openlibs(L);

  lua_register(L, "print", lua_custom_print);
  lua_register(L, "write", lua_custom_write);
  lua_register(L, "delay", lua_delay);
  lua_register(L, "edit", lua_cmd_edit);
  lua_register(L, "hardware_control", lua_hardware_control);
  lua_register(L, "run", lua_dofile);
  lua_register(L, "flash", lua_load_hex);
  lua_register(L, "inkey", lua_global_inkey);
  lua_register(L, "waitkey", lua_global_waitkey);


  // Eine neue Tabelle für die Systemfunktionen in Lua erstellen
  lua_newtable(L);
  lua_pushcfunction(L, sys_timer);    lua_setfield(L, -2, "timer");
  lua_pushcfunction(L, sys_get_time); lua_setfield(L, -2, "gettime");
  lua_pushcfunction(L, sys_get_date); lua_setfield(L, -2, "getdate");
  lua_pushcfunction(L, lua_sys_load); lua_setfield(L, -2, "load");
  lua_pushcfunction(L, lua_sys_info); lua_setfield(L, -2, "info");
  lua_pushcfunction(L, lua_sys_hexmon); lua_setfield(L, -2, "hexmon");
  lua_pushcfunction(L, lua_sys_get_hardware_data); lua_setfield(L, -2, "get_hardware_data");
  // Die Tabelle global unter dem Namen "system" registrieren
  lua_setglobal(L, "sys");

  // Eine neue Tabelle für die SD-Bibliothek in Lua erstellen
  lua_newtable(L);
  // Die C++ Funktionen der Tabelle zuweisen
  lua_pushcfunction(L, lua_sd_ls);     lua_setfield(L, -2, "ls");
  lua_pushcfunction(L, lua_sd_remove); lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, lua_sd_mkdir);  lua_setfield(L, -2, "mkdir");
  lua_pushcfunction(L, lua_sd_rmdir);  lua_setfield(L, -2, "rmdir");
  lua_pushcfunction(L, lua_sd_cd);     lua_setfield(L, -2, "cd");
  lua_pushcfunction(L, lua_sd_copy);   lua_setfield(L, -2, "copy");
  lua_pushcfunction(L, lua_sd_rename); lua_setfield(L, -2, "rename");
  lua_pushcfunction(L, lua_sd_exists); lua_setfield(L, -2, "exist");
  lua_pushcfunction(L, lua_sd_append); lua_setfield(L, -2, "append");
  lua_pushcfunction(L, lua_sd_write);  lua_setfield(L, -2, "write");
  lua_pushcfunction(L, lua_sd_read_lines); lua_setfield(L, -2, "readline");
  lua_pushcfunction(L, lua_sd_mount);   lua_setfield(L, -2, "mount");
  lua_pushcfunction(L, lua_sd_unmount); lua_setfield(L, -2, "unmount");
  lua_pushcfunction(L, lua_sd_cat);     lua_setfield(L, -2, "cat");
  lua_pushcfunction(L, lua_sd_get_file_list); lua_setfield(L, -2, "listfile");
  lua_pushcfunction(L, lua_sd_pwd);     lua_setfield(L, -2, "pwd");
  
  lua_setglobal(L, "sd");         // Die Tabelle global unter dem Namen "sd" registrieren

  srand(analogRead(0) + micros());  // Einmaliger Seed für die Zufallszahlen beim Systemstart

  // Neue Tabelle für die VGA-Grafik erstellen
  lua_newtable(L);
  lua_pushcfunction(L, lua_vga_color);         lua_setfield(L, -2, "color");
  lua_pushcfunction(L, lua_vga_pset);          lua_setfield(L, -2, "pset");
  lua_pushcfunction(L, lua_vga_line);          lua_setfield(L, -2, "line");
  lua_pushcfunction(L, lua_vga_rect);          lua_setfield(L, -2, "rect");
  lua_pushcfunction(L, lua_vga_box);           lua_setfield(L, -2, "box");
  lua_pushcfunction(L, lua_vga_ellipse);       lua_setfield(L, -2, "ellipse");
  lua_pushcfunction(L, lua_vga_filledellipse); lua_setfield(L, -2, "fillellipse");
  lua_pushcfunction(L, lua_vga_text);          lua_setfield(L, -2, "text");
  lua_pushcfunction(L, lua_vga_cls);           lua_setfield(L, -2, "cls");
  lua_pushcfunction(L, lua_vga_pos);           lua_setfield(L, -2, "pos");
  lua_pushcfunction(L, lua_vga_print);         lua_setfield(L, -2, "print");
  lua_pushcfunction(L, lua_vga_get_colors);    lua_setfield(L, -2, "gcolor");
  lua_pushcfunction(L, lua_vga_set_title);     lua_setfield(L, -2, "setTitle");
  lua_pushcfunction(L, lua_vga_close_window);  lua_setfield(L, -2, "closeWindow");
  lua_pushcfunction(L, lua_vga_open_window);   lua_setfield(L, -2, "openWindow");
  lua_pushcfunction(L, lua_vga_update_window); lua_setfield(L, -2, "updateWindow");
  lua_setglobal(L, "vga");        // Die Tabelle "vga" registrieren

  // Eine neue globale Tabelle "sprite" in der Lua-VM erstellen
  lua_newtable(L);

  lua_pushcfunction(L, lua_sprite_get);      lua_setfield(L, -2, "get");
  lua_pushcfunction(L, lua_sprite_draw);     lua_setfield(L, -2, "draw");
  lua_pushcfunction(L, lua_sprite_update);   lua_setfield(L, -2, "update");
  lua_pushcfunction(L, lua_sprite_hide);     lua_setfield(L, -2, "hide");
  lua_pushcfunction(L, lua_sprite_cls);      lua_setfield(L, -2, "cls");
  lua_pushcfunction(L, lua_sprite_animate);  lua_setfield(L, -2, "animate");
  lua_pushcfunction(L, lua_sprite_mapload);  lua_setfield(L, -2, "mapload");
  lua_pushcfunction(L, lua_sprite_tsheet);   lua_setfield(L, -2, "tsheet");
  lua_pushcfunction(L, lua_sprite_scroll);   lua_setfield(L, -2, "scroll");
  lua_pushcfunction(L, lua_sprite_load_single); lua_setfield(L, -2, "load");
  lua_pushcfunction(L, lua_sprite_tile_draw);lua_setfield(L, -2, "tile");
  lua_pushcfunction(L, lua_sprite_get_tile); lua_setfield(L, -2, "get_tile");
  lua_pushcfunction(L, lua_sprite_item);     lua_setfield(L, -2, "item");
  
  // Die Tabelle global unter dem Namen "sprite" registrieren
  lua_setglobal(L, "sprite");

  // Tabelle mit den vga_t4 treibereigenen Soundfunktionen
  lua_newtable(L);
  
  lua_pushcfunction(L, lua_native_sound_play); lua_setfield(L, -2, "play");
  lua_pushcfunction(L, lua_native_sound_stop); lua_setfield(L, -2, "stop");
  
  // Die Tabelle global unter dem Namen "sound" registrieren
  lua_setglobal(L, "sound");


  start_screen();
  cursorX = 0;
  vga_print_str("> ");

}


//**************************** Start-Bildschirm, entweder über init.lua-Skript oder Standard ***************************************
FLASHMEM void start_screen() {
  delay(100); // Kurze Pause
  if (SD.exists("/lua/init.lua")) {
    File bootFile = SD.open("/lua/init.lua", FILE_READ);
    if (bootFile) {
      size_t fileSize = bootFile.size();

      char* bootBuffer = (char*)malloc(fileSize + 1);         // Dynamischen temporären Speicher im RAM1 für den Boot-Text anfordern
      if (bootBuffer != NULL) {
        bootFile.readBytes(bootBuffer, fileSize);
        bootBuffer[fileSize] = '\0';
        bootFile.close();

        if (luaL_dostring(L, bootBuffer) != LUA_OK) {         // Übergabe des geladenen Text-Strings an den Lua-Kern
          const char* error_msg = lua_tostring(L, -1);
          vga_print_str("Fehler in init.lua: ");
          vga_print_str(error_msg);
          vga_print_str("\n\r");
          lua_pop(L, 1);
        }
        free(bootBuffer);                                     // Speicher wieder freigeben
      } else {
        vga_print_str("Fehler: Zu wenig RAM fuer Boot-Puffer!\n\r");
        bootFile.close();
      }
    } else {
      vga_print_str("Fehler: Konnte init.lua nicht oeffnen!\n\r");
    }
  } else {

    //keine init.lua da, dann normaler Startbildschirm
    vga_print_str("--- Teensy 4.1 Standalone Lua Computer ---\n");
    vga_print_str("\n");
    vga_print_str("USB-Tastatur & VGA bereit.\n");
  }

  // Cursor für die anschließende REPL-Eingabe
  vga_print_str("> ");
}

//######################################################## LOOP ########################################################
void loop() {
  myusb.Task();
  handleRepeat();                                       // Prüfen, ob eine Taste wiederholt werden muss

  if (lastUsbChar != -1) {
    //Serial.print(lastUsbChar);
    handleIncomingChar(lastUsbChar);
    lastUsbChar = -1;
  }

  while (Serial.available() > 0) {
    char c = Serial.read();
    handleIncomingChar(c);
  }

  if (millis() - lastCursorBlink >= blinkInterval) {
    lastCursorBlink = millis();
    cursorVisible = !cursorVisible; // Zustand umkehren

    int vgaX = cursorX * 8;
    int vgaY = cursorY * 8;
    // Die beiden Leisten fest verankern
    drawTitleBar();
    drawStatusBar();

    if (cursorVisible) {
      vga.drawText(vgaX, vgaY, "_", fColor, bColor, false);
    } else {
      char currentChar = termBuffer[cursorY][cursorX];
      if (currentChar == '\0') {
        vga.drawText(vgaX, vgaY, " ", fColor, bColor, false);    // Leerzeichen, falls leer
      } else {
        char singleCharStr[] = { currentChar, '\0' };
        vga.drawText(vgaX, vgaY, singleCharStr, fColor, bColor, false);
      }
    }
  }
}
