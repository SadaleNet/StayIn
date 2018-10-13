#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ilomusi.h"

#define FRAME_PERIOD 100

int main(void){
	/* TODO: game initialization goes here */
	
	//Initialize display
	graphicSetDrawBuffer(lcdDmaBuffer);

	uint32_t previousTick = systemGetTick();
	uint32_t previousButtonHeldTick = previousTick;
	while(true){
		//wait for completion of the previous rendering
		while(!graphicIsDisplayReady()){} //Do not start drawing until the rendering is completed
		graphicClearDisplay(false);

		/* TODO: per-frame code to be executed goes here */


		graphicDisplay(lcdDmaBuffer);

		//Frame limiting
		int32_t timeElapsed = (systemGetTick()-previousTick);
		if(timeElapsed<FRAME_PERIOD)
			systemSleep(FRAME_PERIOD-timeElapsed, boostMode);
		previousTick = systemGetTick();
	}
	return 0;
}
