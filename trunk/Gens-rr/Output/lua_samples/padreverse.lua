-- this is a simple demonstration script
-- that swaps the left and right buttons of player 1's controller

-- register a function to run before the emulation of every frame
gens.registerbefore( function ()

	-- get the current input
	curjoy = joypad.get(1)

	-- set the input to be the same but with left and right switched around
	joypad.set(1, {left = curjoy.right, right = curjoy.left})

end)
