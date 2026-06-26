local tableview = require("table_view")
local fc = 252
local bc = 64
vga.cls()
vga.text(1,2,"ENTER=Run/CD | C=Cat | D=Del | E=Edit | Z=CD hoch| ESC=Ende",fc,bc)

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

    -- Der universelle Selektor gibt uns nun JEDEN Tastendruck zurück
    local gewaehlteZeile, gedrueckteTaste = tableview.zeigeSelektor(titel, spalten, dateien)

    -- FALL 1: ESC gedrückt
    if not gewaehlteZeile or gedrueckteTaste == 27 then
        naechsteAktion = "TERMINAL"
        imManager = false 

    -- FALL 2: Die Universal-Taste 'Z' / 'z' (ZURÜCK / Nach oben) wurde abgefangen!
    elseif gedrueckteTaste == 122 or gedrueckteTaste == 90 then
        if aktuellerPfad ~= "/" then
            zielOrdner = ".." -- "Ebene nach oben" anfordern
            naechsteAktion = "WECHSEL_ORDNER"
            imManager = false
        else
            vga.text(2, 2, "Bereits im Hauptverzeichnis!", 196, 0)
            delay(500)
        end

    -- FALL 3: Eine Zeile wurde ausgewählt und es war eine Standardaktion (ENTER oder E)
    elseif gewaehlteZeile and dateien[gewaehlteZeile] then
        local dateiName = dateien[gewaehlteZeile][1]
        local dateiTyp  = dateien[gewaehlteZeile][3]
        
        -- --- ES IST EINE DATEI ---
        if dateiTyp == "DATEI" then
            if gedrueckteTaste == 13 then -- ENTER: Starten
                dateiZumStarten = dateiName
                naechsteAktion = "START_DATEI"
                imManager = false 
                
            elseif gedrueckteTaste == 101 or gedrueckteTaste == 69 then -- E: Editieren
                vga.cls()
                edit(dateiName) 
                naechsteAktion = "RELOAD_MANAGER"
                imManager = false 
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
    -- print("Starte: " .. dateiZumStarten .. "...\n\r")
    local chunk = run(dateiZumStarten)
    if chunk then chunk() end

elseif naechsteAktion == "TERMINAL" then
    vga.cls()
    sd.cd("/lua") 
    print(" ")
    print(">") 
end