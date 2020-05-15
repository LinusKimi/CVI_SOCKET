#include <ansi_c.h>
#include <tcpsupp.h>
#include <cvirte.h>		
#include <userint.h>
#include <formatio.h>
#include "server.h"

#include "crc.h"

#define tcpChk(f) if ((g_TCPError=(f)) < 0) {ReportTCPError(); goto Done;} else
	
// 帧头长度
#define HEADER_LEN 32
// 指示数据长度
#define LENGTH_DATA 4
// 实际数据长度
#define DATA_SIZE 16384

// 整包数据长度
#define TEMP_SIZE (HEADER_LEN+LENGTH_DATA+DATA_SIZE + 2)
	
// 接收缓存长度
#define REV_SIZE (TEMP_SIZE * 3)

// 文件刷新阈值
#define FRAMECNT 1500

	
static int panelHandle;
static int savefileHandle;

// custom pagedata
int serverstatus = 0;
int DirStatus = 0;
char DirPath[1000];

int datetime = 0;
char datetimeChar[50];

// file parm
int saveFileEnable = 0;
int savefileFrameCnt = 0;
int savefileCnt = 0;
int savefileMinCnt = 0;
char saveFilePath[1000];


// TCP 
static unsigned int g_hconversation;
static int			g_TCPError = 0;

char tempAddress[256] = {0};
char tempPort[256] = {0};
int portNum = 0;

const uint8_t g_header[]={'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a'};
// Databuf 
uint8_t *g_tcp_buf;
uint8_t *g_data_buf;
uint8_t *g_temp_buf;

int recvDataLen = 0;
int recvDataIndex = 0;
int searchHeadCnt = 0;
int continueRecv = 0;
int getDataEnable = 0;

uint8_t crcTemp[2] = {0,0};

// messageWindwos 
static void ReportMSG (char *msg, char *title)
{
	char messageBuf[1024];
	sprintf(messageBuf, "%s", msg);
	MessagePopup(title, messageBuf);
}

// TextBox message
static void ReportTextBox(char *msg, char *title)
{
	int itemsCnt = 0;
	char messageBuf[1024];
	sprintf(messageBuf, "[%s] : %s", title, msg);
	InsertTextBoxLine(panelHandle, PANEL_TEXTBOX, -1, messageBuf);
	GetNumTextBoxLines(panelHandle, PANEL_TEXTBOX, &itemsCnt);
	if(itemsCnt > 11)
	{
		DeleteTextBoxLine(panelHandle,PANEL_TEXTBOX, 0);
	}
}

// TCP error
static void ReportTCPError (void)
{
	if (g_TCPError < 0)
		{
		char	messageBuffer[1024];
		sprintf(messageBuffer, 
			"TCP library error message: %s\nSystem error message: %s", 
			GetTCPErrorString (g_TCPError), GetTCPSystemErrorString());
		ReportTextBox (messageBuffer,"Error");
		}
}

// select file path callback
int CVICALLBACK FilePathSelect (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	
	switch (event)
	{
		case EVENT_COMMIT:
			DirStatus = DirSelectPopup(" ", "save Binary file path", 1, 1, DirPath);
			if(DirStatus)
			{
				SetCtrlVal(panelHandle, PANEL_STRING,DirPath);
			}
			break;
	}
	return 0;
}

// save file start and stop
int CVICALLBACK SAVEFILECLICK (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	
	switch (event)
	{
		case EVENT_COMMIT:
			
			if(saveFileEnable == 0)	// enable save file
			{				
				sprintf(saveFilePath, "%s\\savefile_%d_%d.bin", DirPath, savefileCnt,savefileMinCnt);
				savefileHandle = OpenFile(saveFilePath, VAL_READ_WRITE, VAL_APPEND, VAL_BINARY);
				if(savefileHandle <= 0)
				{
					SetCtrlVal(panelHandle, PANEL_TOGGLEBUTTON_2, 1);
					ReportMSG("Save file open error!","ERROR");
				}
				else
				{
					savefileCnt++;
					saveFileEnable = 1;
				}
			}
			else
			{				
				saveFileEnable = 0;
				if(savefileHandle <= 0)
				{}
				else
				{
					CloseFile(savefileHandle);
				}
			}
			break;
	}
	return 0;
}

int CVICALLBACK ServerTCPCB (unsigned handle, int event, int error,
							 void *callbackData);


