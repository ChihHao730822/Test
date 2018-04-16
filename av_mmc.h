
#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3usbconst.h>
#include <cyu3uart.h>
#include <cyu3sib.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include "cyfx3s_msc.h"

#define CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE                 (0x00)
#define CY_U3P_MMC_CMD1_SEND_OP_COND                     (0x01)
#define CY_U3P_SD_MMC_CMD2_ALL_SEND_CID                  (0x02)
#define CY_U3P_SD_CMD3_SEND_RELATIVE_ADDR                (0x03)
#define CY_U3P_MMC_CMD3_SET_RELATIVE_ADDR                (0x03)
#define CY_U3P_SD_MMC_CMD4_SET_DSR                       (0x04)
#define CY_U3P_MMC_CMD5_SLEEP_AWAKE                      (0x05)
#define CY_U3P_SD_MMC_CMD5_IO_SEND_OP_COND               (0x05)
#define CY_U3P_SD_MMC_CMD6_SWITCH                        (0x06)
#define CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD          (0x07)
#define CY_U3P_SD_CMD8_SEND_IF_COND                      (0x08)
#define CY_U3P_MMC_CMD8_SEND_EXT_CSD                     (0x08)
#define CY_U3P_SD_MMC_CMD9_SEND_CSD                      (0x09)
#define CY_U3P_SD_MMC_CMD10_SEND_CID                     (0x0A)
#define CY_U3P_MMC_CMD11_READ_DAT_UNTIL_STOP             (0x0B)
#define CY_U3P_SD_CMD11_VOLTAGE_SWITCH                   (0x0B)
#define CY_U3P_SD_MMC_CMD12_STOP_TRANSMISSION            (0x0C)
#define CY_U3P_SD_MMC_CMD13_SEND_STATUS                  (0x0D)
#define CY_U3P_MMC_CMD14_BUSTEST_R                       (0x0E)
#define CY_U3P_SD_MMC_CMD15_GO_INACTIVE_STATE            (0x0F)
#define CY_U3P_SD_MMC_CMD16_SET_BLOCKLEN                 (0x10)
#define CY_U3P_SD_MMC_CMD17_READ_SINGLE_BLOCK            (0x11)
#define CY_U3P_SD_MMC_CMD18_READ_MULTIPLE_BLOCK          (0x12)
#define CY_U3P_MMC_CMD19_BUSTEST_W                       (0x13)
#define CY_U3P_SD_CMD19_SEND_TUNING_PATTERN              (0x13)
#define CY_U3P_MMC_CMD20_WRITE_DAT_UNTIL_STOP            (0x14)
#define CY_U3P_SD_MMC_CMD21_RESERVED_21                  (0x15)
#define CY_U3P_SD_MMC_CMD22_RESERVED_22                  (0x16)
#define CY_U3P_MMC_CMD23_SET_BLOCK_COUNT                 (0x17)
#define CY_U3P_SD_MMC_CMD24_WRITE_BLOCK                  (0x18)
#define CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK         (0x19)
#define CY_U3P_MMC_CMD26_PROGRAM_CID                     (0x1A)
#define CY_U3P_SD_MMC_CMD27_PROGRAM_CSD                  (0x1B)
#define CY_U3P_SD_MMC_CMD28_SET_WRITE_PROT               (0x1C)
#define CY_U3P_SD_MMC_CMD29_CLR_WRITE_PROT               (0x1D)
#define CY_U3P_SD_MMC_CMD30_SEND_WRITE_PROT              (0x1E)
#define CY_U3P_SD_MMC_CMD31_RESERVED                     (0x1F)
#define CY_U3P_SD_CMD32_ERASE_WR_BLK_START               (0x20)
#define CY_U3P_SD_CMD33_ERASE_WR_BLK_END                 (0x21)
#define CY_U3P_SD_MMC_CMD34_RESERVED                     (0x22)
#define CY_U3P_MMC_CMD35_ERASE_GROUP_START               (0x23)
#define CY_U3P_MMC_CMD36_ERASE_GROUP_END                 (0x24)
#define CY_U3P_SD_MMC_CMD37_RESERVED                     (0x25)
#define CY_U3P_SD_MMC_CMD38_ERASE                        (0x26)
#define CY_U3P_MMC_CMD39_FAST_IO                         (0x27)
#define CY_U3P_MMC_CMD40_GO_IRQ_STATE                    (0x28)
#define CY_U3P_SD_MMC_CMD41_RESERVED                     (0x29)
#define CY_U3P_SD_MMC_CMD42_LOCK_UNLOCK                  (0x2A)
#define CY_U3P_SD_MMC_CMD43_RESERVED                     (0x2B)
#define CY_U3P_SD_MMC_CMD44_RESERVED                     (0x2C)
#define CY_U3P_SD_MMC_CMD45_RESERVED                     (0x2D)
#define CY_U3P_SD_MMC_CMD46_RESERVED                     (0x2E)
#define CY_U3P_SD_MMC_CMD47_RESERVED                     (0x2F)
#define CY_U3P_SD_MMC_CMD48_RESERVED                     (0x30)
#define CY_U3P_SD_MMC_CMD49_RESERVED                     (0x31)
#define CY_U3P_SD_MMC_CMD50_RESERVED                     (0x32)
#define CY_U3P_SD_MMC_CMD51_RESERVED                     (0x33)
#define CY_U3P_SD_MMC_CMD52_IO_RW_DIRECT                 (0x34)
#define CY_U3P_SD_MMC_CMD53_IO_RW_EXTENDED               (0x35)
#define CY_U3P_SD_MMC_CMD54_RESERVED                     (0x36)
#define CY_U3P_SD_MMC_CMD55_APP_CMD                      (0x37)
#define CY_U3P_SD_MMC_CMD56_GEN_CMD                      (0x38)
#define CY_U3P_SD_MMC_CMD57_RESERVED                     (0x39)
#define CY_U3P_SD_MMC_CMD58_RESERVED                     (0x3A)
#define CY_U3P_SD_MMC_CMD59_RESERVED                     (0x3B)
#define CY_U3P_SD_MMC_CMD60_RW_MULTIPLE_REGISTER         (0x3C)
#define CY_U3P_SD_MMC_CMD61_RW_MULTIPLE_BLOCK            (0x3D)


