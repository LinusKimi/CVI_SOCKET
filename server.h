/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: MainPanelCB */
#define  PANEL_LED                        2       /* control type: LED, callback function: (none) */
#define  PANEL_SERVER_IP                  3       /* control type: string, callback function: (none) */
#define  PANEL_SERVER_PORT                4       /* control type: string, callback function: (none) */
#define  PANEL_TOGGLEBUTTON               5       /* control type: textButton, callback function: ServerOpenClose */
#define  PANEL_TEXTBOX                    6       /* control type: textBox, callback function: (none) */
#define  PANEL_STRING                     7       /* control type: string, callback function: (none) */
#define  PANEL_FILEPATHBUTTON             8       /* control type: command, callback function: FilePathSelect */
#define  PANEL_TOGGLEBUTTON_2             9       /* control type: textButton, callback function: SAVEFILECLICK */
#define  PANEL_DECORATION                 10      /* control type: deco, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK FilePathSelect(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MainPanelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SAVEFILECLICK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ServerOpenClose(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif