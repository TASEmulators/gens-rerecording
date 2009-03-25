
-- simple sonic 1 speedometer display
-- demonstrates reading memory and writing text,
-- as well as predicting future frames and reading memory from them


-- create space in memory for a savestate
state = savestate.create()

-- register a function to run when the screen gets updated
gui.register( function ()

	-- get the X and Y velocity of the player
	xvel = memory.readwordsigned(0xffd010) -- sonic the hedgehog 1
	yvel = memory.readwordsigned(0xffd012)

	-- look 2 frames into the future, pretending the B button is held,
	-- and get what the X and Y velocity of the player will be
	savestate.save(state)
	for n=1,2 do
		joypad.set(1, {B = true})
		gens.emulateframeinvisible()
	end
	jumpxvel = memory.readwordsigned(0xffd010)
	jumpyvel = memory.readwordsigned(0xffd012)
	savestate.load(state)

	-- print the velocities onscreen, near the top-left corner,
	-- in a yellow color with a black outline
	message = string.format("velocity: %d, %d", xvel, yvel)
	gui.text(10, 50, message, "#FFFF00FF", "black")
	message = string.format("jump vel: %d, %d", jumpxvel, jumpyvel)
	gui.text(10, 58, message, "#FFFF007F", "black")
	message = string.format("speed:    %d", memory.readwordsigned(0xffd014))
	gui.text(10, 66, message, "#FFFF00FF", "black")
	
end)
