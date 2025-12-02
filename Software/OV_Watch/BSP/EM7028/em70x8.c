#include "em70x8.h"

#define CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE();
iic_bus_t EM7028_bus = 
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

static uint8_t EM7028_WaitReady(void)
{
	uint8_t i = 5;

	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}

	return (i == 0U);
}

uint8_t  EM7028_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	dat = IIC_Read_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr);
	return dat;
}

void  EM7028_WriteOneReg(unsigned char RegAddr, unsigned char dat)
{
	IIC_Write_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr, dat);
}

uint8_t EM7028_Get_ID()
{
	return EM7028_ReadOneReg(ID_REG);
}

uint8_t EM7028_hrs_init()
{
	CLK_ENABLE;
	IICInit(&EM7028_bus);

	if(EM7028_WaitReady())
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);				
	EM7028_WriteOneReg(HRS2_DATA_OFFSET, 0x00);
	EM7028_WriteOneReg(HRS2_CTRL, 0x40);
	EM7028_WriteOneReg(HRS2_GAIN_CTRL, 0x7f);		
	EM7028_WriteOneReg(HRS1_CTRL, 0x47);
	EM7028_WriteOneReg(INT_CTRL, 0x00);
	return 0;
}

uint8_t EM7028_hrs_Enable()
{
	return EM7028_hrs_EnableContinuous();
}

uint8_t EM7028_hrs_EnableContinuous(void)
{
	if(EM7028_WaitReady())
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x08);
	return 0;
}

uint8_t EM7028_hrs_EnablePulse(void)
{
	if(EM7028_WaitReady())
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x80);
	return 0;
}

uint8_t EM7028_hrs_DisEnable()
{
	if(EM7028_WaitReady())
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);
	return 0;
}

uint16_t EM7028_Get_HRS1(void)
{
	uint16_t dat;
	dat = EM7028_ReadOneReg(HRS1_DATA0_H);
	dat <<= 8;
	dat |= EM7028_ReadOneReg(HRS1_DATA0_L);
	return dat;
}

uint16_t EM7028_Get_HRS2(void)
{
	uint16_t dat;

	dat = EM7028_ReadOneReg(HRS2_DATA0_H);
	dat <<= 8;
	dat |= EM7028_ReadOneReg(HRS2_DATA0_L);

	return dat;
}
