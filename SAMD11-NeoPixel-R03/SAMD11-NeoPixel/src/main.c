// References
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// SAM D21 SERCOM SPI Configuration
// http://ww1.microchip.com/downloads/en/AppNotes/00002465A.pdf
// https://github.com/jeelabs/embello/blob/master/explore/1450-dips/leds/main.cpp
// https://github.com/rogerclarkmelbourne/WS2812B_STM32_Libmaple
// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/

#include <asf.h>
#include <math.h>
#include <NeoPixels.h>

#include <ytxHeader.h>


/*! \brief Encode System Exclusive messages.
 SysEx messages are encoded to guarantee transmission of data bytes higher than
 127 without breaking the MIDI protocol. Use this static method to convert the
 data you want to send.
 \param inData The data to encode.
 \param outSysEx The output buffer where to store the encoded message.
 \param inLength The length of the input buffer.
 \return The length of the encoded output buffer.
 @see decodeSysEx
 Code inspired from Ruin & Wesen's SysEx encoder/decoder - http://ruinwesen.com
 */
uint8_t encodeSysEx(uint8_t* inData, uint8_t* outSysEx, uint8_t inLength)
{
    uint8_t outLength  = 0;     // Num bytes in output array.
    uint8_t count          = 0;     // Num 7bytes in a block.
    outSysEx[0]         = 0;

    for (unsigned i = 0; i < inLength; ++i)
    {
        const uint8_t data = inData[i];
        const uint8_t msb  = data >> 7;
        const uint8_t body = data & 0x7f;

        outSysEx[0] |= (msb << count);
        outSysEx[1 + count] = body;

        if (count++ == 6)
        {
            outSysEx   += 8;
            outLength  += 8;
            outSysEx[0] = 0;
            count       = 0;
        }
    }
    return outLength + count + (count != 0 ? 1 : 0);
}

/*! \brief Decode System Exclusive messages.
 SysEx messages are encoded to guarantee transmission of data bytes higher than
 127 without breaking the MIDI protocol. Use this static method to reassemble
 your received message.
 \param inSysEx The SysEx data received from MIDI in.
 \param outData    The output buffer where to store the decrypted message.
 \param inLength The length of the input buffer.
 \return The length of the output buffer.
 @see encodeSysEx @see getSysExArrayLength
 Code inspired from Ruin & Wesen's SysEx encoder/decoder - http://ruinwesen.com
 */
uint8_t decodeSysEx(uint8_t* inSysEx, uint8_t* outData, uint8_t inLength)
{
    uint8_t count  = 0;
    uint8_t msbStorage = 0;

    for (unsigned i = 0; i < inLength; ++i)
    {
        if ((i % 8) == 0)
        {
            msbStorage = inSysEx[i];
        }
        else
        {
            outData[count++] = inSysEx[i] | ((msbStorage & 1) << 7);
            msbStorage >>= 1;
        }
    }
    return count;
}

//CRC-8 - algoritmo basato sulle formule di CRC-8 di Dallas/Maxim
//codice pubblicato sotto licenza GNU GPL 3.0
uint8_t CRC8(const uint8_t *data, uint8_t len)
{
	uint8_t crc = 0x00;
	while (len--) {
		uint8_t extract = *data++;
		for (uint8_t tempI = 8; tempI; tempI--)
		{
			uint8_t sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum) {
				crc ^= 0x8C;
			}
			extract >>= 1;
		}
	}
	return crc;
}

uint16_t checkSum(const uint8_t *data, uint8_t len)
{
	uint16_t sum = 0x00;
	for (uint8_t i = 0; i < len; i++)
	sum ^= data[i];

	return sum;
}

void SendToMaster(uint8_t command){
	if(SERCOM2->USART.INTFLAG.bit.DRE){
		SERCOM2->USART.DATA.reg = command;
		return 1;
	}else{
		return 0;
	}
}

