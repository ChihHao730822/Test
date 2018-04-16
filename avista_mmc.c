/**
 * Avista MP
 *
 */
#include <avista_mmc.h>//add

extern uint32_t glCmdIndex;
extern CyU3PDmaChannel glChHandleUARTtoCPU;

/*glCardType :
 *  1  = eMMC
 * 	2  = SD_CARD2.0
*/
uint32_t glCardType = CY_U3P_SIB_DEV_NONE; //define card type

uint32_t emmcSendCommand(uint8_t portId, uint8_t cmdIdex, uint32_t cmdArg, uint32_t cmdRespLenth)
{
	CyU3PCardMgrSendCmd(portId, cmdIdex,cmdRespLenth, cmdArg, 0);

		if (waitForResponse(portId) == rcvResponse)
		{
			readResp(portId);
			return 0;
		}
		else
			return 1;

}

void mmc_Cmd0_Go_Idle(uint32_t CmdArg,uint8_t portId)
{
	uint32_t *dataCfgPtr = (uint32_t *)PORTADDRESS(SDMMC_DATA_CFG,portId);
	*dataCfgPtr = 0xf;//set sdmmc_data_cfg to correct configuration

	set_Mmc_Clk(1,portId);//set clock to 400k
	bus_width_switch(0x0,portId);//set bus_width to 1bit_SDR

	glCardType = CY_U3P_SIB_DEV_NONE;

	CyU3PCardMgrSendCmd(portId,CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE,0,CmdArg,0);
	CyU3PDebugPrint(4,"\r Send CMD0 : %x\n",CmdArg);
}

void mmc_cmd1_rdy(uint32_t CmdArg,uint8_t portId)
{
	uint32_t timeout = 50;
	CyU3PReturnStatus_t status;
	uint32_t *respPtr0 = (uint32_t*) SDMMC_RESP_REG0;
	do
	{
		status = mmc_Cmd1_Send_Op_Cond(CmdArg, portId);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(4, "\r Send CMD1 Fail \n");
		}

	} while ((*respPtr0) != 0xc0ff8080 && timeout--);
}

uint32_t mmc_Cmd1_Send_Op_Cond(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD1_SEND_OP_COND,
	CY_U3P_MMC_R3_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		glCardType = CY_U3P_SIB_DEV_MMC;//if cmd1 had response,set device type to eMMC.
		uint32_t *respPtr0 = (uint32_t *)PORTADDRESS(SDMMC_RESP_REG0,portId);
		if((*respPtr0 >> 24) == 0xc0)
		{
			CyU3PDebugPrint(4, "\r high capacity \n");
			setHighCapacity(portId,1);//1 high capacity
		}
		else if((*respPtr0 >> 24) == 0x80)
		{
			CyU3PDebugPrint(4, "\r not high capacity \n");
			setHighCapacity(portId,0);//0 not high capacity
		}
		readResp(portId);
		return 0;
	}
	else
		return 1;

}

uint32_t mmc_Cmd2_All_Send_CID(uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD2_ALL_SEND_CID,
	CY_U3P_MMC_R2_RESP_BITS, 0x00, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		uint32_t *cmdIdx = PORTADDRESS(SDMMC_CMD_IDX, portId);
		uint32_t *respPtr0 = PORTADDRESS(SDMMC_RESP_REG0, portId);
		CyU3PDebugPrint(4, "\r CMD%d  %x %x %x %x \n", *cmdIdx, *respPtr0,
				*(respPtr0 + 1), *(respPtr0 + 2), *(respPtr0 + 3));
		return 0;
	}
	else
		return 1;
}

uint32_t mmc_Cmd3_Set_Relative_Addr(uint32_t rca,uint8_t portId)
{
	return emmcSendCommand(portId, CY_U3P_MMC_CMD3_SET_RELATIVE_ADDR, rca, CY_U3P_MMC_R1_RESP_BITS);
}
uint32_t mmc_Cmd5_Sleep_Awake(uint32_t arg,uint8_t portId)
{
	return emmcSendCommand(portId, CY_U3P_MMC_CMD5_SLEEP_AWAKE, arg, CY_U3P_MMC_R1B_RESP_BITS);
}

uint32_t mmc_Cmd6_Switch (uint32_t CmdArg,uint8_t portId)
{
	return emmcSendCommand(portId, CY_U3P_SD_MMC_CMD6_SWITCH, CmdArg, CY_U3P_MMC_R1B_RESP_BITS);
}

uint32_t mmc_Cmd7_Select_Deselect_Card(uint32_t arg,uint8_t portId)
{
	return emmcSendCommand(portId, CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD, arg, CY_U3P_MMC_R1_RESP_BITS);
//	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD,
//	CY_U3P_MMC_R1_RESP_BITS, arg, 0);

//	if (waitForResponse(portId) == rcvResponse)
//	{
//		readResp(portId);
//		return 0;
//	}
//	else
//		return 1;

}

int mmc_Cmd8_Send_Extcsd(uint8_t *buff,uint8_t portId,uint32_t arg)
{

	CyU3PReturnStatus_t status;
	uint16_t numBlks;
	uint8_t respdata[7]; //byte
	CyU3PDmaBuffer_t BufIn;

	BufIn.buffer = (uint8_t*)buff;
	BufIn.count = 0;
	BufIn.size = 512;
	BufIn.status = 0;
	CyU3PSibSetBlockLen(0, 512);

	numBlks = 0x1;
	CyU3PMemSet (buff, 0, 512);
	CyU3PDmaChannelReset(&glChHandleStorIn);
	status = CyU3PDmaChannelSetupRecvBuffer(&glChHandleStorIn, &BufIn);
	if (status == CY_U3P_SUCCESS)
	{
		status = CyU3PSibVendorAccess(portId, CY_U3P_MMC_CMD8_SEND_EXT_CSD,
				arg, CY_U3P_MMC_R1_RESP_BITS, respdata, numBlks, 1, 3);

		if (status == CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(8, "\r Send cmd8 ok\n");

			uint32_t i;
			for (i = 0; i < 512; i += 0x10)
				CyU3PDebugPrint(8,
						"\r %x %x %x %x %x %x %x %x   %x %x %x %x %x %x %x %x\n",
						buff[i], buff[i + 1], buff[i + 2], buff[i + 3],
						buff[i + 4], buff[i + 5], buff[i + 6], buff[i + 7],
						buff[i + 8], buff[i + 9], buff[i + 0xa], buff[i + 0xb],
						buff[i + 0xc], buff[i + 0xd], buff[i + 0xe],
						buff[i + 0xf]);

		}
		else
		{
			CyU3PDebugPrint(8, "\r Send cmd8 fail, %x\n", status);
			/* Abort the DMA Channel */
			CyU3PDmaChannelReset(&glChHandleStorIn);
			return 1;
		}
	}else{
		CyU3PDebugPrint(4,"\r status = %x\n",status);
		return 1;
	}
	return 0;
}

