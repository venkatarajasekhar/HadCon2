/*
 * apfelApi.c
 *
 *  Created on: 22.05.2013
 *      Author: Peter Zumbruch, GSI, P.Zumbruch@gsi.de
 */

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "api_global.h"
#include "api_define.h"
#include "api.h"
#include "apfelApi.h"
#include "apfel.h"

/*eclipse specific setting, not used during build process*/
#ifndef __AVR_AT90CAN128__
#include <avr/iocan128.h>
#endif

static const char filename[] PROGMEM = __FILE__;

static const char string_3dots[] 	PROGMEM = "...";
//static const char string_empty[]  PROGMEM = "";
static const char string_sX[]   	PROGMEM = "%s%X";
//static const char string_sx[]     PROGMEM = "%s%x";
static const char string_PORT[]     PROGMEM = "PORT";

static const char string_sKommax[]  PROGMEM = "%s,%x";

static char byte[3]= "00";

static const char apfelApiCommandKeyword00[] PROGMEM = "dac";
static const char apfelApiCommandKeyword01[] PROGMEM = "d";
static const char apfelApiCommandKeyword02[] PROGMEM = "incr";
static const char apfelApiCommandKeyword03[] PROGMEM = "+";
static const char apfelApiCommandKeyword04[] PROGMEM = "decr";
static const char apfelApiCommandKeyword05[] PROGMEM = "-";
static const char apfelApiCommandKeyword06[] PROGMEM = "testpulse";
static const char apfelApiCommandKeyword07[] PROGMEM = "t";
static const char apfelApiCommandKeyword08[] PROGMEM = "autocalib";
static const char apfelApiCommandKeyword09[] PROGMEM = "a";
static const char apfelApiCommandKeyword10[] PROGMEM = "ampl";
static const char apfelApiCommandKeyword11[] PROGMEM = "list";
static const char apfelApiCommandKeyword12[] PROGMEM = "l";
static const char apfelApiCommandKeyword13[] PROGMEM = "status";
static const char apfelApiCommandKeyword14[] PROGMEM = "s";
static const char apfelApiCommandKeyword15[] PROGMEM = "portAddressSetEnableMask";
static const char apfelApiCommandKeyword16[] PROGMEM = "pasem";
static const char apfelApiCommandKeyword17[] PROGMEM = "enablePortAddressSet";
static const char apfelApiCommandKeyword18[] PROGMEM = "epas";
static const char apfelApiCommandKeyword19[] PROGMEM = "disablePortAddressSet";
static const char apfelApiCommandKeyword20[] PROGMEM = "dpas";
static const char apfelApiCommandKeyword21[] PROGMEM = "addPortAddressSet";
static const char apfelApiCommandKeyword22[] PROGMEM = "apas";
static const char apfelApiCommandKeyword23[] PROGMEM = "removePortAddressSet";
static const char apfelApiCommandKeyword24[] PROGMEM = "rpas";
static const char apfelApiCommandKeyword25[] PROGMEM = "usToSleep";
static const char apfelApiCommandKeyword26[] PROGMEM = "uts";
static const char apfelApiCommandKeyword27[] PROGMEM = "apfel_enable";
static const char apfelApiCommandKeyword28[] PROGMEM = "reset";

const char* apfelApiCommandKeywords[] PROGMEM = {
        apfelApiCommandKeyword00, apfelApiCommandKeyword01, apfelApiCommandKeyword02, apfelApiCommandKeyword03, apfelApiCommandKeyword04,
		apfelApiCommandKeyword05, apfelApiCommandKeyword06, apfelApiCommandKeyword07, apfelApiCommandKeyword08, apfelApiCommandKeyword09,
		apfelApiCommandKeyword10, apfelApiCommandKeyword11, apfelApiCommandKeyword12, apfelApiCommandKeyword13, apfelApiCommandKeyword14,
		apfelApiCommandKeyword15, apfelApiCommandKeyword16, apfelApiCommandKeyword17, apfelApiCommandKeyword18, apfelApiCommandKeyword19,
		apfelApiCommandKeyword20, apfelApiCommandKeyword21, apfelApiCommandKeyword22, apfelApiCommandKeyword23, apfelApiCommandKeyword24,
		apfelApiCommandKeyword25, apfelApiCommandKeyword26, apfelApiCommandKeyword27, apfelApiCommandKeyword28
};

apfelApiConfig apfelApiConfiguration;
bool apfelApiInitialized = false;
apfelApiConfig *ptr_apfelApiConfiguration = &apfelApiConfiguration;

void apfelApiInit(void)
{
	/* initial configuration*/
	ptr_apfelApiConfiguration->hardwareInit         = false;
//	ptr_apfelApiConfiguration->apfelConfiguration     = apfelGetConfiguration();
}


void apfelApi(struct uartStruct *ptr_uartStruct)
{
	if (!apfelApiInitialized )
	{
		apfelApiInit();
		apfelApiInitialized = true;
	}

	switch(ptr_uartStruct->number_of_arguments)
	{
		case 0:
			apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_STATUS, 1);
			break;
		default:
		{
			if (ptr_uartStruct->number_of_arguments > 0)
			{
				int subCommandIndex = -1;
				// find matching command keyword
				subCommandIndex = apiFindCommandKeywordIndex(setParameter[1], apfelApiCommandKeywords, apfelApiCommandKeyNumber_MAXIMUM_NUMBER);
				if ( 0 <= subCommandIndex )
				{
					apfelApiSubCommands(ptr_uartStruct, subCommandIndex, 2);
				}
				else
				{
					CommunicationError_p(ERRA, SERIAL_ERROR_no_valid_command_name, true, NULL);
				}
			}
			else
			{
				CommunicationError_p(ERRG, GENERAL_ERROR_invalid_argument, true, NULL);
			}
			break;
		}
	}
	return;
}

/*
 * void apfelApiSubCommands(struct uartStruct *ptr_uartStruct, int16_t subCommandIndex)
 *
 * parses for sub command keyword and calls the corresponding functions
 */

