local tableview = require("table_view")
local fc = 252
local bc = 64
vga.cls()
vga.setTitle("ENTER=Run/CD | F1=Edit | F2=Cat | BACKSPACE=CD.. | DEL=Del | ESC=Ende")

--vga.text(1,2,"ENTER=Run/CD | F1=Edit | F2=Cat | BACKSPACE=CD.. | DEL=Del | ESC=Ende",fc,bc)

local spalten = { "Dateiname", "Groesse", "Typ" }
local dateien = sd.listfile()

-- Wir holen uns den aktuellen Pfad live aus dem Core
local aktuellerPfad = sd.pwd()
local titel = "DATEI-MANAGER | ORDNER: " .. aktuellerPfad

local naechsteAktion = "TERMINAL" 
local zielOrdner     = nil
local dateiZumStarten = nil

local imManager = true
while imManager do

    -- Der universelle Selektor gibt uns nun JEDEN Tastendruck zurueck
    local gewaehlteZeile, gedrueckteTaste = tableview.zeigeSelektor(titel, spalten, dateien)

    -- vga.text(1,49,gedrueckteTaste,255,1,false) -- Debug

    -- FALL 1: ESC gedrückt
    if not gewaehlteZeile or gedrueckteTaste == 27 then
        naechsteAktion = "TERMINAL"
        imManager = false 

    -- FALL 2: Die Backspace (ZURUECK / Verzeichnis Nach oben)
    elseif gedrueckteTaste == 127 then --197 then
        if aktuellerPfad ~= "/" then
            zielOrdner = ".." -- "Ebene nach oben" anfordern
            naechsteAktion = "WECHSEL_ORDNER"
            imManager = false
        else
            vga.text(2, 2, "Bereits im Hauptverzeichnis!", 196, 0)
            delay(500)
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
                vga.openWindow(0,120, 180, 400, 120, 255, 96, "SICHERHEITSABFRAGE", fensterInhalt, 196)

                -- Warn-Sound über Ihre neue native Sound-Engine abspielen!
                --sound.play(0, 440, 12)
                --delay(80)
                --sound.play(0, 330, 12)
                
                -- 2. Interne Schleife: Auf Bestätigung warten
                local warteAufAntwort = true
                while warteAufAntwort do
                    local antwort = inkey()
                    
                    -- Taste J -> LOESCHEN BESTAETIGT
                    if antwort == 106 or antwort == 74 then
                        sd.remove(dateiName) -- Aus dem Verzeichnis loeschen
                        
                        --sound.play(1, 1500, 10)
                        --delay(100)
                        --sound.stop(1)
                        
                        -- Fenster sauber schliessen 
                        vga.closeWindow(0)
                        
                        naechsteAktion = "RELOAD_MANAGER"
                        imManager = false
                        warteAufAntwort = false
                        
                    -- Taste N oder ESC -> ABBRUCH
                    elseif antwort == 110 or antwort == 78 or antwort == 27 then
                        warteAufAntwort = false
                        
                        -- Fenster sauber schliessen 
                        vga.closeWindow(0)
                        
                        -- loeschen und die Tabelle im naechsten Frame neu drueberzeichnen
                        vga.cls() 
                        vga.text(1,2,"ENTER=Run/CD | F1=Edit | F2=Cat | F4=CD.. | DEL=Del | ESC=Ende",fc,bc)
                    end
                    delay(10) 
                end
                
            end
        end

    -- FALL 4: Eine Zeile wurde ausgewaehlt und es war eine Standardaktion (ENTER oder E)
    elseif gewaehlteZeile and dateien[gewaehlteZeile] then
        local dateiName = dateien[gewaehlteZeile][1]
        local dateiTyp  = dateien[gewaehlteZeile][3]
        
        -- --- ES IST EINE DATEI ---
        if dateiTyp == "DATEI" then
            if gedrueckteTaste == 13 then -- ENTER: Starten
                dateiZumStarten = dateiName
                naechsteAktion = "START_DATEI"
                imManager = false 
                
            elseif gedrueckteTaste == 194 then -- F1: Editieren
                vga.cls()
                edit(dateiName) 
                naechsteAktion = "RELOAD_MANAGER"
                imManager = false

            elseif gedrueckteTaste == 195 then -- F2: Cat - Datei ansehen
                vga.cls()
                sd.cat(dateiName) 
                naechsteAktion = "RELOAD_MANAGER"
                imManager = false
                print("")
                print("  ==========  Taste ==========") 
                waitkey()
            end
            
        -- --- ES IST EIN ORDNER ---
        elseif dateiTyp == "ORDNER" then
            if gedrueckteTaste == 13 then -- ENTER auf einen Ordner: Hineingehen
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
    vga.pos(1,48)
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
    vga.cls()
    sd.cd("/lua") 
    print(" ")
    print(">") 
end