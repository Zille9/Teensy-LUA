package.loaded["table_view"] = nil 
local tableview = require("table_view")
local fc = 252
local bc = 64
vga.cls()
vga.setTitle("ENTER=Run/CD | F1=Edit | F2=Show | BACKSPACE=CD.. | DEL=Del | ESC=Ende")

local spalten = { "Dateiname", "Groesse", "Typ" }
local dateien = sd.listfile()

-- Wir holen uns den aktuellen Pfad live aus dem Core
local aktuellerPfad = sd.pwd()
local titel = " DATEI-MANAGER "

vga.setStatus("Pfad: " .. aktuellerPfad)

local naechsteAktion = "TERMINAL" 
local zielOrdner     = nil
local dateiZumStarten = nil

local zeileGemerkt = 1
local zeileMerken  = false

-- Funktion zum Zeichnen des Mini-Vorschaubildes
local function zeichneVorschau(dateiName)
    -- Bereich rechts oben schwarzer Kasten mit Rahmen fuer Mini-Bild
       vga.box(450, 30, 164, 124, 0) -- Hintergrund 
       vga.rect(449, 29, 449+165, 29+125, fc) -- Rahmen 
    
    if dateiName and dateiName:sub(-4):lower() == ".bmp" then
       
       return 1

    elseif dateiName and dateiName:sub(-4):lower() == ".jpg" then
       
       return 2 
   
    else
        -- keine BMP, keine JPG 
       return nil

    end
end

local imManager = true

