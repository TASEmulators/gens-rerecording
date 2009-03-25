-- demonstrating reading and writing memory and checking joypad input
-- basically just a weird speedup cheat for sonic 1
--
-- of course if you record a movie with this script running
-- it will be unwatchable except when viewed with the same script

print("script started on frame ", gens.framecount(), ".")

gens.registerafter( function ()
	xspeed = memory.readwordsigned(0xffd010)
	buttons = joypad.get(1)
	if buttons.left then xspeed = xspeed - 64 end
	if buttons.right then xspeed = xspeed + 64 end
	memory.writeword(0xffd010, xspeed)
end)

