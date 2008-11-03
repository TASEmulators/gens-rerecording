#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

#include "G_main.h"
#include "rom.h"
#include "movie.h"
#include "save.h"
#include "G_ddraw.h"

using namespace std;

extern const char* GensPlayMovie(const char* filename, bool silent);
extern int Paused;

//TODO: -readonly

//To add additional commandline options
//1) add the identifier (-rom, -play, etc) into the argCmds array
//2) add a variable to store the argument in the list under "Strings that will get parsed"
//3) add an entry in the switch statement in order to assign the variable
//4) add code under the "execute commands" section to handle the given commandline

void ParseCmdLine(LPSTR lpCmdLine, HWND HWnd)
{
	string argumentList;					//Complete command line argument
	argumentList.assign(lpCmdLine);			//Assign command line to argumentList
	int argLength = argumentList.size();	//Size of command line argument

	//List of valid commandline args
	string argCmds[] = {"-cfg", "-rom", "-play", "-loadstate", "-pause"};	//Hint:  to add new commandlines, start by inserting them here.

	//Strings that will get parsed:
	string CfgToLoad;		//Cfg filename
	string RomToLoad;		//ROM filename
	string MovieToLoad;		//Movie filename
	string StateToLoad;		//Savestate filename
	string PauseGame;		//adelikat: If user puts anything after -pause it will flag true, documentation will probably say put "1".  There is no case for "-paused 0" since, to my knowledge, it would serve no purpose

	//Temps for finding string list
	int commandBegin = 0;	//Beginning of Command
	int commandEnd = 0;		//End of Command
	string newCommand;		//Will hold newest command being parsed in the loop
	string trunc;			//Truncated argList (from beginning of command to end of argumentList

	//--------------------------------------------------------------------------------------------
	//Commandline parsing loop
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
			CfgToLoad = newCommand;
			break;
		case 1:
			RomToLoad = newCommand;
			break;
		case 2:
			MovieToLoad = newCommand;
			break;
		case 3:
			StateToLoad = newCommand;
			break;
		case 4:
			PauseGame = newCommand;
			break;
		}
	}
	//--------------------------------------------------------------------------------------------
	//Execute commands
	
	char *x;	//Temp variable used to convert strings to non const char arrays
	//Cfg
	if (CfgToLoad[0])
	{
		x = _strdup(CfgToLoad.c_str());
		Load_Config(x, NULL);
		strcpy(Str_Tmp, "config loaded from ");
		strcat(Str_Tmp, CfgToLoad.c_str());
		Put_Info(Str_Tmp, 2000);
	}

	//ROM
	if (RomToLoad[0]) Pre_Load_Rom(HWnd, RomToLoad.c_str());
	
	//Movie
	if (MovieToLoad[0]) GensPlayMovie(MovieToLoad.c_str(), 1);
	
	//Loadstate
	if (StateToLoad[0])
	{
		x = _strdup(StateToLoad.c_str());
		Load_State(x);
	}

	//Paused
	if (PauseGame[0] && MovieToLoad[0]) Paused = 1;
	


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