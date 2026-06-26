-- lade Modul auf jeden Fall neu (wichtig bei Aenderungen)
package.loaded["space_invaders"] = nil
local invaders = require("space_invaders")

-- Das Spiel im interaktiven Vollbildmodus starten!
invaders.start()