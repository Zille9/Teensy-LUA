-- ============================================================================
-- SPERRSICHERER REQUIRE-FIX
-- ============================================================================
local originales_require = require

require = function(modulName)
    -- RAM-Cache prüfen
    if package.loaded[modulName] then
        return package.loaded[modulName]
    end

    local bereinigterName = string.gsub(modulName, "%.", "/")
    
    -- Das sind die Pfade, die nacheinander abgefragt werden
    local suchMuster = {
        "lua/lib/?.lua",
        "lua/?.lua",
        "?.lua"
    }

    for _, muster in ipairs(suchMuster) do
        local pfad = string.gsub(muster, "%?", bereinigterName)
        
        -- C++ Ladebefehl feuern
        local chunk, err = sys.load(pfad)
        
        if chunk then
            -- Datei geladen! Jetzt ausführen
            local result = chunk()
            -- Im RAM sichern und zurückgeben
            package.loaded[modulName] = result or true
            return package.loaded[modulName]
        else
            -- WICHTIG: Wenn die Datei existierte, aber kaputt war, bricht das System ab
            if err and err ~= "Datei existiert nicht!" then
                error("Syntaxfehler in Modul '" .. pfad .. "': " .. tostring(err))
            end
        end
    end

    -- Fallback für eingebaute C-Module
    local status, result = pcall(originales_require, modulName)
    if status then 
        return result 
    end

    error("Modul '" .. modulName .. "' wurde nirgends auf der SD-Karte gefunden!")
end

-- Widget laden (Suchpfad löst es zu /lua/widget/clock.lua auf)
local clock = require("clock")

-- Einmalig beim Start das Zifferblatt zeichnen
-- clock.init()

-- local letztesUhrUpdate = 0
