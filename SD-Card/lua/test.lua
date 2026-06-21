local function parseCSVWithHeader(zeilen, separator)
    separator = separator or ";"
    local datensaetze = {}
    local header = nil
    
    for _, zeile in ipairs(zeilen) do
        -- 1. Unsichtbare Windows-Zeichen (\r) entfernen und trimmen
        zeile = string.gsub(zeile, "\r", "")
        zeile = string.match(zeile, "^%s*(.-)%s*$")
        
        -- Nur verarbeiten, wenn die Zeile nicht leer ist und kein Kommentar ist
        if zeile ~= "" and not string.match(zeile, "^#") then
            local werte = {}
            
            -- Wir splitten die Zeile per Hand auf, um die Einschränkungen der Schleife zu umgehen
            -- Das Suchmuster findet jedes Zeichen, das NICHT das Trennzeichen ist
            local pattern = "([^" .. "%" .. separator .. "]+)"
            for wert in string.gmatch(zeile, pattern) do
                -- Leerzeichen um den Wert entfernen
                local sauber = string.match(wert, "^%s*(.-)%s*$") or wert
                
                -- Versuchen in eine Zahl zu wandeln, sonst Text lassen
                local zahl = tonumber(sauber)
                table.insert(werte, zahl or sauber)
            end
            
            -- Wenn wir Werte in der Zeile gefunden haben
            if #werte > 0 then
                if not header then
                    -- Die allererste gültige Zeile wird unser Header
                    header = werte
                else
                    -- Folgende Zeilen werden mit dem Header verknüpft
                    local datensatz = {}
                    for spaltenIndex, spaltenName in ipairs(header) do
                        datensatz[spaltenName] = werte[spaltenIndex]
                    end
                    table.insert(datensaetze, datensatz)
                end
            end
        end
    end
    
    return datensaetze
end

local zeilen = sd.readline("log.csv")

if zeilen then
    print("--- ROHE DATEN AUS DER DATEI (Zeile 1 & 2) ---")
    print("Zeile 1 (Header): " .. tostring(zeilen[1]))
    print("Zeile 2 (Werte) : " .. tostring(zeilen[2]))
    print("----------------------------------------------")
    
    -- Jetzt parsen (Wir übergeben das Semikolon explizit)
    local logDaten = parseCSVWithHeader(zeilen, ";")
    
    if #logDaten > 0 then
        local erster = logDaten[1]
        print("Parser-Inhalt der ersten Zeile:")
        for k, v in pairs(erster) do
            print("Spalte: [" .. tostring(k) .. "] -> Wert: [" .. tostring(v) .. "]")
        end
    else
        print("Fehler: Parser konnte keine Zeilen verknüpfen.")
    end
else
    print("Fehler: Datei konnte nicht gelesen werden.")
end