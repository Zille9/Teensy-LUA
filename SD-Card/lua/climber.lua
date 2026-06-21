-- ============================================================================
-- RETRO CLIMBER FÜR TEENSY VGA (640x480)
-- ============================================================================

-- 1. VGA-Farben (RGB565)
local FARBE_SCHWARZ = 0
local FARBE_WEISS   = 255
local FARBE_GRAU    = 114
local FARBE_ROT     = 196
local FARBE_GRUEN   = 24
local FARBE_GELB    = 252

-- 2. Spiel-Konfiguration
local SpielerGroesse = 12
local GegnerGroesse  = 8
local SpielerGeschw  = 7
local EtagenAbstand  = 70
local StartY         = 420 -- Unterste Etage

-- Etagen-Y-Koordinaten berechnen (5 Etagen)
local etagenY = { StartY, StartY - EtagenAbstand, StartY - (EtagenAbstand*2), StartY - (EtagenAbstand*3), StartY - (EtagenAbstand*4) }

-- Leitern definieren { xPosition, vonEtage, zuEtage }
local leitern = {
    { x = 150, von = 1, zu = 2 },
    { x = 450, von = 2, zu = 3 },
    { x = 200, von = 3, zu = 4 },
    { x = 400, von = 4, zu = 5 }
}

-- 3. Spiel-Variablen
local spielerX = 50
local spielerY = StartY - SpielerGroesse
local aktuelleEtage = 1
local aufLeiter = false

local gegner = {} -- Array für rollende Hindernisse {x, y, dx, etage}
local punkte = 0
local spielAktiv = true

local letztesUpdate = sys.timer()
local geschwindigkeit = 30 -- Update-Intervall der Physik (ca. 33 FPS)
local letzterGegnerSpawn = sys.timer()
local spawnIntervall = 2500 -- Alle 2.5 Sekunden ein neues Hindernis

-- Hilfsfunktion für Text-Positionierung (Zeichen-Raster: geteilt durch 8)
local function textPos(pixelX, pixelY)
    vga.pos(math.floor(pixelX / 8), math.floor(pixelY / 8))
end

local function spielInitialisieren()
    spielerX = 50
    spielerY = StartY - SpielerGroesse
    aktuelleEtage = 1
    aufLeiter = false
    gegner = {}
    punkte = 0
    spielAktiv = true
    vga.cls()
end

local function spawnGegner()
    -- Hindernisse starten zufällig auf Etage 2, 3, 4 oder 5
    local etage = math.random(2, 5)
    local richtung = (math.random(0, 1) == 0) and 2 or -2
    local startX = (richtung > 0) and 20 or 600
    
    table.insert(gegner, {
        x = startX,
        y = etagenY[etage] - GegnerGroesse,
        dx = richtung,
        etage = etage
    })
end

local function zeichneSpielfeld()
    vga.cls()
    vga.color(FARBE_WEISS, FARBE_SCHWARZ)
    
    -- Titel & Punkte
    textPos(240, 10)
    vga.print("--- RETRO CLIMBER ---")
    textPos(50, 10)
    vga.color(FARBE_GELB, FARBE_SCHWARZ)
    vga.print("PUNKTE: " .. punkte)
    vga.color(FARBE_WEISS, FARBE_SCHWARZ)

    -- 1. Etagen zeichnen (Horizontale Linien)
    for i = 1, #etagenY do
        vga.line(20, etagenY[i], 620, etagenY[i], FARBE_GRAU)
    end
    
    -- Ziel-Plattform oben markieren
    vga.line(300, etagenY[5] - 2, 340, etagenY[5] - 2, FARBE_GELB)
    textPos(305, etagenY[5] - 15)
    vga.print("ZIEL")

    -- 2. Leitern zeichnen (Hintergrund-Elemente)
    for _, l in ipairs(leitern) do
        local yVon = etagenY[l.von]
        local yZu  = etagenY[l.zu]
        -- Zwei vertikale Holme
        vga.line(l.x - 6, yVon, l.x - 6, yZu, FARBE_WEISS)
        vga.line(l.x + 6, yVon, l.x + 6, yZu, FARBE_WEISS)
        -- Sprossen im Abstand von 6 Pixeln
        for sy = yVon, yZu, -6 do
            vga.line(l.x - 6, sy, l.x + 6, sy, FARBE_WEISS)
        end
    end

    -- 3. Gegner zeichnen (Rote Boxen)
    for _, g in ipairs(gegner) do
        vga.box(g.x, g.y, GegnerGroesse, GegnerGroesse, FARBE_ROT)
    end

    -- 4. Spieler zeichnen (Grüne Box)
    vga.box(spielerX, spielerY, SpielerGroesse, SpielerGroesse, FARBE_GRUEN)
end

-- ============================================================================
-- MAIN LOOP
-- ============================================================================
spielInitialisieren()
local spielLaeuftNoch = true

