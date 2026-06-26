-- ============================================================================
-- HIGH-SPEED SPRITE INTERAKTIV-DEMO (sprite_demo.lua)
-- TESTET DAS NEUE EXTRA-HOCHPERFORMANTE SPIELE-FRAMEWORK
-- ============================================================================

-- 1. Bildschirm mit deiner neuen Kachel-Engine blitzschnell schwarz fluten
sprite.cls(0)

vga.text(18, 2, "=== NATIVES SPRITE & DOUBLEBUFFER DEMO ===", 255, 0)
vga.text(22, 4, "Steuerung: Pfeiltasten | ESC: Beenden", 127, 0)

-- 2. WIR GENERIEREN EIN REINES HARDWARE-SPRITE AUF SLOT 0
-- Dazu zeichnen wir kurz eine Grafik auf den Schirm und grabben sie
vga.box(0, 0, 16, 16, 240) -- Orange Basis-Kachel
vga.line(0, 0, 16, 16, 255)     -- Weisses Kreuz durch die Mitte
vga.line(16, 0, 0, 16, 255)

-- Deine umgeschriebene C++ Funktion schnappt sich das 16x16 Pixel große Stück!
sprite.get(0, 0, 0)

-- Schirm wieder säubern, damit das Start-Konstrukt verschwindet
sprite.cls(1)

-- Start-Variablen für das Spieler-Objekt
local playerX = 312
local playerY = 232
local geschw = 8
local sichtbar = true
local blinkTimer = 0

-- ============================================================================
-- DIE ULTRA-FLÜSSIGE SCHLEIFE (Nutzt ausschließlich sprite.*)
-- ============================================================================
local running = true
while running do

    -- A) TASTENABFRAGE (Reine ASCII-Werte)
    local taste = inkey()
    if taste == 27 then -- ESC
        running = false
    elseif taste == 216 then -- PFEIL LINKS
        playerX = playerX - geschw
        if playerX < 10 then playerX = 10 end
    elseif taste == 215 then  -- PFEIL RECHTS
        playerX = playerX + geschw
        if playerX > 614 then playerX = 614 end
    elseif taste == 218 then                -- PFEIL HOCH
        playerY = playerY - geschw
        if playerY < 40 then playerY = 40 end
    elseif taste == 217 then                -- PFEIL RUNTER
        playerY = playerY + geschw
        if playerY > 450 then playerY = 450 end
    elseif taste == 32 then                 -- LEERTASTE: Unsichtbar-Modus triggern
        sichtbar = not sichtbar
    end

    -- B) HINTERGRUND-BUFFER AKTUALISIEREN
    -- Wir löschen den kompletten Hintergrund im unsichtbaren Speicher in Mikrosekunden!
    --sprite.cls(0)

    -- Status-Texte im Hintergrund platzieren
    --vga.text(18, 2, "=== NATIVES SPRITE & DOUBLEBUFFER DEMO ===", 255, 0)
    --vga.text(2, 58, string.format("X: %03d  Y: %03d", playerX, playerY), 127, 0)

    -- C) SPRITE-RENDERING
    if sichtbar then
        -- Zeichnet das Sprite in den unsichtbaren Puffer
        sprite.draw(playerX, playerY, 0)
    else
        -- Teilt deiner Kachel-Engine mit, dass dieses Sprite ausgeblendet wird
        sprite.hide(0)
    end

    -- ====================================================================
    -- D) DER HARDWARE-FLIP (vga.run_gfxengine)
    -- Schiebt die gesamte vorbereitete Szene flackerfrei auf den Schirm!
    -- ====================================================================
    sprite.update()

    delay(16) -- Synchronisation auf solide ~60 Bilder pro Sekunde
end

-- Nach dem Beenden Schirm putzen fürs Terminal
sprite.cls(0)
sprite.update()