/** \section SDMMC_Response MMC card response length
    \brief These macros define constants denoting the length in bits of various
    response types defined in the SD and MMC specifications. These lengths are
    excluding the Start, direction and CRC bits.
 */

#define CY_U3P_MMC_RESP_None                       	  (0x0)
#define CY_U3P_MMC_R1_RESP_BITS                       (0x1)
#define CY_U3P_MMC_R2_RESP_BITS                       (0x2)
#define CY_U3P_MMC_R3_RESP_BITS                       (0x3)
#define CY_U3P_MMC_R1B_RESP_BITS                      (0x4)


static CyU3PDmaChannel glChHandleMscOut;                    /* DMA channel for OUT endpoint. */
static CyU3PDmaChannel glChHandleMscIn;                     /* DMA channel for IN endpoint. */
static CyU3PEvent  glMscAppEvent;

CyBool_t CmdPaser(uint8_t *glMscCbwBuffer)
{
	uint8_t mmc_cmd = glMscCbwBuffer[21]; //Defined which CMD

	switch (mmc_cmd)
	{
	case CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE: //CMD 0
	{
		mmc_go_idle(glMscCbwBuffer);
	}
		break;

	case CY_U3P_MMC_CMD1_SEND_OP_COND: //CMD 1
	{

	}
		break;

	case CY_U3P_SD_MMC_CMD2_ALL_SEND_CID: //CMD 2
	{

	}
		break;
	case CY_U3P_MMC_CMD3_SET_RELATIVE_ADDR: //CMD 3
	{

	}
		break;
	case CY_U3P_MMC_CMD5_SLEEP_AWAKE: //CMD 5
	{

	}
		break;
	case CY_U3P_SD_MMC_CMD6_SWITCH: //CMD 6
	{
		mmc_SendSwitchCommand(glMscCbwBuffer);
		//mmc_SendSwitchCommand2 (glMscCbwBuffer);
	}
		break;
	case CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD: //CMD 7
	{

	}
		break;

	case CY_U3P_MMC_CMD8_SEND_EXT_CSD: //CMD 8
	{
		mmc_ExtCsd();
	}
		break;
	case CY_U3P_SD_MMC_CMD13_SEND_STATUS: //CMD 13
	{
		mmc_CheckStatus();
	}
		break;
	case CY_U3P_MMC_CMD23_SET_BLOCK_COUNT: //CMD 23
	{
		;
	}
		break;
	case CY_U3P_SD_MMC_CMD17_READ_SINGLE_BLOCK: //CMD17 18
	case CY_U3P_SD_MMC_CMD18_READ_MULTIPLE_BLOCK:
	{
		//mmc_read_block(glMscCbwBuffer);
	}
		break;

	case CY_U3P_SD_MMC_CMD24_WRITE_BLOCK: //CMD 24 25
	case CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK:
	{
		//mmc_write_block(glMscCbwBuffer);
	}

		break;
	case CY_U3P_MMC_CMD35_ERASE_GROUP_START: //CMD 35
		{

		}
			break;
	case CY_U3P_MMC_CMD36_ERASE_GROUP_END: //CMD 36
		{

		}
			break;
	case CY_U3P_SD_MMC_CMD38_ERASE: //CMD 38
		{

		}
			break;
	case CY_U3P_SD_MMC_CMD60_RW_MULTIPLE_REGISTER: //CMD 60
	{

	}
		break;
	default:
	{

	}
		break;
	}
	return CyFalse;
}

