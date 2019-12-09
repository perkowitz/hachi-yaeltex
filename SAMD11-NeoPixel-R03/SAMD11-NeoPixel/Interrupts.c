#include "ytxHeader.h"

/*edbg_usart handler*/
void RX_Handler(void){
	uint8_t rcvByte = 0;
	
	if(SERCOM2->USART.INTFLAG.bit.RXC){			// if RX interrupt flag is set
		rcvByte = SERCOM2->USART.DATA.reg;		// get data from register
		SERCOM2->USART.INTFLAG.bit.RXC = 0;						// clear interrupt flag
				
		if (rcvByte == CMD_ALL_LEDS_OFF && !receivingLEDdata)	{
			setAll(0,0,0);
			pixelsShow();
		}else if (rcvByte == INIT_VALUES && !receivingInit){
			receivingInit = true;	
		}else if (receivingInit){
			rx_buffer[rxArrayIndex++] = rcvByte;
			if (rxArrayIndex == INIT_ENDOFFRAME){
				rxArrayIndex = 0;
				rcvdInitValues = true;
				receivingInit = false;
			}else if(SERCOM2->USART.INTFLAG.bit.DRE){
				SERCOM2->USART.DATA.reg = rcvByte;
			}
		}else if(rcvByte == NEW_DATA_BYTE && rxArrayIndex != 0){	// Si llega un dato valido, resetea, corregir
			rxArrayIndex = 0;
			onGoingFrame = true;
		}else if(rcvByte == 255 && ((rxArrayIndex+1) == rx_buffer[msgLength])){
			rxArrayIndex = 0;
			onGoingFrame = false;

			uint16_t checkSumCalc = 2019 - checkSum(rx_buffer, B+1);

			uint16_t checkSumRecv = (uint16_t) (rx_buffer[checkSum_MSB]<<8) | 
									(uint16_t)  rx_buffer[checkSum_LSB];

			//uint8_t crcCalc = CRC8(rx_buffer, B+1);

			//if(checkSumCalc == checkSumRecv && crcCalc == rx_buffer[CRC]){
			if(checkSumCalc == checkSumRecv){
				if(rx_buffer[frameType] == ENCODER_CHANGE_FRAME){
					ringBuffer[writeIdx].ringN		= rx_buffer[nRing];
					ringBuffer[writeIdx].ringState	= rx_buffer[ringStateH] << 8 | rx_buffer[ringStateL];
					ringBuffer[writeIdx].ringR		= rx_buffer[R];
					ringBuffer[writeIdx].ringG		= rx_buffer[G];
					ringBuffer[writeIdx].ringB		= rx_buffer[B];	
				}

				if(++writeIdx >= RING_BUFFER_LENGTH)	writeIdx = 0;
				receivingLEDdata = false;
				if(SERCOM2->USART.INTFLAG.bit.DRE){
					SERCOM2->USART.DATA.reg = rx_buffer[nRing];
				}	
			}
		}else {
			rx_buffer[rxArrayIndex++] = rcvByte;
			receivingLEDdata = true;
		}		
	}
}

/** Handler for the device SysTick module, called when the SysTick counter
*  reaches the set period.
*
*  \note As this is a raw device interrupt, the function name is significant
*        and must not be altered to ensure it is hooked into the device's
*        vector table.
*/
void SysTick_Handler(void)
{
	//if(blinkLED){
		//if(--ticks == 0 && blinkLEDcount){
		//if(--ticks == 0){
			//ticks = LED_BLINK_TICKS;
			//port_pin_toggle_output_level(LED_YTX_PIN);
			//blinkLEDcount--;
		//}
		if(ledShow){
			if( --tickShow == 0){
				tickShow = LED_SHOW_TICKS;
				ledShow = false;
				pixelsShow();
			}
		}
	//}

}