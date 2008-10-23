#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

#include "rom.h"
#include "movie.h"

using namespace std;

//To add additional commandline options
//1) add the identifier (-rom, -play, etc) into the argCmds array
//2) add a variable to store the argument in the list under "Strings that will get parsed
//3) add an entry in the switch statement in order to assign the variable
//4) add code under the "execute commands" section to handle the given commandline

void ParseCmdLine(LPSTR lpCmdLine, HWND HWnd)
{
	string argumentList;					//Complete command line argument
	argumentList.assign(lpCmdLine);			//Assign command line to argumentList
	int argLength = argumentList.size();	//Size of command line argument

	//List of valid commandline args
	string argCmds[] = {"-rom", "-play"};	//Hint:  to add new commandlines, start by inserting them here.

	//Strings that will get parsed:
	string RomToLoad;		//ROM name
	string MovieToLoad;		//Movie file

	//Temps for finding string list
	int commandBegin = 0;	//Beginning of Command
	int commandEnd = 0;		//End of Command
	string newCommand;		//Will hold newest command being parsed in the loop
	string trunc;			//Truncated argList (from beginning of command to end of argumentList


	//Commandline parsing loop--------------------------------------------------------------------
	for (int x = 0; x < (sizeof argCmds / sizeof string); x++)
	{
		commandBegin = argumentList.find(argCmds[x]) + argCmds[x].size() + 1;	//Find beginning of new command
		trunc = argumentList.substr(commandBegin);								//Truncate argumentList
		commandEnd = trunc.find(" ");											//Find next space, if exists, new command will end here
		if (commandEnd < 0) commandEnd = argLength;								//If no space, new command will end at the end of list
		newCommand = argumentList.substr(commandBegin, commandEnd);				//assign freshly parsed command to newCommand

		//Assign newCommand to appropriate variable
		switch (x)
		{
		case 0:
			RomToLoad = newCommand;
			break;
		case 1:
			MovieToLoad = newCommand;
			break;
		}
	}
	//--------------------------------------------------------------------------------------------

	//Execute commands
	
	//ROM
	if (RomToLoad[0]) Pre_Load_Rom(HWnd, RomToLoad.c_str());
	
	//Movie
	if (MovieToLoad[0]) 
	{
	
	}
/* OLD CODE	
		char Str_Tmpy[1024];
		int src;

#ifdef CC_SUPPORT
//		src = CC_Connect("CCGEN://Stef:gens@emu.consoleclassix.com/sonicthehedgehog2.gen", (char *) Rom_Data, CC_End_Callback);
		src = CC_Connect(lpCmdLine, (char *) Rom_Data, CC_End_Callback);

		if (src == 0)
		{
			Load_Rom_CC(CCRom.RName, CCRom.RSize);
			Build_Main_Menu();
		}
		else if (src == 1)
		{
			MessageBox(HWnd, "Error during connection", NULL, MB_OK);
		}
		else if (src == 2)
		{
#endif
		src = 0;
		
		if (lpCmdLine[src] == '"')
		{
			src++;
			
			while ((lpCmdLine[src] != '"') && (lpCmdLine[src] != 0))
			{
				Str_Tmpy[src - 1] = lpCmdLine[src];
				src++;
			}

			Str_Tmpy[src - 1] = 0;
		}
		else
		{
			while (lpCmdLine[src] != 0)
			{
				Str_Tmpy[src] = lpCmdLine[src];
				src++;
			}

			Str_Tmpy[src] = 0;
		}

		Pre_Load_Rom(HWnd, Str_Tmpy);

#ifdef CC_SUPPORT
		}
#endif
*/	
}