vga.cls()

-- 1. Kreiszahl PI nutzen (Wichtig: In Lua eine Konstante, KEINE Funktion!)
vga.print("PI ist: ")
vga.print(math.pi)
vga.print("\n\r")

-- 2. Quadratwurzel berechnen (heißt jetzt standardmäßig sqrt)
local wurzel = math.sqrt(144)
vga.print("Wurzel aus 144: ")
vga.print(wurzel)
vga.print("\n\r")

-- 3. Zufallszahlen im Kreis ausgeben (heißt jetzt math.random)
for i = 1, 10 do
    local zahl = math.random(1, 6) -- Würfel von 1 bis 6
    vga.print(zahl)
    vga.print(" ")
end