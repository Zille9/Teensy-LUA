-- ============================================================================
-- INTERAKTIVER JAHRESKALENDER (Raster-Anzeige 3x4 Monate)
-- MIT PRIVATER JAHRESEINGABE UND NATIVEM RTC-HEUTE-HIGHLIGHT
-- ============================================================================

-- --- CONFIGURATION (Farben für Ihr VGA-System) ---
local COL_TITEL       = 255 -- Weiß
local COL_MONAT       = 127 -- Cyan
local COL_WOCHENTAG   = 240 -- Orange
local COL_TAGE        = 255 -- Weiß
local COL_HEUTE       = 196 -- Rot (Markierung für den aktuellen Tag)
local COL_HINTERGRUND = 0   -- Schwarz

-- Monatsnamen und Tage
local MONATE = { "Januar", "Februar", "Maerz", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" }
local TAGE_PRO_MONAT = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }

-- --- MATHEMATISCHE HILFSFUNKTIONEN ---

-- Prüft, ob ein Jahr ein Schaltjahr ist
local function istSchaltjahr(jahr)
    return (jahr % 4 == 0 and jahr % 100 ~= 0) or (jahr % 400 == 0)
end

-- Zellers Kongruenz: Berechnet den Wochentag für den 1. eines Monats
-- Rückgabe: 1 = Montag, 2 = Dienstag, ..., 7 = Sonntag
local function wochentagErster(monat, jahr)
    local m = monat
    local y = jahr
    if m < 3 then
        m = m + 12
        y = y - 1
    end
    local k = y % 100
    local j = math.floor(y / 100)
    
    -- Zeller-Formel für den 1. Tag des Monats
    local h = (1 + math.floor((13 * (m + 1)) / 5) + k + math.floor(k / 4) + math.floor(j / 4) - 2 * j) % 7
    
    -- Umrechnung auf ISO-Wochentag (1 = Montag, ..., 7 = Sonntag)
    local ISO_Wochentag = ((h + 5) % 7) + 1
    return ISO_Wochentag
end

-- --- ZEICHEN-LOGIK ---

-- Zeichnet das komplette 3x4 Kalender-Raster für das gewählte Jahr
local function zeichneKalender(zielJahr)
    vga.cls()
    
    -- Heute-Datum aus Ihrer umgebauten RTC-C++-Funktion holen
    local heuteTag, heuteMonat, heuteJahr = sys.getdate()
    
    -- Große Hauptüberschrift zentrieren
    local titelText = "=== JAHRESKALENDER " .. zielJahr .. " ==="
    vga.text(26, 1, titelText, COL_TITEL, COL_HINTERGRUND)
    vga.text(20, 2, "Druecke eine Taste fuer neue Eingabe / ESC zum Beenden", 127, 0)

    -- Schaltjahr-Anpassung für den Februar
    if istSchaltjahr(zielJahr) then
        TAGE_PRO_MONAT[2] = 29
    else
        TAGE_PRO_MONAT[2] = 28
    end

    -- Schleife über alle 12 Monate im 3x4-Spalten-Raster
    for m = 1, 12 do
        -- Spalten- und Zeilen-Index im Raster berechnen
        local spalte = (m - 1) % 3  -- 0, 1, 2
        local zeile  = math.floor((m - 1) / 3) -- 0, 1, 2, 3
        
        -- Startkoordinaten im Textraster (80x60er Schirm)
        local startX = 2 + (spalte * 26)
        local startY = 5 + (zeile * 13)
        
        -- Monatsname zentriert über dem Block drucken
        local monatsName = MONATE[m]
        local pad = math.floor((20 - string.len(monatsName)) / 2)
        vga.text(startX + pad, startY, monatsName, COL_MONAT, COL_HINTERGRUND)
        
        -- Wochentags-Kopfzeile (Mo bis So)
        vga.text(startX, startY + 1, "Mo Di Mi Do Fr Sa So", COL_WOCHENTAG, COL_HINTERGRUND)
        vga.text(startX, startY + 2, "--------------------", COL_WOCHENTAG, COL_HINTERGRUND)
        
        -- Wo fängt der erste Tag an?
        local startWochentag = wochentagErster(m, zielJahr)
        
        local aktuellerDruckX = startX + ((startWochentag - 1) * 3)
        local aktuellerDruckY = startY + 3
        
        -- Alle Tage des Monats nacheinander einsetzen
        for tag = 1, TAGE_PRO_MONAT[m] do
            local tagText = string.format("%2d", tag)
            
            -- HIGHLIGHT-LOGIK: Ist das der heutige Tag laut RTC?
            local farbe = COL_TAGE
            local hintergrund = COL_HINTERGRUND
            if tag == heuteTag and m == heuteMonat and zielJahr == heuteJahr then
                farbe = COL_TITEL
                hintergrund = COL_HEUTE -- Roter Hintergrund blockiert den heutigen Tag
            end
            
            -- Tag auf den Bildschirm bringen
            vga.text(aktuellerDruckX, aktuellerDruckY, tagText, farbe, hintergrund)
            
            -- Cursor für den nächsten Tag weiterschieben (3 Textspalten Platz)
            aktuellerDruckX = aktuellerDruckX + 3
            
            -- Wenn Sonntag (7. Spalte) erreicht ist -> Zeilenumbruch im Monatsblock
            if (aktuellerDruckX - startX) >= 21 then
                aktuellerDruckX = startX
                aktuellerDruckY = aktuellerDruckY + 1
            end
        end
    end