int main (int argc, char *argv[])
{
	char tempBuf[20] = {0};
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "server.uir", PANEL)) < 0)
		return -1;
	
	(uint8_t *)g_tcp_buf	=	(uint8_t *)malloc(sizeof(uint8_t)*REV_SIZE);
	(uint8_t *)g_data_buf	=	(uint8_t *)malloc(sizeof(uint8_t)*DATA_SIZE);
	(uint8_t *)g_temp_buf	=	(uint8_t *)malloc(sizeof(uint8_t)*TEMP_SIZE);
	
	crcInit();
	if (GetTCPHostAddr (tempBuf, 20) >= 0)
	{
	    SetCtrlVal (panelHandle, PANEL_SERVER_IP, tempBuf);
		ReportTextBox("Get IP successfully!","msg");
	}
	else
	{
		ReportTextBox("Get IP Error!","ERROR");
	}
	SetCtrlAttribute (panelHandle, PANEL_TOGGLEBUTTON_2, ATTR_DIMMED,1);
	
	
	DisplayPanel (panelHandle);
	RunUserInterface ();
	DiscardPanel (panelHandle);	
	return 0;
}


// exit app callback
int CVICALLBACK MainPanelCB (int panel, int event, void *callbackData,
							 int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_CLOSE:
			
			// free the malloc 
			free(g_tcp_buf);
			free(g_temp_buf);
			free(g_data_buf);
			
			QuitUserInterface (0);
			break;
		default:
			break;
	}
	return 0;
}


// TCP server open and close CTRL
int CVICALLBACK ServerOpenClose (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	
	switch (event)
	{
		case EVENT_COMMIT:
			if(serverstatus == 0)
			{	
				if(GetCtrlVal(panelHandle,PANEL_SERVER_PORT, tempPort) < 0)
				{
					ReportTextBox("IP and Port ERROR!","ERROR");
				}
				else
				{
					portNum = atoi(tempPort);
				
					// use local hostaddress
					if (RegisterTCPServer (portNum, ServerTCPCB, 0) < 0)
					{
						ReportTextBox("Server registration failed!","ERROR");
						SetCtrlVal(panelHandle, PANEL_TOGGLEBUTTON,1);
					}
					else
					{
						SetCtrlVal(panelHandle, PANEL_TOGGLEBUTTON,0);
						SetCtrlVal(panelHandle, PANEL_LED, 1);
						serverstatus = 1;
						
						// ctrl disable
						SetCtrlAttribute (panelHandle, PANEL_SERVER_IP, ATTR_DIMMED,1);
						SetCtrlAttribute (panelHandle, PANEL_SERVER_PORT, ATTR_DIMMED,1);
						SetCtrlAttribute (panelHandle, PANEL_FILEPATHBUTTON, ATTR_DIMMED,1);	
						SetCtrlAttribute (panelHandle, PANEL_TOGGLEBUTTON_2, ATTR_DIMMED,0);
						
						ReportTextBox("Started successfully!","msg");
					}
				}
			}
			else
			{
				if(UnregisterTCPServer(portNum) < 0)
				{
					ReportMSG("TCP Server close failed, Please restart the application!","ERROR");
				}
				
				g_hconversation = 0;
				recvDataIndex = 0;
				searchHeadCnt = 0;
				SetCtrlVal(panelHandle, PANEL_TOGGLEBUTTON,1);
				SetCtrlVal(panelHandle, PANEL_LED, 0);
				serverstatus = 0;
				
				SetCtrlAttribute (panelHandle, PANEL_SERVER_IP, ATTR_DIMMED,0);
				SetCtrlAttribute (panelHandle, PANEL_SERVER_PORT, ATTR_DIMMED,0);
				SetCtrlAttribute (panelHandle, PANEL_FILEPATHBUTTON, ATTR_DIMMED,0);
				SetCtrlAttribute (panelHandle, PANEL_TOGGLEBUTTON_2, ATTR_DIMMED,1);
				
				ReportTextBox("Closed successfully!","msg");
			}
				
			break;
	}
	return 0;
}