uint8_t apfelApiSubCommands(struct uartStruct *ptr_uartStruct, int16_t subCommandIndex, uint8_t parameterIndex)
{
	uint8_t result = apiCommandResult_UNDEFINED;
	// find matching command keyword

	if ( 0 > subCommandIndex )
	{
		subCommandIndex = apiFindCommandKeywordIndex(setParameter[1], apfelApiCommandKeywords, apfelApiCommandKeyNumber_MAXIMUM_NUMBER);
	}

    /* generate header */
	switch (parameterIndex)
	{
		case 1:
			createReceiveHeader(ptr_uartStruct, uart_message_string, BUFFER_SIZE);
			break;
		case 0:
		default:
			/* else */
			createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, ptr_uartStruct->commandKeywordIndex, subCommandIndex, apfelApiCommandKeywords);
			break;
	}

	switch (subCommandIndex)
	{
		case apfelApiCommandKeyNumber_STATUS:
			/* show apfel status of control bits and operation settings*/
			result = apfelApiSubCommandShowStatus();
			break;
		case apfelApiCommandKeyNumber_S:
			/* show apfel status of control bits and operation settings*/
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_STATUS, 0);
			break;

		case apfelApiCommandKeyNumber_DAC:
			result = apfelApiSubCommandDac();
			break;

		case apfelApiCommandKeyNumber_D:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_DAC, 0);
			break;

		case apfelApiCommandKeyNumber_INCR:
			result = apfelApiSubCommandIncrementDac();
			break;

		case apfelApiCommandKeyNumber_PLUS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_INCR, 0);
			break;

		case apfelApiCommandKeyNumber_DECR:
			result = apfelApiSubCommandDecrementDac();
			break;

		case apfelApiCommandKeyNumber_MINUS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_DECR, 0);
			break;

		case apfelApiCommandKeyNumber_TESTPULSE:
			result = apfelApiSubCommandTestPulse();
			break;

		case apfelApiCommandKeyNumber_T:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_TESTPULSE, 0);
			break;

		case apfelApiCommandKeyNumber_AUTOCALIB:
			result = apfelApiSubCommandAutoCalib();
			break;

		case apfelApiCommandKeyNumber_A:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_AUTOCALIB, 0);
			break;

		case apfelApiCommandKeyNumber_AMPL:
			result = apfelApiSubCommandAmplification();
			break;

		case apfelApiCommandKeyNumber_LIST:
			result = apfelApiSubCommandListIds();
			break;

		case apfelApiCommandKeyNumber_L:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_LIST, 0);
			break;

		case apfelApiCommandKeyNumber_PORT_ADDRESS_SET_ENABLE_MASK:
			result = apfelApiSubCommandPortAddressSetEnableMask();
			break;

		case apfelApiCommandKeyNumber_PASEM:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_PORT_ADDRESS_SET_ENABLE_MASK, 0);
			break;

		case apfelApiCommandKeyNumber_ENABLE_PORT_ADDRESS_SET:
			result = apfelApiSubCommandEnablePortAddressSet();
			break;

		case apfelApiCommandKeyNumber_EPAS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_ENABLE_PORT_ADDRESS_SET, 0);
			break;

		case apfelApiCommandKeyNumber_DISABLE_PORT_ADDRESS_SET:
			result = apfelApiSubCommandDisablePortAddressSet();
			break;

		case apfelApiCommandKeyNumber_DPAS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_DISABLE_PORT_ADDRESS_SET, 0);
			break;

		case apfelApiCommandKeyNumber_ADD_PORT_ADDRESS_SET:
			result = apfelApiSubCommandAddPortAddressSet();
			break;

		case apfelApiCommandKeyNumber_APAS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_ADD_PORT_ADDRESS_SET, 0);
			break;

		case apfelApiCommandKeyNumber_REMOVE_PORT_ADDRESS_SET:
			result = apfelApiSubCommandRemovePortAddressSet();
			break;

		case apfelApiCommandKeyNumber_RPAS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_REMOVE_PORT_ADDRESS_SET, 0);
			break;

		case apfelApiCommandKeyNumber_US_TO_SLEEP :
			result = apfelApiSubCommandUsToSleep ();
			break;

		case apfelApiCommandKeyNumber_UTS:
			result = apfelApiSubCommands(ptr_uartStruct, apfelApiCommandKeyNumber_US_TO_SLEEP , 0);
			break;

		case apfelApiCommandKeyNumber_APFEL_ENABLE:
			result = apfelApiSubCommandApfelEnable();
			break;

		case apfelApiCommandKeyNumber_RESET:
			result = apfelApiSubCommandReset();
			break;
		default:
			result = -1;
			break;
	}

	/* not a recursive call */
	if (0 < parameterIndex)
	{
		apfelApiSubCommandsFooter( result );
	}
	return result;
}

void apfelApiSubCommandsFooter( uint16_t result )
{
	switch (result)
	{
		case apiCommandResult_SUCCESS_WITH_OUTPUT:
			UART0_Send_Message_String_p(uart_message_string, BUFFER_SIZE - 1);
			break;
		case apiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT:
			/* verbose response to commands*/
			if (debugLevelVerboseDebug <= globalDebugLevel && ((globalDebugSystemMask >> debugSystemAPFEL) & 1))
			{
				strncat_P(uart_message_string, PSTR("OK"), BUFFER_SIZE - 1);
				UART0_Send_Message_String_p(uart_message_string, BUFFER_SIZE - 1);
			}
			else
			{
				clearString(uart_message_string, BUFFER_SIZE);
			}
			break;
		case apiCommandResult_FAILURE_NOT_A_SUB_COMMAND:
			CommunicationError_p(ERRA, SERIAL_ERROR_no_valid_command_name, true, PSTR("not a sub command"));
			break;
		case apiCommandResult_SUCCESS_QUIET:
		case apiCommandResult_FAILURE_QUIET:
			clearString(uart_message_string, BUFFER_SIZE);
			/* printouts elsewhere generated */
			break;
		case apiCommandResult_FAILURE:
		default:
			CommunicationError_p(ERRA, dynamicMessage_ErrorIndex, true, PSTR("command failed"));
			break;
	}

}

