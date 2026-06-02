#include "minimal_display.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "EPD_2in9_V2.h"
#include <stdlib.h>
#include <stdio.h>

void Display_SensorData(const char *line1, const char *line2, const char *line3, const char *line4, const char *line5)
{
    if (DEV_Module_Init() != 0) {
        printf("DEV_Module_Init failed\r\n");
        return;
    }

    EPD_2IN9_V2_Init();

    UWORD Imagesize = ((EPD_2IN9_V2_WIDTH % 8 == 0)? (EPD_2IN9_V2_WIDTH / 8 ): (EPD_2IN9_V2_WIDTH / 8 + 1)) * EPD_2IN9_V2_HEIGHT;
    UBYTE *BlackImage = (UBYTE *)malloc(Imagesize);
    if (BlackImage == NULL) {
        printf("malloc failed\r\n");
        EPD_2IN9_V2_Sleep();
        DEV_Module_Exit();
        return;
    }

    Paint_NewImage(BlackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    if (line1 != NULL) Paint_DrawString_EN(10, 4, line1, &Font16, BLACK, WHITE);
    if (line2 != NULL) Paint_DrawString_EN(10, 28, line2, &Font12, BLACK, WHITE);
    if (line3 != NULL) Paint_DrawString_EN(10, 48, line3, &Font12, BLACK, WHITE);
    if (line4 != NULL) Paint_DrawString_EN(10, 68, line4, &Font12, BLACK, WHITE);
    if (line5 != NULL) Paint_DrawString_EN(10, 88, line5, &Font12, BLACK, WHITE);

    EPD_2IN9_V2_Display_Base(BlackImage);
    DEV_Delay_ms(1500);

    free(BlackImage);
    EPD_2IN9_V2_Sleep();
    DEV_Module_Exit();
}


