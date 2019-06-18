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

#define NUM_LEDS		128
#define ENC1_LED_PIN	PIN_PA15
// 	system_gclk_gen_get_hz(GCLK_GENERATOR_1) -> 32768 KHz
//	32768*366 = ~12 MHz
//	48 MHz / 12MHz = 4 veces por segundo entra a la interrupcion => 250ms
#define ONE_SEC	system_gclk_gen_get_hz(GCLK_GENERATOR_0)/1000
#define ONE_SEC_TICKS			1000
#define QUARTER_SEC_TICKS		250

#define LED_BLINK_TICKS	ONE_SEC_TICKS/2

//#define ONE_SEC	48000000

volatile uint32_t ticks = LED_BLINK_TICKS;
bool activeRainbow = false;
bool ledsUpdateOk = true;

typedef enum SerialBytes{
	msgLength = 0, nRing, ringStateH, ringStateL, R, G, B, checkSum_MSB, checkSum_LSB, CRC, ENDOFFRAME
};

void usart_read_callback(struct usart_module *const usart_module);
void usart_write_callback(struct usart_module *const usart_module);

void configure_usart(void);
void configure_usart_callbacks(void);

//! [module_inst]
struct usart_module usart_instance;
//! [module_inst]

//! [rx_buffer_var]
#define MAX_RX_BUFFER_LENGTH   11
uint8_t rxBufferIndex = 0;
bool rxComplete = false;

bool blinkLED = false;
uint8_t blinkLEDcount = 0;

volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];
volatile uint8_t rx_data;
//! [rx_buffer_var]

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

//! [callback_funcs]
void usart_read_callback(struct usart_module *const usart_module)
{
	
	rx_buffer[rxBufferIndex] = rx_data;
	
	if(rx_data == 255 && ((rxBufferIndex+1) >= rx_buffer[msgLength]) && ledsUpdateOk){
		rxBufferIndex = 0;

		uint16_t checkSumCalc = 2019 - checkSum(rx_buffer, B+1);

		uint16_t checkSumRecv = (uint16_t) (rx_buffer[checkSum_MSB]<<8) | 
								(uint16_t)  rx_buffer[checkSum_LSB];

		uint8_t crcCalc = CRC8(rx_buffer, B+1);

		//if(checkSumCalc == checkSumRecv && crcCalc == rx_buffer[CRC]){
		if(checkSumCalc == checkSumRecv){
			rxComplete = true;
			ledsUpdateOk = false;
		}
	}else{
		rxBufferIndex++;
	}
	
}

void usart_write_callback(struct usart_module *const usart_module)
{
	//port_pin_set_output_level(LED_0_PIN, 0);
	//port_pin_toggle_output_level(LED_0_PIN);
}
//! [callback_funcs]


//! [setup]
void configure_usart(void)
{
	//! [setup_config]
	struct usart_config config_usart;
	//! [setup_config]
	//! [setup_config_defaults]
	usart_get_config_defaults(&config_usart);
	//! [setup_config_defaults]

	//! [setup_change_config]
	config_usart.baudrate    = 250000;
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
		EDBG_CDC_MODULE, &config_usart) != STATUS_OK) {
	}
	//! [setup_set_config]
	
	//! [setup_enable]
	usart_enable(&usart_instance);
	//! [setup_enable]
}

void configure_usart_callbacks(void)
{
	//! [setup_register_callbacks]
	usart_register_callback(&usart_instance,
	usart_write_callback, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
	usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);
	//! [setup_register_callbacks]

	//! [setup_enable_callbacks]
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
	//! [setup_enable_callbacks]
}
//! [setup]



/** Configure LED0, turn it off*/
static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);

	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
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
		if(--ticks == 0){
			ticks = LED_BLINK_TICKS;
			port_pin_toggle_output_level(LED_0_PIN);
			blinkLEDcount--;
		}
	//}

}


void UpdateLEDs(uint8_t nLedRing, uint8_t ringH, uint8_t ringL, uint8_t intR, uint8_t intG, uint8_t intB) {
	uint16_t ringState = ringH << 8 | ringL;
	
	for (int i = 0; i < 16; i++) {
		if (ringState&(1<<i)) {
			setPixelColor( 16*nLedRing + i , intR, intG, intB); // Draw new pixel
			//encoderLEDStrip1.setPixelColor( 16 * spiBuffer[nRing] + i , encoderLEDStrip1.Color( 127, 0, 0)); // Draw new pixel
		} else {
			setPixelColor( 16*nLedRing + i , 0, 0, 0); // Draw new pixel
		}
	}
	pixelsShow();
	
}

// PA22
int main (void)
{
	system_init();
	delay_init();
	config_led();
	configure_usart();
	configure_usart_callbacks();

	pixelsBegin(NUM_LEDS, ENC1_LED_PIN, NEO_GRB + NEO_KHZ800);
	
	
	/*Configure system tick to generate periodic interrupts */
	SysTick_Config(ONE_SEC);

	blinkLED = true;
	blinkLEDcount = 4;

	//	while(1);
	setBrightness(40);
	setAll(0,0,0);
	pixelsShow();
	delay(200);
	
	while (1) {
		//usart_read_job(&usart_instance, (uint16_t *)&rx_data);
		//if(rxComplete){
			//rxComplete = false;
			//port_pin_toggle_output_level(LED_0_PIN);
			//UpdateLEDs(rx_buffer[nRing], rx_buffer[ringStateH], rx_buffer[ringStateL], rx_buffer[R], rx_buffer[G], rx_buffer[B]);
			//ledsUpdateOk = true;
		//}
		rainbow(10);
		//if(activeRainbow){
			//rainbow(10);
		//}
		//meteorRain(0xf0,0x00,0xf0,1, 200, true, 40);
		//port_pin_toggle_output_level(LED_0_PIN);
		//setPixelColor(0,128,0,0);
		//pixelsShow();
		//delay(1000);
		//port_pin_toggle_output_level(LED_0_PIN);
		//setPixelColor(0,0,128,0);
		//pixelsShow();
		//delay(1000);
		// 		showStrip();
		// 		delay(5);
		//FadeInOut(0xff, 0x77, 0x00);
		//RGBLoop();
		// 		        cometRacer(1, 0, 0);    // red
		// 		        cometRacer(0, 1, 0);    // green
		// 		        cometRacer(0, 0, 1);    // blue

	}
}