void read_Extcsd(uint8_t portId,uint32_t arg)
{
	/**
	 * read extCSD and send data to USB host
	 */

	uint32_t *respPtr0 = PORTADDRESS(SDMMC_RESP_REG0, portId);
	uint8_t extCsdBuff[512] =
	{ 0 };
	CyU3PReturnStatus_t status;

	//Check card is eSD
	if (glCardType != CY_U3P_SIB_DEV_MMC)
	{
		CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD8_SEND_EXT_CSD,
		CY_U3P_MMC_R1_RESP_BITS, arg, 0); //Check card is SD card or not

		if (waitForResponse(portId) == rcvResponse)
		{
			glCardType = CY_U3P_SIB_DEV_SD;
			if (((*respPtr0) & 0xfff) == 0x1AA)
			{
				extCsdBuff[2] = 0x1;
				extCsdBuff[3] = 0xAA;
				SendDataToUsb(extCsdBuff, 512);

			}
			else
				glCardType = CY_U3P_SIB_DEV_MMC;

		}else
		{
			glCardType = CY_U3P_SIB_DEV_MMC;
			returnErorToUSB();
		}
		return;
	}


	if (glCardType != CY_U3P_SIB_DEV_SD)
	{

		status = mmc_Cmd8_Send_Extcsd(extCsdBuff, portId,arg);
		if (status == CY_U3P_SUCCESS){
			SendDataToUsb(extCsdBuff, 512);
			readResp(portId);
		}else
			returnErorToUSB();
	}

}

uint32_t mmc_Cmd9_Send_CSD(uint32_t cmdArg, uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD9_SEND_CSD,
	CY_U3P_MMC_R2_RESP_BITS, cmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		uint32_t *cmdIdx = PORTADDRESS(SDMMC_CMD_IDX, portId);
		uint32_t *respPtr0 = PORTADDRESS(SDMMC_RESP_REG0, portId);
		CSDReg[0] = *(respPtr0);CSDReg[1] = *(respPtr0+1);CSDReg[2] = *(respPtr0+2);CSDReg[3] = *(respPtr0+3);
		CyU3PDebugPrint(4, "\r CMD%d -> %x %x %x %x\n", *cmdIdx, CSDReg[0],
				CSDReg[1], CSDReg[2], CSDReg[3]);

		return 0;
	}
	else
		return 1;
}

uint32_t mmc_Cmd10_Send_CID(uint32_t cmdArg, uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD10_SEND_CID,
	CY_U3P_MMC_R2_RESP_BITS, cmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		uint32_t *cmdIdx = PORTADDRESS(SDMMC_CMD_IDX, portId);
		uint32_t *respPtr0 = PORTADDRESS(SDMMC_RESP_REG0, portId);
		CyU3PDebugPrint(4, "\r CMD%d -> %x %x %x %x\n", *cmdIdx, *respPtr0,
				*(respPtr0+1), *(respPtr0+2), *(respPtr0+3));
		return 0;
	}
	else
		return 1;
}

uint32_t mmc_Cmd12_Stop_Transmission(uint32_t cmdArg,uint8_t portId) //cmd12
{
	return emmcSendCommand(portId, CY_U3P_SD_MMC_CMD12_STOP_TRANSMISSION, cmdArg, CY_U3P_MMC_R1B_RESP_BITS);
}

uint32_t mmc_Cmd13_Send_Status(uint32_t cmdArg,uint8_t portId) //cmd13
{
	return emmcSendCommand(portId, CY_U3P_SD_MMC_CMD13_SEND_STATUS, cmdArg, CY_U3P_MMC_R1B_RESP_BITS);
}

void check_Ready(uint8_t portId)
{
	int timeout = 500;
	uint32_t *respPtr0 = (uint32_t *)PORTADDRESS(SDMMC_RESP_REG0,portId);

	mmc_Cmd13_Send_Status(RCA,portId);
	while((timeout--)&&(*respPtr0 != 0x900))
	{
		mmc_Cmd13_Send_Status(RCA,portId);
	}
	if(timeout<=0)
		CyU3PDebugPrint(4, "\r Device never ready\n");

}

uint32_t mmc_Cmd14_Bustest_R(uint8_t portId)
{
	CyU3PReturnStatus_t status;

	CyU3PMemSet(glBusTestBuffer, 0, 512);
	status = CyU3PCardMgrMMCBusTestR(portId,8,glBusTestBuffer);
	if(status != CY_U3P_SUCCESS){
			CyU3PDebugPrint(4, "\r Bus Test read fail %x\n", status);
			returnErorToUSB();
	}else{
		CyU3PDebugPrint(4, "\r glBusTestBuffer[0] : %x \n", glBusTestBuffer[0]);
		CyU3PDebugPrint(4, "\r glBusTestBuffer[1] : %x \n", glBusTestBuffer[1]);

		SendDataToUsb(glBusTestBuffer,512);
	}
	if(glBusTestBuffer != 0){
		CyU3PDmaBufferFree(glBusTestBuffer);
	}
	uint32_t *dataCfgPtr = (uint32_t *)PORTADDRESS(SDMMC_DATA_CFG,portId);
	*dataCfgPtr |= 0xf;

	return status;
}

void mmc_Cmd15_Go_Inactive_State(void)
{
	uint8_t portid = 0;
	CyU3PDebugPrint(4, "\r CMD15 : %x ", RCA);
	CyU3PCardMgrSendCmd(portid,CY_U3P_SD_MMC_CMD15_GO_INACTIVE_STATE,0,RCA,0);

}

void mmc_Cmd18_Read_Multiple_Block(uint32_t startAddr, uint16_t numBlks)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	status = CyU3PDmaChannelSetXfer(&glChHandleMscIn, 512 * (uint16_t) numBlks);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n",
				status);
		return ;
	}

	status = CyU3PCardMgrSetupRead(0, 0, 1, (uint16_t) numBlks, startAddr);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "\r Send Read fail %x\n", status);
		CyU3PDmaChannelReset(&glChHandleMscIn);
		return;
	}

	readResp(0);
}

