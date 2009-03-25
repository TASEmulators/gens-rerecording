
-- the GUI library has a drawbox function that normally only draws a color with alpha blending,
-- but what if you want to draw a box with additive blending, subtractive blending, or anything else?
-- we could loop through all the pixels and use gui.readpixel/gui.writepixel, but that's horrendously slow.
-- instead, the gui.box() function allows you to pass in an arbitrary function instead of a color, which is much faster.
-- let's use this to extend the GUI library with some box drawing functions that use custom blend modes:

-- draws a box with additive blending (adding an R,G,B color of your choice).
-- the color components R,G,B can also be negative if you want subtractive blending.
-- an outline color like 'red' or 0x204080A0 may also be specified, but it is optional.
gui.addbox = function(x1,y1,x2,y2, R,G,B, outline)
	gui.box(x1,y1,x2,y2, function(r,g,b) return r+R,g+G,b+B end, outline)
end

-- draws a box with multiplicative blending (multiplying by an R,G,B factor of your choice).
gui.multbox = function(x1,y1,x2,y2, R,G,B, outline)
	gui.box(x1,y1,x2,y2, function(r,g,b) return r*R,g*G,b*B end, outline)
end

-- converts whatever is in the box to grayscale.
gui.graybox = function(x1,y1,x2,y2, outline)
	gui.box(x1,y1,x2,y2, function(r,g,b) local lum = (r*.3+g*.59+b*.11) return lum,lum,lum end, outline)
end

-- inverts the brightness of whatever is in the box. (approximately)
gui.invertbox = function(x1,y1,x2,y2, outline)
	gui.box(x1,y1,x2,y2, function(r,g,b) return 255-(g+b)*0.75,255-(r+b)*0.75,255-(g+r)*0.75 end, outline)
end


-- now let's give these custom blend modes a try.
-- register a function that draws some boxes whenever Gens draws the frame:

gui.register( function()
	gui.graybox( 120,  4,260, 80,          'black'  ) -- convert to grayscale, with a black outline
	gui.invertbox(20, 20,150,150,          'red'    ) -- invert brightness, with a red outline
	gui.multbox(   0,140,100,240, 1,.25,8, "#0020FF") -- purplish multiplier, with a blue outline
	gui.addbox(  200, 60,320,240, 0,80,0            ) -- additive green, and no outline specified -> automatic green outline
	gui.box(     120,200,180,220, {255,255,255,64}  ) -- transparent white, and no outline specified -> automatic white outline
	                                                  -- could also use "#FFFFFF40" or 0xFFFFFF40 instead of {255,255,255,64}
	-- let's draw something at the mouse cursor too for the heck of it:
	inp = input.get()
	gui.invertbox(inp.xmouse-2, inp.ymouse-2, inp.xmouse+2, inp.ymouse+2)
	gui.pixel(inp.xmouse, inp.ymouse, 'red')
end)


-- so, what if you want to use these blend modes on something that's not rectangular, like, say, a triangle?
-- well, unfortunately it's not supported yet, sorry.
-- for now you can fall back on the slower gui.readpixel()/gui.writepixel() method when boxes aren't enough.
