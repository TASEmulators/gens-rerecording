--******Subtitle Lua Script******

--To create a .sub file
--Each subtitle is an individual line of a standard ASCII file. It must be the same name as the movie, with a .sub extension. It must also be in the same directory as the movie.
--The first number is the frame to start displaying the subtitle.
--The second number is the number of seconds to display the subtitle (optional)
--The third number is the xposition (optional)
--The fourth number is the yposition (optional)
-- If the 2nd-4th are not set, they use defaults.
-- Defaults are set by the first subtitle, or by this file if they are not present in the first subtitle
-- To set defaults and have a different first subtitle, simply use a line with only the defaults. 








--Begin Code
--Default values, if not set
x_pos = 20
y_pos = 200
Durration = 10;

sub_file =  movie.getname();
load_file = string.sub(sub_file,1,string.len(sub_file)-4) .. ".sub";


Subtitles = io.open(load_file, "r");
if Subtitles == nil then
error("Load Failed!");

else
print("Loaded file " .. load_file);
end;


frame =  Subtitles:read("*n");
durr = Subtitles:read("*n");
if durr == nil then 
	     durr = Durration;
else
  Durration = durr;
end;
cxpos = Subtitles:read("*n");
if cxpos == nil then 
	     cxpos = x_pos;
else
  x_pos = cxpos;
end;

cypos = Subtitles:read("*n");
if cypos == nil then 
	     cypos = y_pos;
else
  y_pos = cypos;
end;

outs = Subtitles:read("*l");


--If the first line is just setting defaults, ignore the first line immediately!
if string.len(outs) <= 1 then
frame = 0;
durr = 0;
end;

--used to determine whether or not a subtitle is active
active_subtitle = 0;


gui.register( function ()
if (active_subtitle == 1)  then
   --if there is currently a subtitle showing
   gui.text(cxpos,cypos,outs); 
   if (movie.framecount() > (frame+durr*60)-1) then
      active_subtitle = 0; --reset subtitle
      frame =  Subtitles:read("*n"); --get next subtitle
	  durr = Subtitles:read("*n");
	  if durr == nil then 
	     durr = Durration;
	  end;
	  cxpos = Subtitles:read("*n");
		if cxpos == nil then 
	    	 cxpos = x_pos;
		end;
		cypos = Subtitles:read("*n");
		if cypos == nil then 
	    	 cypos = y_pos;
		end;
      outs = Subtitles:read("*l");
   end;   
   
      
elseif ((movie.framecount() >= frame) and (movie.framecount() < (frame+durr*60))) then   
    gui.text(cxpos,cypos,outs);
    active_subtitle = 1;
    
    
elseif (movie.framecount() > frame) then
    while (frame < movie.framecount()) do
       frame =  Subtitles:read("*n");
	   durr = Subtitles:read("*n");
	   if durr == nil then 
	   	  durr = Durration;
		end;
		cxpos = Subtitles:read("*n");
		if cxpos == nil then 
	    	 cxpos = x_pos;
		end;
		cypos = Subtitles:read("*n");
		if cypos == nil then 
	    	 cypos = y_pos;
		end;
		outs = Subtitles:read("*l");
		if frame == nil then
		    frame = 0;
			break
		end;
	 end;    	
end;
end);


