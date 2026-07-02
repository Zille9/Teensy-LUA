-- ============================================================================
-- TETRIS ARCADE-MODUL (tetris.lua)
-- HIGH-PERFORMANCE ENGINE MIT FLACKERFREIEM VGA-text
-- ============================================================================
local Tetris = {}

-- --- DESIGN-EINSTELLUNGEN (VGA 640x480) ---
local SLOT_BLOCK = 4        -- Sprite-Slot für das Grundquadrat eines Steins
local MATRIX_X = 220        -- Start-X auf dem Bildschirm (zentriert)
local MATRIX_Y = 60         -- Start-Y auf dem Bildschirm
local BLOCK_SIZE = 16       -- Jedes Quadrat ist 16x16 Pixel groß

-- --- SPIEL-ZUSTAND ---
local score = 0
local lines = 0
local level = 1
local gameOver = false
local fallSpeed = 25        -- Alle wie viele Frames der Stein fällt (weniger = schneller)
local frameZaehler = 0

-- Das Spielfeld: 10 Spalten breit, 20 Reihen hoch (0 = leer, >0 = Farbindex)
local spielfeld = {}

-- Definition der 7 Tetris-Steine (Tetrominos)
local STEINE = {
    { {1,1,1,1} },                                      -- I (Cyan)
    { {1,1,1}, {0,1,0} },                               -- T (Lila)
    { {1,1,1}, {1,0,0} },                               -- L (Orange)
    { {1,1,1}, {0,0,1} },                               -- J (Blau)
    { {1,1,0}, {0,1,1} },                               -- Z (Rot)
    { {0,1,1}, {1,1,0} },                               -- S (Grün)
    { {1,1}, {1,1} }                                    -- O (Gelb)
}
local FARBEN = { 31, 224, 200, 4, 192, 24, 240 }        -- VGA-Farbindizes für die Steine

-- Aktueller und nächster Stein
local aktuellerStein = {}
local aktuellerFarbIndex = 1
local steinX, steinY = 4, 1
local naechsterStein = {}
local naechsterFarbIndex = 1

