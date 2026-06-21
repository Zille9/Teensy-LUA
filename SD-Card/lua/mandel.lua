-- ===================================================
-- LUA MANDELBROT (APFELMÄNNCHEN) FÜR TEENSY 4.1 VGA
-- ===================================================
local dauer = sys.timer()

local fcolor, bcolor=vga.gcolor()

-- Farbschema auf Schwarz (0) setzen und Bildschirm leeren
vga.color(15, 0)
vga.cls()

vga.pos(0, 0)
vga.print("Berechne Apfelmaennchen in 256 Farben...\n\r")

-- Auflösung der VGA-Karte (Anpassen an Ihre exakte Pixel-Breite/Höhe)
local screenWidth  = 640
local screenHeight = 480

-- Sichtfenster für die komplexe Ebene (Mandelbrot-Koordinaten)
local minRe = -2.0
local maxRe =  1.0
local minIm = -1.2
local maxIm =  1.2

-- Maximale mathematische Rechentiefe pro Pixel
local maxIterations = 128

-- Vorberechnete Faktoren für maximale Geschwindigkeit in der Schleife
local factorRe = (maxRe - minRe) / screenWidth
local factorIm = (maxIm - minIm) / screenHeight

-- Hauptschleife über alle VGA-Pixel (Y-Zeilen)
for y = 0, screenHeight - 1 do
    -- Imaginärteil für diese Zeile berechnen
    local c_im = maxIm - (y * factorIm)
    
    -- Schleife über alle X-Pixel (Spalten)
    for x = 0, screenWidth - 1 do
        -- Realteil für dieses Pixel berechnen
        local c_re = minRe + (x * factorRe)
        
        -- Startwerte für die Mandelbrot-Gleichung (Z = 0)
        local z_re = 0.0
        local z_im = 0.0
        local z_re2 = 0.0
        local z_im2 = 0.0
        
        local iteration = 0
        
        -- Die eigentliche Fraktal-Formel: Z = Z^2 + C
        while (z_re2 + z_im2 <= 4.0) and (iteration < maxIterations) do
            z_im = (2.0 * z_re * z_im) + c_im
            z_re = z_re2 - z_im2 + c_re
            
            -- Quadrat-Werte für den nächsten Durchlauf zwischenspeichern
            z_re2 = z_re * z_re
            z_im2 = z_im * z_im
            
            iteration = iteration + 1
        end
        
        -- Pixel einfärben basierend auf den Iterationen
        if iteration < maxIterations then
            -- 256-FARBEN-EFFEKT: Mathematisches Farb-Mapping
            -- Wir nutzen die Iteration, multiplizieren sie und maskieren sie, 
            -- damit ein fließender Farbverlauf entsteht. 
            -- Sie können hier mit den Zahlen spielen, um andere Farbpaletten zu erzeugen!
            local colorIdx = (iteration * 4) % 256
            
            -- Verhindern, dass die Hintergrundfarbe (0 = Schwarz) für das Fraktal genutzt wird
            if colorIdx == 0 then colorIdx = 1 end 
            
            vga.pset(x, y, colorIdx)
        else
            -- Innenteil des Apfelmännchens bleibt tiefschwarz
            vga.pset(x, y, 0)
        end
    end
    
    -- Dem Teensy-System alle 4 Zeilen Zeit geben, USB-Interrupts zu verarbeiten,
    -- damit die Tastatur und die PC-Schnittstelle nicht einfrieren!
    if (y % 4 == 0) then
        delay(1) 
    end
end
------- Farben zuruecksetzen ------
vga.color(fcolor,bcolor)

------- Home-Position -------------
vga.pos(0, 1)
------- Zeichnungsdauer anzeigen ----
vga.print((sys.timer()-dauer)/1000 .. " sek")
print(" ")
