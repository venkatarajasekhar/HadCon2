/*
 * spiApi.c
 *
 *  Created on: 22.05.2013
 *      Author: Peter Zumbruch, GSI, P.Zumbruch@gsi.de
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_global.h"
#include "api_define.h"
#include "api.h"
#include "spiApi.h"
#include "spi.h"

static const char filename[] PROGMEM = __FILE__;

#warning TODO: optimize memory management of big data array
spiByteDataArray spiWriteData;
static char byte[3]= "00";


static const char spiCommandKeyword00[] PROGMEM = "status"; 			 /* show spi status of control bits and operation settings*/
static const char spiCommandKeyword01[] PROGMEM = "write"; 				 /* write <args>, directly setting automatically CS */
static const char spiCommandKeyword02[] PROGMEM = "add"; 				 /* add <args> to write buffer*/
static const char spiCommandKeyword03[] PROGMEM = "transmit"; 			 /* transmit write buffer auto set of CS controlled via cs_auto_enable*/
static const char spiCommandKeyword04[] PROGMEM = "read"; 				 /* read read Buffer*/
static const char spiCommandKeyword05[] PROGMEM = "cs"; 				 /* set chip select: cs <0|1|H(IGH)|L(OW)|T(RUE)|F(ALSE)>, CS is low active */
static const char spiCommandKeyword06[] PROGMEM = "cs_bar"; 			 /* same as cs but inverse logic*/
static const char spiCommandKeyword07[] PROGMEM = "cs_set"; 		     /* releases cs, optionally only for <cs mask>*/
static const char spiCommandKeyword08[] PROGMEM = "cs_release"; 		 /* completing byte by zero at the end or the beginning, due to odd hex digit*/
static const char spiCommandKeyword09[] PROGMEM = "cs_select_mask";		 /* chip select output pin byte mask */
static const char spiCommandKeyword10[] PROGMEM = "cs_pins"; 			 /* set hardware addresses of multiple CS outputs*/
static const char spiCommandKeyword11[] PROGMEM = "cs_auto_enable"; 	 /* cs_auto_enable [<a(ll)|w(rite)|t(ransmit)> <0|1 ..>: enable/disable automatic setting for write and transmit actions*/
static const char spiCommandKeyword12[] PROGMEM = "purge"; 				 /* purge write/read buffers*/
static const char spiCommandKeyword13[] PROGMEM = "purge_write_buffer";  /* purge write buffer*/
static const char spiCommandKeyword14[] PROGMEM = "purge_read_buffer";   /* purge read buffer*/
static const char spiCommandKeyword15[] PROGMEM = "show_write_buffer"; 	 /* show content of write buffer, detailed when increasing DEBG level */
static const char spiCommandKeyword16[] PROGMEM = "show_read_buffer"; 	 /* show content of read buffer, detailed when increasing DEBG level */
static const char spiCommandKeyword17[] PROGMEM = "control_bits"; 		 /* get/set SPI control bits*/
static const char spiCommandKeyword18[] PROGMEM = "spi_enable"; 		 /* get/set enable SPI*/
static const char spiCommandKeyword19[] PROGMEM = "data_order"; 		 /* get/set bit endianess*/
static const char spiCommandKeyword20[] PROGMEM = "master"; 			 /* get/set master mode*/
static const char spiCommandKeyword21[] PROGMEM = "clock_polarity"; 	 /* get/set clock polarity*/
static const char spiCommandKeyword22[] PROGMEM = "clock_phase"; 		 /* get/set clock phase*/
static const char spiCommandKeyword23[] PROGMEM = "speed"; 				 /* get/set speed */
static const char spiCommandKeyword24[] PROGMEM = "speed_divider"; 		 /* get/set speed divider*/
static const char spiCommandKeyword25[] PROGMEM = "double_speed"; 		 /* get/set double speed*/
static const char spiCommandKeyword26[] PROGMEM = "transmit_byte_order"; /* MSB/LSB, big endian */
static const char spiCommandKeyword27[] PROGMEM = "complete_byte"; 		 /* completing byte by zero at the end or the beginning, due to odd hex digit*/

const char* spiCommandKeywords[] PROGMEM = {
        spiCommandKeyword00, spiCommandKeyword01, spiCommandKeyword02, spiCommandKeyword03, spiCommandKeyword04,
		spiCommandKeyword05, spiCommandKeyword06, spiCommandKeyword07, spiCommandKeyword08, spiCommandKeyword09,
		spiCommandKeyword10, spiCommandKeyword11, spiCommandKeyword12, spiCommandKeyword13, spiCommandKeyword14,
		spiCommandKeyword15, spiCommandKeyword16, spiCommandKeyword17, spiCommandKeyword18, spiCommandKeyword19,
		spiCommandKeyword20, spiCommandKeyword21, spiCommandKeyword22, spiCommandKeyword23, spiCommandKeyword24,
		spiCommandKeyword25, spiCommandKeyword26, spiCommandKeyword27  };


