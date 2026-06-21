local function parseCSVWithHeader(zeilen, separator)
    separator = separator or ";"
    local datensaetze = {}
    local header = nil
    
    for _, zeile in ipairs(zeilen) do
        zeile = zeile:match("^%s*(.-)%s*$")
        
        if zeile ~= "" and not zeile:match("^#") then
            local werte = {}
            local SuchZeile = zeile .. separator
            
            for wert in SuchZeile:gmatch("(.-)" .. "%" .. separator) do
                -- HIER DIE KORREKTUR: Nur trimmen, wenn wert nicht nil ist
                local neuwert
                if neuwert then
                   neuwert = neuwert:match("^%s*(.-)%s*$")
                end
                
                -- Wenn das Feld leer oder nil ist, Standardwert setzen
                if neuwert == nil or neuwert == "" then
                   neuwert = "Not" 
                else
                    neuwert = tonumber(neuwert) or neuwert
                end
                table.insert(werte, neuwert)
            end
            
            if not header then
                header = werte
            else
                local datensatz = {}
                for spaltenIndex, spaltenName in ipairs(header) do
                    datensatz[spaltenName] = werte[spaltenIndex]
                end
                table.insert(datensaetze, datensatz)
            end
        end
    end
    
    return datensaetze
end


-- 1. CSV-Datei von SD-Karte laden
local zeilen = sd.readline("log.csv")
print(zeilen)

if zeilen then
    -- 2. CSV mit Header-Unterstützung parsen
    local logDaten = parseCSVWithHeader(zeilen, ";")
    
    -- 3. Komfortabler Zugriff über die Spaltennamen
    for _, datensatz in ipairs(logDaten) do
        
        -- HIER: Direkter Zugriff über die Namen aus der Header-Zeile!
        local zeit  = datensatz.Zeitstempel
        local temp  = datensatz.Temperatur
        local feucht = datensatz.Luftfeuchtigkeit
        
        -- Ausgabe auf dem PC-Monitor
        print(zeit,temp,feucht)
        
        -- Beispiel-Logik für eine Überwachung:
        if temp and temp > 23.0 then
           print("-> Warnung: Temperatur zu hoch!")
        end
    end
else
    print("Fehler beim Laden der CSV-Datei.")
end

local zeil = sd.readline("log.csv")

local logDaten = parseCSVWithHeader(zeilen, ";")

if logDaten and #logDaten > 0 then
    print("Parser hat Daten gefunden!")
    
    -- Wir schauen uns den allerersten Datensatz im Detail an:
    local ersterDatensatz = logDaten[1]
    
    print("Verfügbare Spaltennamen in dieser Datei:")
    for schluessel, wert in pairs(ersterDatensatz) do
        print("-> Gefundener Schlüssel: '" .. tostring(schluessel) .. "' mit Wert: " .. tostring(wert))
    end
else
    print("Der Parser konnte KEINE Datensätze anlegen. Tabelle ist leer!")
end