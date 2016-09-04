/*
* This file is part of EventAssistant
* Copyright (C) 2016 Bernardo Giordano
*
* This software is provided 'as-is', 
* without any express or implied warranty. 
* In no event will the authors be held liable for any damages 
* arising from the use of this software.
*
* This code is subject to the following restrictions:
*
* 1) The origin of this software must not be misrepresented; 
* 2) You must not claim that you wrote the original software. 
*
*/

/* PKCONT 
	0 : currentEntry
	1 : boxnumber
	2 : indexnumber
	3 : nature counter
	4 : hidden power counter
	5 : clone boxnumber
	6 : clone indexnumber
	7 : IVs index
	8 : EVs index
*/

#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "http.h"
#include "catch.h"
#include "util.h"
#include "database.h"
#include "pokemon.h"

#define ENTRIES 11

#define citra 1 // 0: citra debug enabled
				// 1: citra debug disabled: application meant to run correctly on the console

#define V1 2
#define V2 2
#define V3 1

#define DAY 4
#define MONTH 9
#define YEAR 16

void exitServices() {
	romfsExit();
	sdmcExit();
	aptExit();
	gfxExit();
}

void intro(PrintConsole topScreen, PrintConsole bottomScreen, int currentEntry, char* menuEntries[]){
	consoleSelect(&bottomScreen);
	printf("\x1b[2J");
	printf("\nDatabase definitions updated to %d/%d/%d", DAY, MONTH, YEAR);
	printf("\x1b[26;0HEvent Assistant v%d.%d.%d", V1, V2, V3);
	printf("\n\nBernardo Giordano & ctrulib");
	consoleSelect(&topScreen);
	printf("\x1b[2J");
	printf("\x1b[47;1;34m                  EventAssistant                  \x1b[0m\n");

	refresh(currentEntry, topScreen, menuEntries, ENTRIES);

	consoleSelect(&topScreen);
	printf("\x1b[29;10HPress Start to save, B to exit");
}

