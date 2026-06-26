-- ============================================================================
-- DUAL HARDWARE DASHBOARD (dashboard.lua)
-- PARALLELE DARSTELLUNG VON ZWEI TACHOS (CPU-TEMP & HEAP-RAM)
-- ============================================================================

-- Tacho-Modul über das funktionierende require()-System laden
local gauge = require("gauge_view")

vga.cls()

-- --- STATISCHER AUFBAU DES DASHBOARDS (Einmalig zeichnen) ---
vga.text(21, 2, "========================================", 127, 0)
vga.text(21, 3, "      TEENSY 4.1 HARDWARE DASHBOARD     ", 255, 0)
vga.text(21, 4, "========================================", 127, 0)
vga.text(26, 6, "Druecke die ESC-Taste zum Verlassen", 240, 0)

-- TACHO 1 (Links): CPU-Temperatur
-- ID=1, Mitte=(170, 260), Radius=85, Bereich=0-100, Einheit="Grad C"
gauge.zeichneSkala(1, 170, 260, 85, 0, 100, "Grad C")
vga.text(14, 39, "CPU-TEMPERATUR", 255, 0)

-- TACHO 2 (Rechts): Interner Heap-Speicher
-- ID=2, Mitte=(470, 260), Radius=85, Bereich=0-512, Einheit="KB frei"
gauge.zeichneSkala(2, 470, 260, 85, 0, 1024, "KB frei")
vga.text(51, 39, "FREIER HEAP-RAM", 255, 0)

-- --- DATA LOOP VARIABLEN ---
local laufzeitAktiv = true
local letztesUpdate = 0
local UPDATE_INTERVALL = 500 -- Aktualisierung alle 500 Millisekunden

-- ============================================================================
-- LIVE UPDATE HAUPTSCHLEIFE
-- ============================================================================
while laufzeitAktiv do

    -- 1. ECHTZEIT-TASTENABFRAGE (ESC=27 schließt das Dashboard)
    local taste = inkey()
    if taste == 27 then
        laufzeitAktiv = false
    end

    -- 2. ZEITGESTEUERTES MESSWERTE-UPDATE
    local jetzt = sys.timer()
    if (jetzt - letztesUpdate) >= UPDATE_INTERVALL and laufzeitAktiv then
        letztesUpdate = jetzt
        
        -- A) CPU-TEMPERATUR ERMITTELN (Echtzeit-Registerabfrage)
        
        local cpu_temp, freier_heap = sys.get_hardware_data()
        
        -- B) FREIEN RAM-HEAP ERMITTELN
        
        

        -- ====================================================================
        -- DIE FLACKERFREIEN UPDATES FÜR BEIDE ID-ZEIGER
        -- ====================================================================
        -- Tacho 1 (Links): Aktualisiert nur Nadel 1 mit der CPU-Temperatur
        gauge.updateWert(1, 170, 260, 85, cpu_temp, 0, 100)
        
        -- Tacho 2 (Rechts): Aktualisiert nur Nadel 2 mit dem Heap-Speicher
        gauge.updateWert(2, 470, 260, 85, freier_heap, 0, 512)
        
    end

    delay(20) -- System entlasten
end

-- Nach dem Verlassen Schirm putzen fürs Terminal
vga.cls()