/*edbg_usart handler*/
void RX_Handler(void){
	uint8_t rcvByte = 0;
	
	if(SERCOM2->USART.INTFLAG.bit.RXC){			// if RX interrupt flag is set
		rcvByte = SERCOM2->USART.DATA.reg;		// get data from register
		SERCOM2->USART.INTFLAG.bit.RXC = 0;						// clear interrupt flag
		
		if (rcvByte == CMD_ALL_LEDS_OFF && !receivingLEDdata)	{		// TURN ALL LEDS OFF COMMAND
			turnAllOffFlag = true;
		}else if (rcvByte == CHANGE_BRIGHTNESS && !receivingLEDdata && !receivingBrightness)	{	// CHANGE BRIGHTNESS COMMAND
			receivingBrightness = true;
		}else if (receivingBrightness){										// CHANGE BRIGHTNESS COMMAND - BYTE 2 - NEW BRIGHTNESS
			receivingBrightness = false;	
			currentBrightness = rcvByte;
			changeBrightnessFlag = true;
		}else if (rcvByte == BANK_INIT && !receivingBank && !receivingLEDdata){			// BANK INIT COMMAND
			receivingBank = true;
		}else if (rcvByte == BANK_END && receivingBank && receivingLEDdata){			// BANK END COMMAND
			receivingBank = false;
			receivingLEDdata = false;
			updateBank = true;
			ledShow = true;
		}else if (rcvByte == INIT_VALUES && !receivingInit && !rcvdInitValues){	// INIT VALUES COMMAND
			receivingInit = true;
			rxArrayIndex = 0;
		}else if (receivingInit  && !rcvdInitValues){							// INIT VALUES BYTES
			rx_bufferEnc[rxArrayIndex++] = rcvByte;
			if (rxArrayIndex == INIT_ENDOFFRAME){
				rxArrayIndex = 0;
				rcvdInitValues = true;
				receivingInit = false;
			}
		}else if(rcvByte == NEW_FRAME_BYTE){	// FIRST BYTE OF A DATA FRAME
			rxArrayIndex = 0;
			receivingLEDdata = true;
		}else if(rcvByte == END_OF_FRAME_BYTE && receivingLEDdata){		// LAST BYTE OF A DATA FRAME
			
			// checksum to encoded frame, from 2nd byte, and length is total length without length and checksum bytes (msb and lsb)
			uint16_t checkSumCalc = (2019 - checkSum(&rx_bufferEnc[e_fill1], rx_bufferEnc[e_msgLength]-3)) & 0x3FFF;	

			uint16_t checkSumRecv = (uint16_t) (rx_bufferEnc[e_checkSum_MSB]<<7) | (uint16_t)  rx_bufferEnc[e_checkSum_LSB];
			
			//uint8_t crcCalc = CRC8(rx_bufferEnc, B+1);

			//if(checkSumCalc == checkSumRecv && crcCalc == rx_bufferEnc[CRC]){
			if(checkSumCalc == checkSumRecv){
				SendToMaster(rx_bufferEnc[e_checkSum_LSB]); // ack is checksum LSB byte
				
				// length and checksum MSB,LSB are not encoded
				uint8_t decodedFrameSize = decodeSysEx(&rx_bufferEnc[e_fill1], rx_bufferDec, rx_bufferEnc[e_msgLength]-3);	
				
				ringBuffer[writeIdx].updateStrip	= rx_bufferDec[d_frameType];
				if(	ringBuffer[writeIdx].updateStrip == ENCODER_CHANGE_FRAME || 
					ringBuffer[writeIdx].updateStrip == ENCODER_SWITCH_CHANGE_FRAME){
					ringBuffer[writeIdx].updateN		=	rx_bufferDec[d_nRing];
					ringBuffer[writeIdx].updateO		=	rx_bufferDec[d_orientation];
					ringBuffer[writeIdx].updateState	=	rx_bufferDec[d_ringStateH] << 8 |
															rx_bufferDec[d_ringStateL];	
				}else if(	ringBuffer[writeIdx].updateStrip == DIGITAL1_CHANGE_FRAME || 
							ringBuffer[writeIdx].updateStrip == DIGITAL2_CHANGE_FRAME){
					ringBuffer[writeIdx].updateN		= rx_bufferDec[d_nDigital];
					ringBuffer[writeIdx].updateState	= rx_bufferDec[d_digitalState];		
				}
				ringBuffer[writeIdx].updateValue		= rx_bufferDec[d_currentValue];
				ringBuffer[writeIdx].updateMin			= rx_bufferDec[d_minVal];
				ringBuffer[writeIdx].updateMax			= rx_bufferDec[d_maxVal];
				ringBuffer[writeIdx].updateR			= rx_bufferDec[d_R];
				ringBuffer[writeIdx].updateG			= rx_bufferDec[d_G];
				ringBuffer[writeIdx].updateB			= rx_bufferDec[d_B];

				if(++writeIdx >= RING_BUFFER_LENGTH)	writeIdx = 0;
				
				if(!receivingBank) receivingLEDdata = false;
				
			}
		}else if(receivingLEDdata){
			//if (rxArrayIndex == frameType)
			//{
				//while(0);
			//}
			rx_bufferEnc[rxArrayIndex++] = rcvByte;
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
	if( --tickShow == 0){
		tickShow = LED_SHOW_TICKS;
		if(ledShow && !receivingLEDdata){			
			ledShow = false;
			if(!updateBank){
				switch(whichStripToShow){
					case ENCODER_CHANGE_FRAME:
					case ENCODER_SWITCH_CHANGE_FRAME:{
						if(indexChanged < 16)
							pixelsShow(ENCODER1_STRIP);
						else
							pixelsShow(ENCODER2_STRIP);
					}
					break;
					case DIGITAL1_CHANGE_FRAME:{
						pixelsShow(DIGITAL1_STRIP);
					}
					break;
					case DIGITAL2_CHANGE_FRAME:{
						pixelsShow(DIGITAL2_STRIP);
					}
					break;
					case FB_CHANGE_FRAME:{
						pixelsShow(FB_STRIP);
					}
					break;
					default:break;
				}
				whichStripToShow = 0;	
			}else{
				updateBank = false;
				pixelsShow(ENCODER1_STRIP);
				pixelsShow(ENCODER2_STRIP);
				pixelsShow(DIGITAL1_STRIP);
				pixelsShow(DIGITAL2_STRIP);
				//pixelsShow(FB_STRIP);								
			}
		}
	}
	//}

}

//! [setup]
void configure_usart(void)
{
	uint8_t instance_index;
	//! [setup_config]
	struct usart_config config_usart;
	//! [setup_config]
	//! [setup_config_defaults]
	usart_get_config_defaults(&config_usart);
	//! [setup_config_defaults]

	//! [setup_change_config]
	config_usart.baudrate    = BAUD_RATE;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;

	//! [setup_change_config]
	//usart_instance.hw->USART.CTRLA.bit.RXPO = 0x3;
	//usart_instance.hw->USART.CTRLA.bit.TXPO = 0x0;
	//usart_instance.hw->USART.CTRLB.bit.RXEN = true;
	//! [setup_set_config]
	while (usart_init(&usart_instance,
		SERCOM2, &config_usart) != STATUS_OK) {
	}
	
	// Interrupt UART CONFIG from https://github.com/avrxml/asf/blob/master/thirdparty/freertos/demo/oled1_xpro_example/cdc.h
	
	//! [setup_set_config]
	/* SERCOM2 handler enabled */
	NVIC_SetPriority(SERCOM2_IRQn, 0);
	//system_interrupt_enable(SERCOM2_IRQn);
	
	// Inject our own interrupt handler
	instance_index = _sercom_get_sercom_inst_index(EDBG_CDC_MODULE);
	_sercom_set_handler(instance_index, RX_Handler);
	
	//! [setup_enable]
	usart_enable(&usart_instance);
	// Enable the UART transceiver
	//usart_enable_transceiver(&usart_instance, USART_TRANSCEIVER_TX);
	//usart_enable_transceiver(&usart_instance, USART_TRANSCEIVER_RX);
	//! [setup_enable]
	/* receive complete interrupt set */
	SERCOM2->USART.INTENSET.reg = SERCOM_USART_INTFLAG_RXC;
}


/** Configure LED0, turn it off*/
static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);

	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_YTX_PIN, &pin_conf);
	port_pin_set_output_level(LED_YTX_PIN, LED_0_INACTIVE);
}