end

-- ============================================================================
-- INTERAKTIVE HAUPTSCHLEIFE
-- ============================================================================
local running = true

while running do
    -- 1. Eingabe-Aufforderung im Terminal (oder über ein einfaches Textfenster)
    vga.cls()
    vga.text(15, 20, "========================================", COL_WOCHENTAG, 0)
    vga.text(15, 21, "        OS KALENDER-STEUERUNG           ", COL_TITEL, 0)
    vga.text(15, 22, "========================================", COL_WOCHENTAG, 0)
    vga.text(15, 24, "Bitte geben Sie ein Jahr ein (z.B. 2026): ", COL_TAGE, 0)
    
    -- Wir holen uns das aktuelle Jahr als Standardwert aus der RTC, falls der User nur ENTER drückt
    local _, _, rtcJahr = sys.getdate()
    
    -- Hier nutzen wir Ihre Tastatur-Eingabelogik.
    -- Da inkey() Zeichen einzeln liest, bauen wir eine kleine String-Sammlung auf:
    local eingabeString = ""
    vga.text(57, 24, "_", COL_TITEL, 0) -- Cursor-Dummy
    
    while true do
        local t = waitkey(false) -- Nutzt Ihre blockierende Tastatur-Wartefunktion
        
        if t == 13 or t == "enter" then -- ENTER -> Eingabe fertig
            break
        elseif t == 27 then -- ESC -> Komplett abbrechen und zurück ins Terminal
            running = false
            break
        elseif t == 127 and string.len(eingabeString) > 0 then -- BACKSPACE
            eingabeString = string.sub(eingabeString, 1, -2)
            vga.text(57 + string.len(eingabeString), 24, "  ") -- Altes Zeichen putzen
        elseif t >= 48 and t <= 57 then -- Nur Zahlen zulassen
            if string.len(eingabeString) < 4 then
                eingabeString = eingabeString .. string.char(t)
                vga.text(57, 24, eingabeString .. "_", COL_TITEL, 0)
            end
        end
        delay(10)
    end
    
    -- Wenn die Schleife nicht per ESC abgebrochen wurde, Kalender anzeigen
    if running then
        local gewaehltesJahr = tonumber(eingabeString) or rtcJahr
        
        -- Kalender-Raster aufbauen
        zeichneKalender(gewaehltesJahr)
        
        -- Warten, bis der Benutzer eine Taste drückt. ESC beendet, jede andere Taste führt zurück zur Eingabe
        local endTaste = waitkey(false)
        if endTaste == 27 then
            running = false
        end
    end
end

-- Nach dem Beenden Schirm putzen fürs Terminal
vga.cls()