int main() {
	gfxInitDefault();
	aptInit();
	sdmcInit();

	PrintConsole topScreen, bottomScreen;
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
	
	Result rc = romfsInit();
	if (rc)
		printf("romfsInit error: %08lX\n", rc);
	
	mkdir("sdmc:/3ds", 0777);
	mkdir("sdmc:/3ds/data", 0777);
	mkdir("sdmc:/3ds/data/EventAssistant", 0777);
	mkdir("sdmc:/3ds/data/EventAssistant/builds", 0777);
	
	if (!(isHBL())) {
		// checking updates
		bool autoupdate = false;
		consoleSelect(&topScreen);
		printf("\x1b[13;9HDo you want to check for updates?\x1b[15;15HSTART: YES | B: SKIP");
		while (aptMainLoop()) {
			gspWaitForVBlank();
			hidScanInput();

			if (hidKeysDown() & KEY_START) {
				autoupdate = true;
				break;
			}
			
			if (hidKeysDown() & KEY_B)
				break;

			gfxFlushBuffers();
			gfxSwapBuffers();
		}
		
		if (autoupdate) {
			int temp = 0;
			char* ver = (char*)malloc(6 * sizeof(u8));
			snprintf(ver, 6, "%d.%d.%d", V1, V2, V3);
			
			printf("\x1b[2J");
			printf("Checking automatically for updates...\n\n");
			Result ret = downloadFile(bottomScreen, "https://raw.githubusercontent.com/BernardoGiordano/EventAssistant/master/resources/ver.ver", "/3ds/data/EventAssistant/builds/ver.ver");	
			if (ret != 0) {
				exitServices();
				return 0;
			}
			
			printf("\nComparing...");
			FILE *fptr = fopen("3ds/data/EventAssistant/builds/ver.ver", "rt");
			if (fptr == NULL)
				return 15;
			fseek(fptr, 0, SEEK_END);
			u32 contentsize = ftell(fptr);
			char *verbuf = (char*)malloc(contentsize);
			if (verbuf == NULL) 
				return 8;
			rewind(fptr);
			fread(verbuf, contentsize, 1, fptr);
			fclose(fptr);

			remove("/3ds/data/EventAssistant/builds/ver.ver");			
			
			for (int i = 0; i < 5; i++)
				if (*(ver + i) == *(verbuf + i))
					temp++;
			
			if (temp < 5) {
				update(topScreen, bottomScreen);
				exitServices();
				return 0;
			}
		}
	}

	int game = 0;
	bool save = true;
	
	//X, Y, OR, AS
	const u64 ids[4] = {0x0004000000055D00, 0x0004000000055E00, 0x000400000011C400, 0x000400000011C500};
	char *gamesList[4] = {"Pokemon X", "Pokemon Y", "Pokemon Omega Ruby", "Pokemon Alpha Sapphire"};
	
	consoleSelect(&bottomScreen);
	printf("\x1b[2J");
	printf("\x1b[14;5HPress A to continue, B to exit");
	
	consoleSelect(&topScreen);
	printf("\x1b[2J");
	printf("\x1b[31mCHOOSE GAME.\x1b[0m Cart has priority over digital copy.");
	refresh(game, topScreen, gamesList, 4);

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		
		if (hidKeysDown() & KEY_B) {
			exitServices();
			return 0;
		}
		
		if (hidKeysDown() & KEY_DUP) {
			if (game == 0)
				game = 3;
			else if (game > 0)
				game--;	
			
			refresh(game, topScreen, gamesList, 4);
		}
		
		if (hidKeysDown() & KEY_DDOWN) {
			if (game == 3)
				game = 0;
			else if (game < 3)
				game++;
			
			refresh(game, topScreen, gamesList, 4);
		}
		
		if (hidKeysDown() & KEY_A)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}
	
	consoleSelect(&topScreen);
	printf("\x1b[7;0HLoading save...");
	
	fsStart();
	#if citra
	FS_Archive saveArch;	
	
	if (!(openSaveArch(&saveArch, ids[game]))) {
		errDisp(bottomScreen, 1, BOTTOM);
		exitServices();
		return -1;
	}

	Handle mainHandle;
	FSUSER_OpenFile(&mainHandle, saveArch, fsMakePath(PATH_ASCII, "/main"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	#endif
	
	u64 mainSize = 0;
	
	#if citra
	FSFILE_GetSize(mainHandle, &mainSize);
	
	switch(game) {
		case 0 : {
			if (mainSize != 415232)
				errDisp(bottomScreen, 13, BOTTOM);
			break;
		}
		case 1 : {
			if (mainSize != 415232)
				errDisp(bottomScreen, 13, BOTTOM);
			break;
		}
		case 2 : {
			if (mainSize != 483328)
				errDisp(bottomScreen, 13, BOTTOM);
			break;
		}
		case 3 : {
			if (mainSize != 483328)
				errDisp(bottomScreen, 13, BOTTOM);
			break;
		}
		exitServices();
		return -1;
	}
	#endif
	
	u8* mainbuf = malloc(mainSize);
	
	#if citra
	FSFILE_Read(mainHandle, NULL, 0, mainbuf, mainSize);
	
	char *bakpath = (char*)malloc(80 * sizeof(char));
	
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);

	int hours = timeStruct->tm_hour;
	int minutes = timeStruct->tm_min;
	int seconds = timeStruct->tm_sec;
	int day = timeStruct->tm_mday;
	int month = timeStruct->tm_mon + 1;
	int year = timeStruct->tm_year +1900;
		
	snprintf(bakpath, 80, "/3ds/data/EventAssistant/main_%s_%i-%i-%i-%02i%02i%02i", gamesList[game], day, month, year, hours, minutes, seconds);
	printf("\n\nSaving backup to %s\n", bakpath);
	
	FILE *f = fopen(bakpath, "wb");
	fwrite(mainbuf, 1, mainSize, f);
	fclose(f);
	
	free(bakpath);
	#endif

	char *menuEntries[ENTRIES] = {"Gen VI's Event Database", "Gen VI's Save file editor", "Gen VI's Pokemon editor", "Mass injecter", "Wi-Fi distributions", "Code distributions", "Local distributions", "Capture probability calculator", "Common PS dates database", "Credits", "Update .cia to latest commit build"};
	int currentEntry = 0;
	
	// initializing save file editor variables
	int nInjected[3] = {0, 0, 0};
	int injectCont[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	// initializing pokemon editor variables
	int pokemonCont[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	consoleSelect(&topScreen);
	printf("\x1b[0m");
	intro(topScreen, bottomScreen, currentEntry, menuEntries);
	
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		if (hidKeysDown() & KEY_START)
			break;
		
		if (hidKeysDown() & KEY_B) {
			save = false;
			break;
		}

		if (hidKeysDown() & KEY_DUP) {
			if (currentEntry == 0)
				currentEntry = ENTRIES - 1;
			else if (currentEntry > 0)
				currentEntry--;
			
			refresh(currentEntry, topScreen, menuEntries, ENTRIES);
		}

		if (hidKeysDown() & KEY_DDOWN) {
			if (currentEntry == ENTRIES - 1)
				currentEntry = 0;
			else if (currentEntry < ENTRIES - 1)
				currentEntry++;

			refresh(currentEntry, topScreen, menuEntries, ENTRIES);
		}

		if (hidKeysDown() & KEY_A) {
			switch (currentEntry) {
				case 0 : {
					eventDatabase(topScreen, bottomScreen, mainbuf, game);
					break;
				}

				case 1 : {
					int ret = saveFileEditor(topScreen, bottomScreen, mainbuf, game, nInjected, injectCont);
					consoleSelect(&bottomScreen);
					
					if (ret == 1) 
						infoDisp(bottomScreen, 1, BOTTOM);
					else if (ret != -1 && ret != 1) 
						errDisp(bottomScreen, ret, BOTTOM);

					if (ret != -1)
						waitKey(KEY_B);
					
					break;
				}
				
				case 2 : {
					int ret = PKEditor(topScreen, bottomScreen, mainbuf, game, pokemonCont);
					consoleSelect(&bottomScreen);
					if (ret == 1)
						infoDisp(bottomScreen, 1, BOTTOM);
					else if (ret != 1 && ret != 0) 
						errDisp(bottomScreen, ret, BOTTOM);

					if (ret != 0)
						waitKey(KEY_B);
					
					break;
				}
				
				case 3 : {
					int ret = massInjecter(topScreen, bottomScreen, mainbuf, game);
					consoleSelect(&bottomScreen);
					if (ret == 1)
						infoDisp(bottomScreen, 1, BOTTOM);
					else if (ret != 1 && ret != 0) 
						errDisp(bottomScreen, ret, BOTTOM);

					if (ret != 0)
						waitKey(KEY_B);

					break;
				}

				case 4 :  {
					printDistro(topScreen, bottomScreen, "https://raw.githubusercontent.com/BernardoGiordano/EventAssistant/master/resources/worldwide1.txt");
					break;
				}

				case 5 : {
					printDistro(topScreen, bottomScreen, "https://raw.githubusercontent.com/BernardoGiordano/EventAssistant/master/resources/worldwide2.txt");
					break;
				}

				case 6 : {
					printDistro(topScreen, bottomScreen, "https://raw.githubusercontent.com/BernardoGiordano/EventAssistant/master/resources/local.txt");
					break;
				}

				case 7 : {
					catchrate(topScreen, bottomScreen);
					break;
				}

				case 8 : {
					psDates(topScreen, bottomScreen);
					break;
				}

				case 9 : {
					credits(topScreen, bottomScreen);
					break;
				}

				case 10 : {
					update(topScreen, bottomScreen);
					break;
				}
			}
			consoleSelect(&bottomScreen);
			printf("\x1b[2J");
			consoleSelect(&topScreen);
			printf("\x1b[2J");
			intro(topScreen, bottomScreen, currentEntry, menuEntries);
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
	}
	
	#if citra
	if (save) {
		infoDisp(bottomScreen, 2, BOTTOM);
		gfxFlushBuffers();
		gfxSwapBuffers();
		
		rewriteCHK(mainbuf, game);

		FSFILE_Write(mainHandle, NULL, 0, mainbuf, mainSize, FS_WRITE_FLUSH);
	}
	
	FSFILE_Close(mainHandle);
	
	if (save)
		FSUSER_ControlArchive(saveArch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
	
	FSUSER_CloseArchive(saveArch);
	#endif 
	
	free(mainbuf);
	fsEnd();
	
	exitServices();
	return 0;
}