apiCommandResult apfelApiSubCommandShowStatus              (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandDac                     (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandIncrementDac            (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandDecrementDac            (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandTestPulse               (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandAutoCalib               (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandAmplification           (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandListIds                 (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandAddPortAddressSet       (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandRemovePortAddressSet    (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandUsToSleep               (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandApfelEnable             (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandReset                   (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandPortAddressSetEnableMask(void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandEnablePortAddressSet    (void){return apiCommandResult_SUCCESS_QUIET;}
apiCommandResult apfelApiSubCommandDisablePortAddressSet   (void){return apiCommandResult_SUCCESS_QUIET;}


#if 0
/*
 * *** sub commands with minimum 1 argument
 */

uint8_t apfelApiSubCommandWrite(struct uartStruct *ptr_uartStruct, uint16_t parameterIndex)
{
	uint8_t result = apfelApiCommandResult_UNDEFINED;

	if ( true == ptr_spiApiConfiguration->autoPurgeReadBuffer )
	{
		apfelApiSubCommandPurgeReadBuffer();
	}

	if ( apfelApiCommandResult_FAILURE > apfelApiPurgeAndFillWriteArray(ptr_uartStruct, parameterIndex))
	{
		/*apfelWrite*/
		result = apfelWriteAndReadWithChipSelect(ptr_spiApiConfiguration->transmitByteOrder, ptr_spiApiConfiguration->csExternalSelectMask);

		if (apfelApiCommandResult_FAILURE <= result)
		{
			result = apfelApiCommandResult_FAILURE_QUIET;
		}
		else
		{
			/*reporting*/
			if (true == ptr_spiApiConfiguration->reportTransmit)
			{
				apfelApiSubCommandShowWriteBuffer(ptr_uartStruct);
				result = apfelApiCommandResult_SUCCESS_QUIET;
			}
			else
			{
				result = apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
			}
		}
	}
	else
	{
		result = apfelApiCommandResult_FAILURE_QUIET;
	}

	if ( true == ptr_spiApiConfiguration->autoPurgeWriteBuffer )
	{
		apfelApiSubCommandPurgeWriteBuffer();
	}

	return result;
}

uint8_t apfelApiSubCommandAdd(struct uartStruct *ptr_uartStruct)
{
	switch( ptr_uartStruct->number_of_arguments -1 )
	{
		case 0: /* APFEL a */
			/* not allowed command,
			 * either since a is not an even digit value,
			 * or a without any further arguments makes no sense.
			 *
			 * failure will be reported with in write command */

			return apfelApiSubCommandWrite(ptr_uartStruct, 1);
			break;
		case 1:
		default:
	   		if ( apfelApiCommandResult_FAILURE > apfelApiAddToWriteArray(ptr_uartStruct, 2))
			{
				return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
			}
			else
			{
				return apfelApiCommandResult_FAILURE_QUIET;
			}
			break;
	}
}

/*
 * *** sub commands with no argument
 */

uint8_t apfelApiSubCommandShowStatus(void)
{
	/* header */
	createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, ptr_uartStruct->commandKeywordIndex, apfelApiCommandKeyNumber_STATUS, apfelApiCommandKeywords);
	UART0_Send_Message_String_p(NULL,0);

	apfelApiShowStatusChipSelect();
	apfelApiShowStatusControls();
	apfelApiShowStatusApiSettings();
	apfelApiShowStatusBuffer();

	return apfelApiCommandResult_SUCCESS_QUIET;
}

void apfelApiShowStatus( uint8_t status[], uint8_t size )
{
	uint16_t nArguments = ptr_uartStruct->number_of_arguments;
	clearString(uart_message_string, BUFFER_SIZE);

	ptr_uartStruct->number_of_arguments = 1;
	for (size_t commandIndex = 0; commandIndex < size; ++commandIndex)
	{
		apfelApiSubCommands(ptr_uartStruct, status[commandIndex], 2);
	}
	ptr_uartStruct->number_of_arguments = nArguments;

}

void apfelApiShowStatusApiSettings(void)
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_TRANSMIT_BYTE_ORDER,
			apfelApiCommandKeyNumber_TRANSMIT_REPORT,
			apfelApiCommandKeyNumber_AUTO_PURGE_READ_BUFFER,
			apfelApiCommandKeyNumber_AUTO_PURGE_WRITE_BUFFER };
	apfelApiShowStatus( list, sizeof(list));
}

void apfelApiShowStatusBuffer(void)
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_SHOW_WRITE_BUFFER,
			apfelApiCommandKeyNumber_SHOW_READ_BUFFER };
	apfelApiShowStatus( list, sizeof(list));
}

void apfelApiShowStatusChipSelect(void)
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_CS,
			apfelApiCommandKeyNumber_CS_BAR,
			apfelApiCommandKeyNumber_CS_PINS,
			apfelApiCommandKeyNumber_CS_SELECT_MASK };
	apfelApiShowStatus( list, sizeof(list));
}

void apfelApiShowStatusControls()
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_CONTROL_BITS,
	};
	apfelApiShowStatus( list, sizeof(list));
}

void apfelApiShowStatusControlBits()
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_APFEL_ENABLE,
			apfelApiCommandKeyNumber_DATA_ORDER,
			apfelApiCommandKeyNumber_MASTER,
			apfelApiCommandKeyNumber_CLOCK_POLARITY,
			apfelApiCommandKeyNumber_CLOCK_PHASE };

	apfelApiShowStatus( list, sizeof(list));
	apfelApiShowStatusSpeed();
}

void apfelApiShowStatusSpeed(void)
{
	uint8_t list[] = {
			apfelApiCommandKeyNumber_SPEED,
			apfelApiCommandKeyNumber_DOUBLE_SPEED,
			apfelApiCommandKeyNumber_SPEED_DIVIDER };
	apfelApiShowStatus( list, sizeof(list));
}

uint8_t apfelApiSubCommandTransmit(void)
{
	uint8_t result = apfelWriteAndReadWithoutChipSelect( ptr_spiApiConfiguration->transmitByteOrder );
	if (  apfelApiCommandResult_FAILURE <= result)
	{
		result = apfelApiCommandResult_FAILURE_QUIET;
		return result;
	}
	if ( true == ptr_spiApiConfiguration->reportTransmit )
	{
		apfelApiSubCommandShowWriteBuffer(ptr_uartStruct);
		return apfelApiCommandResult_SUCCESS_QUIET;
	}
	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiSubCommandWriteBuffer(void)
{
	uint8_t result = apfelApiCommandResult_UNDEFINED;
	uint8_t csSelectMask = 0xFF;

	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /*default*/
			csSelectMask = ptr_spiApiConfiguration->csExternalSelectMask;
		break;
		case 1: /*else*/
		default:
			result = apiAssignParameterToValue( 2, &csSelectMask, apiVarType_UINT8, 0, 0xFF );
			if ( apfelApiCommandResult_FAILURE <= result )
			{
				return apfelApiCommandResult_FAILURE_QUIET;
			}
			break;
	}

	if ( true == ptr_spiApiConfiguration->autoPurgeReadBuffer )
	{
		apfelApiSubCommandPurgeReadBuffer();
	}

	result = apfelWriteAndReadWithChipSelect( ptr_spiApiConfiguration->transmitByteOrder, csSelectMask );

	if ( 0 != result )
	{
		result = apfelApiCommandResult_FAILURE_QUIET;
	}
	else
	{
		if ( true == ptr_spiApiConfiguration->reportTransmit )
		{
			apfelApiSubCommandShowWriteBuffer(ptr_uartStruct);
			result = apfelApiCommandResult_SUCCESS_QUIET;
		}
		else
		{
			result = apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
		}
	}

	if ( true == ptr_spiApiConfiguration->autoPurgeWriteBuffer )
	{
		apfelApiSubCommandPurgeWriteBuffer();
	}

	return result;
}

uint8_t apfelApiSubCommandReset(void)
{
	apfelInit();
	apfelApiInit();
	apfelEnable(true);
	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiSubCommandPurge(void)
{
	apfelApiSubCommandPurgeWriteBuffer();
	apfelApiSubCommandPurgeReadBuffer();

	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiSubCommandPurgeWriteBuffer(void)
{
	apfelPurgeWriteData();
	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiSubCommandPurgeReadBuffer(void)
{
	apfelPurgeReadData();
	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiSubCommandRead(void)
{
	if (APFEL_MSBYTE_FIRST == ptr_spiApiConfiguration->transmitByteOrder)
	{
		return apfelApiShowBufferContent( ptr_uartStruct, &apfelReadData, -1, apfelApiCommandKeyNumber_READ);
	}
	else
	{
		return apfelApiShowBufferContent( ptr_uartStruct, &apfelReadData,  1, apfelApiCommandKeyNumber_READ);
	}
}

/*
 * *** commands with optional arguments
 */

uint8_t apfelApiSubCommandCsStatus(struct uartStruct *ptr_uartStruct)
{
	return apfelApiCsStatus(ptr_uartStruct, false);
}

uint8_t apfelApiSubCommandCsBarStatus(struct uartStruct *ptr_uartStruct)
{
	return apfelApiCsStatus(ptr_uartStruct, true);
}

uint8_t apfelApiCsStatus(struct uartStruct *ptr_uartStruct, bool invert) /*optional external mask*/
{
	uint8_t mask = 0;
	uint16_t result = apfelApiCommandResult_SUCCESS_QUIET;

	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /* show all */
			mask = 0xFF;
		break;
		case 1: /* show mask*/
		default:
			result = apiAssignParameterToValue(2, &mask, apiVarType_UINT8, 0,0xFF);
		break;
	}

	if (apfelApiCommandResult_FAILURE > result)
	{
		apfelApiShowChipSelectStatus(0xFF & mask, invert);
		result = apfelApiCommandResult_SUCCESS_WITH_OUTPUT;
	}

	return result;
}

void apfelApiShowChipSelectStatus(uint8_t mask, bool invert)
{
	bool status = 0;
	uint8_t statusArray = apfelGetCurrentChipSelectBarStatus();
	uint8_t activeMask = apfelGetChipSelectArrayStatus();
	for (int chipSelectIndex = 0; chipSelectIndex < APFEL_CHIPSELECT_MAXIMUM; ++chipSelectIndex)
	{
		if (mask & (0x1 << chipSelectIndex))
		{
			if (activeMask & (0x1 << chipSelectIndex))
			{
				status = ((statusArray & (0x1 << chipSelectIndex)) != 0 );
				//status = (chipSelectIndex+1)%2 > 0; /*dummy*/
				if (!invert)
				{
					status ^= 0x1;
				}
				snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%s%i:%i "), uart_message_string,
						chipSelectIndex + 1,
						status);
			}
			else
			{
				snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%s%i:- "), uart_message_string, chipSelectIndex + 1);
			}
		}
	}
}


uint8_t apfelApiSubCommandCsSet(void)
{
	return apfelApiCsSetOrCsRelease( true );
}

uint8_t apfelApiSubCommandCsRelease(void)
{
	return apfelApiCsSetOrCsRelease( false );
}

uint8_t apfelApiCsSetOrCsRelease( bool set )
{
	uint8_t externalChipSelectMask = 0;
	uint16_t result = apfelApiCommandResult_SUCCESS_QUIET;

	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /* set all active within configuration's mask */
			externalChipSelectMask = ptr_spiApiConfiguration->csExternalSelectMask;
		break;
		case 1: /* show mask*/
		default:
			result = apiAssignParameterToValue(2, &externalChipSelectMask, apiVarType_UINT8, 0,0xFF);
		break;
	}

	if (apfelApiCommandResult_FAILURE > result)
	{
		if ( set )
		{
			apfelSetChosenChipSelect(externalChipSelectMask);
		}
		else
		{
			apfelReleaseChosenChipSelect(externalChipSelectMask);
		}

		/* report */
		apfelApiShowStatus( ({ uint8_t list[] = { apfelApiCommandKeyNumber_CS }; &list[0];}), 1);
		result = apfelApiCommandResult_SUCCESS_QUIET;

	}

	return result;

}

uint8_t apfelApiSubCommandCsSelectMask(struct uartStruct *ptr_uartStruct)
{
	return apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &(ptr_spiApiConfiguration->csExternalSelectMask), apiVarType_UINT8, 0, 0xFF, true, NULL);
}

uint8_t apfelApiSubCommandAutoPurgeReadBuffer(struct uartStruct *ptr_uartStruct)
{
	return apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &(ptr_spiApiConfiguration->autoPurgeReadBuffer), apiVarType_BOOL_TrueFalse, 0, 0xFF, true, NULL);
}

uint8_t apfelApiSubCommandAutoPurgeWriteBuffer(struct uartStruct *ptr_uartStruct)
{
	return apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &(ptr_spiApiConfiguration->autoPurgeWriteBuffer), apiVarType_BOOL_TrueFalse, 0, 0xFF, true, NULL);
}

uint8_t apfelApiSubCommandTransmitReport(struct uartStruct *ptr_uartStruct)
{
	return apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &(ptr_spiApiConfiguration->reportTransmit), apiVarType_BOOL_TrueFalse, 0, 0xFF, true, NULL);
}