void spiApi(struct uartStruct *ptr_uartStruct)
{
	switch(ptr_uartStruct->number_of_arguments)
	{
		case 0:
#warning TODO: show status ???			/*status*/

			break;
		default:
			if (isNumericArgument(setParameter[1], MAX_LENGTH_PARAMETER)) /*write*/
			{
				/*uart and uart_remainder to spi data */

				if ( 0 < spiApiFillWriteArray(ptr_uartStruct, 1) )
				{
					//spiWrite(spiWriteData,temp.length);
				}
			}
			else /*sub command*/
			{
				// find matching command keyword: -1
				spiApiSubCommands(ptr_uartStruct, -1);
			}
			break;
	}
	return;
}


size_t spiApiFillWriteArray(struct uartStruct *ptr_uartStruct, uint16_t parameterStartIndex)
{
	/* reset array length*/
	spiWriteData.length = 0;

	spiApiAddToWriteArray(ptr_uartStruct, parameterStartIndex);

	spiApiShowWriteBufferContent();
	return spiWriteData.length;
}

size_t spiApiAddToWriteArray(struct uartStruct *ptr_uartStruct, uint16_t argumentIndex)
{
	int8_t result;

    while( spiWriteData.length < sizeof(spiWriteData.data) -1 && (int) argumentIndex <= ptr_uartStruct->number_of_arguments )
    {
    	if ( argumentIndex < MAX_PARAMETER)
    	{
    		printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*)  ( filename ),
    				PSTR("setParameter[%i]: \"%s\""), argumentIndex, setParameter[argumentIndex]);

    		/* get contents from parameter container */
    		/* spiWriteData should be increased */
    		result = spiAddNumericParameterToByteArray(NULL, &spiWriteData, argumentIndex);

    		if ( 0 > result )
    		{
    			break;
    		}
    	}
    	else
    	{
    		/* get contents from remainder container */

    		result = spiAddNumericParameterToByteArray(&resultString[0], &spiWriteData, -1);
    		if ( 0 > result )
    		{
    			break;
    		}
    	}
  		printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*)  ( filename ),
    				PSTR("------------------------"));
  		argumentIndex++;
    }

    return spiWriteData.length;

}//END of function

int8_t spiAddNumericParameterToByteArray(const char string[], spiByteDataArray* data, uint8_t index)
{
	int8_t result;
	printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("para to byte array"));

	if ( NULL == data)
	{
		CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
		return -1;
	}

	/* get string string[] */
	if ( 0 > index || MAX_PARAMETER <= index  )
	{
		if ( NULL == string)
		{
			CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
			return -1;
		}
		result = spiAddNumericStringToByteArray( string , data );
		if (-1 == result)
		{
			return -1;
		}
	}
	/* get string from setParameter at the index */
	else
	{
		printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("para string to byte array"));
		result = spiAddNumericStringToByteArray( setParameter[index] , data );
		if (-1 == result)
		{
			return -1;
		}
	}
	return 0;
}

/*
 * int8_t spiAddNumericStringToByteArray(const char string[], spiByteDataArray* data)
 *
 *
 */