int noData_VendorCmd(uint8_t Cmd, uint32_t cmdArg, uint32_t respType)
{
	uint8_t respLen; //bit
	uint8_t respdata[7];
	uint8_t R2respdata[19];
	uint32_t response;
	CyU3PReturnStatus_t status;

	switch (respType)
	{
	case CY_U3P_MMC_RESP_None:
		respLen = 0;
		CyU3PSibVendorAccess(0, Cmd, cmdArg, respLen, respdata, 0, 0,
						0);
		break;
	case CY_U3P_MMC_R1_RESP_BITS:
	case CY_U3P_MMC_R3_RESP_BITS:
	case CY_U3P_MMC_R1B_RESP_BITS:
		respLen = 56;
		status = CyU3PSibVendorAccess(0, Cmd, cmdArg, respLen, respdata, 0, 0,
				0);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(4, "command : %d fail\n", Cmd);
			return 1;
		}
		response = (((uint32_t) respdata[6] << 24)
				| ((uint32_t) respdata[5] << 16) | ((uint32_t) respdata[4] << 8)
				| ((uint32_t) respdata[3]));
		break;
	case CY_U3P_MMC_R2_RESP_BITS:
		respLen = 152;
		status = CyU3PSibVendorAccess(0, Cmd, cmdArg, respLen, R2respdata, 0, 0, 0);
	}

	return response;
}

CyBool_t mmc_go_idle(uint8_t *glMscCbwBuffer)
{
	uint32_t cmdArg;
	uint8_t Cmd = CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE;
	cmdArg = (((uint32_t) glMscCbwBuffer[17] << 24)
			| ((uint32_t) glMscCbwBuffer[18] << 16)
			| ((uint32_t) glMscCbwBuffer[19] << 8)
			| ((uint32_t) glMscCbwBuffer[20]));
	uint32_t resp;
	//resp = noData_VendorCmd(Cmd, cmdArg, CY_U3P_MMC_RESP_None);
	uint8_t respLen = 0; //bit
		uint8_t *respdata=NULL; //byte
		uint16_t numblks = 0;
		CyBool_t isRead = 0;
		uint8_t socket = 0;
	CyU3PSibVendorAccess(0, Cmd, cmdArg, respLen, respdata, numblks, isRead, socket);
	CyU3PDebugPrint(4, "CMD0 : %d -> %x \n", cmdArg,resp);
	return 0;
}

int mmc_ExtCsd()
{
	char extcsd[512];
	uint32_t count;

	CyU3PReturnStatus_t status;
	status = CyU3PSibGetMMCExtCsd (0,extcsd);

	for(count=0;count<512;count++)
	{
		CyU3PDebugPrint(4,"\r%x\n",extcsd[count]);

	}

	return 0;
}

int mmc_CheckStatus(void) //cmd13
{
	uint8_t portid = 0;
	uint32_t resp_status;
	CyU3PSibGetCardStatus (portid, &resp_status);
	return resp_status;
}

CyBool_t mmc_read_block1(uint8_t *glMscCbwBuffer)
{

	return 0;
}

int mmc_write_block1(uint32_t cmdArg, uint8_t count,const uint8_t *buff)
{
	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t BufOUT_t;
	uint8_t portid;
	portid =0;

	uint8_t *tbuff;
	tbuff = CyU3PMemAlloc(512);
	if(tbuff==NULL)
	{
		CyU3PDebugPrint(4,"memAlloc Fail\r\n");
		return 0;
	}
	uint32_t i;
	for(i=0;i<512;i++)
		tbuff[i]=0x55;

	BufOUT_t.buffer = (uint8_t *)tbuff;
	BufOUT_t.count = (uint16_t)count * 512;
	BufOUT_t.size = (uint16_t)count * 512;
	BufOUT_t.status = 0;

	status = CyU3PSibReadWriteRequest(CyFalse, 0, 0, (uint16_t)count, (uint32_t)cmdArg, 0);
	if(status != CY_U3P_SUCCESS)
		{
			//Abort the DMA channel
		CyU3PDebugPrint(4,"ReadWriteRequest Fail\r\n");
		}
	status = CyU3PDmaChannelSetupSendBuffer (&glChHandleMscIn,  &BufOUT_t);
	if(status != CY_U3P_SUCCESS)
			{
				//Abort the DMA channel
			CyU3PDebugPrint(4,"SendBuffer Fail\r\n");
			}
	/*
	if(status != CY_U3P_SUCCESS)
	{
		//Abort the DMA channel
		CyU3PDmaChannelReset(&glChHandleMscIn);
	}

	status = CyU3PDmaChannelSetupSendBuffer (&glChHandleMscIn,  &BufOUT_t);

	    if (status == CY_U3P_SUCCESS)
	    {
	        status = CyU3PEventGet (&glMscAppEvent, CY_FX_MSC_DATASENT_EVENT_FLAG, CYU3P_EVENT_OR_CLEAR, &evStat,
	                CYU3P_WAIT_FOREVER);
	        if (status == CY_U3P_SUCCESS)
	            status = CyU3PDmaChannelWaitForCompletion (&glChHandleMscIn, CYU3P_WAIT_FOREVER);
	    }

	    if (status != CY_U3P_SUCCESS)
	    {
	        CyU3PSibAbortRequest (portid);
	        CyU3PDmaChannelReset (&glChHandleMscIn);
	    }
*/
	return 0;
}

