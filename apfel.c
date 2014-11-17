/*
 * apfel.c
 *
 *  Created on: 12.06.2013
 *      Author: Florian Brabetz, GSI, f.brabetz@gsi.de
 */


#include <stdio.h>
#include "api.h"
#include "apfel.h"
#include "read_write_register.h"
#include <avr/io.h>
#include <stdbool.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

/*eclipse specific setting, not used during build process*/
#ifndef __AVR_AT90CAN128__
#include <avr/iocan128.h>
#endif

uint16_t apfelPortAddressSetMask = 0;

apfelPortAddressSet apfelPortAddressSets[APFEL_MAX_N_PORT_ADDRESS_SETS];

//----------------
void apfelInit(void)
{
	uint8_t i;

	apfelSetUsToDelay(APFEL_DEFAULT_US_TO_DELAY);

	for (i = APFEL_PORT_ADDRESS_SET_0; i < APFEL_PORT_ADDRESS_SET_MAXIMUM; i++)
	{
		apfelRemovePortAddressSet(i);
	}

	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_0, &PORTA, PA1, PA2, PA3, PA4);
	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_1, &PORTA, PA4, PA5, PA6, PA7);
	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_2, &PORTC, PC1, PC2, PC3, PC4);
	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_3, &PORTC, PC4, PC5, PC6, PC7);
	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_4, &PORTF, PF1, PF2, PF3, PF4);
	apfelAddOrModifyPortAddressSet(APFEL_PORT_ADDRESS_SET_5, &PORTF, PF4, PF5, PF6, PF7);

	for (i = APFEL_PORT_ADDRESS_SET_0; i < APFEL_PORT_ADDRESS_SET_MAXIMUM; i++)
	{
		apfelInitPortAddressSet(i);
	}

	for (i = APFEL_PORT_ADDRESS_SET_0; i < APFEL_PORT_ADDRESS_SET_MAXIMUM; i++)
	{
		apfelEnablePortAddressSet(i);
	}
}

void apfelSetUsToDelay(double us)
{
	apfelUsToDelay = us;
}

double apfelGetUsToDelay(void)
{
	return apfelUsToDelay;
}

apfelPortAddressSet * apfelGetPortAddressSetArray(void)
{
	return apfelPortAddressSets;
}

volatile uint8_t * apfelGetPortFromPortAddressSet(uint8_t portAddressSetIndex)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return NULL;
	}
	return (apfelPortAddressSets[portAddressSetIndex]).ptrPort;
}

apfelPinSetUnion apfelGetPinsFromPortAddressSet(uint8_t portAddressSetIndex)
{
	return apfelPortAddressSets[portAddressSetIndex].pins;
}

apfelPortAddressStatusUnion apfelGetStatusFromPortAddressSet(uint8_t portAddressSetIndex)
{
	return apfelPortAddressSets[portAddressSetIndex].status;
}


apiCommandResult apfelAddOrModifyPortAddressSet(uint8_t portAddressSetIndex, volatile uint8_t *ptrCurrentPort,
		                                        uint8_t pinIndexDIN, uint8_t pinIndexDOUT,
		                                        uint8_t pinIndexCLK, uint8_t pinIndexSS)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return apiCommandResult_FAILURE;
	}
	if (NULL == ptrCurrentPort)
	{
		return apiCommandResult_FAILURE;
	}
    if (pinIndexDIN > 7 || pinIndexDOUT > 7|| pinIndexCLK > 7|| pinIndexSS > 7)
    {
    	return apiCommandResult_FAILURE;
    }
    if (pinIndexDIN < 1 || pinIndexDOUT < 1|| pinIndexCLK < 1|| pinIndexSS < 1)
    {
    	return apiCommandResult_FAILURE;
    }

	(apfelPortAddressSets[portAddressSetIndex]).pins.bits.bPinCLK = pinIndexCLK  - 1;
	(apfelPortAddressSets[portAddressSetIndex]).pins.bits.bPinDIN = pinIndexDIN  - 1;
	(apfelPortAddressSets[portAddressSetIndex]).pins.bits.bPinDOUT= pinIndexDOUT - 1;
	(apfelPortAddressSets[portAddressSetIndex]).pins.bits.bPinSS  = pinIndexSS   - 1;
	(apfelPortAddressSets[portAddressSetIndex]).ptrPort = ptrCurrentPort;
	(apfelPortAddressSets[portAddressSetIndex]).status.bits.bIsEnabled = 0;
	(apfelPortAddressSets[portAddressSetIndex]).status.bits.bIsInitialized = 0;
	(apfelPortAddressSets[portAddressSetIndex]).status.bits.bIsSet         = 1;

	return apiCommandResult_SUCCESS_QUIET;
}