int8_t spiAddNumericStringToByteArray(const char string[], spiByteDataArray* data)
{
	uint64_t value;
	size_t numberOfDigits;

	if ( NULL == data)
	{
		CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
		return -1;
	}

	if ( NULL == string)
	{
		CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
		return -1;
	}

	if ( isNumericArgument(string, MAX_LENGTH_COMMAND - MAX_LENGTH_KEYWORD - 2))
	{
		/* get number of digits*/
		numberOfDigits = getNumberOfHexDigits(string, MAX_LENGTH_COMMAND - MAX_LENGTH_KEYWORD - 2 );

		/* check if new element, split into bytes, MSB to LSB,
		 * to be added would exceed the maximum size of data array */

		size_t numberOfBytes = ((numberOfDigits + numberOfDigits%2) >> 1);
		printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("no. bytes/digits %i %i"), numberOfBytes, numberOfDigits);

		if ( data->length + numberOfBytes < sizeof(data->data) -1 )
		{
			if ( 16 >= numberOfDigits ) /*64 bit*/
			{
				if ( 0 == getUnsignedNumericValueFromParameterString(string, &value) )
				{
					printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ),
								PSTR("value %#08lx%08lx"), value >> 32, value && 0xFFFFFFFF);
					/* byte order: MSB to LSB / big endian*/
					for (int8_t byteIndex = numberOfBytes - 1; byteIndex >= 0; byteIndex--)
					{
						/* 0x0 					.. 0xFF*/
						/* 0x000 				.. 0xFFFF*/
						/* 0x00000 				.. 0xFFFFFF*/
						/* 0x0000000 			.. 0xFFFFFFFF*/
						/* 0x000000000	 		.. 0xFFFFFFFFFF*/
						/* 0x00000000000		.. 0xFFFFFFFFFFFF*/
						/* 0x0000000000000		.. 0xFFFFFFFFFFFFFF*/
						/* 0x000000000000000	.. 0xFFFFFFFFFFFFFFFF*/
						/* ... */
#warning TODO add case add leading 0 at end for odd number of digits
						data->data[data->length] = 0xFF & (value >> (8 * byteIndex));
						data->length++;
						printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ),
								PSTR("data[%i]=%x"), data->length - 1, data->data[data->length -1]);
					}
				}
				else
				{
					CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, TRUE, PSTR("%s"), string);
					return -1;
				}
			} /* > 64 bit*/
			else
			{
				printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR(">64 bit"));

				clearString(byte, sizeof(byte));
				strcat_P(byte, PSTR("00"));

				printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("init byte '%s'"), byte);
				size_t charIndex = 0;

				/* in case of odd number of digits*/
#warning TODO add case add leading 0 at end for odd number of digits
				/* add leading 0 in front
				 */
				{
					if (numberOfDigits%2 )
					{
						printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("odd"));
						byte[0]='0';
						byte[1]=string[charIndex];
						charIndex++;
					}
					else
					{
						printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("even"));
						charIndex = 0;
					}
					while (charIndex + 1 < numberOfDigits)
					{

						byte[0]=string[charIndex];
						byte[1]=string[charIndex + 1];
						charIndex+=2;
						printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ), PSTR("strtoul '%s'"), byte);
						data->data[data->length] = strtoul(byte, NULL, 16);
						data->length++;
						printDebug_p(debugLevelEventDebug, debugSystemSPI, __LINE__, (const char*) ( filename ),
								PSTR("data[%i]=%x"), data->length - 1, data->data[data->length -1]);
						//spiAddNumericStringToByteArray(&byte[0], data);
					}
				}
			}
		}
		else
		{
			CommunicationError_p(ERRA, dynamicMessage_ErrorIndex, TRUE, PSTR("too many bytes to add"));
			return -1;
		}

	}
	else
	{
		CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, TRUE, PSTR("%s"), string);
		return -1;
	}

	return 0;
}

void spiApiShowWriteBufferContent(void)
{
	/* header */
	createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, commandKeyNumber_SPI, spiCommandKeyNumber_SHOW_WRITE_BUFFER, spiCommandKeywords);
	snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%s write buffer - elements: %i"), uart_message_string, spiWriteData.length );
	UART0_Send_Message_String(NULL,0);

	/* data */
	clearString(message, BUFFER_SIZE);
	for (size_t byteIndex = 0; byteIndex < spiWriteData.length; ++byteIndex)
	{
		snprintf(message, BUFFER_SIZE -1, "%s %02X", message, spiWriteData.data[byteIndex]);
	}

	printDebug_p(debugLevelEventDebug, debugSystemSPI, -1, NULL, PSTR("wb:%s"), message);

	createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, commandKeyNumber_SPI, spiCommandKeyNumber_SHOW_WRITE_BUFFER, spiCommandKeywords);

	strncat(uart_message_string, message, BUFFER_SIZE -1 );
	UART0_Send_Message_String(NULL,0);

}

#warning TODO: add function to display write buffer content
void spiApiSubCommands(struct uartStruct *ptr_uartStruct, int16_t subCommandIndex)
{
	subCommandIndex = 0;
	// find matching command keyword
/*	while ( subCommandIndex < commandDebugKeyNumber_MAXIMUM_NUMBER )
	{
		if ( 0 == strncmp_P(setParameter[1], (const char*) ( pgm_read_word( &(commandDebugKeywords[subCommandIndex])) ), MAX_LENGTH_PARAMETER) )
		{
				printDebug_p(debugLevelEventDebug, debugSystemDEBUG, __LINE__, PSTR(__FILE__), PSTR("keyword %s matches"), &setParameter[1][0]);
			break;
		}
		else
		{
				printDebug_p(debugLevelEventDebug, debugSystemDEBUG, __LINE__, PSTR(__FILE__), PSTR("keyword %s doesn't match"), &setParameter[1][0]);
		}
		subCommandIndex++;
	}
	*/
}

//spiWrite()
//spiRead() ?
//spiInit()
//spiEnable()