-- Initialisiert das Spielfeld und baut das Block-Sprite
function Tetris.init()
    vga.cls()
    
    -- 1. HARDWARE-SPRITE FÜR EINEN BLOCK GENERIEREN
    vga.box(0, 0, BLOCK_SIZE, BLOCK_SIZE, 0)
    vga.box(1, 1, BLOCK_SIZE-2, BLOCK_SIZE-2, 255)   -- Weißer Grundblock (Farbe wird im Draw angepasst)
    vga.box(3, 3, BLOCK_SIZE-6, BLOCK_SIZE-6, 100)   -- Innerer Schatten für 3D-Look
    sprite.get(0, 0, SLOT_BLOCK)
    vga.cls()
    
    -- 2. MATRIX LEEREN
    spielfeld = {}
    for r = 1, 20 do
        spielfeld[r] = {}
        for s = 1, 10 do spielfeld[r][s] = 0 end
    end
    
    -- 3. ZUSTAND ZURÜCKSETZEN
    score = 0
    lines = 0
    level = 1
    gameOver = false
    fallSpeed = 30
    frameZaehler = 0
    
    --math.randomseed(os.freeram() or 1234) -- Zufall streuen über freien RAM-Wert
    
    -- Erste Steine generieren
    naechsterFarbIndex = math.random(1, #STEINE)
    naechsterStein = STEINE[naechsterFarbIndex]
    Tetris.neuerStein()
end

-- Holt den nächsten Stein in die Mitte und würfelt den Folgestein aus
function Tetris.neuerStein()
    aktuellerStein = naechsterStein
    aktuellerFarbIndex = naechsterFarbIndex
    steinX = 4
    steinY = 1
    
    naechsterFarbIndex = math.random(1, #STEINE)
    naechsterStein = STEINE[naechsterFarbIndex]
    
    -- Prüfen, ob der neue Stein sofort kollidiert -> Game Over
    if Tetris.pruefeKollision(steinX, steinY, aktuellerStein) then
        gameOver = true
    end
end

-- Mathematische Kollisionsprüfung gegen Spielfeldränder und feste Blöcke
function Tetris.pruefeKollision(tx, ty, stein)
    for r = 1, #stein do
        for s = 1, #stein[r] do
            if stein[r][s] ~= 0 then
                local feldX = tx + s - 1
                local feldY = ty + r - 1
                
                -- Wände prüfen
                if feldX < 1 or feldX > 10 or feldY > 20 then return true end
                -- Festen Block prüfen
                if feldY > 0 and spielfeld[feldY][feldX] ~= 0 then return true end
            end
        end
    end
    return false
end

-- Rotiert die 2D-Matrix des aktuellen Steins um 90 Grad nach rechts
function Tetris.rotiereStein()
    local neu = {}
    local r_max = #aktuellerStein
    local s_max = #aktuellerStein[1]
    
    for s = 1, s_max do
        neu[s] = {}
        for r = 1, r_max do
            neu[s][r] = aktuellerStein[r_max - r + 1][s]
        end
    end
    
    if not Tetris.pruefeKollision(steinX, steinY, neu) then
        aktuellerStein = neu
    end
end

-- Verschmilzt den gefallenen Stein fest mit der Spielfeldmatrix
function Tetris.steinSperren()
    for r = 1, #aktuellerStein do
        for s = 1, #aktuellerStein[r] do
            if aktuellerStein[r][s] ~= 0 then
                local feldY = steinY + r - 1
                local feldX = steinX + s - 1
                if feldY > 0 then
                    spielfeld[feldY][feldX] = aktuellerFarbIndex
                end
            end
        end
    end
    
    Tetris.pruefeLinien()
    Tetris.neuerStein()
end

-- Prüft auf volle Reihen, löscht sie und erhöht Score/Level
function Tetris.pruefeLinien()
    local geloeschteLinien = 0
    local r = 20 -- Wir starten ganz unten
    
    while r >= 1 do
        local voll = true
        for s = 1, 10 do
            if spielfeld[r][s] == 0 then 
                voll = false
                break 
            end
        end
        
        if voll then
            local volleZeile = spielfeld[r]
            table.remove(spielfeld , r)
            
            -- Oben eine neue, leere Zeile reinschieben
            for s = 1, 10 do volleZeile[s] = 0 end
            table.insert(spielfeld, 1, volleZeile)
            
            geloeschteLinien = geloeschteLinien + 1
            -- WICHTIG: 'r' wird hier NICHT verändert! 
            -- Da alles nachgerückt ist, prüfen wir im nächsten Durchlauf dieselbe Zeilennummer noch einmal.
        else
            -- Nur wenn die Zeile NICHT voll war, gehen wir eine Etage höher
            r = r - 1
        end
    end
    
    if geloeschteLinien > 0 then
        lines = lines + geloeschteLinien
        local punkteTab = {100, 300, 500, 800}
        score = score + (punkteTab[geloeschteLinien] or 100) * level
        
        level = math.floor(lines / 5) + 1
        fallSpeed = math.max(5, 30 - (level * 3))

        collectgarbage("step",1)

    end
end

-- Spiel-Logik pro Frame updaten
function Tetris.update()
    frameZaehler = frameZaehler + 1
    
    if frameZaehler >= fallSpeed then
        frameZaehler = 0
        -- Stein eine Stufe absenken
        if not Tetris.pruefeKollision(steinX, steinY + 1, aktuellerStein) then
            steinY = steinY + 1
        else
            Tetris.steinSperren()
        end
    end
end

-- Der flackerfreie Render-Pass unter Nutzung Ihres VGA-texts
local function rendern()
    --vga.box(0, 0, 640, 480, 0)
    
    -- 1. SPIELFELD-RAHMEN ZEICHNEN
    local breite = 10 * BLOCK_SIZE
    local hoehe = 20 * BLOCK_SIZE
    vga.rect(MATRIX_X - 4, MATRIX_Y - 4, MATRIX_X - 4 + breite + 8, MATRIX_Y - 4 +hoehe + 8, 40) -- Grauer Außenrahmen
    --vga.box(MATRIX_X, MATRIX_Y, breite, hoehe, 0)                  -- Schwarzes Spielfeld-Innere

    -- 2. FESTE BLÖCKE IM SPIELFELD ZEICHNEN
    for r = 1, 20 do
        for s = 1, 10 do
            local farbe = spielfeld[r][s]
            if farbe > 0 then
                -- Wir zeichnen die festen Blöcke direkt als VGA-Rechtecke mit 1px Rahmen für 3D-Look
                local bx = MATRIX_X + (s - 1) * BLOCK_SIZE
                local by = MATRIX_Y + (r - 1) * BLOCK_SIZE
                vga.box(bx, by, BLOCK_SIZE - 1, BLOCK_SIZE - 1, FARBEN[farbe])
                vga.box(bx + 2, by + 2, BLOCK_SIZE - 5, BLOCK_SIZE - 5, 179) -- Innerer Schatten
            end
        end
    end

    -- 3. AKTUELL FALLENDEN STEIN ZEICHNEN
    local hw_id = 10 
    for r = 1, #aktuellerStein do
        for s = 1, #aktuellerStein[r] do
            if aktuellerStein[r][s] ~= 0 then
                sprite.draw(hw_id, MATRIX_X + (steinX + s - 2) * BLOCK_SIZE, MATRIX_Y + (steinY + r - 2) * BLOCK_SIZE, SLOT_BLOCK)
                hw_id = hw_id + 1
            end
        end
    end

    -- Nicht genutzte Hardware-Sprites aufräumen
    for id = hw_id, 64 do sprite.hide(id) end

    -- 4. NÄCHSTEN STEIN IN DER VORSCHAU ZEICHNEN
    local vX, vY = 440, 100
    vga.rect(vX - 4, vY - 4, vX - 4 + 85, vY - 4 + 85, 40)
    --vga.box(vX, vY, 77, 77, 0)
    for r = 1, #naechsterStein do
        for s = 1, #naechsterStein[r] do
            if naechsterStein[r][s] ~= 0 then
                -- Vorschau-Blöcke statisch zeichnen (ohne HW-ID Konflikte)
                vga.box(vX + 12 + (s-1)*BLOCK_SIZE, vY + 16 + (r-1)*BLOCK_SIZE, BLOCK_SIZE-1, BLOCK_SIZE-1, FARBEN[naechsterFarbIndex])
            end
        end
    end

    -- ====================================================================
    -- 5. HUD PER TRANSPARENTEM text DRAUFSETZEN (BOMBENFEST!)
    -- ====================================================================
    vga.text(28, 2, "=== TETRIS ARCADE ===", 47,0)
    
    -- Status-Block Links neben dem Spielfeld
    vga.text(5, 12, "SCORE:", 255)
    vga.text(5, 15, string.format("%06d", score), 240,0)
    
    vga.text(5, 21, "LINES:", 255)
    vga.text(5, 24, string.format("%03d", lines), 24,0)
    
    vga.text(5, 27, "LEVEL:", 255)
    vga.text(5, 30, string.format("%02d", level), 196,0)

    -- Info-Block Rechts neben der Vorschau
    vga.text(55, 10, "NEXT PIECE:", 255)
    
    vga.text(55, 37, "STEUERUNG:", 40)
    vga.text(55, 40, "A / D : Links/Rechts", 255,0)
    vga.text(55, 42, "W     : Rotieren", 255,0)
    vga.text(55, 45, "S     : Schnell Fall", 255,0)
    vga.text(55, 48, "ESC   : Beenden", 196,0)
    --sprite.update()
end

-- Hauptschleife (Engine-Loop)
function Tetris.run()
    Tetris.init()
    local running = true
    
    while running do
        local taste = inkey()
        
        if taste == 27 then -- ESC
            running = false
            
        -- A oder Pfeil-Links: Nach Links verschieben
        elseif (taste == 97 or taste == 216) then
            if not Tetris.pruefeKollision(steinX - 1, steinY, aktuellerStein) then
                steinX = steinX - 1
            end
            
        -- D oder Pfeil-Rechts: Nach Rechts verschieben
        elseif (taste == 100 or taste == 215) then
            if not Tetris.pruefeKollision(steinX + 1, steinY, aktuellerStein) then
                steinX = steinX + 1
            end
            
        -- S oder Pfeil-Unten: Schneller herabfallen lassen
        elseif (taste == 115 or taste == 218) then

            if not Tetris.pruefeKollision(steinX, steinY + 1, aktuellerStein) then
                steinX = steinX
                steinY = steinY + 1
                score = score + 1 -- Bonuspunkt für aktives Runterdrücken
            end
        -- W oder Pfeil-Oben: Rotieren
        elseif (taste == 119 or taste == 217) then
            Tetris.rotiereStein()
        end
    --Logik updaten (Stein fällt zeitgesteuert)
        if not gameOver then
            Tetris.update()
        else
            rendern()
            vga.text(34, 20, " GAME OVER ", 192,0)
            vga.text(30, 23, "ESC zum Verlassen", 255,0)
            local tast = waitkey() 
            if tast == 27 then
               running = false
            end
        end
        if running then
           rendern()
           sprite.update()
                      
        end
    
     end  
-- Nach Spielende alles blitzblank putzen
    for id = 0, 250 do sprite.hide(id) end
    collectgarbage("collect") -- Sofort RAM1 freischaufeln!
    vga.cls()
    print("> ")
end
-- Sofortzündung beim Aufruf
Tetris.run()
return Tetris
