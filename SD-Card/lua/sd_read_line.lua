-- 1. Datei von der SD-Karte einlesen ---
local zeilen = sd.readLine("config.txt")

if zeilen then
    -- 2. Zeilen parsen ---
    local meineEinstellungen = parseConfig(zeilen)
    
    -- 3. Direkt als Variablen im Skript nutzen! ---
    print("Geladene Lautstärke:", meineEinstellungen.Lautstaerke) -- Gibt 80 (als Zahl) aus ---
    print("Geladener Modus:", meineEinstellungen.Modus) --- Gibt "VGA_Mode_3" aus
    
    -- Da es echte Zahlen sind, können Sie sofort damit rechnen: ---
    if meineEinstellungen.Lautstaerke > 75 then
        print("Warnung: Die Lautstärke ist sehr hoch eingestellt!")
    end
else
    print("Fehler: Konfiguration konnte nicht geladen werden.")
end