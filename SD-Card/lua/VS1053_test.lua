local MP3_XDCS    = 2   -- Datenkanal des VS1053 Musikchips
local musicHandle = nil
local musicActive = false

function playBackgroundMusic(dateiname)
    -- Direkt unsere neue native Brücke nutzen (resolve_lua_path läuft intern im C++)
    musicHandle = sd.open(dateiname, "r")
    
    if not musicHandle then
        print("FEHLER: Konnte Musikdatei nicht oeffnen: " .. dateiname)
        musicActive = false
        return false
    end
    
    print("Spiele Musik ueber sd-Modul: " .. dateiname)
    musicActive = true
    return true
end

function updateAudioStream()
    if not musicActive or not musicHandle then return end

    -- Solange der Musikchip Daten anfordert (DREQ ist HIGH)
    while vga.musicReady() do
        -- 32 Bytes direkt von der SD-Karte als fertiges Zahlen-Array lesen
        local datenBuffer = sd.read(musicHandle, 32)
        
        if not datenBuffer then
            -- Dateiende erreicht (sd.read gibt nil zurück): Song von vorne starten
            sd.seek(musicHandle, 0)
            break
        end
        
        -- Das Zahlen-Array direkt an den Musikchip streamen (Null Kopieraufwand!)
        spi.writeBuffer(datenBuffer, MP3_XDCS)
    end
end