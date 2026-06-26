---- neues skript -----
local fcolor, bcolor = vga.gcolor()

for i=0,255 do
  vga.color(0,i)
  vga.text(" ")
  vga.text(i)
  vga.text(" ")
  
end
---- Farben zuruecksetzen ----
vga.color(fcolor,bcolor)

---- Naechste Zeile ----------
print("\n\r")
print("-Taste-")
waitkey()