while imManager do

    -- Selektor gibt JEDEN Tastendruck zurueck
    local gewaehlteZeile, gedrueckteTaste = tableview.zeigeSelektor(titel, spalten, dateien, zeileGemerkt)
    if zeileMerken == true then
       zeileGemerkt = 1
       zeileMerken = false
    end

    -- FALL 1: ESC gedrückt
    if not gewaehlteZeile or gedrueckteTaste == 27 then
        naechsteAktion = "TERMINAL"
        imManager = false 

    -- FALL 2: Die Backspace (ZURUECK / Verzeichnis Nach oben)
    elseif gedrueckteTaste == 127 then --Backspace
        if aktuellerPfad ~= "/" then
            zielOrdner = ".." -- "Ebene nach oben" anfordern
            naechsteAktion = "WECHSEL_ORDNER"
            imManager = false
        else
            vga.setStatus("Bereits im Hauptverzeichnis!", 196, 0)
            delay(1000)
            vga.setStatus("Pfad: " .. aktuellerPfad)
        end
    
    -- FALL 3: F5 FUER LOESCHEN (DELETE) MIT SICHERHEITSABFRAGE
    elseif gedrueckteTaste == 212 then 
        if gewaehlteZeile and dateien[gewaehlteZeile] then
            local dateiName = dateien[gewaehlteZeile][1]
            local dateiTyp  = dateien[gewaehlteZeile][3]
            
            if dateiTyp == "DATEI" then
                
                -- 1. Fenster oeffnen 
                local fensterInhalt = "Datei wirklich loeschen?\n\n" .. dateiName .. "\n\n[J]A  /  [N]EIN oder ESC"
                
                -- Window(id, x, y, h, w, fcolor, bcolor, Titel, Fenstertext, Titelcolor)
                vga.openWindow(0,120, 100, 400, 120, 255, 96, "SICHERHEITSABFRAGE", fensterInhalt, 196)

                
                -- 2. Schleife: Auf Bestaetigung warten
                local warteAufAntwort = true
                while warteAufAntwort do
                    local antwort = inkey()
                    
                    -- Taste J -> LOESCHEN BESTAETIGT
                    if antwort == 106 or antwort == 74 then
                        sd.remove(dateiName) -- Aus dem Verzeichnis loeschen
                        
                        
                        -- Fenster schliessen 
                        vga.closeWindow(0)
                        
                        naechsteAktion = "RELOAD_MANAGER"
                        imManager = false
                        warteAufAntwort = false
                        
                    -- Taste N oder ESC -> ABBRUCH
                    elseif antwort == 110 or antwort == 78 or antwort == 27 then
                        warteAufAntwort = false
                        
                        -- Fenster sauber schliessen 
                        vga.closeWindow(0)
                        
                        -- loeschen und Tabelle neu zeichnen
                        vga.cls() 
                        vga.setTitle("ENTER=Run/CD | F1=Edit | F2=Cat | F4=CD.. | DEL=Del | ESC=Ende",fc,bc)
                        vga.setStatus("Pfad: " .. aktuellerPfad)
                      end
                    delay(10) 
                end
                
            end
        end

    -- FALL 4: Eine Zeile ausgewaehlt und Standardaktion (ENTER oder E)
    elseif gewaehlteZeile and dateien[gewaehlteZeile] then
        local dateiName = dateien[gewaehlteZeile][1]
        local dateiTyp  = dateien[gewaehlteZeile][3]
        
        -- --- EINE DATEI ---
        if dateiTyp == "DATEI" then

            if gedrueckteTaste == 13 then -- ENTER: Starten
               -- Pruefen, ob die Datei auf ".bmp" oder ".BMP" endet
                if dateiName:sub(-4):lower() == ".bmp" then
                    vga.cls() -- Bildschirm loeschen fuer das Bild
                    
                    -- Parameter: x_offset, y_offset, dateiname, [skalierung]
                    local erfolg = vga.bmpLoad(0, 0, dateiName, 1.0)
                    
                    if not erfolg then
                        print("Fehler beim Laden der BMP-Datei!")
                    end
                    
                    -- auf Taste Warten
                    waitkey()
                    
                    -- Manager neu laden
                    naechsteAktion = "RELOAD_MANAGER"
                    imManager = false
                elseif dateiName:sub(-4):lower() == ".jpg" then
                                       
                    -- Parameter: x_offset, y_offset, dateiname, [skalierung]
                    local erfolg = vga.jpg(dateiName)
                    
                    if not erfolg then
                        print("Fehler beim Laden der JPG-Datei!")
                    end
                    
                    -- auf Taste Warten
                    waitkey()
                    
                    -- Manager neu laden
                    naechsteAktion = "RELOAD_MANAGER"
                    imManager = false
                elseif dateiName:sub(-4):lower() == ".hex" then
                                       
                    -- Parameter: x_offset, y_offset, dateiname, [skalierung]
                    local erfolg = sys.flash(dateiName)
                    
                    if not erfolg then
                        print("Fehler beim Laden der HEX-Datei!")
                    end
                    
                    -- Manager neu laden
                    naechsteAktion = "RELOAD_MANAGER"
                    imManager = false
                else
                    -- normale Lua-Skripte / Programme
                    dateiZumStarten = dateiName
                    naechsteAktion = "START_DATEI"
                    imManager = false 
                end
                
            elseif gedrueckteTaste == 194 then -- F1: Editieren
                vga.cls()
                edit(dateiName) 
                naechsteAktion = "RELOAD_MANAGER"
                imManager = false

            elseif gedrueckteTaste == 195 then -- F2: Cat - Datei ansehen
                if zeichneVorschau(dateiName) == 1 then
                   zeileMerken = true
                   zeileGemerkt = gewaehlteZeile
                   vga.bmpLoad(452, 32, dateiName, 0.25)
                elseif zeichneVorschau(dateiName) == 2 then
                   zeileMerken = true
                   zeileGemerkt = gewaehlteZeile
                   vga.jpg(dateiName, 452, 32, 4)
                else        
                   vga.cls()
                   sd.cat(dateiName)
                   naechsteAktion = "RELOAD_MANAGER"
                   imManager = false
                   print("")
                   print("  ==========  Taste ==========")
                   waitkey() 
                end
                
            end
            
        -- --- ES IST EIN ORDNER ---
        elseif dateiTyp == "ORDNER" then
            if gedrueckteTaste == 13 then -- ENTER - Ordner wechseln
                zielOrdner = dateiName
                naechsteAktion = "WECHSEL_ORDNER"
                imManager = false 
            end
        end
    end
    
end

-- ============================================================================
-- DIE ENTSCHEIDUNGS-WEICHE (Vollkommen entkoppelt)
-- ============================================================================
if naechsteAktion == "WECHSEL_ORDNER" then
    sd.cd(zielOrdner)                  
    package.loaded["table_view"] = nil 
    run("/lua/fileman.lua")                 

elseif naechsteAktion == "RELOAD_MANAGER" then
    package.loaded["table_view"] = nil 
    run("/lua/fileman.lua")                 

elseif naechsteAktion == "START_DATEI" then
    local chunk = run(dateiZumStarten)
    if chunk then chunk() end

elseif naechsteAktion == "TERMINAL" then
    collectgarbage("collect")

    vga.cls()
    sd.cd("/lua") 
    print(" ")
    print(">") 
end