apiCommandResult apfelRemovePortAddressSet(uint8_t portAddressSetIndex)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return apiCommandResult_FAILURE;
	}

	apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinCLK          = 0;
	apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinDIN          = 0;
	apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinDOUT         = 0;
	apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinSS           = 0;
	apfelPortAddressSets[portAddressSetIndex].ptrPort                    = NULL;
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsEnabled     = 0;
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsInitialized = 0;
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsSet         = 0;

	return apiCommandResult_SUCCESS_QUIET;
}

apiCommandResult apfelEnablePortAddressSet(uint8_t portAddressSetIndex)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return apiCommandResult_FAILURE;
	}
	if (0 == apfelPortAddressSets[portAddressSetIndex].status.bits.bIsSet)
	{
		return apiCommandResult_FAILURE;
	}
	if (0 == apfelPortAddressSets[portAddressSetIndex].status.bits.bIsInitialized)
	{
		return apiCommandResult_FAILURE;
	}

	apfelPortAddressSetMask |= (0x1 << portAddressSetIndex);
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsEnabled = 1;

	return apiCommandResult_SUCCESS_QUIET;
}

apiCommandResult apfelDisblePortAddressSet(uint8_t portAddressSetIndex)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return apiCommandResult_FAILURE;
	}

	apfelPortAddressSetMask &= ~(0x1 << portAddressSetIndex);
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsEnabled = 0;

	return apiCommandResult_SUCCESS_QUIET;
}


apiCommandResult apfelInitPortAddressSet(uint8_t portAddressSetIndex)
{
	if ( APFEL_PORT_ADDRESS_SET_MAXIMUM <= portAddressSetIndex)
	{
		return apiCommandResult_FAILURE;
	}

	if (0 == apfelPortAddressSets[portAddressSetIndex].status.bits.bIsSet)
	{
		return apiCommandResult_FAILURE;
	}


	// PINA address 0x00
	// DDRA address 0x01
	// PORTA address 0x02
	// decrement address by two (now pointing to DDRx) and set the appropriate bit to one
	// pin becomes an output pin

	// input pin / no pull up
	*(apfelPortAddressSets[portAddressSetIndex].ptrPort - 2) &= ~(0x1 << apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinDIN);
    *(apfelPortAddressSets[portAddressSetIndex].ptrPort    ) &=  (0x1 << apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinDIN);

    // output pins
	*(apfelPortAddressSets[portAddressSetIndex].ptrPort - 2) |= (0x1 << apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinCLK);
	*(apfelPortAddressSets[portAddressSetIndex].ptrPort - 2) |= (0x1 << apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinDOUT);
	*(apfelPortAddressSets[portAddressSetIndex].ptrPort - 2) |= (0x1 << apfelPortAddressSets[portAddressSetIndex].pins.bits.bPinSS);

	//port address set is initialized
	apfelPortAddressSets[portAddressSetIndex].status.bits.bIsInitialized = 1;

	return apiCommandResult_SUCCESS_QUIET;
}



/* read/write bits*/
apiCommandResult apfelWriteBit(uint8_t bit, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDOUT > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	uint8_t mask = (1 << pinCLK | 1 << pinDOUT | 1 << pinSS);

	bit = bit?1:0;
    ss  = ss?1:0;

	//clock Low + data low
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock Low  + data "bit"
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | bit << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock High + data "bit"
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(1 << pinCLK | bit << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock High + data low
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(1 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock low  + data low
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }

	return apiCommandResult_SUCCESS_QUIET;
}

apiCommandResult apfelReadBitSequence(uint8_t nBits, uint32_t* bits, uint8_t portAddress, uint8_t ss, uint8_t pinDIN, uint8_t pinCLK, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDIN > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}
	if (NULL == bits)
	{
		return apiCommandResult_FAILURE_QUIET;
	}
	if (32 < nBits)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	ss = ss?1:0;
    *bits = 0;

	//make sure we start from clock 0
	if ( false == apfelSetClockAndDataLine(portAddress, (uint8_t)(0 << pinCLK | ss << pinSS), (uint8_t)(1 << pinCLK)))
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	// initial bit read differently !!! TNX^6 Peter Wieczorek ;-)
	// since apfel needs first falling clock edge to activate the output pad
	// therefore first cycle then read data.
	//clock high
	if ( false == apfelSetClockAndDataLine(portAddress, (uint8_t)(1 << pinCLK | ss << pinSS), (uint8_t)(1 << pinCLK)))
	{
		return apiCommandResult_FAILURE_QUIET;
	}
	//clock low
	if ( false == apfelSetClockAndDataLine(portAddress, (uint8_t)(0 << pinCLK | ss << pinSS), (uint8_t)(1 << pinCLK)))
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	//read first data bit in
    //LSB first

	*bits |= apfelGetDataInLine( portAddress, pinDIN );

	#warning check correct output
    //MSB first
	//*bits |= apfelGetDataInLine( portAddress, pinDIN ) << (nBits -1) ;

	//read remaining (max 31 bits)
	for (uint8_t n=1; n<nBits-1; n++)
	{
		//clock high
		if ( false == apfelSetClockAndDataLine(portAddress, (uint8_t)(1 << pinCLK | ss << pinSS), (uint8_t)(1 << pinCLK)))
		{
			return apiCommandResult_FAILURE_QUIET;
		}

		//read data bit in
	    //LSB first

		*bits |= apfelGetDataInLine( portAddress, pinDIN ) << n;
#warning check correct output
		//MSB first
		//*bits |= apfelGetDataInLine( portAddress, pinDIN ) << (nBits -1 - n) ;
		//_delay_us(apfelUsToDelay);

		//clock low
		if ( false == apfelSetClockAndDataLine(portAddress, (uint8_t)(0 << pinCLK | ss << pinSS), (uint8_t)(1 << pinCLK)))
		{
			return apiCommandResult_FAILURE_QUIET;
		}
	}

	return apiCommandResult_SUCCESS_QUIET;
}

