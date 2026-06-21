local a = sys.timer()
local running=true
local b = 0
local c = 0
while running do
     b=b+1
     
     if sys.timer() - a == 1000 then
        running=false
     end
end

vga.openWindow(0,200,200,160,100,255,4,"Benchmark",b .. " Zeilen/sek.",3)


if waitkey(0) > 0 then

   vga.closeWindow(0)

end