int CVICALLBACK ServerTCPCB (unsigned handle, int event, int error,
							 void *callbackData)
{
	char msgBuf[256]={0};
	char addrBuf[31];
	
	switch (event)
	{
		case TCP_CONNECT:
			if(g_hconversation)
			{
				tcpChk (GetTCPPeerAddr (handle, addrBuf, 31));
				sprintf (msgBuf, "Refusing conection %s", addrBuf); 
				tcpChk (DisconnectTCPClient (handle));
				ReportTextBox(msgBuf,"ERROR");
			}
			else
			{
				g_hconversation = handle;
				tcpChk (GetTCPPeerAddr (g_hconversation, addrBuf, 31));
				sprintf (msgBuf, "New connection  %s",addrBuf);
				tcpChk (SetTCPDisconnectMode (g_hconversation, TCP_DISCONNECT_AUTO));
				ReportTextBox(msgBuf,"client");
			}
				
			break;
		case TCP_DISCONNECT:
			if (handle == g_hconversation)
			{
				g_hconversation = 0;
				// reset recvbuf
				recvDataIndex = 0;
				searchHeadCnt = 0;
				ReportTextBox("Client disconnected","client");
			}
			break;
		case TCP_DATAREADY:
			recvDataLen = ServerTCPRead(g_hconversation, &g_tcp_buf[recvDataIndex], REV_SIZE - recvDataIndex, 100);
			
			recvDataIndex += recvDataLen;
			
			if(recvDataIndex >= DATA_SIZE) 
			{
				if(continueRecv == 0)	// continue recvdata
				{
					for(; searchHeadCnt < recvDataIndex-HEADER_LEN; searchHeadCnt++)
					{
											
						if((g_header[0] == g_tcp_buf[searchHeadCnt])  
								&&(g_header[HEADER_LEN / 2] == g_tcp_buf[searchHeadCnt + HEADER_LEN / 2])
						  		&&(g_header[HEADER_LEN - 1] == g_tcp_buf[searchHeadCnt + HEADER_LEN - 1]))
						{
							if(searchHeadCnt > (REV_SIZE - TEMP_SIZE))
							{
								for(uint32_t i = 0; i < (recvDataIndex - searchHeadCnt); i++)
									g_tcp_buf[i] = g_tcp_buf[i + searchHeadCnt];
								
								recvDataIndex = recvDataIndex - searchHeadCnt;						
								searchHeadCnt = 0;
							}
							
							if(recvDataIndex < (searchHeadCnt+TEMP_SIZE))
							{	
								continueRecv = 1;
								break;
							}
							else
							{
								getDataEnable = 1;
								break;
							}
						}
					}
				}
				else
				{
			
					if(recvDataIndex >= (searchHeadCnt+TEMP_SIZE))
					{
						continueRecv = 0;
						getDataEnable = 1;
					}
					
				}
				
				if(getDataEnable)
				{
					memcpy(g_temp_buf, &g_tcp_buf[searchHeadCnt],TEMP_SIZE);
					// crc 
					width_t crc = crcCompute(g_temp_buf, TEMP_SIZE - 2);
					crcTemp[0] = (uint8_t)(crc % 256);
					crcTemp[1] = (uint8_t)(crc / 256);

					if((crcTemp[0] == g_temp_buf[TEMP_SIZE - 2])&&(crcTemp[1] == g_temp_buf[TEMP_SIZE - 1]))
					//if((crcTemp[0] == 0x66)&&(crcTemp[1] == 0xE2))
					{
						sprintf(datetimeChar,"Recv data : %d",datetime);
						datetime++;
						ReportTextBox(datetimeChar,"msg");
						
						if(saveFileEnable)
						{
							if(savefileFrameCnt >= FRAMECNT)
							{
								savefileFrameCnt = 0;
								CloseFile(savefileHandle);
								
								savefileMinCnt++;
								sprintf(saveFilePath, "%s\\savefile_%d_%d.bin", DirPath, savefileCnt,savefileMinCnt);
								savefileHandle = OpenFile(saveFilePath, VAL_READ_WRITE, VAL_APPEND, VAL_BINARY);
							}
							
							if(savefileHandle <= 0)
							{
								saveFileEnable = 0;
								UnregisterTCPServer(portNum);
								ReportMSG("FileHandle is empty,\nPlace restart the application.","ERROR");
							}
							else
							{
								savefileFrameCnt++;
								memcpy(g_data_buf, &g_temp_buf[HEADER_LEN + LENGTH_DATA], DATA_SIZE);
								WriteFile(savefileHandle, g_data_buf,DATA_SIZE);
								//WriteFile(savefileHandle,&g_temp_buf[HEADER_LEN+2], DATA_SIZE);
							}
						}
					}
					else
					{
						saveFileEnable = 0;
						if(savefileHandle > 0)
						{
							CloseFile(savefileHandle);
						}
						UnregisterTCPServer(portNum);
						
						ReportMSG("CRC is ERROR,\n Plave restart the application.","ERROR");
					}
					
					// clear tcp buf, get surplus data , reset start index
					uint32_t surplusLen = recvDataIndex - searchHeadCnt - TEMP_SIZE;
					for(uint32_t i = 0; i < surplusLen; i++)
						g_tcp_buf[i] = g_tcp_buf[i + searchHeadCnt + TEMP_SIZE];										
					recvDataIndex = recvDataIndex - searchHeadCnt - TEMP_SIZE;	//
					searchHeadCnt = 0;
					
					getDataEnable = 0;
				}
				
				// if tcp buf is full
				if(recvDataIndex == REV_SIZE)
				{
					// if don't find the head and getdata
					if((continueRecv == 0) && (getDataEnable == 0))
					{
						recvDataIndex = 0;
						searchHeadCnt = 0;
					}
				}
			}
			
			
			break;
		
	}
	
Done:	
	return 0;
}




