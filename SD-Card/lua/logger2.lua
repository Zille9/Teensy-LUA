-- Beispiel-Tabelle mit Daten im Lua-Speicher
local datenSammlung = {
    { zeit = "12:00:00"; temp = 22.5; feuchte = 45 };
    { zeit = "12:05:00"; temp = 22.8; feuchte = 44 };
    { zeit = "12:10:00"; temp = 23.1; feuchte = 43 }
}

local function speichereGanzeTabelle(dateiname, daten)
    -- Start mit dem Header
    local csvInhalt = "Zeitstempel;Temperatur;Luftfeuchtigkeit\n"
    
    -- Alle Datensätze aus der Tabelle in den String einfügen
    for _, ds in ipairs(daten) do
        csvInhalt = csvInhalt .. string.format("%s;%.1f;%d\n", ds.zeit, ds.temp, ds.feuchte)
    end
    
    -- Datei komplett schreiben (überschreibt alten Inhalt)
    return sd.write(dateiname, csvInhalt)
end

-- Aufruf der Funktion
if speichereGanzeTabelle("log_komplett.csv", datenSammlung) then
    print("Ganze Tabelle erfolgreich gespeichert!")
else
    print("Fehler beim Speichern der Tabelle.")
end