long mapl(long x, long in_min, long in_max, long out_min, long out_max){
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void UpdateLEDs(uint8_t nStrip, uint8_t nToChange, uint8_t newValue, uint8_t min, uint8_t max,
				bool vertical,  uint16_t newState, uint8_t intR, uint8_t intG, uint8_t intB) {
	uint8_t brightnessMult = 1;
	uint8_t minMaxDif = abs(max-min);
	int8_t lastLedOn = 0;
	
	if(nStrip == ENCODER_CHANGE_FRAME){		// ROTARY CHANGE
		bool ledOnOrOff = false;
		bool ledForSwitch = false;
		
		for (int i = 0; i < 16; i++) {
			ledOnOrOff = newState&(1<<i);
			if(vertical){
				ledOnOrOff &= ((ENCODER_MASK_V>>i)&1);
				ledForSwitch = ((ENCODER_SWITCH_V_ON>>i)&1);
			}
			else{
				ledOnOrOff &= ((ENCODER_MASK_H>>i)&1);
				ledForSwitch = ((ENCODER_SWITCH_H_ON>>i)&1);
			}
				
			if (ledOnOrOff && !ledForSwitch) {
				if(nToChange < 16){
					setPixelColor(ENCODER1_STRIP, 16*nToChange + i , intR, intG, intB); // Draw new pixel
					if(!vertical && i != 13){
						setPixelColor(ENCODER1_STRIP, 16*nToChange + i + 1 , intR/brightnessMult, intG/brightnessMult, intB/brightnessMult); // Draw new pixel
					}
				}else{
					setPixelColor(ENCODER2_STRIP, 16*(nToChange-16) + i , intR, intG, intB); // Draw new pixel- 16 LEDs each ring, 16 encoders on the first strip
				}
				lastLedOn = i;
			} else if(ledForSwitch){
				// IF IT IS A LED FOR THE SWITCH, DO NOTHING
			} else {
				if(nToChange < 16){
					setPixelColor(ENCODER1_STRIP, 16*nToChange + i , NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
					//if(!vertical && i != 13 && lastLedOn >= 0){
						//if(minMaxDif > 48){
							//brightnessMult = (minMaxDif/13) + 1 - abs(newValue - min)%(minMaxDif/13);	
						//}
						//setPixelColor(ENCODER1_STRIP, 16*nToChange + lastLedOn + 1 , intR/brightnessMult, intG/brightnessMult, intB/brightnessMult); // Draw new pixel
					//}
				}else{
					setPixelColor(ENCODER2_STRIP, 16*(nToChange-16) + i , NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
				}
			}
		}
	}else if(nStrip == ENCODER_SWITCH_CHANGE_FRAME){		// SWITCH CHANGE
		bool ledOnOrOff = 0;
		bool ledForRing = false;
		for (int i = 0; i < 16; i++) {
			ledOnOrOff = newState&(1<<i);
			if(vertical){
				ledOnOrOff &= ((ENCODER_SWITCH_V_ON>>i)&1);		// Check if we are at a switch LED
				ledForRing = ((ENCODER_MASK_V>>i)&1);			// or a ring LED
			}
			else{
				ledOnOrOff &= ((ENCODER_SWITCH_H_ON>>i)&1);
				ledForRing = ((ENCODER_MASK_H>>i)&1);
			}
				
			if (ledOnOrOff && !ledForRing) {		// If it is a SWITCH LED and it's supposed to be ON
				if(nToChange < 16){
					setPixelColor(ENCODER1_STRIP, 16*nToChange + i , intR, intG, intB); // Draw new pixel
				}else{
					setPixelColor(ENCODER2_STRIP, 16*(nToChange-16) + i , intR, intG, intB); // Draw new pixel
				}
			} else if(ledForRing){
				// IF IT IS A LED FOR THE RING, DO NOTHING
			} else {
				if(nToChange < 16){
					setPixelColor(ENCODER1_STRIP, 16*nToChange + i , NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
				}else{
					setPixelColor(ENCODER2_STRIP, 16*(nToChange-16) + i , NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
				}
			}
		}
	}else if(nStrip == DIGITAL1_CHANGE_FRAME){
		if (newState){
			setPixelColor(DIGITAL1_STRIP, nToChange, intR, intG, intB); // Draw new pixel
		}else{
			setPixelColor(DIGITAL1_STRIP, nToChange, NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
		}
	}else if(nStrip == DIGITAL2_CHANGE_FRAME){
		if (newState){
			setPixelColor(DIGITAL2_STRIP, (nToChange-numDigitals1), intR, intG, intB); // Draw new pixel
		}else{
			setPixelColor(DIGITAL2_STRIP, (nToChange-numDigitals1), NP_OFF, NP_OFF, NP_OFF); // Draw new pixel
		}
	}
}

// PA22
int main (void)
{
	system_init();
	delay_init();
	config_led();
	configure_usart();
	//configure_usart_callbacks();

	/*Configure system tick to generate periodic interrupts */
	SysTick_Config(ONE_SEC/1000);

	/* Enable Interrupts */
	void __enable_irq(void);
	
	for(int i = 0; i < 2; i++){
		port_pin_set_output_level(LED_YTX_PIN, LED_0_ACTIVE);
		delay(50);
		port_pin_set_output_level(LED_YTX_PIN, LED_0_INACTIVE);
		delay(50);	
	}
	
	port_pin_set_output_level(LED_YTX_PIN, LED_0_ACTIVE);
	while(!rcvdInitValues);
	numEncoders = rx_bufferEnc[nEncoders];
	numDigitals1 = rx_bufferEnc[nDigitals1];
	numDigitals2 = rx_bufferEnc[nDigitals2];
	numAnalogFb = rx_bufferEnc[nAnalog];
	currentBrightness = rx_bufferEnc[nBrightness];
	bool rainbowOn = rx_bufferEnc[nRainbow];
	port_pin_set_output_level(LED_YTX_PIN, LED_0_INACTIVE);
	//numEncoders = 8;
	//numDigitals1 = 16;
	//numDigitals2 = 12;
	//numAnalogFb = 0;
	//currentBrightness = 30;
	
	if(numEncoders){
		if(numEncoders>16){
			pixelsBegin(ENCODER1_STRIP, NUM_LEDS_ENCODER*16, ENC1_STRIP_PIN, NEO_GRB + NEO_KHZ800);
			pixelsBegin(ENCODER2_STRIP, NUM_LEDS_ENCODER*(numEncoders-16), ENC2_STRIP_PIN, NEO_GRB + NEO_KHZ800);
		}else{
			pixelsBegin(ENCODER1_STRIP, NUM_LEDS_ENCODER*numEncoders, ENC1_STRIP_PIN, NEO_GRB + NEO_KHZ800);
		}
	}
	if(numDigitals1){
		pixelsBegin(DIGITAL1_STRIP, numDigitals1, DIG1_STRIP_PIN, NEO_GRB + NEO_KHZ800);
	}
	if(numDigitals2){
		pixelsBegin(DIGITAL2_STRIP, numDigitals2, DIG2_STRIP_PIN, NEO_GRB + NEO_KHZ800);
	}
	if(numAnalogFb){
		pixelsBegin(FB_STRIP, numAnalogFb, FB_STRIP_PIN, NEO_GRB + NEO_KHZ800);
	}
	
	for(int s = 0; s < MAX_STRIPS; s++){
		if(begun[s]){
			setBrightness(s, currentBrightness);
		}
	}
	setAll(NP_OFF,NP_OFF,NP_OFF);
	
	if(rainbowOn) rainbowAll(4);

	//fadeAllTo(NP_OFF, 2);
		
	SendToMaster(END_OF_RAINBOW);
	int i = 0;
	while (1) {		
		if(readIdx != writeIdx){
			UpdateLEDs(	ringBuffer[readIdx].updateStrip,
						ringBuffer[readIdx].updateN,
						ringBuffer[readIdx].updateValue,
						ringBuffer[readIdx].updateMin,
						ringBuffer[readIdx].updateMax,
						ringBuffer[readIdx].updateO, 
						ringBuffer[readIdx].updateState, 
						ringBuffer[readIdx].updateR,
						ringBuffer[readIdx].updateG,
						ringBuffer[readIdx].updateB);
			
			indexChanged = ringBuffer[readIdx].updateN;
			ledShow = true;
			whichStripToShow = ringBuffer[readIdx].updateStrip;
			
			if(++readIdx >= RING_BUFFER_LENGTH)	readIdx = 0;
		}
		
		if(changeBrightnessFlag){
			changeBrightnessFlag = false;
			for(int s = 0; s < MAX_STRIPS; s++){
				if(begun[s]){
					setBrightness(s, currentBrightness);
					pixelsShow(s);
				}
			}
		}
		
		if(turnAllOffFlag){
			turnAllOffFlag = false;
			setAll(NP_OFF,NP_OFF,NP_OFF);
			for(int s = 0; s < MAX_STRIPS; s++){
				if(begun[s]){
					pixelsShow(s);
				}
			}
		}
	}
}