uint32_t mmc_Cmd19_Bustest_W(uint8_t portId)
{
	CyU3PReturnStatus_t status;

	glBusTestBuffer = (uint8_t *)CyU3PDmaBufferAlloc (512);
	if(glBusTestBuffer == 0){
		CyU3PDebugPrint(4, "\r memory allocation fail\n");
		return 1;
	}

	uint32_t *dataCfgPtr = (uint32_t *)PORTADDRESS(SDMMC_DATA_CFG,portId);
	*dataCfgPtr &= 0;

	status = CyU3PCardMgrMMCBusTestW(portId,8,glBusTestBuffer);
	if(status != CY_U3P_SUCCESS){
		CyU3PDmaBufferFree(glBusTestBuffer);
		CyU3PDebugPrint(4, "\r Bus Test Write fail %x\n", status);
	}

	return status;
}

uint32_t mmc_Cmd21_Tunning_Block(uint32_t CmdArg,uint8_t portId)
{

	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t BufIn_t;
	uint8_t *tuningBlockBuffer = (uint8_t *) CyU3PDmaBufferAlloc(512);
	if (tuningBlockBuffer == 0)
	{
		CyU3PDebugPrint(4, "\r memory allocation fail\n");
		return 1;
	}

	CyU3PMemSet(tuningBlockBuffer, 0, 512);

	CyU3PSibSetBlockLen(portId, 128);
	CyU3PSibSetNumBlocks(portId, 1);
	CyU3PSibSetActiveSocket(portId,3);

	BufIn_t.buffer = (uint8_t*) tuningBlockBuffer;
	BufIn_t.count = 0;
	BufIn_t.size = 512;
	BufIn_t.status = 0;

	status = CyU3PDmaChannelSetupRecvBuffer(&glChHandleStorIn, &BufIn_t);
	if (status == CY_U3P_SUCCESS)
	{
//		status = CyU3PSibVendorAccess(portId, CY_U3P_MMC_CMD21_SEND_TUNING_BLOCK,
//						0, CY_U3P_MMC_R1_RESP_BITS, respdata, 1, 1, 3);
		CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD21_SEND_TUNING_BLOCK,	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 1<<2);
		status = waitForResponse(portId);
		if (status == rcvResponse)
		{
			readResp(portId);
		}
	}else
	{
		CyU3PDebugPrint(4, "\r Send cmd21 fail %x\n", status);
		CyU3PDmaChannelReset(&glChHandleStorIn);
	}
	CyU3PDmaChannelReset(&glChHandleStorIn);
	if (tuningBlockBuffer != 0)
		{
			CyU3PDmaBufferFree(tuningBlockBuffer);
		}
	return status;
}

uint32_t mmc_Cmd23_Set_Block_Count (uint32_t CmdArg,uint8_t portId) //23
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD23_SET_BLOCK_COUNT,
		CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

		if (waitForResponse(portId) == rcvResponse){
			readResp(portId);
			return 0;
		}
		else
		{
			return 1;
		}

}

void mmc_Cmd25_Write_Multiple_Block(uint32_t startAddr ,uint16_t numBlks)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	status = CyU3PDmaChannelSetXfer(&glChHandleMscOut, 512 * numBlks);

	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n",
				status);
		CyFxAppErrorHandler(status);
	}

	status = CyU3PCardMgrSetupWrite(0, 0, 0, numBlks, startAddr);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "\r Send cmd25 fail %x\n", status);
		CyU3PDmaChannelReset(&glChHandleMscOut);
		return ;
	}
	readResp(0);
}

void programCID_CSD(uint32_t cmd)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	status = CyU3PDmaChannelSetXfer(&glChHandleMscOut, 16);

	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n",
				status);
		CyFxAppErrorHandler(status);
	}

	CyU3PSibSetBlockLen(0, 16);
	CyU3PSibSetNumBlocks(0, 0x1);

	CyU3PSibSetActiveSocket(0, 0); //(port Id,socket number)
	CyU3PCardMgrSendCmd(0, cmd,
	CY_U3P_MMC_R1_RESP_BITS, 0, 0);
	status = waitForResponse(0);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "waitForResponse fail, %x\n", status);
		return;
	}
	uint32_t *mmcCS = (uint32_t *) PORTADDRESS(SDMMC_CS, 0);
	*mmcCS |= WRDCARD;
	status = CyU3PCardMgrWaitForInterrupt(0, BLOCK_COMP, 0xffff);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "CyU3PCardMgrWaitForInterrupt fail, %x\n", status);
		return;
	}
}

//void mmc_Cmd26_Program_Cid(void)
//{
//
//	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
//	status = CyU3PDmaChannelSetXfer(&glChHandleMscOut, 16);
//
//	if (status != CY_U3P_SUCCESS)
//	{
//		CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n",
//				status);
//		CyFxAppErrorHandler(status);
//	}
//
//	CyU3PSibSetBlockLen(0, 16);
//	CyU3PSibSetNumBlocks(0, 0x1);
//
//	CyU3PSibSetActiveSocket(0, 0);//(port Id,socket number)
//	CyU3PCardMgrSendCmd(0, CY_U3P_MMC_CMD26_PROGRAM_CID,
//			CY_U3P_MMC_R1_RESP_BITS, 0, 0);
//	status = waitForResponse(0);
//	if (status != CY_U3P_SUCCESS)
//	{
//		CyU3PDebugPrint(4, "waitForResponse fail, %x\n", status);
//		return;
//	}
//	uint32_t *mmcCS = (uint32_t *) PORTADDRESS(SDMMC_CS, 0);
//	*mmcCS |= WRDCARD;
//	status = CyU3PCardMgrWaitForInterrupt(0, BLOCK_COMP, 0xffff);
//	if (status != CY_U3P_SUCCESS)
//	{
//		CyU3PDebugPrint(4, "CyU3PCardMgrWaitForInterrupt fail, %x\n", status);
//		return;
//	}
//}
//
//void mmc_Cmd27_Program_Csd(void)
//{
//
//	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
//		status = CyU3PDmaChannelSetXfer(&glChHandleMscOut, 16);
//
//		if (status != CY_U3P_SUCCESS)
//		{
//			CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n",
//					status);
//			CyFxAppErrorHandler(status);
//		}
//
//		CyU3PSibSetBlockLen(0, 16);
//		CyU3PSibSetNumBlocks(0, 0x1);
//
//		CyU3PSibSetActiveSocket(0, 0);
//		CyU3PCardMgrSendCmd(0, CY_U3P_SD_MMC_CMD27_PROGRAM_CSD,CY_U3P_MMC_R1_RESP_BITS, 0, 0);
//		status = waitForResponse(0);
//		if (status != CY_U3P_SUCCESS)
//		{
//			CyU3PDebugPrint(4, "waitForResponse fail, %x\n", status);
//			return;
//		}
//
//		uint32_t *mmcCS = (uint32_t *) PORTADDRESS(SDMMC_CS, 0);
//		*mmcCS |= WRDCARD;
//
//		status = CyU3PCardMgrWaitForInterrupt(0, BLOCK_COMP, 0xffff);
//		if (status != CY_U3P_SUCCESS)
//		{
//			CyU3PDebugPrint(4, "CyU3PCardMgrWaitForInterrupt fail, %x\n", status);
//			return;
//		}
//}

