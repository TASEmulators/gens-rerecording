
-- this is sort of a "racing ghost" display for comparing savestates
-- 
-- it draws a rough hitbox over your position and more importantly
-- draws the same hitbox over where the other savestates were
-- at the current in-game time
-- thus you can easily judge whether you're faster or slower
-- than you were the last few times
-- 
-- additionally, you can watch someone else's movie and then save on a slot you won't overwrite
-- then play from the start and you can tell whether you're faster than they were
-- (this can be quite useful for making TASes)
-- note that you must save and keep a savestate around for its ghost to be drawn
-- you might find the "Save Backup State" hotkey useful for this purpose (it saves to state #10)
-- 
-- currently this script only works for Sonic 1 (and derivatives like Sonic 1 Megamix)
-- because the addresses such as 0xffd008 are game-specific
-- also, the hitbox is probably not as accurate as it could be
-- etc., etc.
-- but the intent is for this to be a relatively easy-to-understand script
-- (that nevertheless gives a glimpse at a lot of useful things you could do)
-- rather than fully-featured and robust-as-possible


-- define which variable live across multiple runs of the script
gens.persistglobalvariables({
	currentstatenumber = 0,	
})

-- a record of frame information... because it's per-savestate,
-- we use registersave instead of persistglobalvariables to save it.
-- looks like: {savenumber1={time1={x1,y1},time2={x2,y2},...},savenumber2=...}
frameinforecords = {} -- initially it's an empty table


-- a helper function to get the info we want to store per in-game frame
-- (for now this is only the player position)
function getperframeinfo()
	local x = memory.readwordsigned(0xffd008)
	local y = memory.readwordsigned(0xffd00c)
	local info = {x,y}
	return info
end

-- a helper function to combine the current in-game time
-- and current level into a single unique number.
-- most games could just return gens.getframecount() instead
-- but in-level time tends to be better in the case of a Sonic game
function getinstantid()
	local level = 1 + memory.readword(0xfffe10)
	local frames = memory.readbyte(0xfffe25)
	local seconds = memory.readbyte(0xfffe24)
	local minutes = memory.readbyte(0xfffe23)
	local id = frames + 60*(seconds + 60*(minutes + 10*(level)))

	-- unfortunately the frame number stored at 0xfffe25
	-- will sometimes fail to increment evenly (1,2,3,3,5,6,7,7,9,...)
	-- so pretend it got incremented if the value stays the same for 2 frames
	-- (this isn't the best way of doing this but it mostly works)
	if lastidvalid and id == lastid then
		id = id + 1
	end

	return id
end


-- a helper function to draw the info of the given frame
function renderframeinfo(records, id, tracknumber)

	if records == nil then return end

	local info = records[id]
	local notcurrent = tracknumber ~= currentstatenumber

	-- don't draw anything if there's no info for this frame or it's after the last frame
	if info == nil then return end
	if notcurrent and records.endid and (id > records.endid) then return end

	-- draw a box where the player is
	local camx = memory.readwordsigned(0xfff700);
	local camy = memory.readwordsigned(0xfff704);
	local x = info[1] - camx
	local y = info[2] - camy
	local hwidth = 10
	local hheight = 16
	local color        = notcurrent and "#7F7FFF7F" or "#FF7FFF3F"
	local outlinecolor = notcurrent and "#000000FF" or "#FFFFFFFF"
	gui.drawbox(x-hwidth, y-hheight, x+hwidth, y+hheight, color, outlinecolor)

	-- draw the savestate number somewhere in the box
	if notcurrent then
		x = x + (tracknumber%6) * 4 - 10
		y = y + (tracknumber/6) * 6 - 15
		local message = string.format("%d", tracknumber)
		gui.text(x, y, message, "#FFFFFF")
	end

end



-- register a function to run when gens draws the screen
gui.register( function ()

	-- don't draw if there's no player (this check could be better)
	if memory.readword(0xffd000) == 0 then return end

	-- draw the information for this frame FOR ALL STATES
	local id = lastidvalid and lastid or getinstantid()
	for statenumber,records in pairs(frameinforecords) do
		renderframeinfo(records, id, statenumber)
	end

end)


-- register a function to run after the emulation of every frame
gens.registerafter( function ()

	-- don't update if there's no player (this check could be better)
	if memory.readword(0xffd000) == 0 then return end

	local id = getinstantid()

	-- this is so the hack that's in getinstantid() will work...
	if id == lastid then
		lastidvalid = false
	else
		lastid = id
		lastidvalid = true
	end

	-- just in case...
	if frameinforecords[currentstatenumber] == nil then frameinforecords[currentstatenumber] = {} end

	-- store the new information for this frame
	frameinforecords[currentstatenumber][id] = getperframeinfo()

end)


-- register a function to run whenever the game boots up or resets
-- (this also runs once immediately if the game is already running)
gens.registerstart( function ()

	-- run some startup code: retrieve the ghost info stored in each savestate
	sound.clear() -- as a courtesy, clear the sound before operations like this that can take more than 0.5 seconds or so
	frameinforecords = {}
	for n = 0,10 do -- (1-9, 0, and also the hidden #10 if it's there)
		frameinforecords[n] = savestate.loadscriptdata(n) or {}
		gens.wait()
	end

end)


-- register a function that gets called when a numbered savestate is saved.
-- the return value(s) becomes the savestate's scriptdata saved alongside it.
savestate.registersave( function (statenumber)

	-- if the user switched which state they're saving to,
	-- replace that state's info table with the current one
	if statenumber ~= currentstatenumber then
		frameinforecords[statenumber] = copytable(frameinforecords[currentstatenumber])
		currentstatenumber = statenumber
	end

	-- keep track of the last frame reached that's been saved
	frameinforecords[currentstatenumber].endid = getinstantid()

	-- save all the perframeinfo records for this state
	return frameinforecords[currentstatenumber]

end)


-- register a function that gets called when a numbered savestate is loaded.
savestate.registerload( function (statenumber)

	-- just switch the info record track to that state's table
	currentstatenumber = statenumber

end)



-- note about savestate.registerload!
-- if you add arguments after the statenumber,
-- the savestate's scriptdata gets automatically passed in to them,
-- like so:
--[[
		savestate.registerload( function (statenumber, records)
			currentstatenumber = statenumber
			frameinforecords[currentstatenumber] = records
		end)
  ]]
-- that works great usually, but in our case "records" is a potentially HUGE table,
-- so it would start taking a long time every time you load a savestate.
-- that's why we use savestate.loadscriptdata(n) to load the tables only once
-- when the script starts up, then we just reuse them in memory after that.
-- in case you're wondering, the main reason that loading big tables can be slow
-- is that creating and replacing such tables really strains Lua's garbage collector.