apiCommandResult apfelWriteClockSequence(uint32_t num, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	while (num)
	{
		if (apiCommandResult_FAILURE < apfelWriteBit(0, portAddress, ss, pinCLK, pinDOUT, pinSS))
		{
			return apiCommandResult_FAILURE_QUIET;
		}
		num--;
	}
	return apiCommandResult_SUCCESS_QUIET;
}

uint8_t apfelSetClockAndDataLine( uint8_t portAddress, uint8_t value, uint8_t mask)
{
	uint8_t set = ((REGISTER_READ_FROM_8BIT_REGISTER(portAddress) & (~mask)) | (value & mask ));
	if (set && mask != mask && (REGISTER_WRITE_INTO_8BIT_REGISTER_AND_READBACK(portAddress, set)))
	{
		return false;
	}
	_delay_us(apfelUsToDelay);
	return true;
}


apiCommandResult apfelClearDataInput(uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDOUT > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	uint8_t mask = (1 << pinCLK | 1 << pinDOUT | 1 << pinSS);

	static uint8_t nBits = APFEL_N_COMMAND_BITS + APFEL_N_VALUE_BITS + APFEL_N_CHIP_ID_BITS;

    ss  = ss?1:0;

    for (int i = 0; i < nBits; i++)
    {
    	//clock Low + data low
    	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }

    	//clock Low + data low
    	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
    	_delay_us(apfelUsToDelay);

    	apfelWriteClockSequence(1, portAddress, ss, pinCLK, pinDOUT, pinSS);
    }

    // tuning
    for (int i = 0; i < 10; i++)
    {
    	//clock Low + data low
    	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0   << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
    	_delay_us(apfelUsToDelay);
    }

	return apiCommandResult_SUCCESS_QUIET;
}

//writeBitSequence #bits #data #endianess (0: little, 1:big)
apiCommandResult apfelWriteBitSequence(uint8_t num, uint32_t data, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDOUT > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	while (num)
	{
		if ( apiCommandResult_FAILURE > apfelWriteBit( (data >> ( num -1 ) ) & 0x1 , portAddress, ss, pinCLK, pinDOUT, pinSS))
		{
			return apiCommandResult_FAILURE_QUIET;
		}
		num--;
	}

	return apiCommandResult_SUCCESS_QUIET;
}

//mandatory header to all command sequences
apiCommandResult apfelStartStreamHeader(uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDOUT > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	uint8_t mask = (1 << pinCLK | 1 << pinDOUT | 1 << pinSS);

	ss  = ss?1:0;

	if ( apiCommandResult_FAILURE > apfelClearDataInput(portAddress, ss, pinCLK, pinDOUT, pinSS)){ return apiCommandResult_FAILURE_QUIET; }

	//clock Low + data low
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0 << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock Low  + data high
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 1 << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock High + data high
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(1 << pinCLK | 1 << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock low + data high
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 1 << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }
	//# clock low  + data low
	if ( false == apfelSetClockAndDataLine( portAddress, ((uint8_t)(0 << pinCLK | 0 << pinDOUT | ss << pinSS)), mask)) { return apiCommandResult_FAILURE_QUIET; }

	return apiCommandResult_SUCCESS_QUIET;
}

/*
 * High Level commands
 */
