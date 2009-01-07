
-- this is sort of a "racing ghost" display for comparing savestates
-- 
-- it draws a rough hitbox over your position and more importantly
-- draw the same hitbox over where the other savestates were
-- at the current in-game time
-- thus you can easily judge whether you're faster or slower
-- than you were the last few times
-- 
-- additionally, you can watch someone else's movie and then save on a slot you won't overwrite
-- then play from the start and you can tell whether you're faster than they were
-- (this can be quite useful for making TASes)
-- note that you must save and keep a savestate around for its ghost to be drawn
-- you might find the "Save Backup State" hotkey useful for this purpose
-- 
-- currently this script only works for Sonic 1 (and derivatives like Sonic 1 Megamix)
-- because the addresses such as 0xffd008 are game-specific
-- also, the hitbox is probably not as accurate as it could be
-- etc., etc.
-- but the intent is for this to be a relatively easy-to-understand script
-- rather than fully-featured and robust-as-possible


-- a helper function to get the info we want to store per in-game frame
-- (for now this is only the player position)
function getperframeinfo()
	local x = memory.readwordsigned(0xffd008)
	local y = memory.readwordsigned(0xffd00c)
	local info = {x=x, y=y}
	return info
end

-- a helper function to combine the current in-game time
-- and current level into a single unique number
function getinstantid()
	local level = 1 + memory.readword(0xfffe10)
	local frames = memory.readbyte(0xfffe25)
	local seconds = memory.readbyte(0xfffe24)
	local minutes = memory.readbyte(0xfffe23)
	local id = frames + 60*(seconds + 60*(minutes + 10*(level)))

	-- unfortunately the frame number stored at 0xfffe25
	-- will sometimes fail to increment evenly (1,2,3,3,5,6,7,7,9,...)
	-- so pretend it got incremented if the value stays the same for 2 frames
	if lastidvalid and id == lastid then
		id = id + 1
	end

	return id
end

-- record of frame information, initially empty
-- (the key is an instantid, the value is a perframeinfo)
currentframeinforecords = {}
savestateframeinforecords = {} -- per savestate

-- a helper function to draw the info of the given frame
function renderframeinfo(records, id, tracknumber)

	if records == nil then return end

	info = records[id]

	if info == nil then return end

	-- draw a box where the player is
	camx = memory.readwordsigned(0xfff700);
	camy = memory.readwordsigned(0xfff704);
	x = info.x - camx
	y = info.y - camy
	hwidth = 10
	hheight = 16
	if tracknumber > 0 then
		color = "#7F7FFF7F"
	else
		color = "#FF7FFF3F"
	end
	if tracknumber > 0 then
		outlinecolor = "#000000FF"
	else
		outlinecolor = "#FFFFFFFF"
	end
	gui.drawbox(x-hwidth, y-hheight, x+hwidth, y+hheight, color, outlinecolor)
	
	-- draw the savestate number somewhere in the box
	if tracknumber > 0 then
		x = x + (tracknumber%6) * 4 - 10
		y = y + (tracknumber/6) * 6 - 15
		message = string.format("%d", tracknumber)
		gui.text(x, y, message, "#FFFFFF")
	end

end

-- register a function to run after the emulation of every frame
gens.registerafter( function ()

	-- don't update if there's no player (this check could be better)
	if memory.readword(0xffd000) == 0 then return end

	-- store the new information for this frame
	id = getinstantid()

	if id == lastid then
		lastidvalid = false
	else
		lastid = id
		lastidvalid = true
	end

	currentframeinforecords[id] = getperframeinfo()


end)

-- register a function to run when gens draws the screen
gui.register( function ()

	-- don't draw if there's no player (this check could be better)
	if memory.readword(0xffd000) == 0 then return end

	-- draw the information for this frame FOR ALL STATES
	for statenumber,records in pairs(savestateframeinforecords) do
		renderframeinfo(records, id, statenumber)
	end
	renderframeinfo(currentframeinforecords, id, -1)

end)


-- a function to associate the info records with savestates the user makes
savestate.registersave( function (statenumber)

	-- only save the frame record info into the numbered savestates
	if statenumber == nil then return end

	-- save all the perframeinfo records for this state
	savestateframeinforecords[statenumber] = copytable(currentframeinforecords)
	return currentframeinforecords -- the return value(s) becomes the argument(s) after the statenumber of the registered load function

end)

-- a function to load the info records associated with the savestates the user loads
savestate.registerload( function (statenumber, savedinforecords)

	-- only restore the frame record info from numbered savestates
	if statenumber == nil then return end

	-- sanity check
	if savedinforecords == nil then savedinforecords = {} end

	-- restore all the perframeinfo records for this state
	savestateframeinforecords[statenumber] = savedinforecords
	currentframeinforecords = copytable(savedinforecords)

end)


-- startup code: retrieve the ghost info stored in each savestate
do
	sound.clear() -- as a courtesy, clear the sound before operations like this that can take more than 0.5 seconds or so
	savestateframeinforecords = {}
	for x = 0,10 do -- (1-9, 0, and also the hidden #10 if it's there)
		savestate.load(x,"userdataonly")
	end
	currentframeinforecords = {}
end

