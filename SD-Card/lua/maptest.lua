-- ============================================================================
-- FULL RETRO JUMP-'N'-RUN ENGINE (game.lua)
-- INTEGRATION VON TASTENABFRAGE, PHYSICS, SCROLLING UND ITEM-COLLECTION
-- ============================================================================

-- --- SPIEL-CONFIGURATION ---
local MUENZ_ID = 5      -- Kachel-ID für die einsammelbare Münze
local GRAVITY  = 1    -- Schwerkraft-Beschleunigung pro Frame
local JUMP_IMPULS = -16 -- Sprungkraft nach oben
local LAUF_GESCHW = 4   -- Laufgeschwindigkeit in Pixeln

-- --- START-VARIABLEN (Spieler-Koordinaten auf dem Bildschirm) ---
local playerX   = 100
local playerY   = 200
local velocityY = 0      -- Aktuelle Fall-/Sprunggeschwindigkeit
local istAmBoden = false
local score     = 0

-- ============================================================================
-- 1. HARDWARE-INITIALISIERUNG
-- ============================================================================
sprite.cls(0) -- Bildschirm über das Kachelsystem blitzschnell schwärzen

-- Grafiken und Level-Layout von der SD-Karte laden
-- (Ersetzen Sie die Namen bei Bedarf durch Ihre echten Dateinamen)
sprite.tsheet("test2.bmp")  -- Lädt alle 256 Kacheln in den RAM
sprite.mapload("level1.map") -- Lädt die riesige, scrollbare Tilemap
sprite.load("t1.bmp",1) --Sprite laden

-- Eine Kachel-Animation im Hintergrund starten (z.B. fließendes Wasser auf Tile 10)
sprite.animate(5, 37, 5)
sprite.update()
-- ============================================================================
-- 2. ULTRA-FLÜSSIGE SPEILE-HAUPTSCHLEIFE (~60 FPS)
-- ============================================================================
local running = true
while running do

    -- --- A) HARDWARE-TASTENABFRAGE (Reine ASCII-Werte via inkey) ---
    local taste = inkey()

    if taste == 27 then -- ESC: Spiel sofort beenden
        running = false
        end

        -- --- B) SCHWERKRAFT & VERTIKALE KOLLISION (Fallen & Landen) ---
        velocityY = velocityY + GRAVITY
        local testY = playerY + velocityY

        -- Abfrage Ihrer C++ Eckenprüfung: Was befindet sich unter den Füßen?
        local kachelUnten = sprite.get_tile(playerX, testY)

        if kachelUnten == 2 and kachelUnten ~= MUENZ_ID then
            -- Kollision mit solidem Boden! Fall stoppen und Spieler erden
            velocityY = 0
            istAmBoden = true
            else
                playerY = testY
                istAmBoden = false
        end
                -- --- C) HORIZONTALE BEWEGUNG & SMART-SIDESCROLLING ---
                --if taste == 215 then -- PFEIL LINKS (Laufen nach links)
                    -- Prüfen, ob links frei (Luft = 0 oder Münze) ist
                --  local kachelLinks = sprite.get_tile(playerX - LAUF_GESCHW, playerY)
                    --if kachelLinks == 0 or kachelLinks == MUENZ_ID then
                        --if playerX > 100 then
                            -- Spieler bewegt sich frei auf der linken Schirmhälfte
                            --playerX = playerX - LAUF_GESCHW
                            --else
                               -- Kamera-Scroll: Map wird rückwärts nach links geschoben (-LAUF_GESCHW)
                    sprite.scroll(LAUF_GESCHW,0)
                               -- end
                               -- end

                -- elseif taste == 216 then -- PFEIL RECHTS (Laufen nach rechts)
                                  -- Prüfen, ob rechts frei (Luft = 0 oder Münze) ist
                                   -- local kachelRechts = sprite.get_tile(playerX + LAUF_GESCHW, playerY)
                                   -- if kachelRechts == 0 or kachelRechts == MUENZ_ID then
                                        --if playerX < 320 then
                                            -- Spieler bewegt sich frei zur Schirmmitte
                                            --playerX = playerX + LAUF_GESCHW
                                            --else
                                                -- Kamera-Scroll: Map wird vorwärts nach rechts geschoben (+LAUF_GESCHW)
                                                -- Neue Spalten werden im Hintergrund unsichtbar aus dem PSRAM nachgeladen!
                --sprite.scroll(-LAUF_GESCHW,0)
                                        --end
                  --end
            -- end

                if taste==32 and istAmBoden then
                   velocityY = JUMP_IMPULS
                   istAmBoden = false
                end
                -- --- E) INTERAKTIVES ITEM-EINSAMMELN (Münzen checken) ---
                -- Ruft Ihre C++ 4-Punkte-Funktion auf.
                -- Der Parameter '1' löscht die Münze bei Berührung sofort aus dem PSRAM und vom VGA-Schirm!
                local muenzTreffer = sprite.item(playerX, playerY, MUENZ_ID, 1)
                if muenzTreffer == 1 then
                   score = score + 10 -- Highscore um 10 Punkte erhöhen
                end
                -- --- F) STARTEN-RENDERING (Alles im unsichtbaren Puffer vorbereiten) ---
                -- Das Spieler-Sprite (z.B. Grafik-Index 20 aus dem geladenen BMP) zeichnen
                sprite.draw(playerX, playerY-16, 1)
                -- Score-Text fixiert in der oberen linken Ecke des Bildschirms einblenden
                vga.text(2, 1, "SCORE: " .. string.format("%04d", score), 255, 0)
                -- ====================================================================
                -- G) DER ZENTRALE HARDWARE-FLIP:
                -- Schaltet die Hintergrund-Animationen weiter und wirft die gesamte
                -- Spielszene zu 100 % flackerfrei auf Ihre VGA-Röhre!
                -- ====================================================================
                                                        
  -- Eine Kachel-Animation im Hintergrund starten (z.B. fließendes Wasser auf Tile 10)
  sprite.animate(5, 37, 5)
  sprite.update()

  delay(16) -- Framerate auf saubere ~60 FPS drosseln
end

-- Nach dem Drücken von ESC: Bildschirm säubern und zurück zum Terminal wechseln
sprite.hide(1)
sprite.cls(1)
sprite.update()
