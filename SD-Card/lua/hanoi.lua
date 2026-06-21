-- ============================================================================
-- TÜRME VON HANOI FÜR TEENSY VGA
-- ============================================================================

-- 1. VGA-Farben (RGB565 Beispielwerte)
local FARBE_SCHWARZ = 0
local FARBE_WEISS   = 255
local FARBE_GRAU    = 114
local FARBE_ROT     = 196
local FARBE_GRUEN   = 24
local FARBE_BLAU    = 11
local FARBE_GELB    = 252
local FARBE_MAGENTA = 163

local scheibenFarben = { FARBE_ROT, FARBE_GELB, FARBE_BLAU, FARBE_GRUEN, FARBE_MAGENTA,FARBE_WEISS }

-- 2. Spiel-Konfiguration
local anzahlScheiben = 5 
local turmX = { 160, 320, 480 } 
local basisY = 350               
local stabHoehe = 150            
local scheibenHoehe = 15          
local maxScheibenBreite = 100    

local tuerme = { {}, {}, {} }
local zuege = 0
local auswahlTurm = nil 
local statusText = "Waehle Quell-Turm (1-3) oder ESC/Q"

-- Hilfsfunktion: Wandelt Pixel in Zeichen-Positionen um (geteilt durch 8)
local function textPos(pixelX, pixelY)
    vga.pos(math.floor(pixelX / 8), math.floor(pixelY / 8))
end

-- 3. Funktionen
local function spielInitialisieren()
    tuerme = { {}, {}, {} }
    zuege = 0
    auswahlTurm = nil
    statusText = "Waehle Quell-Turm (1-3) oder ESC/Q"
    
    for i = anzahlScheiben, 1, -1 do
        table.insert(tuerme[1], i) -- Scheiben auf den ersten Turm legen
    end
    vga.cls()
end

local function zeichneSpielfeld()
    vga.cls()
    vga.color(FARBE_WEISS, FARBE_SCHWARZ)
    
    -- Titel anzeigen (Text-Raster!)
    textPos(220, 30)
    vga.print("--- TUERME VON HANOI ---")
    
    -- Bodenplatte quer (Pixel-Grafik)
    vga.box(80, basisY, 480, 10, FARBE_GRAU)
    
    for t = 1, 3 do
        local x = turmX[t]
        -- Vertikaler Stab (Pixel-Grafik)
        vga.box(x - 4, basisY - stabHoehe, 8, stabHoehe, FARBE_GRAU)
        
        -- Turm-Nummern darunter schreiben (Text-Raster!)
        textPos(x , basisY + 32)
        vga.print(tostring(t))
        
        -- Scheiben auf diesem Turm zeichnen (Pixel-Grafik)
        for sIndex, groesse in ipairs(tuerme[t]) do
            local breite = 30 + (groesse * (maxScheibenBreite / anzahlScheiben))
            local sx = x - (breite / 2)
            local sy = basisY - (sIndex * scheibenHoehe)
            
            local farbe = scheibenFarben[groesse] or FARBE_WEISS
            vga.box(sx, sy, breite, scheibenHoehe, farbe)
            vga.line(sx, sy, sx + breite, sy, FARBE_SCHWARZ)
            vga.line(sx, sy + scheibenHoehe, sx + breite, sy + scheibenHoehe, FARBE_SCHWARZ)
        end
    end
    
    -- Wenn ein Quell-Turm gewählt wurde, Markierung anzeigen
    if auswahlTurm then
        local ax = turmX[auswahlTurm]
        textPos(ax - 8, basisY - stabHoehe - 20)
        vga.print("[X]")
        statusText = "Waehle Ziel-Turm (1-3) fuer die Scheibe"
    end
    
    -- Status und Info-Texte anzeigen (Text-Raster!)
    textPos(60, 390)
    vga.color(FARBE_GELB, FARBE_SCHWARZ)
    vga.print("Zuege: " .. zuege)
    
    textPos(60, 420)
    vga.color(FARBE_WEISS, FARBE_SCHWARZ)
    vga.print(statusText)
end

local function bewegeScheibe(von, nach)
    if #tuerme[von] == 0 then
        statusText = "Fehler: Quell-Turm ist leer!"
        auswahlTurm = nil
        return
    end
    
    local obersteVon = tuerme[von][#tuerme[von]]
    
    if #tuerme[nach] > 0 then
        local obersteNach = tuerme[nach][#tuerme[nach]]
        if obersteVon > obersteNach then
            statusText = "Ungueltiger Zug! Groessere nicht auf kleinere."
            auswahlTurm = nil
            return
        end
    end
    
    table.remove(tuerme[von])
    table.insert(tuerme[nach], obersteVon)
    
    zuege = zuege + 1
    statusText = "Zug erfolgreich! Naechster Zug: (1-3)"
    auswahlTurm = nil
end

local function pruefeSieg()
    -- Wenn alle Scheiben auf dem 2. oder 3. Turm liegen
    if #tuerme[2] == anzahlScheiben or #tuerme[3] == anzahlScheiben then
        zeichneSpielfeld()
        
        textPos(260, 110)
        vga.color(FARBE_GRUEN, FARBE_SCHWARZ)
        vga.print("!!! GEWONNEN !!!")
        
        textPos(252, 140)
        vga.color(FARBE_WEISS, FARBE_SCHWARZ)
        vga.print("Sieg in " .. zuege .. " Zuegen!")
        textPos(200, 170)
        vga.print("Druecke ENTER fuer neues Spiel")
        
        while true do
            local t = inkey()
            if t == 13 or t == "enter" then
                spielInitialisieren()
                break
            elseif t == 27 or t == "q" or t == "Q" then
                return false
            end
            delay(10)
        end
    end
    return true
end

-- ============================================================================
-- MAIN LOOP (Das Hauptprogramm, das gefehlt hatte)
-- ============================================================================
spielInitialisieren()
zeichneSpielfeld()

local spielLaeuftNoch = true

while spielLaeuftNoch do
    local taste = inkey()
    
    if taste then
        -- Abbruch-Tasten abfangen (Code 27 = ESC, 'q' oder 'Q')
        if taste == 27 or taste == "q" or taste == "Q" then
            spielLaeuftNoch = false
        
        -- Tasten '1', '2' oder '3' (bzw. entsprechende ASCII-Codes 49, 50, 51)
        elseif taste == "1" or taste == 49 then
            if not auswahlTurm then 
                auswahlTurm = 1 
            else 
                bewegeScheibe(auswahlTurm, 1) 
            end
            zeichneSpielfeld()
            
        elseif taste == "2" or taste == 50 then
            if not auswahlTurm then 
                auswahlTurm = 2 
            else 
                bewegeScheibe(auswahlTurm, 2) 
            end
            zeichneSpielfeld()
            
        elseif taste == "3" or taste == 51 then
            if not auswahlTurm then 
                auswahlTurm = 3 
            else 
                bewegeScheibe(auswahlTurm, 3) 
            end
            zeichneSpielfeld()
        end
        
        -- Prüfen, ob das Spiel gelöst wurde
        if spielLaeuftNoch then
            spielLaeuftNoch = pruefeSieg()
        end
    end
    
    delay(10) -- Entlastung für den Teensy Kern
end

-- Spielfeld aufräumen bei Rückkehr ins Terminal
vga.cls()
print("Tuerme von Hanoi beendet. Zurueck zum Terminal.")