uint32_t mmc_Cmd28_Set_Write_Prot(uint32_t CmdArg,uint8_t portId)
{

	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD28_SET_WRITE_PROT,
	CY_U3P_MMC_R1B_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t mmc_Cmd29_Clr_Write_Prot(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD29_CLR_WRITE_PROT,
	CY_U3P_MMC_R1B_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}
uint32_t mmc_Cmd30_Send_Write_Prot(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD30_SEND_WRITE_PROT,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}
uint32_t mmc_Cmd31_Send_Write_Prot_Type(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD31_SEND_WRITE_PROT_TYPE,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t sd_Cmd32_Erase_Wr_Blk_Start(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_CMD32_ERASE_WR_BLK_START,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t sd_Cmd33_Erase_Wr_Blk_End(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_CMD33_ERASE_WR_BLK_END,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t mmc_Cmd35_Erase_Group_Start(uint32_t CmdArg,uint8_t portId)
{

	CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD35_ERASE_GROUP_START,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t mmc_Cmd36_Erase_Group_End(uint32_t CmdArg,uint8_t portId)
{

	CyU3PCardMgrSendCmd(portId, CY_U3P_MMC_CMD36_ERASE_GROUP_END,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t mmc_Cmd38_Erase(uint32_t CmdArg,uint8_t portId)
{

	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD38_ERASE,
	CY_U3P_MMC_R1B_RESP_BITS, CmdArg, 0);

	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}
/*
void mmc_cmd39_fast_io(void)
{

}
void mmc_cmd40_go_irq_state(void)
{

}

*/

uint32_t SD_Acmd41_SD_Send_Op_Cond(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_ACMD41_SD_SEND_OP_COND,CY_U3P_MMC_R3_RESP_BITS, CmdArg, 0);
	if (waitForResponse(portId) == rcvResponse)
	{
		glCardType = CY_U3P_SIB_DEV_SD;
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}


uint32_t mmc_cmd42_lock_unlock(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD42_LOCK_UNLOCK,
			CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);
	if (waitForResponse(portId) == rcvResponse)
	{
		glCardType = CY_U3P_SIB_DEV_SD;
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}
/*
void mmc_cmd44_queued_task_params(void)
{

}
void mmc_cmd45_queued_task_address(void)
{

}
void mmc_cmd46_excute_read_task(void)
{

}
void mmc_Cmd47_Excute_Write_Task(void)
{

}
void mmc_Cmd48_CMDQ_Task_Mgmt(void)
{

}
void mmc_cmd49_set_time(void)
{

}
void mmc_cmd53_Protocol_Rd(void)
{

}
void mmc_cmd54_Rrotocol_Wr(void)
{

}

*/
uint32_t mmc_cmd55_App_Cmd(uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD55_APP_CMD,CY_U3P_MMC_R1_RESP_BITS, 0, 0);
	if (waitForResponse(portId) == rcvResponse)
	{
		glCardType = CY_U3P_SIB_DEV_SD;
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

/*
void mmc_cmd56_Gen_Cmd(void)
{

}

*/
uint32_t mmc_Cmd60(uint32_t CmdArg,uint8_t portId)
{
	CyU3PCardMgrSendCmd(portId, CY_U3P_SD_MMC_CMD60_RW_MULTIPLE_REGISTER,
	CY_U3P_MMC_R1_RESP_BITS, CmdArg, 0);
	//CyU3PThreadSleep(1);
	if (waitForResponse(portId) == rcvResponse)
	{
		readResp(portId);
		return 0;
	}
	else
	{
		return 1;
	}
}

void initToTranStat(void)
{
	mmc_Cmd0_Go_Idle(0x0, 0);
	CyU3PThreadSleep(50);
	mmc_cmd1_rdy(0x40ff8080, 0);
	mmc_Cmd2_All_Send_CID(0);
	mmc_Cmd3_Set_Relative_Addr(RCA, 0);
	mmc_Cmd9_Send_CSD(RCA, 0);
	mmc_Cmd7_Select_Deselect_Card(RCA, 0);
	mmc_Cmd6_Switch(0x03b90100, 0);
	mmc_Cmd6_Switch(0x03b70600, 0);
	bus_width_switch(0x6, 0);
//	mmc_Cmd6_Switch(0x03b90300, 0);
	set_Mmc_Clk(6, 0); //SET_CLK
	check_Ready(0);
}


void CmdPaser(uint8_t *glMscCbwBuffer)
{
	uint8_t mmc_cmd = glMscCbwBuffer[21]; //Defined which CMD
	uint8_t lun = glMscCbwBuffer[13] & 0x0F;
	uint8_t portId = lun >= CY_FX_SIB_PARTITIONS ? 1 : 0;
	uint32_t cardIns = *(uint32_t*) SDMMC_STATUS;
	uint32_t checkS0S1_Ins = ((cardIns & S0S1_INS)>>9) & 0x1;
	uint32_t arg1, arg2;
	CyU3PReturnStatus_t status = 0;
	uint32_t dataLength;


	arg1 = (((uint32_t) glMscCbwBuffer[17] << 24)
			| ((uint32_t) glMscCbwBuffer[18] << 16)
			| ((uint32_t) glMscCbwBuffer[19] << 8)
			| ((uint32_t) glMscCbwBuffer[20]));
	arg2 =	(((uint8_t) glMscCbwBuffer[22] << 8)
					| ((uint8_t) glMscCbwBuffer[23]));


	if (checkS0S1_Ins != glCardIsIns)
	{
		CyU3PDebugPrint(4, "\r no Card Insert  <<<<<<<<<<<<<<<<<<<<\n");

		if (((mmc_cmd >= 0) && (mmc_cmd <= 60)) || (mmc_cmd ==83))
		{
			if ((mmc_cmd != 18) && (mmc_cmd != 25))
			{
				glMscDataBuffer[0] = 0xff;
				glMscDataBuffer[1] = 0xff;
				glMscDataBuffer[2] = 0xff;
				glMscDataBuffer[3] = 0xff;
				SendDataToUsb(glMscDataBuffer, 512);
			}
			return ;
		}

	}

	switch (mmc_cmd)
	{
	case CY_U3P_SD_MMC_CMD12_STOP_TRANSMISSION: //CMD 12
	{
		/*Intend to avoid cmd12 issued at the wrong timing that data was  not receive completed by card,
		 * added block receive complete check*/

//		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
//				&dataLength))
//		{
//			return;
//		}
		uint32_t *idxReg = (uint32_t *) SDMMC_CMD_IDX;
		uint32_t *INTRReg = (uint32_t *) SDMMC_INTR;

		//if (glPrevCmd == CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK)
		if (*idxReg == CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK)
		{
			while (1)
			{
				CyU3PBusyWait(10);
				if ((*INTRReg & DATAOUTOFBUSY) && (*INTRReg & BLOCK_COMP))
				{
					status = mmc_Cmd12_Stop_Transmission(arg1, portId);
					break;
				}
			}
		}
		else
		{

			status = mmc_Cmd12_Stop_Transmission(arg1, portId);
		}

		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;
	case CY_U3P_SD_MMC_CMD13_SEND_STATUS: //CMD 13
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd13_Send_Status(arg1, portId);

		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;

	case CY_U3P_SD_MMC_CMD17_READ_SINGLE_BLOCK: //CMD17 18
		glCmdIndex = 17;
	case CY_U3P_SD_MMC_CMD18_READ_MULTIPLE_BLOCK:
	{
		mmc_Cmd18_Read_Multiple_Block(arg1, arg2);
		status = CyU3PDmaChannelWaitForCompletion(&glChHandleMscIn,
		CYU3P_WAIT_FOREVER);
		if (status != CY_U3P_SUCCESS)
			CyU3PDebugPrint(4,
					"\r CyU3PDmaChannelWaitForCompletion fail, Error code = %x\n",
					status);
		//if (glMscCbwBuffer[16] == 0)
		glMscState = CY_FX_MSC_STATE_STATUS;
//			else
//				glMscState = CY_FX_MSC_STATE_DATA;
//			readResp(portId);
		glCmdIndex = 0;
	}
		break;
	case CY_U3P_SD_MMC_CMD24_WRITE_BLOCK: //CMD 24 25
		glCmdIndex = 24;
	case CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK:
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyFalse, CyTrue,
				(arg2 * 512), &dataLength))
		{
			return;
		}
		mmc_Cmd25_Write_Multiple_Block(arg1, arg2);
		status = CyU3PDmaChannelWaitForCompletion(&glChHandleMscOut,
		CYU3P_WAIT_FOREVER);
		if (status != CY_U3P_SUCCESS)
			CyU3PDebugPrint(4,"\r lWaitForCompletion fail, Error code = %d\n",status);
//			if (glMscCbwBuffer[16] == 0)
		glMscState = CY_FX_MSC_STATE_STATUS;
//			else
//				glMscState = CY_FX_MSC_STATE_DATA;
//			readResp(portId);
		glCmdIndex = 0;
	}

		break;
	case CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE: //CMD 0
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		mmc_Cmd0_Go_Idle(arg1, portId);
		status = 0xFF;
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case CY_U3P_MMC_CMD1_SEND_OP_COND: //CMD 1
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd1_Send_Op_Cond(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case CY_U3P_SD_MMC_CMD2_ALL_SEND_CID: //CMD 2
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd2_All_Send_CID(portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_MMC_CMD3_SET_RELATIVE_ADDR: //CMD 3
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd3_Set_Relative_Addr(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_MMC_CMD5_SLEEP_AWAKE: //CMD 5
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd5_Sleep_Awake(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD6_SWITCH: //CMD 6
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd6_Switch(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD: //CMD 7
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd7_Select_Deselect_Card(arg1, portId);

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case CY_U3P_MMC_CMD8_SEND_EXT_CSD: //CMD 8
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		read_Extcsd(portId, arg1);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD9_SEND_CSD: //CMD 9
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd9_Send_CSD(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD10_SEND_CID: //CMD 10
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		status = mmc_Cmd10_Send_CID(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case CY_U3P_MMC_CMD14_BUSTEST_R: //CMD 14
	{
		mmc_Cmd14_Bustest_R(portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD15_GO_INACTIVE_STATE: //CMD 15
	{
		mmc_Cmd15_Go_Inactive_State();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD16_SET_BLOCKLEN: //CMD 16
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, (1 * 512),
				&dataLength))
		{
			return;
		}
		CyU3PCardMgrSetBlockSize(portId, arg1);
		//SendRespToUsb();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case CY_U3P_MMC_CMD19_BUSTEST_W: //CMD 19
	{
		bus_width_switch(0x2, portId); //set bus_width to 8bit_SDR
		status = mmc_Cmd19_Bustest_W(portId);

		glMscState = CY_FX_MSC_STATE_STATUS;

	}
		break;

	case CY_U3P_MMC_CMD21_SEND_TUNING_BLOCK: //CMD 21
	{
		status = mmc_Cmd21_Tunning_Block(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;

	}
		break;
	case CY_U3P_MMC_CMD23_SET_BLOCK_COUNT: //CMD 23
	{
		status = mmc_Cmd23_Set_Block_Count(arg1, portId);
		//SendRespToUsb();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_MMC_CMD26_PROGRAM_CID: //CMD26
	{
		programCID_CSD(CY_U3P_MMC_CMD26_PROGRAM_CID);
		//mmc_Cmd26_Program_Cid();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD27_PROGRAM_CSD: //CMD27
	{
		programCID_CSD(CY_U3P_SD_MMC_CMD27_PROGRAM_CSD);
		//mmc_Cmd27_Program_Csd();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD28_SET_WRITE_PROT: //CMD28
	{
		mmc_Cmd28_Set_Write_Prot(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD29_CLR_WRITE_PROT: //CMD29
	{
		mmc_Cmd29_Clr_Write_Prot(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD30_SEND_WRITE_PROT: //CMD30
	{
		mmc_Cmd30_Send_Write_Prot(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD31_SEND_WRITE_PROT_TYPE: //CMD31
	{
		mmc_Cmd31_Send_Write_Prot_Type(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_CMD32_ERASE_WR_BLK_START: // SD CMD32
	{
		sd_Cmd32_Erase_Wr_Blk_Start(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_CMD33_ERASE_WR_BLK_END: //SD CMD33
	{
		sd_Cmd33_Erase_Wr_Blk_End(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_MMC_CMD35_ERASE_GROUP_START: //CMD 35
	{
		mmc_Cmd35_Erase_Group_Start(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_MMC_CMD36_ERASE_GROUP_END: //CMD 36
	{
		mmc_Cmd36_Erase_Group_End(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD38_ERASE: //CMD 38
	{
		mmc_Cmd38_Erase(arg1, portId);

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_ACMD41_SD_SEND_OP_COND: //SD CMD 41
	{
		SD_Acmd41_SD_Send_Op_Cond(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD42_LOCK_UNLOCK: //CMD 42
	{
		SD_Acmd41_SD_Send_Op_Cond(arg1, portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD55_APP_CMD: //CMD 55
	{
		mmc_cmd55_App_Cmd(portId);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case CY_U3P_SD_MMC_CMD60_RW_MULTIPLE_REGISTER: //CMD 60
	{
		status = mmc_Cmd60(arg1, portId);

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 66: //CMD 0x42, for device
	{
		bus_width_switch(arg1, portId);
		returnErorToUSB();
		//glMscState = CY_FX_MSC_STATE_STATUS;
		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;
	case 67: //Device clock 0x43
	{
		set_Mmc_Clk(arg1, portId);
		returnErorToUSB();
//		glMscState = CY_FX_MSC_STATE_STATUS;
		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;
	case 68: //control GPIO 0x44
	{
		ctrlGPIO(arg1, arg2);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 69: //mask data3 detect 0x45
	{
		uint32_t *cardDet = (uint32_t*) SDMMC_INTR_MASK;
		if (arg1)
			*cardDet &= ~(1 << 10);
		else
			*cardDet |= (1 << 10);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 70: //0x046
	{
		mmc_Cmd0_Go_Idle(0xf0f0f0f0, portId);
		CyU3PThreadSleep(1);
		mmc_Cmd60(0xff, portId); //60
		set_Mmc_Clk(1, portId); //400k
		mmc_cmd1_rdy(0x40ff8080, portId);
		mmc_Cmd2_All_Send_CID(portId);
		mmc_Cmd3_Set_Relative_Addr(RCA, portId);
		set_Mmc_Clk(6, portId); //SET_CLK to 48MHZ
		mmc_Cmd7_Select_Deselect_Card(RCA, portId);
		check_Ready(portId);
		mmc_Cmd6_Switch(0x03b90100, portId);
		check_Ready(portId);
		mmc_Cmd6_Switch(0x03b70200, portId);
		check_Ready(portId);
		bus_width_switch(0x2, portId);
		mmc_Cmd60(0x55, portId); //60
		check_Ready(portId);
		uint8_t extCsdBuff[512];
		mmc_Cmd8_Send_Extcsd(extCsdBuff, portId, 0);
		check_Ready(portId);

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 71: //0x47
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyFalse, CyFalse, 0,
				&dataLength))
		{
			return;
		}
		readRegisterValue();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 72: //0x48
	{
		initToTranStat();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 73: //0x049
	{
		CyU3PDmaChannelReset(&glChHandleMscIn);
		returnErorToUSB();
		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;
	case 74: //0x4a
	{
		CyU3PDmaChannelReset(&glChHandleMscIn);
		CyU3PDebugPrint(4, "\r SendRespToUsb\n");
		SendRespToUsb(portId);
		//status = CY_U3P_SUCCESS;//???
		glMscState = CY_FX_MSC_STATE_DATA;
	}
		break;
	case 75: //0x4b : CLK enable/disable
	{
		clkEnable(arg1, portId);
		returnErorToUSB();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
//	case 76: //0x4c
//	{
//		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyTrue, CyTrue, 512,
//				&dataLength))
//		{
//			return;
//		}
//
//		loadPortNumber();
//		glMscState = CY_FX_MSC_STATE_STATUS;
//	}
//		break;
	case 77: //0x4d
	{
		if (!CyFxMscApplnCheckCbwParams(0, CyFalse, CyFalse, CyFalse, 0,
				&dataLength))
		{
			return;
		}
		av_write(glMscCbwBuffer);
		glMscState = CY_FX_MSC_STATE_STATUS;

	}
		break;
	case 78: //0x4e
	{
		cmdLineLow();
		returnErorToUSB(); //not real error,just send a message to host
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 79: //0x4f
	{
		CyU3PDeviceGpioRestore(41);
		returnErorToUSB();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 80: //0x50
	{
		toggleData0();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 81: //0x51
	{
		CyU3PSibStart();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 82: //0x52
	{
		CyU3PSibStop();
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 83: //0x53
	{
		checkCardDetect();
	}
		break;
	case 84: //0x54
	{

		uint32_t *dllCtrl = (uint32_t*) SDMMC_DLL_CTRL;
//		uint32_t *modeCfg = (uint32_t *) SDMMC_MODE_CFG;
		uint32_t *dataCfgPtr = (uint32_t *) PORTADDRESS(SDMMC_DATA_CFG, 0);
		*dataCfgPtr &= 0;
		CyU3PGpioSimpleSetValue(45, 0);
		clkEnable(CLKDISABLE, portId);
		CyU3PBusyWait(100);

		*dllCtrl |= 1;
		*dllCtrl &= ~(0xf << 3);
		*dllCtrl |= (arg1 << 3);

		CyU3PBusyWait(100);
		clkEnable(CLKENABLE, portId);
		CyU3PDebugPrint(4, "\r dllCtrl	%x\n", *dllCtrl);
//		int i;
//		for(i=1;i<=16;i++)
//		{
//			CyU3PDebugPrint(4,"\r --------step %d------\n\n",i-1);
//
//			status = mmc_Cmd21_Tunning_Block(arg1,portId);
//			CyU3PDebugPrint(4, "\r dllCtrl	%x\n", *dllCtrl);
//			if(status != 0){
//
//				initToTranStat();
//				clkEnable(CLKDISABLE,portId);
//				CyU3PBusyWait(100);
//				*dllCtrl &= ~(0xf<<3);
//				*dllCtrl |= (i<<3);
//				CyU3PBusyWait(100);
//				clkEnable(CLKENABLE,portId);
//
//			}else{
//				CyU3PDebugPrint(4,"\r tuning done\n");
//				break;
//			}
//		}

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 100: //0x64
	{
		DSAppDefine(); //create a new thread.
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 101: //0x65
	{
		DSPinThreadEntySuspend(arg1);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 102: //0x66
	{
		writeToina231(0x95, 0x04, 0x55, 0x55);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 103: //0x67
	{
		configComplexGPIO(26); // Configure GPIO 26 as input for count mode

		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 104: //0x68
	{
		uint32_t threshold = 0;
		CyU3PGpioComplexSampleNow(26, &threshold); //show counts
		CyU3PDebugPrint(8, "\r %d \n", threshold);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;

	case 105: //0x69
	{
		setSCLConfig();
		setSDAConfig(1);
		pushPullSDA(1);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 106: //0x6a
	{
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 107: //0x6b
	{
		CyU3PDmaBuffer_t dmaBuf;
		CyU3PReturnStatus_t status;

		/* Prepare the DMA Buffer */
		dmaBuf.buffer = glMscDataBuffer;
		dmaBuf.status = 0;
		dmaBuf.size = 16; /* Round up to a multiple of 16.  */
		dmaBuf.count = 1;
		status = CyU3PDmaChannelSetupRecvBuffer(&glChHandleUARTtoCPU, &dmaBuf);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDmaChannelReset(&glChHandleUARTtoCPU);
			CyU3PDebugPrint(4, "\CyU3PDmaChannelSetupRecvBuffer fail%x\n",
					status);
		}
//		CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
//		uint8_t rxTxByte = 0x5a, actualCount = 0;
//		actualCount = CyU3PUartTransmitBytes (&rxTxByte, 1, &apiRetStatus);
//		if (actualCount != 0)
//		{
//			/* Check status */
//			if (apiRetStatus != CY_U3P_SUCCESS)
//			{
//				CyU3PDebugPrint(4,"\rCyU3PUartTransmitBytes fail, %x\n",actualCount);
//				/* Error handling */
//				CyFxAppErrorHandler(apiRetStatus);
//			}
//		}
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 253: //0xfd
	{
//			uint32_t *SIBSCK_ADDRESS ;
//			int i;
//			for(i = 0;i<2;i++)
//			{
//				SIBSCK_ADDRESS = (uint32_t *)(0xe0028000+i*0x80);
//				CyU3PDebugPrint(4,"\r =====SCK %x=====\n",i);
//				CyU3PDebugPrint(4,"\r SIBSCK_ADDRESS %x\n",SIBSCK_ADDRESS);
//				CyU3PDebugPrint(4,"\r SCK_DSCR %x\n",*SIBSCK_ADDRESS);
//				CyU3PDebugPrint(4,"\r SCK_SIZE %x\n",*(SIBSCK_ADDRESS+1));
//				CyU3PDebugPrint(4,"\r SCK_COUNT %x\n",*(SIBSCK_ADDRESS+2));
//				CyU3PDebugPrint(4,"\r SCK_STATUS %x\n",*(SIBSCK_ADDRESS+3));
//				CyU3PDebugPrint(4,"\r SCK_INTR %x\n",*(SIBSCK_ADDRESS+4));
//				CyU3PDebugPrint(4,"\r DSCR_BUFFER %x\n",*(SIBSCK_ADDRESS+8));
//				CyU3PDebugPrint(4,"\r DSCR_SYNC %x\n",*(SIBSCK_ADDRESS+9));
//				CyU3PDebugPrint(4,"\r DSCR_CHAIN %x\n",*(SIBSCK_ADDRESS+10));
//				CyU3PDebugPrint(4,"\r DSCR_SIZE %x\n",*(SIBSCK_ADDRESS+11));
//				CyU3PDebugPrint(4,"\r EVENT %x\n",*(SIBSCK_ADDRESS+30));
//
//			}
		/*uint32_t *DSCR_ADDRESS= (uint32_t*)0x40000010;
		 for(i = 0;i<5;i++)
		 {
		 CyU3PDebugPrint(4, "\r =====DSCR %x=====\n", i);
		 CyU3PDebugPrint(4,"\r DSCR_ADDRESS %x\n",DSCR_ADDRESS);
		 CyU3PDebugPrint(4, "\r DSCR_BUFFER %x\n", *DSCR_ADDRESS++);
		 CyU3PDebugPrint(4, "\r DSCR_SYNC %x\n", *(DSCR_ADDRESS++));
		 CyU3PDebugPrint(4, "\r DSCR_CHAIN %x\n", *(DSCR_ADDRESS++));
		 CyU3PDebugPrint(4, "\r DSCR_SIZE %x\n", *(DSCR_ADDRESS++));
		 //DSCR_ADDRESS += 0x10;
		 }*/
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	case 254: //0xfe
	{
		switchFAorMSCMode(arg1);
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
//	case 255: //0xff
//		{//return FW version
//			CyU3PMemSet(glMscDataBuffer, 0, 512);
//			glMscDataBuffer[0] = AU_MP_FW_VER[0];
//			glMscDataBuffer[1] = AU_MP_FW_VER[1];
//			glMscDataBuffer[2] = AU_MP_FW_VER[2];
//			glMscDataBuffer[3] = AU_MP_FW_VER[3];
//			glMscDataBuffer[4] = AU_MP_FW_VER[4];
//			glMscDataBuffer[5] = AU_MP_FW_VER[5];
//			glMscDataBuffer[6] = AU_MP_FW_VER[6];
//			glMscDataBuffer[7] = AU_MP_FW_VER[7];
//			SendDataToUsb(glMscDataBuffer, 512);
//		}
//			break;
	default:
	{
		glMscState = CY_FX_MSC_STATE_STATUS;
	}
		break;
	}


	if ((mmc_cmd >= 0) && (mmc_cmd <= 60))
	{
		if ((mmc_cmd != 8) && (mmc_cmd != 14) &&(mmc_cmd != 18) && (mmc_cmd != 25) && (mmc_cmd != 26) && (mmc_cmd != 27) &&(mmc_cmd != 17) && (mmc_cmd != 24))
		{
			if(status == CY_U3P_SUCCESS)
				SendRespToUsb(portId);
			else
				returnErorToUSB();
		}
	}

	//glPrevCmd = mmc_cmd;

}
/*
int mmc_read_block( uint8_t drv, uint8_t *buff, uint32_t addr,uint8_t count)
{
	CyU3PReturnStatus_t status;
	//uint16_t numBlks;
	uint8_t respLen = 56; //bit
	uint8_t respdata[7]; //byte
	CyU3PDmaBuffer_t BufIn_t;
	//uint8_t count = 3;

	BufIn_t.buffer = (uint8_t*)buff;
	BufIn_t.count = 0;
	BufIn_t.size = 512*count;
	BufIn_t.status = 0;

	//cmdArg = 0x400;
	//numBlks = 0x3;

	status = CyU3PDmaChannelSetupRecvBuffer(&glChHandleStorIn, &BufIn_t);
	if (status == CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4,"\r Send Read Cmd\n");
		status = CyU3PSibVendorAccess((uint8_t)drv, 18, (uint32_t) addr, respLen, respdata, count, 1, 3);
		if(status == CY_U3P_SUCCESS )
		{
			CyU3PDebugPrint(8, "\r\n Send Read Cmd ok\n");
			uint32_t i = 0;
			//for(i=0;i<1024;i+=8)
				CyU3PDebugPrint(8, "\r \n",buff[i],buff[i+1],buff[i+2],buff[i+3],buff[i+4],buff[i+5],buff[i+6],buff[i+7]);
		}
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(8, "\r\n Send Read Cmd\n");

			CyU3PDmaChannelReset(&glChHandleStorIn);
			return 1;
		}
	}else{
		CyU3PDebugPrint(4,"status = %x\n",status);
		return 1;
	}
	return 0;
}

void av_read(uint8_t *glMscCbwBuffer)
{

	CyU3PReturnStatus_t status;
	uint32_t arg;
	uint8_t numBlks;
	uint8_t *buff;

	arg = (((uint32_t) glMscCbwBuffer[17] << 24)
			| ((uint32_t) glMscCbwBuffer[18] << 16)
			| ((uint32_t) glMscCbwBuffer[19] << 8)
			| ((uint32_t) glMscCbwBuffer[20]));
	numBlks = (((uint8_t)glMscCbwBuffer[22] << 8) | ((uint8_t)glMscCbwBuffer[23]));

	buff= CyU3PDmaBufferAlloc(512*numBlks);

	if(buff==0)
	{
		CyU3PDebugPrint (4, "\rFailed to allocate  buffer\r\n");
		return ;
	}
	CyU3PMemSet (buff, 0, 512*numBlks);
	status = mmc_read_block(0,buff,arg,(uint8_t)numBlks);
	if (status == CyTrue)
	{
		uint32_t i;
		for (i = 0; i < 512 * numBlks; i += 16)
			CyU3PDebugPrint(8,
					"\r 0x%x : %x %x %x %x %x %x %x %x %x  %x %x %x %x %x %x %x\n",
					i, buff[i], buff[i + 1], buff[i + 2], buff[i + 3],
					buff[i + 4], buff[i + 5], buff[i + 6], buff[i + 7],
					buff[i + 8], buff[i + 9], buff[i + 10], buff[i + 11],
					buff[i + 12], buff[i + 13], buff[i + 14], buff[i + 15]);
	}
	if(status != CyTrue)
	{
		CyU3PDmaBufferFree(buff);
		return ;
	}
	CyU3PDmaBufferFree(buff);
}
*/
int mmc_write_block(uint8_t *wbuff, uint32_t addr, uint8_t numblks)
{
	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t BufOut;
	BufOut.buffer = (uint8_t*) wbuff;
	BufOut.count = (uint16_t) numblks * 512;
	BufOut.size = (uint16_t) numblks * 512;
	BufOut.status = 0;


	//status = CyU3PDmaChannelSetupSendBuffer(&glChHandleStorOut, &BufOut);
//	if (status == CY_U3P_SUCCESS)
//	{
		//status = CyU3PSibVendorAccess(0, 25, addr, CY_U3P_MMC_R1B_RESP_BITS, respdata, numblks,0, 4);
		status = CyU3PCardMgrSetupWrite(0,0,4,numblks,addr); // if use this API, "glMscState" need set to "CY_FX_MSC_STATE_DATA";
		if (status == CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(8, "\r Send Write Cmd ok\n");
			status = CyU3PDmaChannelSetupSendBuffer(&glChHandleStorOut, &BufOut);
			if (status != CY_U3P_SUCCESS)
				{
					CyU3PDebugPrint(4, "CyU3PDmaChannelSetupSendBuffer fail status = %x\n",status);
				}

		}
		else{
			CyU3PDebugPrint(4, "\r Send Write cmd fail %x\n", status);
			return CyFalse;
		}
//	}
//	else{
//		CyU3PDebugPrint(4, "CyU3PDmaChannelSetupSendBuffer fail status = %x\n",status);
//		return CyFalse;
//	}
		status = CyU3PDmaChannelSetupSendBuffer(&glChHandleStorOut, &BufOut);
								if (status != CY_U3P_SUCCESS)
									{
										CyU3PDebugPrint(4, "CyU3PDmaChannelSetupSendBuffer fail status = %x\n",status);
									}
	return CyTrue;
}

void av_write(uint8_t *glMscCbwBuffer)
{

	CyU3PReturnStatus_t status;
	uint32_t arg;
	uint8_t numBlks;

	uint8_t *buff;

	arg = (((uint32_t) glMscCbwBuffer[17] << 24)
			| ((uint32_t) glMscCbwBuffer[18] << 16)
			| ((uint32_t) glMscCbwBuffer[19] << 8)
			| ((uint32_t) glMscCbwBuffer[20]));
	numBlks = (((uint8_t)glMscCbwBuffer[22] << 8) | ((uint8_t)glMscCbwBuffer[23]));
	//max numBlks is 0x70
	if(numBlks==0)
	{
		return;
	}
	buff= CyU3PDmaBufferAlloc(512*numBlks);
	if(buff==0)
	{
		CyU3PDebugPrint (4, "\rFailed to allocate  buffer\r\n");
		return ;
	}

	CyU3PMemSet (buff, glMscCbwBuffer[16], 512*numBlks);
	status = mmc_write_block(buff,arg,numBlks);

	if(status != CyTrue)
	{
		CyU3PDmaBufferFree(buff);
		return ;
	}
	CyU3PDmaBufferFree(buff);
}

