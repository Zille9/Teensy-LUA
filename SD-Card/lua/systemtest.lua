-- Globales Farbschema: Cyan (11) auf Schwarz (0)
vga.color(255, 1)
vga.cls() -- Leert Schirm und termBuffer!

-- Titelzeile im 8x8 Raster ausgeben
vga.text(2, 1, "--- SPEICHER- UND GRAFIKVALIDIERUNG ---",0,90)

-- Geometrische Formen zeichnen (Pixel-Koordinaten)
vga.rect(16, 32, 150, 100, 252) -- Leerer gelber Rahmen
vga.box(180, 32, 300, 100, 190)  -- Gefuellte rote Box

vga.ellipse(80, 170, 40, 30, 205)       -- Leere hellgruene Ellipse
vga.fillellipse(240, 170, 50, 25, 76) -- Gefuellte blaue Ellipse
delay(5000)

-- mathematische Funktionen ------
vga.pos(10,30)
print("Sin(81)=")
vga.pos(18,30)
print(math.sin(81))

vga.pos(10,32)
print("(5+6)/7=")
vga.pos(18,32)
a=(5+6)/7
print(a)

delay(5000)


-- Text dynamisch an die aktuelle Position haengen
vga.cls()
vga.text("Dateien auf SD-Karte:")

-- Moduluebergreifend arbeiten: Verzeichnis sauber gefiltert ausgeben!
sd.ls("*.lua")