uint8_t apfelApiSubCommandCsPins(struct uartStruct *ptr_uartStruct)
{
	/*set/get hardware addresses of multiple CS outputs*/

	uint8_t channelIndex = 0;
    uint16_t result = 0;

	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /*read all*/
			return apfelApiShowChipSelectAddress(-1);
		break;
		case 1: /*read one*/
		default:
			result = apiAssignParameterToValue(2,  &(channelIndex), apiVarType_UINT8, 1, 0x8);
			if ( result < apfelApiCommandResult_FAILURE )
			{
				result = apfelApiShowChipSelectAddress(channelIndex);
			}
			break;
	}
	return result;
}


/* uint8_t apfelApiShowChipSelectAddress(int8_t chipSelectIndex)
 *
 * 	-1		: 	all
 * 	1 ... 8 :	selected
 * 	else 	:	error
 */
uint8_t apfelApiShowChipSelectAddress(int8_t chipSelectIndex)
{
	if (0 == chipSelectIndex || 8 < chipSelectIndex)
	{
		CommunicationError_p(ERRA, SERIAL_ERROR_arguments_exceed_boundaries, true, NULL);
		return apfelApiCommandResult_FAILURE_QUIET;
	}

	uint8_t activeMask = apfelGetChipSelectArrayStatus();
	for (int index = 0; index < APFEL_CHIPSELECT_MAXIMUM; ++index)
	{
		if ( 0 < chipSelectIndex && index != chipSelectIndex -1 )
		{
			continue;
		}

		if ( 0 < chipSelectIndex || (activeMask & (0x1 << index)))
		{
			volatile uint8_t *portPointer = apfelGetPortFromChipSelect(index);
			snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%s%i:"), uart_message_string, index + 1);

			switch((int) portPointer)
			{
				case (int) &PORTA:
				case (int) &PORTB:
				case (int) &PORTC:
				case (int) &PORTD:
				case (int) &PORTE:
				case (int) &PORTF:
				case (int) &PORTG:
				{
					strncat_P(uart_message_string, string_PORT, BUFFER_SIZE - 1);
					switch((int) portPointer)
					{
						case (int) &PORTA:
								strncat_P(uart_message_string, PSTR("A"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTB:
								strncat_P(uart_message_string, PSTR("B"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTC:
								strncat_P(uart_message_string, PSTR("C"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTD:
								strncat_P(uart_message_string, PSTR("D"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTE:
								strncat_P(uart_message_string, PSTR("E"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTF:
								strncat_P(uart_message_string, PSTR("F"), BUFFER_SIZE - 1);
						break;
						case (int) &PORTG:
								strncat_P(uart_message_string, PSTR("G"), BUFFER_SIZE - 1);
						break;
					}
				}
				break;
				default:
					snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%s%p"), uart_message_string, portPointer);
					break;
			}

			snprintf_P(uart_message_string, BUFFER_SIZE - 1, string_sKommax, uart_message_string,
					apfelGetPinFromChipSelect(index));

			if ( chipSelectIndex > 0 )
			{
				strncat_P(uart_message_string, PSTR(","), BUFFER_SIZE - 1);
				bool isUsed = (apfelGetCurrentChipSelectArray())[index].isUsed;
				apiShowValue(uart_message_string, &isUsed, apiVarType_BOOL_OnOff );
//				apiShowValue(NULL, &((apfelGetCurrentChipSelectArray())[index].isUsed), apiVarType_BOOL_OnOff );
			}
			strncat_P(uart_message_string, PSTR(" "), BUFFER_SIZE - 1);
		}
	}
	return apfelApiCommandResult_SUCCESS_WITH_OUTPUT;
}


uint8_t apfelApiSubCommandShowWriteBuffer(struct uartStruct *ptr_uartStruct)
{
	return apfelApiShowBuffer( ptr_uartStruct, &apfelWriteData, apfelApiCommandKeyNumber_SHOW_WRITE_BUFFER);
}

uint8_t apfelApiShowBuffer(struct uartStruct *ptr_uartStruct, apfelByteDataArray *buffer, int8_t subCommandKeywordIndex)
{
	bool revert = false;
    uint16_t result = 0;
    int16_t nBytes = 0;

	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /*show all*/
			nBytes = 0;
		break;
		case 1: /*read selected*/
			result = apiAssignParameterToValue(2,  &(nBytes), apiVarType_UINT16, 0, INT16_MAX);
			if ( apfelApiCommandResult_FAILURE <= result)
			{
				return result;
			}
			break;
		case 2: /*read selected, optionally inverted*/
		default:
			result = apiAssignParameterToValue(2,  &(nBytes), apiVarType_UINT16, 0, INT16_MAX);
			if ( apfelApiCommandResult_FAILURE <= result)
			{
				return result;
			}
			result = apiAssignParameterToValue(3,  &(revert), apiVarType_BOOL, 0, 0xFF);
			if ( apfelApiCommandResult_FAILURE <= result)
			{
				return result;
			}
			if (revert)
			{
				nBytes = 0 - nBytes;
			}
			break;
	}

	return apfelApiShowBufferContent( ptr_uartStruct, buffer, nBytes, subCommandKeywordIndex);
}

uint8_t apfelApiSubCommandShowReadBuffer(struct uartStruct *ptr_uartStruct)
{
	return apfelApiShowBuffer( ptr_uartStruct, &apfelReadData, apfelApiCommandKeyNumber_SHOW_READ_BUFFER);
}

uint8_t apfelApiSubCommandControlBits(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,
			         &(ptr_spiApiConfiguration->apfelConfiguration.data), apiVarType_UINT16, 0, 0x1FF, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
		UART0_Send_Message_String_p(uart_message_string, BUFFER_SIZE);

		apfelApiShowStatusControlBits();

		result = apfelApiCommandResult_SUCCESS_QUIET;
	}
	return result;
}

uint8_t apfelApiSubCommandSpiEnable(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bSpe;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_BOOL_TrueFalse, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bSpe = 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandDataOrder(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bDord;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_UINT8, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bDord = 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandMaster(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bMstr;

	/* TODO: implement client ??? */
	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_BOOL_TrueFalse, 0x1, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bMstr= 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandClockPolarity(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bCpol;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_UINT8, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bCpol = 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandClockPhase(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bCpha;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_UINT8, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bCpha = 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandSpeed(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_UINT8, 0, 0x3, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr = 0x3 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}

uint8_t apfelApiSubCommandSpeedDivider(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t divider = 0;
	uint8_t shift = 0;

	switch(ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr & 0x3)
	{
		case 0:
			divider = 0x4;
			shift = 2;
			break;
		case 1:
			divider = 0x10;
			shift = 4;
			break;
		case 2:
			divider = 0x40;
			shift = 6;
			break;
		case 3:
			divider = 0x80;
			shift = 7;
			break;
	}
	switch(ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x & 0x1)
	{
		case 0:
			break;
		case 1:
			divider >>= 1 ;
			shift -= 1;
			break;
	}

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &divider, apiVarType_UINT8, 0, 0x80, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			switch(divider)
			{
				case 0x02:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x0;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x1;
					shift = 1;
					break;
				case 0x04:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x0;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x0;
					shift = 2;
					break;
				case 0x08:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x1;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x1;
					shift = 3;
					break;
				case 0x10:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x1;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x0;
					shift = 4;
					break;
				case 0x20:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x2;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x1;
					shift = 5;
					break;
				case 0x40:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x2;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x0;
					shift = 6;
					break;
				case 0x80:
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpr   = 0x3;
					ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x0;
					shift = 7;
					break;
				default:
					CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, true, NULL);
					result = apfelApiCommandResult_FAILURE_QUIET;
					break;
			}
			if ( apfelApiCommandResult_FAILURE > result )
			{
				apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
			}
		}
		snprintf_P(uart_message_string,  BUFFER_SIZE - 1, PSTR("%s (%luHz @ %luHz)"), uart_message_string, (unsigned long) (F_CPU >> shift), (unsigned long) (F_CPU) );
	}

	return result;
}

uint8_t apfelApiSubCommandDoubleSpeed(struct uartStruct *ptr_uartStruct)
{
	ptr_spiApiConfiguration->apfelConfiguration = apfelGetConfiguration();
	uint8_t bits = ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x;

	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &bits, apiVarType_BOOL_TrueFalse, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 < ptr_uartStruct->number_of_arguments -1)
		{
			ptr_spiApiConfiguration->apfelConfiguration.bits.bSpi2x = 0x1 & bits;
			apfelSetConfiguration(ptr_spiApiConfiguration->apfelConfiguration);
		}
	}
	return result;
}


/*apfel api settings*/
uint8_t apfelApiSubCommandTransmitByteOrder(struct uartStruct *ptr_uartStruct)
{
	uint8_t result = apiShowOrAssignParameterToValue(ptr_uartStruct->number_of_arguments - 1, 2,  &(ptr_spiApiConfiguration->transmitByteOrder), apiVarType_UINT8, 0, 0x1, true, NULL);

	if ( apfelApiCommandResult_FAILURE > result )
	{
		if (0 == ptr_uartStruct->number_of_arguments -1)
		{
			switch (ptr_spiApiConfiguration->transmitByteOrder)
			{
				case apfelApiTransmitByteOrder_MSB:
					strncat_P(uart_message_string, PSTR(" (MSB/big endian)"), BUFFER_SIZE -1);
					break;
				case apfelApiTransmitByteOrder_LSB:
					strncat_P(uart_message_string, PSTR(" (LSB/little endian)"), BUFFER_SIZE -1);
					break;
				default:
					strncat_P(uart_message_string, PSTR(" UNDEFINED"), BUFFER_SIZE -1);
					break;
			}
		}
	}
	return result;
}

uint8_t apfelApiSubCommandCsAddPin(struct uartStruct *ptr_uartStruct)
{
	uint8_t chipSelectNumber = 0;
	apfelPin cs;
    uint8_t result = apfelApiCommandResult_UNDEFINED;
    uint8_t parameterIndex = 0;
	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0:
		case 1:
			CommunicationError_p(ERRA, SERIAL_ERROR_too_few_arguments, true, NULL);
			result = apfelApiCommandResult_FAILURE_QUIET;
			break;
		case 2:
		case 3: /*write*/
		default:
			/* 1st argument, string PORTA...G */
			parameterIndex = 2;
			if ( isNumericArgument(setParameter[parameterIndex], MAX_LENGTH_PARAMETER) )
			{
				result = apiAssignParameterToValue(parameterIndex, &(cs.ptrPort), apiVarType_UINTPTR, UINT64_C(0), UINT64_C(0xFF));
			}
			else
			{
				/* match "port/PORTx/X" ?*/
				if (0 == strncasecmp_P(setParameter[parameterIndex], string_PORT, 3) && 5 == strlen(setParameter[parameterIndex]))
				{
					result = apfelApiCommandResult_SUCCESS_QUIET;
					switch(setParameter[parameterIndex][4])
					{
						case 'A':
						case 'a':
							cs.ptrPort = &PORTA;
							break;
						case 'B':
						case 'b':
							cs.ptrPort = &PORTB;
							break;
						case 'C':
						case 'c':
							cs.ptrPort = &PORTC;
							break;
						case 'D':
						case 'd':
							cs.ptrPort = &PORTD;
							break;
						case 'E':
						case 'e':
							cs.ptrPort = &PORTE;
							break;
						case 'F':
						case 'f':
							cs.ptrPort = &PORTF;
							break;
						case 'G':
						case 'g':
							cs.ptrPort = &PORTG;
							break;
						default:
							CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, true, NULL);
							result = apfelApiCommandResult_FAILURE_QUIET;
							break;
					}
				}
				else
				{
					CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, true, NULL);
					result = apfelApiCommandResult_FAILURE_QUIET;
					break;
				}
			}
			if ( apfelApiCommandResult_FAILURE <= result )
			{
				break;
			}

			/* 2nd argument, pin */
			parameterIndex = 3;
			result = apiAssignParameterToValue(parameterIndex, &(cs.pinNumber), apiVarType_UINT8, 0, 7);
			if ( apfelApiCommandResult_FAILURE <= result )
			{
				break;
			}

			/* optional 3rd argument, chipSelectNumber */
			parameterIndex = 4;
			if ( ptr_uartStruct->number_of_arguments -1 > 2)
			{
				result = apiAssignParameterToValue(parameterIndex, &chipSelectNumber, apiVarType_UINT8, 1, 8);
				if ( apfelApiCommandResult_FAILURE <= result )
				{
					break;
				}
			}
			else
			{
				/* find open slot */
				chipSelectNumber = APFEL_CHIPSELECT0;
				while (chipSelectNumber < APFEL_CHIPSELECT_MAXIMUM)
				{
					if (false == (apfelGetCurrentChipSelectArray()[chipSelectNumber]).isUsed)
					{
						break;
					}
					chipSelectNumber++;
				}
				if ( APFEL_CHIPSELECT_MAXIMUM == chipSelectNumber)
				{
					CommunicationError_p(ERRA, dynamicMessage_ErrorIndex, true, PSTR("max #slots (%i) reached"), APFEL_CHIPSELECT_MAXIMUM);
					result = apfelApiCommandResult_FAILURE_QUIET;
					break;
				}
				/* increase to match software counting of cs [1, 8]*/
				chipSelectNumber++;
			}

			/* check doubles */
			for ( uint8_t slot  = APFEL_CHIPSELECT0; slot < APFEL_CHIPSELECT_MAXIMUM; slot++)
			{
				if ((apfelGetCurrentChipSelectArray()[slot]).isUsed &&
					cs.pinNumber == (apfelGetCurrentChipSelectArray()[slot]).pinNumber &&
					cs.ptrPort == (apfelGetCurrentChipSelectArray()[slot]).ptrPort )
				{
					CommunicationError_p(ERRA, dynamicMessage_ErrorIndex, true, PSTR("port/pin in use"));
					result = apfelApiCommandResult_FAILURE_QUIET;
					break;
				}
			}
			if ( apfelApiCommandResult_FAILURE <= result )
			{
				break;
			}

			/*add cs*/
			/* subtract -1 to match hardware counting of cs [0,7]*/
			result = apfelAddChipSelect(cs.ptrPort, cs.pinNumber, chipSelectNumber-1);
			if ( 0 == result)
			{
				/* report */
				apfelApiShowStatus( ({ uint8_t list[] = { apfelApiCommandKeyNumber_CS_PINS }; &list[0];}), 1);
				result = apfelApiCommandResult_SUCCESS_QUIET;
			}
			else
			{
				result = apfelApiCommandResult_FAILURE_QUIET;
			}
			break;
	}
	return result;
}

uint8_t apfelApiSubCommandCsRemovePin(struct uartStruct *ptr_uartStruct)
{
	uint8_t chipSelectNumber = 0;
	switch (ptr_uartStruct->number_of_arguments - 1)
	{
		case 0: /*print*/
			CommunicationError_p(ERRA, SERIAL_ERROR_too_few_arguments, true, NULL);
			return apfelApiCommandResult_FAILURE_QUIET;
			break;
		case 1: /*write*/
		default:
			/* take second parameter, i.e. first argument to sub command and fill it into value*/
			if ( apfelApiCommandResult_FAILURE > apiAssignParameterToValue(2, &chipSelectNumber, apiVarType_UINT8, 1, 8) )
			{
				if ( 0 == apfelRemoveChipSelect(chipSelectNumber-1))
				{
					apfelApiShowStatus( ({ uint8_t list[] = { apfelApiCommandKeyNumber_CS_PINS }; &list[0];}), 1);
					return apfelApiCommandResult_SUCCESS_QUIET;
				}
				else
				{
					return apfelApiCommandResult_FAILURE_QUIET;
				}
			}
			break;
	}
	return apfelApiCommandResult_SUCCESS_QUIET;
}


/* helpers */

uint8_t apfelApiPurgeAndFillWriteArray(struct uartStruct *ptr_uartStruct, uint16_t parameterStartIndex)
{
	/* reset array*/
	apfelPurgeWriteData();
	return apfelApiAddToWriteArray(ptr_uartStruct, parameterStartIndex);
}

uint8_t apfelApiAddToWriteArray(struct uartStruct *ptr_uartStruct, uint16_t parameterIndex)
{
	uint8_t result = apfelApiCommandResult_UNDEFINED;
	uint8_t nTotalArgs = ptr_uartStruct->number_of_arguments;
	uint8_t argumentCounter = parameterIndex;
    while( apfelWriteData.length < sizeof(apfelWriteData.data) && argumentCounter < nTotalArgs + 1)
    {
		parameterIndex = argumentCounter % MAX_PARAMETER;
		printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename, PSTR("%i"), argumentCounter);

		printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename,
				PSTR("setParameter[%i]: \"%s\""), parameterIndex, setParameter[parameterIndex]);

		/* get contents from parameter container */
		result = apfelApiAddNumericParameterToByteArray(NULL, parameterIndex);
		if ( apfelApiCommandResult_FAILURE <= result )
		{
			return result;
			break;
		}
		argumentCounter++;

		if (0 == argumentCounter % MAX_PARAMETER )
		{
    		if (strlen(decrypt_uartString_remainder))
    		{
    			/* get contents from remainder container */
    			/* reuse setParameter as long as arguments are there*/
    			clearString(resultString, BUFFER_SIZE);
    			strncpy(resultString, decrypt_uartString_remainder, BUFFER_SIZE);
    			clearString(decrypt_uartString_remainder, BUFFER_SIZE);

    			printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename, PSTR("'%s'"), resultString);

    			if ( 1 > uartSplitUartString( resultString )  )
    			{
    				break;
    			}
    		}
			else
			{
				break;
			}
    	}
    }

    return result;
}//END of function

uint8_t apfelApiAddNumericParameterToByteArray(const char string[], uint8_t index)
{
	int8_t result;
	printDebug_p(debugLevelEventDebugVerbose, debugSystemAPFEL, __LINE__, filename, PSTR("para to byte[]"));

	/* get string string[] */
	if ( 0 > index || MAX_PARAMETER <= index  )
	{
		if ( NULL == string)
		{
			CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
			return -1;
		}
		result = apfelApiAddNumericStringToByteArray( string );
		if ( apfelApiCommandResult_FAILURE <= result )
		{
			return result;
		}
	}
	/* get string from setParameter at the index */
	else
	{
		result = apfelApiAddNumericStringToByteArray( setParameter[index] );
		if ( apfelApiCommandResult_FAILURE <= result )
		{
			return result;
		}
	}
	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

uint8_t apfelApiAddNumericStringToByteArray(const char string[])
{
	printDebug_p(debugLevelEventDebugVerbose, debugSystemAPFEL, __LINE__, filename, PSTR("string to byte[]"));

	uint64_t value;
	size_t numberOfDigits;

	if ( NULL == string)
	{
		CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("NULL pointer received"));
		return apfelApiCommandResult_FAILURE_QUIET;
	}

	if ( isNumericArgument(string, MAX_LENGTH_COMMAND - MAX_LENGTH_KEYWORD - 2))
	{
		/* get number of digits*/
		numberOfDigits = getNumberOfHexDigits(string, MAX_LENGTH_COMMAND - MAX_LENGTH_KEYWORD - 2 );

		// only even number of digits are allowed, since only complete bytes are transmitted
		if (numberOfDigits%2 )
		{
			CommunicationError_p(ERRG, dynamicMessage_ErrorIndex, TRUE, PSTR("only EVEN number of digits allowed (%s)"), string);
			return apfelApiCommandResult_FAILURE_QUIET;
		}

		/* check if new element to be added, split into bytes, MSB to LSB,
		 * would exceed the maximum size of data array */

		size_t numberOfBytes = ((numberOfDigits + numberOfDigits%2) >> 1);

		printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename,
				PSTR("#bytes/#digits %i/%i"), numberOfBytes, numberOfDigits);

		if ( apfelWriteData.length + numberOfBytes < sizeof(apfelWriteData.data) -1 )
		{
			printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename,
					PSTR("valueString '%s' "), string);

			if ( 16 >= numberOfDigits ) /*64 bit*/
			{
				if ( 0 == getUnsignedNumericValueFromParameterString(string, &value) )
				{
					for (int8_t byteIndex = numberOfBytes - 1; byteIndex >= 0; byteIndex--)
					{
						apfelAddWriteData( 0xFF & (value >> (8 * byteIndex)) );
					}
				}
				else
				{
					CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, TRUE, PSTR("%s"), string);
					return apfelApiCommandResult_FAILURE_QUIET;
				}
			} /* > 64 bit*/
			else
			{
				size_t charIndex = 0;

				/* skip leading 0x/0X */
				if ( 0 == strncasecmp_P (string, PSTR("0x"), sizeof(string)))
				{
					charIndex+=2;
				}

				while (charIndex + 1 < numberOfDigits)
				{
					byte[0]=string[charIndex];
					byte[1]=string[charIndex + 1];
					charIndex+=2;

					apfelAddWriteData( strtoul(byte, NULL, 16) );
				}
			}

			/* debug report */
			for (int8_t byteIndex = numberOfBytes - 1; byteIndex >= 0; byteIndex--)
			{
				printDebug_p(debugLevelEventDebug, debugSystemAPFEL, __LINE__, filename,
						PSTR("data[%i]=%x"), apfelWriteData.length -1 - byteIndex, apfelWriteData.data[apfelWriteData.length -1 - byteIndex]);
			}
		}
		else
		{
			CommunicationError_p(ERRA, dynamicMessage_ErrorIndex, TRUE, PSTR("too many bytes to add"));
			return apfelApiCommandResult_FAILURE_QUIET;
		}
	}
	else
	{
		CommunicationError_p(ERRA, SERIAL_ERROR_argument_has_invalid_type, TRUE, PSTR("%s"), string);
		return apfelApiCommandResult_FAILURE_QUIET;
	}

	return apfelApiCommandResult_SUCCESS_WITH_OPTIONAL_OUTPUT;
}

