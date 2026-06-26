-- ============================================================================
-- GRAPHICAL DASHBOARD / STARTMENÜ FÜR TEENSY
-- ============================================================================

-- 1. VGA-Farben (RGB565)
local FARBE_SCHWARZ = 0
local FARBE_WEISS   = 255
local FARBE_BLAU    = 3
local FARBE_GELB    = 252
local FARBE_GRAU    = 114
local FARBE_GRUEN   = 24
local FARBE_CYAN    = 127
local FARBE_ORANGE  = 240
-- 2. Spiele-Liste (Tragen Sie hier Ihre exakten Dateinamen von der SD-Karte ein)
local spiele = {
    { name = "Snake (Klassiker)",    datei = "snake.lua" },
    { name = "Tuerme von Hanoi",     datei = "hanoi.lua" },
    { name = "Pong gegen Teensy",    datei = "pong.lua" },
    { name = "Retro Climber",        datei = "climber.lua" },
    { name = "Breakout",             datei = "breakout.lua"}
}
local taste = 0 --- Tastencode ---

vga.cls(FARBE_SCHWARZ)

local auswahl = 1 -- Startposition (1 = erstes Spiel)

-- Hilfsfunktion für Text-Positionierung (Zeichen-Raster: geteilt durch 8)
local function textPos(pixelX, pixelY)
    vga.pos(math.floor(pixelX / 8), math.floor(pixelY / 8))
end

local function zeichneMenue()
    ---vga.cls()
    vga.color(FARBE_WEISS, FARBE_SCHWARZ)
    
    -- Äußerer Design-Rahmen (Pixel-Grafik)
    vga.box(40, 40, 560, 400, FARBE_BLAU)
    vga.box(42, 42, 556, 396, FARBE_BLAU)
    
    -- Haupttitel
    textPos(210, 60)
    vga.color(FARBE_GELB, FARBE_BLAU)
    vga.print("=== TEENSY GAME SYSTEM ===")
    
    textPos(190, 90)
    vga.color(FARBE_ORANGE, FARBE_BLAU)
    vga.print("Waehle ein Spiel aus der Bibliothek")

    -- Spiele-Liste generieren
    local startY = 150
    local zeilenAbstand = 16

    for i = 1, #spiele do
        local aktuellesY = startY + (i - 1) * zeilenAbstand
        
        if i == auswahl then
            -- Markierung für das aktuell ausgewählte Spiel (ein kleiner Pfeil ">")
            textPos(140, aktuellesY)
            vga.color(FARBE_GRUEN, FARBE_BLAU)
            vga.print("-> ")
            
            textPos(180, aktuellesY)
            vga.print(spiele[i].name)
            
            -- Rahmen um den ausgewählten Eintrag ziehen
            vga.rect(130, aktuellesY - 10, 450-120, aktuellesY+5, FARBE_GRUEN)
        else
            -- Normale, nicht ausgewählte Spiele
            textPos(180, aktuellesY)
            vga.color(FARBE_WEISS, FARBE_BLAU)
            vga.print(spiele[i].name)
        end
    end
    
    -- Steuerungshinweise am unteren Rand
    vga.line(60, 380, 580, 380, FARBE_GRAU)
    textPos(90, 400)
    vga.color(FARBE_CYAN, FARBE_BLAU)
    vga.print("[Cursor] Auf/Ab  |  [ENTER] Starten  |  [ESC/Q] Terminal")
end

-- ============================================================================
-- MAIN LOOP
-- ============================================================================
local menueAktiv = true
zeichneMenue()

taste = 0 --- eventuellen Muell im Tastenpuffer loeschen

while menueAktiv do
    taste = inkey()
    
    if taste then
        -- Beenden-Tasten (Escape, q oder Q)
        if taste == 27 or taste == "q" or taste == "Q" then
            menueAktiv = false
            
        -- Navigation nach oben (W oder ASCII-Code)
        elseif taste == 218 then
            auswahl = auswahl - 1
            if auswahl < 1 then auswahl = #spiele end -- Am Anfang nach ganz unten springen
            zeichneMenue()
            
        -- Navigation nach unten (S oder ASCII-Code)
        elseif taste == 217 then
            auswahl = auswahl + 1
            if auswahl > #spiele then auswahl = 1 end -- Am Ende nach ganz oben springen
            zeichneMenue()
            
        -- Spiel starten (ENTER / Code 13)
        elseif taste == 13 then
            local gewaehltesSpiel = spiele[auswahl].datei
            
            -- Prüfen, ob die Datei existiert (nutzt Ihre sd.exist-Funktion)
            if sd.exist(gewaehltesSpiel) then
                vga.cls()
                textPos(200, 220)
                vga.color(FARBE_GELB, FARBE_SCHWARZ)
                vga.print("Lade " .. gewaehltesSpiel .. "...")
                delay(500)
                
                -- Wir nutzen das in C++ registrierte run(), um das Spiel zu starten!
                run(gewaehltesSpiel)
                
                -- Wenn das Spiel beendet wird, kehrt Lua HIERHER zurück.
                -- Wir zeichnen das Menü einfach wieder neu.
                zeichneMenue()
            else
                -- Fehler anzeigen, falls die Datei auf der Karte fehlt
                vga.drawRect(120, 340, 400, 30, FARBE_ROT)
                textPos(140, 350)
                vga.color(FARBE_ROT, FARBE_SCHWARZ)
                vga.print("Fehler: " .. gewaehltesSpiel .. " nicht gefunden!")
                delay(1500)
                zeichneMenue()
            end
        end
    end
    
    delay(10) -- Entlastung für den Core
end

-- Terminal wiederherstellen beim Verlassen des Menüs
vga.cls()
print("Dashboard geschlossen. Zurueck zum Terminal.")