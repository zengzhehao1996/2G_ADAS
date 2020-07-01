/**	
 * |----------------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2014
 * | 
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
#include "tm_stm32f4_otp.h"
#include "stm32f4xx_hal_flash.h"
//#include "tm_stm32f4_disco.h"
#include "my_misc.h"

TM_OTP_Result_t hardOtpWriteByte(uint8_t block, uint8_t byte, uint8_t data)
{
	FLASH_Status status;
	uint8_t checkvalue;
	/* Check input parameters */
	if (
		block >= OTP_BLOCKS ||
		byte >= OTP_BYTES_IN_BLOCK
	) {
		/* Invalid parameters */
		
		/* Return error */
		return TM_OTP_Result_Error;
	}
	
	/* Unlock FLASH */
	HAL_FLASH_Unlock();

	/* Clear pending flags (if any) */  
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
	
	/* Wait for last operation */
	status = FLASH_WaitForLastOperation(50000);
	
	/* If it is not success, return error */
	if (status != HAL_OK){
		/* Lock FLASH */
		HAL_FLASH_Lock();
		
		/* Return error */
		return TM_OTP_Result_Error;
	}

	/* Write byte */
	//status = FLASH_Program_Byte(OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + byte, data);
    status = HAL_FLASH_Program(0,OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + byte, data);
	/* Lock FLASH */
	HAL_FLASH_Lock();
    checkvalue = hardOtpReadByte(block, byte);
    if(checkvalue == data)
    {
        print_log("write block:%d byte:%d data:0x%2x  ok .....vvvvvvvvvvvvvvvvvv\n",block,byte,data);
        return TM_OTP_Result_Ok;
    }
    else
    {
        print_log("write block:%d byte:%d data:0x%2x fail...read:0x%2x xxxxxxxxxxxxxxxxx\n",block,byte,data,checkvalue);
        return TM_OTP_Result_Error;
    }
}

/*******
TM_OTP_Result_t TM_OTP_Write_Version(uint8_t size,char* versioninfo)
{
    for(int i=0;i<size;i++)
    {
        printk(" 0x%2x",versioninfo[i]);
        if(TM_OTP_Write(1,i,versioninfo[i]) != TM_OTP_Result_Ok) 
        {
            return TM_OTP_Result_Error;
        }
        
    }
    printk("]\n");
    return TM_OTP_Result_Ok;
}
*******/

TM_OTP_Result_t hardInfoOtpWrite(uint8_t block,uint8_t position,uint8_t length,char *versioninfo)
{
   
    if(OTP_BYTES_IN_BLOCK <= position+length)
    {
        return TM_OTP_Result_Error;
    }

  
    for(uint8_t i=0;i<length;i++)
    {
        if(hardOtpWriteByte(block,i+position,versioninfo[i]) != TM_OTP_Result_Ok) 
        {
            return TM_OTP_Result_Error;
        }    
    }
    if(hardOtpWriteByte(block,length+position,'\0') != TM_OTP_Result_Ok) 
    {
        return TM_OTP_Result_Error;
    }     
    return TM_OTP_Result_Ok;

}



TM_OTP_Result_t hardVersionOtpWrite(uint8_t block,uint8_t length,uint8_t major,uint8_t minor,uint8_t tiny,uint8_t patch)
{
    if(32 <= length)
    {
        return TM_OTP_Result_Error;
    }

    
    if(hardOtpWriteByte(block,0,major) != TM_OTP_Result_Ok) 
    {
        return TM_OTP_Result_Error;
    }  
    

    if(hardOtpWriteByte(block,1,minor) != TM_OTP_Result_Ok) 
    {
        return TM_OTP_Result_Error;
    }  

    
    if(hardOtpWriteByte(block,2,tiny) != TM_OTP_Result_Ok) 
    {
        return TM_OTP_Result_Error;
    } 
    

    if(hardOtpWriteByte(block,3,patch) != TM_OTP_Result_Ok) 
    {
        return TM_OTP_Result_Error;
    } 

    return  TM_OTP_Result_Ok;

}



TM_OTP_Result_t hardOtpReadVersion(uint8_t block,uint8_t *major,uint8_t *minor,uint8_t *tiny,uint8_t *patch)
{
    uint8_t value = 0;

    
    value = hardOtpReadByte(block,0);
    if(0xFF == value)
    {
        return TM_OTP_Result_Error;
    }
    else
    {
        *major = value;
    }

    value = hardOtpReadByte(block,1);
    if(0xFF == value)
    {
        return TM_OTP_Result_Error;
    }
    else
    {
        *minor = value;
    }

    value = hardOtpReadByte(block,2);
    if(0xFF == value)
    {
        return TM_OTP_Result_Error;
    }
    else
    {
        *tiny = value;
    }    

    value = hardOtpReadByte(block,3);
    if(0xFF == value)
    {
        return TM_OTP_Result_Error;
    }
    else
    {
        *patch = value;
    }

    return TM_OTP_Result_Ok;
}



 uint8_t hardOtpReadInfo(uint8_t block,uint8_t position,char *info)
{
    uint8_t length =0;
    uint8_t value=0;
    while(position+length<OTP_BYTES_IN_BLOCK)
    {
        value = hardOtpReadByte(block,position+length);
        if(value == 0xFF)
        {
            return length;
        }
        else
        {
            info[length]=value;
            length++;
        }
    }
    return length;


}


uint8_t hardOtpReadByte(uint8_t block, uint8_t byte) 
{
	uint8_t data;
	
	/* Check input parameters */
	if (block >= OTP_BLOCKS || byte >= OTP_BYTES_IN_BLOCK) 
	{
		/* Invalid parameters */
		return 0xff;
	}
	
	/* Get value */
	data = *(__IO uint8_t *)(OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + byte);
	/* Return data */
	return data;
}


TM_OTP_Result_t hardOtpBlockLock(uint8_t block) {
	FLASH_Status status;
    uint8_t checkvalue = 11;
	
	/* Check input parameters */
	if (block >= OTP_BLOCKS) {
		/* Invalid parameters */
		
		/* Return error */
		return TM_OTP_Result_Error;
	}
	
	/* Unlock FLASH */
	HAL_FLASH_Unlock();

	/* Clear pending flags (if any) */  
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
	
	/* Wait for last operation */
	status = FLASH_WaitForLastOperation(50000);
	
	/* If it is not success, return error */
	if (status != HAL_OK) {
		/* Lock FLASH */
		HAL_FLASH_Lock();
		
		/* Return error */
		return TM_OTP_Result_Error;
	}
	
	/* Write byte */
	//status = FLASH_Program_Byte(OTP_LOCK_ADDR + block, 0x00);
	status = HAL_FLASH_Program(0,OTP_LOCK_ADDR + block, 0x00);

	/* Lock FLASH */
	HAL_FLASH_Lock();

    checkvalue = *(__IO uint8_t *)(OTP_LOCK_ADDR + block);
    if(checkvalue == 0x00)
    {
        print_log("block lock:%d ..ok  VVVVVVVVVVVVV\n",block);
        return TM_OTP_Result_Ok;
    }
    else
    {
        print_log("block lock:%d  fail....value:%d XXXXXXXXX\n",block,checkvalue);
        return TM_OTP_Result_Error;        
    }

}