apiCommandResult apfelCommandSet(uint16_t command, uint16_t value, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pinCLK > 7 || pinDOUT > 7 || pinSS > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	apfelStartStreamHeader(portAddress, ss, pinCLK, pinDOUT, pinSS);
	//# command + ...
	apfelWriteBitSequence( APFEL_N_COMMAND_BITS, command, portAddress, ss, pinCLK, pinDOUT, pinSS);
	// value
	apfelWriteBitSequence( APFEL_N_VALUE_BITS, value, portAddress, ss, pinCLK, pinDOUT, pinSS);
	//# chipId
	apfelWriteBitSequence( APFEL_N_CHIP_ID_BITS, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

	return apiCommandResult_SUCCESS_QUIET;
}


//#setDac dacNr[1..4] value[ 0 ... 3FF ] chipID[0 ... FF]
apiCommandResult apfelSetDac(uint16_t value, uint8_t dacNr, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if ( 1 > dacNr || dacNr > 4 )
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	if (value > (0x1 << APFEL_N_VALUE_BITS) -1 )
	{
		return apiCommandResult_FAILURE_QUIET;
	}

    apfelCommandSet(APFEL_COMMAND_SET_DAC + dacNr, value, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

	//#3 intermediate clock cycles equiv. 3 writeDataLow
	apfelWriteClockSequence(3, portAddress, ss, pinCLK, pinDOUT, pinSS);

	return apiCommandResult_SUCCESS_QUIET;
}

//#readDac dacNr[1..4] chipID[0 ... FF]
apiCommandResult apfelReadDac(uint32_t* dacValue, uint8_t dacNr, uint8_t chipID,  uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS, uint8_t pinDIN)
{
	if ( NULL == dacValue )
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	if (pinDIN > 7)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	if ( 1 > dacNr || dacNr > 4 )
	{
		return apiCommandResult_FAILURE_QUIET;
	}

    apfelCommandSet(APFEL_COMMAND_READ_DAC + dacNr, 0, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

	apfelReadBitSequence( 2 + APFEL_N_VALUE_BITS + 3, dacValue, portAddress, ss, pinDIN, pinCLK, pinSS);
    //cut out real dacValue
	*dacValue = (*dacValue & ((0x1 << APFEL_N_VALUE_BITS) -1)) >> 3;

	return apiCommandResult_SUCCESS_QUIET;
}

//#autoCalibration chipId[0 ... FF]
apiCommandResult apfelAutoCalibration(uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{

    apfelCommandSet(APFEL_COMMAND_AUTOCALIBRATION, 0, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

    //#calibration sequence clocks 4 * 10bit DAC
    apfelWriteClockSequence( (1 << APFEL_N_VALUE_BITS) << 2, portAddress, ss, pinCLK, pinDOUT, pinSS);

	return apiCommandResult_SUCCESS_QUIET;
}

//#testPulseSequence pulseHeightPattern[0 ... 1F](<<1/5) chipId[0 ... FF]
apiCommandResult apfelTestPulseSequence(uint16_t pulseHeightPattern, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (	(((pulseHeightPattern >> 1) & ((0x1 << 5)-1)) > APFEL_TEST_PULSE_HEIGHT_PATTERN_MAX) ||
		    (((pulseHeightPattern >> 5) & ((0x1 << 5)-1)) > APFEL_TEST_PULSE_HEIGHT_PATTERN_MAX))
	{
		return apiCommandResult_FAILURE_QUIET;
	}

    apfelCommandSet(APFEL_COMMAND_TESTPULSE, pulseHeightPattern, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

    apfelWriteClockSequence( 1, portAddress, ss, pinCLK, pinDOUT, pinSS);

	return apiCommandResult_SUCCESS_QUIET;
}

//#testPulse pulseHeight[0 ... 1F] channel[1 .. 2 ] chipId[0 ... FF]
apiCommandResult apfelTestPulse(uint8_t pulseHeight, uint8_t channel, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (pulseHeight > APFEL_TEST_PULSE_HEIGHT_PATTERN_MAX)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	if (1 > channel || channel > 2)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

	apfelTestPulseSequence( pulseHeight << (1 + (channel == 1)?0:5), chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);
	apfelTestPulseSequence( 0                                      , chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);

	return apiCommandResult_SUCCESS_QUIET;
}

//#setAmplitude channelId[1 ... 2] chipId[0 ... FF]
apiCommandResult apfelSetAmplitude(uint8_t channelId, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (1 > channelId || channelId > 2)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

    return apfelCommandSet(APFEL_COMMAND_SET_AMPLITUDE + 2 - channelId, 0, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);
}

//#resetAmplitude channelId[1 ... 2] chipId[0 ... FF]
apiCommandResult apfelResetAmplitude(uint8_t channelId, uint8_t chipID, uint8_t portAddress, uint8_t ss, uint8_t pinCLK, uint8_t pinDOUT, uint8_t pinSS)
{
	if (1 > channelId || channelId > 2)
	{
		return apiCommandResult_FAILURE_QUIET;
	}

    return apfelCommandSet(APFEL_COMMAND_RESET_AMPLITUDE + ((channelId -1)*2), 0, chipID, portAddress, ss, pinCLK, pinDOUT, pinSS);
}