/* uint8_t apfelApiShowBufferContent(apfelByteDataArray *buffer, int16_t nRequestedBytes, int8_t commandKeywordIndex, int8_t subCommandKeywordIndex, PGM_P commandKeywords[])
 *
 * shows the content of the data in buffer
 * and prints out the corresponding caller's command and sub command keyword
 *
 * Arguments:
 *
 *	buffer:
 * 						pointer to the buffer struct
 * 	nRequestedBytes:
 * 						number of bytes to show
 * 						0:		 	all available (max: buffer->length)
 * 						positive:	first nBytes of buffer,
 * 										data index 0 ... nBytes
 * 						negative:	last  nBytes of buffer,
 * 										data index buffer->length - 1 - nBytes ... buffer->length - 1
 *
 * 	commandKeywordIndex
 * 	subCommandKeywordIndex
 * 	subCommandKeywordArray
 */

uint8_t apfelApiShowBufferContent(struct uartStruct *ptr_uartStruct, apfelByteDataArray *buffer, int16_t nRequestedBytes, int8_t subCommandKeywordIndex)
{
	uint16_t ctr = 0;
	uint16_t maxMessageSize = 0;
	size_t byteIndex = 0;
	int nBytes = nRequestedBytes;

	clearString(message, BUFFER_SIZE);

	if ( 0 < nRequestedBytes ) /* positive */
	{
		byteIndex = 0;
		nBytes = min((int)(buffer->length), nBytes);
	}
	else if ( 0 > nRequestedBytes ) /* negative */
	{
		byteIndex = max((int)(buffer->length) - abs(nBytes),0);
		nBytes = min((int)(buffer->length), abs(nBytes));
	}
	else /* 0 */
	{
		byteIndex = 0;
		nBytes = buffer->length;
	}

	/* print summary header if all elements requested */
	if ( 0 == nRequestedBytes)
	{
		/* summary header */
		createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, ptr_uartStruct->commandKeywordIndex, subCommandKeywordIndex, apfelApiCommandKeywords);
		snprintf_P(uart_message_string, BUFFER_SIZE - 1, PSTR("%selements: %#x (%#i)"), uart_message_string, buffer->length, buffer->length );
		UART0_Send_Message_String_p(NULL,0);
	}

	/* data */
	if (buffer->length)
	{
		maxMessageSize = BUFFER_SIZE - 1 - strlen_P(string_3dots) - strlen(uart_message_string);

		for (int16_t byteCtr = 0; byteIndex < buffer->length && byteCtr < nBytes; byteIndex++, byteCtr++)
		{
			if ( (byteCtr)%8 == 0)
			{
				clearString(message, BUFFER_SIZE);
				ctr++;
				if ( nBytes > 8 )
				{
					snprintf(message, maxMessageSize, "(#%i) ", ctr);
				}
			}

			snprintf(message, maxMessageSize, "%s%02X ", message, buffer->data[byteIndex]);

			if ( 0 == (byteCtr+1)%8 || byteIndex == buffer->length-1 || byteCtr+1 == nBytes )
			{
				createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, ptr_uartStruct->commandKeywordIndex, subCommandKeywordIndex, apfelApiCommandKeywords);

				/* attach "..." at the end, if there are more to come*/
				if (byteCtr+1 < nBytes)
				{
					strncat_P(message, string_3dots, BUFFER_SIZE -1 );
				}

				strncat(uart_message_string, message, BUFFER_SIZE -1 );
				UART0_Send_Message_String_p(NULL,0);
			}
		}
	}
	else
	{
		if (nRequestedBytes)
		{
			/* empty buffer */
			createExtendedSubCommandReceiveResponseHeader(ptr_uartStruct, ptr_uartStruct->commandKeywordIndex, subCommandKeywordIndex, apfelApiCommandKeywords);
			strncat_P(uart_message_string, PSTR("--"),BUFFER_SIZE -1  );
			return apfelApiCommandResult_SUCCESS_WITH_OUTPUT;
		}
	}


	return apfelApiCommandResult_SUCCESS_QUIET;
}

#endif