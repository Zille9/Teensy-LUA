-- 1. Ihr Laser schießt (Kanal 0 = Ton)
-- Ein schicker Chiptune-Gleitflug nach oben
sound.play(0, 400, 15)  -- Startton
delay(20)
sound.play(0, 800, 15)  -- Höherer Ton
delay(20)
sound.play(0, 1500, 15) -- Spitzen-Zischen
sound.stop(0)           -- Stummschalten

-- 2. Sie sammeln eine Münze ein (Das klassische Nintendo-Doppel-Pling)
sound.play(1, 1500, 12)
delay(60)
sound.play(1, 2200, 12)
delay(120)
sound.stop(1)

-- 3. Ein Alien explodiert (Kanal 3 = Rauschen!)
-- Durch das schnelle Herabsenken der Frequenz klingt das Rauschen wie eine Explosion
sound.play(3, 800, 20)  -- Fettes, dichtes Rauschen
delay(50)
sound.play(3, 400, 15)  -- Dumpferes Nachgrollen
delay(80)
sound.play(3, 100, 8)   -- Letztes Ausblasen
sound.stop(3)           -- Auslöschen

-- 4. Game Over (Kanal 2 = Ton, fallende, traurige Melodie)
sound.play(2, 600, 15) delay(150)
sound.play(2, 450, 15) delay(150)
sound.play(2, 300, 15) delay(300)
sound.stop(2)