while spielLaeuftNoch do
    -- 1. Tastaturabfrage
    local taste = inkey()
    if taste then
        if taste == 27 or taste == "q" or taste == "Q" then
            spielLaeuftNoch = false
        end
        
        if spielAktiv then
            -- Seitwärts bewegen (nur wenn nicht mitten auf der Leiter)
            if (taste == 216) and not aufLeiter then
                spielerX = spielerX - SpielerGeschw
                if spielerX < 20 then spielerX = 20 end
            elseif (taste == 215) and not aufLeiter then
                spielerX = spielerX + SpielerGeschw
                if spielerX > 620 - SpielerGroesse then spielerX = 620 - SpielerGroesse end
            
            -- Klettern (w / s)
            elseif (taste == 218) then
                -- Prüfen, ob eine Leiter in Reichweite nach oben existiert
                for _, l in ipairs(leitern) do
                    if aktuelleEtage == l.von and math.abs(spielerX + (SpielerGroesse/2) - l.x) < 10 then
                        aufLeiter = true
                        spielerX = l.x - (SpielerGroesse/2) -- Spieler auf Leiter zentrieren
                        spielerY = spielerY - 4
                        if spielerY <= etagenY[l.zu] - SpielerGroesse then
                            spielerY = etagenY[l.zu] - SpielerGroesse
                            aktuelleEtage = l.zu
                            aufLeiter = false
                            punkte = punkte + 10 -- Punkte fürs Erreichen der nächsten Etage
                        end
                        break
                    end
                end
            elseif (taste == 217) then
                -- Prüfen, ob eine Leiter nach unten existiert
                for _, l in ipairs(leitern) do
                    if aktuelleEtage == l.zu and math.abs(spielerX + (SpielerGroesse/2) - l.x) < 10 then
                        aufLeiter = true
                        spielerX = l.x - (SpielerGroesse/2)
                        spielerY = spielerY + 4
                        if spielerY >= etagenY[l.von] - SpielerGroesse then
                            spielerY = etagenY[l.von] - SpielerGroesse
                            aktuelleEtage = l.von
                            aufLeiter = false
                        end
                        break
                    end
                end
            end
        else
            -- Wenn Game Over, drücke ENTER für Neustart
            if taste == 13 or taste == "enter" then
                spielInitialisieren()
            end
        end
    end

    -- 2. Physik & Grafik-Update (Zeitgesteuert)
    local jetzt = sys.timer()
    if spielAktiv and (jetzt - letztesUpdate) >= geschwindigkeit and spielLaeuftNoch then
        letztesUpdate = jetzt
        
        -- Gegner spawnen
        if (jetzt - letzterGegnerSpawn) >= spawnIntervall then
            letzterGegnerSpawn = jetzt
            spawnGegner()
        end
        
        -- Gegner bewegen
        for i = #gegner, 1, -1 do
            local g = gegner[i]
            g.x = g.x + g.dx
            
            -- Wand-Kollision für Gegner (drehen um)
            if g.x < 20 or g.x > 620 - GegnerGroesse then
                g.dx = -g.dx
            end
            
            -- Kollisionsprüfung mit dem Spieler (AABB Box Collision)
            if g.etage == aktuelleEtage and not aufLeiter then
                if spielerX < g.x + GegnerGroesse and
                   spielerX + SpielerGroesse > g.x and
                   spielerY < g.y + GegnerGroesse and
                   spielerY + SpielerGroesse > g.y then
                    spielAktiv = false -- Getroffen! Game Over
                end
            end
        end
        
        -- Sieg-Bedingung prüfen (Ganz oben angekommen und in der Mitte)
        if aktuelleEtage == 5 and spielerX > 290 and spielerX < 350 then
            vga.cls()
            textPos(240, 200)
            vga.color(FARBE_GRUEN, FARBE_SCHWARZ)
            vga.print("!!! SIEG !!!")
            textPos(200, 230)
            vga.color(FARBE_WEISS, FARBE_SCHWARZ)
            vga.print("Finale Punkte: " .. punkte + 100)
            textPos(180, 260)
            vga.print("Druecke ENTER fuer Neustart")
            spielAktiv = false
        end
        
        -- Neu zeichnen
        if spielAktiv then
            zeichneSpielfeld()
        end
        
    elseif not spielAktiv and spielLaeuftNoch then
        -- Game Over Schirm (wird nur gezeichnet, wenn nicht gewonnen wurde)
        if aktuelleEtage < 5 then
            textPos(240, 200)
            vga.color(FARBE_ROT, FARBE_SCHWARZ)
            vga.print("--- GAME OVER ---")
            textPos(180, 230)
            vga.color(FARBE_WEISS, FARBE_SCHWARZ)
            vga.print("Druecke ENTER fuer Neustart")
        end
    end
    
    delay(5)
end

-- Zurück zum Terminal
vga.cls()
print("Climber beendet. Zurueck zum Terminal.")