CyBool_t mmc_write(uint8_t *glMscCbwBuffer)
{
	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t BufOUT_t;

	uint8_t Cmd;
	uint32_t startAddr;
	uint16_t numBlks;
	CyBool_t isRead = 0;
	startAddr = (((uint32_t)glMscCbwBuffer[17] << 24) | ((uint32_t)glMscCbwBuffer[18] << 16) |
            	((uint32_t)glMscCbwBuffer[19] << 8) | ((uint32_t)glMscCbwBuffer[20]));
	numBlks = (((uint16_t)glMscCbwBuffer[22] << 8) | ((uint16_t)glMscCbwBuffer[23]));

	if(numBlks<2)
		Cmd = CY_U3P_SD_MMC_CMD24_WRITE_BLOCK;
	else
		Cmd = CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK;

	uint8_t respLen = CY_U3P_MMC_R1B_RESP_BITS;
	uint8_t lun = glMscCbwBuffer[13] & 0x0F;
	uint8_t respdata[2];

	CyU3PSibVendorAccess(lun, Cmd, startAddr, respLen, respdata, numBlks, isRead, 0 );

	return 0;
}

CyBool_t mmc_SendSwitchCommand (uint8_t *glMscCbwBuffer)
{
	uint32_t argument;
	CyU3PReturnStatus_t status;
	uint32_t resp_buff;
	argument = (((uint32_t)glMscCbwBuffer[17] << 24) | ((uint32_t)glMscCbwBuffer[18] << 16) |
	                        ((uint32_t)glMscCbwBuffer[19] << 8) | ((uint32_t)glMscCbwBuffer[20]));

	status = CyU3PSibSendSwitchCommand (0, argument, &resp_buff);
	if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(4,"Send CMD6 Fail\r\n");
	    }

	CyU3PDebugPrint(4,"CMD6: %x -> %x\r\n",argument,resp_buff);

	return 0;
}

CyBool_t mmc_SendSwitchCommand2 (uint8_t *glMscCbwBuffer)
{
	uint8_t portid = 0;
	uint8_t Cmd = CY_U3P_SD_MMC_CMD6_SWITCH;
	uint32_t cmdArg,response;
	cmdArg = (((uint32_t)glMscCbwBuffer[17] << 24) | ((uint32_t)glMscCbwBuffer[18] << 16) |
		           ((uint32_t)glMscCbwBuffer[19] << 8) | ((uint32_t)glMscCbwBuffer[20]));
	CyU3PDebugPrint(4,"\r arg : %x \n",cmdArg);

	uint8_t respLen = 56; //bit
	uint8_t respdata[7]; //byte
	uint16_t numblks = 0;
	CyBool_t isRead = 0;
	uint8_t socket = 0;

	CyU3PReturnStatus_t status;
	status = CyU3PSibVendorAccess(portid, Cmd, cmdArg, respLen, respdata, numblks, isRead, socket);

//	extern CyU3PReturnStatus_t
//	CyU3PSibVendorAccess (
//	        uint8_t   portId,               /**< Storage port on which command is to be executed. */
//	        uint8_t   cmd,                  /**< SD/MMC Commande to be sent */
//	        uint32_t  cmdArg,               /**< Command argument */
//	        uint8_t   respLen,              /**< Length of response in bits */
//	        uint8_t  *respData,             /**< Pointer to response data */
//	        uint16_t  numBlks,              /**< Length of data to be transferred in blocks. Set to 0 if there is
//	                                             no data phase. */
//	        CyBool_t  isRead,               /**< Direction of data transfer, in case there is a data phase. */
//	        uint8_t   socket                /**< S-Port socket to be used. */
//	        );

	response = (((uint32_t)respdata[6] << 24) | ((uint32_t)respdata[5] << 16) |
			           ((uint32_t)respdata[4] << 8) | ((uint32_t)respdata[3]));
	if (status != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint(4,"\r Send CMD6 Fail \n");
		    }

	return 0;
}
