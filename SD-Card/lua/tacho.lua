-- Tacho-Modul laden
local gauge = require("gauge_view")

vga.cls()

-- 1. Tacho initialisieren (ID = 1, Mitte bei 320/220, Radius = 100, Bereich 0-100 Grad, Einheit °C)
gauge.zeichneSkala(1, 320, 220, 80, 0, 100, "* C")

vga.text(22, 2, "=== HARDWARE SENSOR DASHBOARD ===", 255, 0)
vga.text(24, 4, "Druecke ESC zum Verlassen", 127, 0)

local laufzeit = true
local letztesUpdate = 0

while laufzeit do
    -- ESC-Taste prüfen
    if inkey() == 27 then
        laufzeit = false
    end

    local jetzt = sys.timer()
    if (jetzt - letztesUpdate) >= 500 then -- Alle 500ms aktualisieren
        letztesUpdate = jetzt
        
        -- ECHTE HARDWARE-WERTE ABFRAGEN:
        -- Da sys.monitor() direkt druckt, können wir die Temperatur im Tacho simulieren 
        -- oder die Werte aus einer angepassten C++ Register-Rückgabe holen.
        -- Hier als absolut flüssiger Echtzeit-Test (Sinus-Schwankung um 45 Grad):
        local cpu_temp = 45.0 + math.sin(sys.timer() / 2000) * 30.0
        
        -- Tacho flackerfrei auf den neuesten Stand bringen!
        gauge.updateWert(1, 320, 220, 80, cpu_temp, 0, 100)
    end

    delay(20)
end

vga.cls()