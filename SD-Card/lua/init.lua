-- ===================================================
-- TEENSY LUA SYSTEM INIT SKRIPT
-- ===================================================

-- 1. Globales Farbschema beim Booten setzen (z.B. Hellblau auf Schwarz)
vga.color(255, 1)
vga.cls()

-- 2. Einen schicken Willkommens-Bildschirm zaubern
vga.rect(10, 10, 320, 50, 240) -- Blauer Rahmen oben
vga.text(4, 2, "*** LUA COMPUTER SYSTEM V1.2 ***", 187, 1)
vga.text(2, 4, "Teensy 4.1 inside - PSRAM & VGA aktiv", 83, 1)
vga.pos(0,8)

-- ============================================================================
-- SPERRSICHERER UND SAUBERER REQUIRE-ERSATZ
-- ============================================================================
local originales_require = require

require = function(modulName)
    -- 1. RAM-Cache prüfen: Wenn das Modul schon geladen ist, sofort zurückgeben
    if package.loaded[modulName] then
        return package.loaded[modulName]
    end

    -- Punkte in Schrägstriche umwandeln (z.B. für verschachtelte Ordner)
    local bereinigterName = string.gsub(modulName, "%.", "/")
    
    -- Das sind die Pfade, die der Reihe nach auf der SD-Karte geprüft werden
    local suchMuster = {
        "lua/lib/?.lua",
        "lua/?.lua",
        "?.lua"
    }

    -- Alle Pfade durchlaufen
    for _, muster in ipairs(suchMuster) do
        local pfad = string.gsub(muster, "%?", bereinigterName)
        
        -- C++ Ladebefehl aufrufen (Liefert: chunk, err)
        local chunk, err = sys.load(pfad) 
        
        -- WICHTIG: Wir prüfen explizit, ob ein gültiger Code-Chunk zurückkam!
        if chunk ~= nil then
            -- Datei erfolgreich geladen und im PSRAM kompiliert! Jetzt ausführen.
            local status, result = pcall(chunk)
            if not status then
                error("\n\rLaufzeitfehler beim Ausführen von '" .. pfad .. "': " .. tostring(result))
            end
            
            -- Das ausgeführte Modul (z.B. die ClockWidget-Tabelle) im RAM cachen
            package.loaded[modulName] = result or true
            return package.loaded[modulName]
        else
            -- Wenn chunk nil ist, prüfen wir, ob es ein Syntaxfehler in deiner clock.lua war
            if err and err ~= "Datei existiert nicht!" then
                error("\n\rSyntaxfehler in Modul '" .. pfad .. "': " .. tostring(err))
            end
        end
    end

    -- Fallback für interne C-Module (falls vorhanden)
    local status, result = pcall(originales_require, modulName)
    if status then 
        return result 
    end

    -- Wenn die Datei beim allerersten Start partout nicht existiert
    error("\n\rModul '" .. modulName .. "' wurde auf der SD-Karte nicht gefunden!")
end
