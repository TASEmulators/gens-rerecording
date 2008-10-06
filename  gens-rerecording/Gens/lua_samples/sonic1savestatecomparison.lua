
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
-- note that you must save and keep a savestate around for its ghost to be drawn
-- you might find the "Save Backup State" hotkey useful for this purpose
-- 
-- currently this script only works for Sonic 1 (and derivatives like Sonic 1 Megamix)
-- because the addresses such as 0xffd008 are game-specific
-- also, the hitbox is probably not as accurate as it could be
-- 
-- this script saves its data in "C:\\luatemp_s1ghostdata.txt" on exit
-- and loads it back in from there on startup
-- so you shouldn't lose it between sessions or script restarts
-- however it has not been tested very thoroughly yet


-- a helper function to get the info we want to store per in-game frame
-- (for now this is only the player position)
function getperframeinfo()
	x = memory.readwordsigned(0xffd008)
	y = memory.readwordsigned(0xffd00c)
	info = {x=x, y=y}
	return info
end

-- a helper function to combine the current in-game time
-- and current level into a single unique number
function getinstantid()
	level = 1 + memory.readword(0xfffe10)
	frames = memory.readbyte(0xfffe25)
	seconds = memory.readbyte(0xfffe24)
	minutes = memory.readbyte(0xfffe23)
	id = frames + 60*(seconds + 60*(minutes + 10*(level)))
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
	renderframeinfo(currentframeinforecords, id, 0)

end)

-- lua doesn't seem to have a built-in way of doing this...
function duplicatetable (table)
	rv = {}
	for key,value in pairs(table) do
		rv[key]=value
	end
	return rv
end

-- in case we have to debug what's in a table,
-- because print(table) is pretty useless
function printtable(o)
	if type(o) == "table" then
		for k,v in pairs(o) do
			printtable(k)
			print("=")
			printtable(v)
		end
	else
		print(o)
	end
end

-- a function to associate the info records with savestates the user makes
state.registersave( function (statenumber)

	-- only save the frame record info into the numbered savestates
	if statenumber == nil then return nil end

	-- save all the perframeinfo records for this state
	savestateframeinforecords[statenumber] = duplicatetable(currentframeinforecords)

end)

-- a function to load the info records associated with the savestates the user loads
state.registerload( function (statenumber)

	-- only restore the frame record info from numbered savestates
	if statenumber == nil then return end

	-- restore all the perframeinfo records for this state
	currentframeinforecords = savestateframeinforecords[statenumber]
	if currentframeinforecords == nil then currentframeinforecords = {} end

end)


-- a function to save a table out to file
function serialize (o)
	if type(o) == "number" then
		io.write(o)
	elseif type(o) == "string" then
		io.write(string.format("%q", o))
	elseif type(o) == "table" then
		io.write("{\n")
		for k,v in pairs(o) do
			io.write("  [")
			serialize(k)
			io.write("] = ")
			serialize(v)
			io.write(",\n")
		end
		io.write("}\n")
	elseif type(o) == "nil" then
		io.write("nil")
	else
		error("cannot serialize a " .. type(o))
	end
end


function deserialize (filename)
--	assert(loadfile(filename)) -- enable to get error information
	loadfile(filename)()
end


-- startup code: load savestateframeinforecords from file
temp = io.open("C:\\luatemp_s1ghostdata.txt", "r") -- check if file exists
if temp ~= nil then -- if it exists then load it
	io.close(temp)
	deserialize("C:\\luatemp_s1ghostdata.txt")
	if savestateframeinforecords == nil then savestateframeinforecords = {} end
end

-- exit code: save savestateframeinforecords to file
gens.registerexit( function()
	sound.clear() -- in case it takes a while to save
	io.output("C:\\luatemp_s1ghostdata.txt")
	io.write("savestateframeinforecords = ")
	serialize(savestateframeinforecords)
end)
