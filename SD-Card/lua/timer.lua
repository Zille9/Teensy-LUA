-- 1. Intervalle definieren (in Millisekunden)
local vgaIntervall = 2000  -- 2 Sekunden
local sdIntervall  = 60000 -- 60 Sekunden

-- 2. Startzeiten für jeden Timer merken
local letzteVgaZeit = sys.timer()
local letzteSdZeit  = sys.timer()

print("Multitasking-Timer gestartet...")

-- 3. Die Hauptschleife, die vom Teensy (C++) permanent aufgerufen wird
function loop()
    local aktuelleZeit = sys.timer()

    -- -------------------------------------------------------------
    -- TIMER 1: VGA-Anzeige aktualisieren (Alle 2 Sekunden)
    -- -------------------------------------------------------------
    if (aktuelleZeit - letzteVgaZeit) >= vgaIntervall then
        letzteVgaZeit = aktuelleZeit -- Timer zurücksetzen
        
        -- Sensorwerte holen (Funktion aus dem vorherigen Schritt)
        local temp, feuchte = sys.getSensorData()
        
        if temp and feuchte then
            -- Live-Werte auf VGA ausgeben
            -- (Beispielbefehle, passen Sie diese an Ihre VGA-Lib an)
            vga.setColors(65535, 0) -- Weiße Schrift, schwarzer Hintergrund
            vga.printAt(10, 20, string.format("Temp: %.1f C", temp))
            vga.printAt(10, 40, string.format("Hum:  %.1f %%", feuchte))
            
            print("[VGA] Anzeige aktualisiert.")
        end
    end

    -- -------------------------------------------------------------
    -- TIMER 2: Daten auf SD-Karte loggen (Alle 60 Sekunden)
    -- -------------------------------------------------------------
    if (aktuelleZeit - letzteSdZeit) >= sdIntervall then
        letzteSdZeit = aktuelleZeit -- Timer zurücksetzen
        
        -- Die bereits gebaute Log-Funktion aufrufen
        if macheMessung then
            macheMessung()
            print("[SD] Daten erfolgreich gesichert.")
        else
            print("[SD] Fehler: macheMessung() nicht definiert.")
        end
    end

    -- -------------------------------------------------------------
    -- HIER KÖNNTEN WEITERE TIMER STEHEN
    -- z.B. eine blinkende Status-LED alle 500 ms
    -- -------------------------------------------------------------
end