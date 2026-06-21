-- Funktion, die Textzeilen parst und in eine Tabelle umwandelt
local function parseConfig(zeilen)
    local config = {}
    
    for _, zeile in ipairs(zeilen) do
        -- 1. Leerzeichen am Anfang/Ende trimmen und Kommentare (# oder //) ignorieren
        zeile = zeile:match("^%s*(.-)%s*$")
        if zeile ~= "" and not zeile:match("^#") and not zeile:match("^//") then
            
            -- 2. Zeile am Gleichheitszeichen trennen (Schluessel = Wert)
            -- Das Pattern sucht nach: Name - optionale Leerzeichen - '=' - optionale Leerzeichen - Wert
            local schluessel, wert = zeile:match("^([^=]+)%s*=%s*(.+)$")
            
            if schluessel and wert then
                -- Leerzeichen vom Schlüssel und Wert entfernen
                schluessel = schluessel:match("^%s*(.-)%s*$")
                wert = wert:match("^%s*(.-)%s*$")
                
                -- 3. Automatische Typkonvertierung: Wenn der Wert eine Zahl ist, als Zahl speichern
                local zahl_wert = tonumber(wert)
                if zahl_wert then
                    config[schluessel] = zahl_wert
                else
                    config[schluessel] = wert
                end
            end
        end
    end
    
    return config
end