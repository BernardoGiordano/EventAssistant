#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include "util.h"
#include "http.h"
#include "fill.h"

#define RIGHE 27
#define MAXPAGES 1

void eventDatabase(PrintConsole topScreen, PrintConsole bottomScreen) {		
	char *database[RIGHE * (MAXPAGES + 1)];
	char *links[RIGHE * (MAXPAGES + 1)];
	
	filldatabase(database, links);
	
	int currentEntry = 0;
	int page = 0;
	
	consoleSelect(&bottomScreen);
	printf("\x1b[2J");
	printf("----------------------------------------");
	printf("\x19\x18 - Move cursor\n");
	printf("L/R - Switch page\n");
	printf("A - Open/close entry\n");
	printf("----------------------------------------");
	printf("\n\nSpecial thanks to:\n\n- Simona Mastroianni\n- Federico Leuzzi\n- Shai Raba'\n- Cosimo Vivoli");
	printf("\x1b[27;0H    Please check your connection....");
	printf("\x1b[29;10HPress START to exit.");
	consoleSelect(&topScreen);		
	printf("\x1b[2J");	
	printf("Page: %d", page);
	
	refreshDB(currentEntry, topScreen, database, RIGHE, page);

	consoleSelect(&topScreen);			
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START)	
			break;
		
		if (kDown & KEY_R) {
			if (page < MAXPAGES) {
				page++;
				consoleSelect(&topScreen);	
				printf("\x1b[2J");
				printf("Page: %d", page);
				
				refreshDB(currentEntry, topScreen, database, RIGHE, page);
			}
		}
		
		if (kDown & KEY_L) {
			if (page > 0) {
				page--;
				consoleSelect(&topScreen);	
				printf("\x1b[2J");
				printf("Page: %d", page);
				
				refreshDB(currentEntry, topScreen, database, RIGHE, page);
			}
		}
		
		if (kDown & KEY_DUP) {
			if (currentEntry == 0) currentEntry = RIGHE - 1;
			else if (currentEntry > 0) currentEntry -= 1;
			
			refreshDB(currentEntry, topScreen, database, RIGHE, page);
		}
		
		if (kDown & KEY_DDOWN) {
			if (currentEntry == RIGHE - 1) currentEntry = 0;
			else if (currentEntry < RIGHE - 1) currentEntry++;
			
			refreshDB(currentEntry, topScreen, database, RIGHE, page);			
		}

 		if (kDown & KEY_A)  {
			consoleSelect(&topScreen);
			printf("\x1b[2J");
			printDistro(topScreen, bottomScreen, links[currentEntry + page * RIGHE]);

			consoleSelect(&bottomScreen);
			printf("\x1b[2J");
			printf("----------------------------------------");
			printf("\x19\x18 - Move cursor\n");
			printf("L/R - Switch page\n");
			printf("A - Open/close entry\n");
			printf("----------------------------------------");
			printf("\x1b[27;0H    Please check your connection....");
			printf("\x1b[29;10HPress START to exit.");			
			consoleSelect(&topScreen);
			printf("\x1b[2J");	
			printf("Page: %d", page);			
			refreshDB(currentEntry, topScreen, database, RIGHE, page);
		}	 
		
		gfxFlushBuffers();
		gfxSwapBuffers();
	}
}

void psDates(PrintConsole topScreen, PrintConsole bottomScreen) {
	consoleSelect(&bottomScreen);
	printf("\x1b[2J");
	printf("----------------------------------------");
	printf("A - Switch page\n");
	printf("----------------------------------------");
	printf("\x1b[28;0H    Please check your connection....");
	
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked1.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked2.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked3.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked4.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked5.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked6.txt");
	printPSdates(topScreen, bottomScreen, "http://eventcatchersitalia.altervista.org/10/hacked7.txt");
}