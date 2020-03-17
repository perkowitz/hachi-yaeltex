/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#include <extEEPROM.h>
#include <MIDI.h>
#include <midi_Defs.h>
#include <Adafruit_NeoPixel.h>

#include <midi_UsbTransport.h>
#include "board_definitions.h"

#define STATUS_LED_BRIGHTNESS   64
#define STATUS_LED_PIN			26

static const unsigned sUsbTransportBufferSize = 256;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

struct MySettings : public midi::DefaultSettings
{
  static const bool Use1ByteParsing = false;
  static const unsigned SysExMaxSize = 256; // Accept SysEx messages up to 256 bytes long.
  static const bool UseRunningStatus = false; // My devices seem to be ok with it.
};

MIDI_CREATE_CUSTOM_INSTANCE(UsbTransport, sUsbTransport, MIDI, MySettings);

// Create a 'MIDI' object using MySettings bound to Serial1.
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDIHW, MySettings);

// STATUS LED
Adafruit_NeoPixel statusLED(1, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t off = statusLED.Color(0, 0, 0);
uint32_t red = statusLED.Color(STATUS_LED_BRIGHTNESS, 0, 0);
uint32_t green = statusLED.Color(0, STATUS_LED_BRIGHTNESS, 0);
uint32_t blue = statusLED.Color(0, 0, STATUS_LED_BRIGHTNESS);
uint32_t magenta = statusLED.Color(STATUS_LED_BRIGHTNESS/2, 0, STATUS_LED_BRIGHTNESS/2);
uint32_t cyan = statusLED.Color(0, STATUS_LED_BRIGHTNESS/2, STATUS_LED_BRIGHTNESS/2);
uint32_t yellow = statusLED.Color(STATUS_LED_BRIGHTNESS/2, STATUS_LED_BRIGHTNESS/2, 0);
uint32_t white = statusLED.Color(STATUS_LED_BRIGHTNESS/3, STATUS_LED_BRIGHTNESS/3, STATUS_LED_BRIGHTNESS/3);

uint32_t statusLEDColor[5] = {off, green, yellow, magenta, red};

extEEPROM eep(kbits_512, 1, 128);//device size, number of devices, page size


#define BOOT_SIGN_ADDR	3

extern uint32_t __sketch_vectors_ptr; // Exported value from linker script
extern void board_init(void);

volatile uint32_t* pulSketch_Start_Address;

#define SPM_PAGESIZE		64
#define PAGES_PER_BLOCK		1

uint8_t sysex_data[SPM_PAGESIZE*PAGES_PER_BLOCK]={0};

#define CMD_DATA_BLOCK_ACK    1
#define CMD_START_BOOTLOADER  2
#define CMD_MAIN_PROGRAM      3
#define CMD_BOOT_DATA_BLOCK   4
#define CMD_DATA_BLOCK_NAK    5

#define SYSEX_OMNI        0x7F
#define SYSEX_ID_1        0x60
#define SYSEX_ID_2        0x07

uint8_t ack_msg[] =
{
	 SYSEX_OMNI, SYSEX_ID_1, SYSEX_ID_2, CMD_DATA_BLOCK_ACK
};

uint8_t nak_msg[] =
{
	 SYSEX_OMNI, SYSEX_ID_1, SYSEX_ID_2, CMD_DATA_BLOCK_NAK
};

uint32_t PAGE_SIZE, PAGES, MAX_FLASH;

uint8_t *stream;
uint8_t sysex_cnt;
uint8_t pagesCnt;
volatile uint8_t rstFlg = 0;


uint32_t current_number;
uint32_t i, length;
uint8_t command, *ptr_data, *ptr;
uint8_t j;
uint32_t u32tmp;


uint32_t make_word(uint8_t idx, uint8_t cnt)
{
	uint8_t ptr = idx + cnt - 1;
	uint32_t ret = 0;
	for (; ptr >= idx; ptr--)
	{
		ret <<= 7;
		ret |= stream[ptr];
	}
	return ret;
}

uint8_t write_block(void)
{
	uint8_t checksum = 0;
	uint8_t i;
	for (i = 0; i < sysex_cnt - 1; i++)
	{
		checksum ^= stream[i];
	}

	uint32_t sysex_address = make_word(4, 4);

	if (sysex_address >= MAX_FLASH)
	{
		//sam_ba_putdata(ptr_monitor_if,ack_msg,sizeof(nak_msg));
		MIDI.sendSysEx(sizeof(nak_msg),nak_msg);
		return 0;
	}
	
	uint8_t cnt = 0, recvd = 0;
	uint8_t bits = 0;
	for (cnt = 0; cnt < (sysex_cnt - 8); cnt++)
	{
		if ((cnt % 8) == 0)
		{
			bits = stream[8 + cnt];
		}
		else
		{
			sysex_data[recvd++] = stream[8 + cnt] | ((bits & 1) << 7);
			bits >>= 1;
		}

		if (recvd >= SPM_PAGESIZE)
		break;
	}

	uint8_t check = stream[sysex_cnt - 1];
	checksum &= 0x7f;

	if ((checksum != check) || (recvd != SPM_PAGESIZE))
	{
		//sam_ba_putdata(ptr_monitor_if,ack_msg,sizeof(nak_msg));
		MIDI.sendSysEx(sizeof(nak_msg),nak_msg);
		return 0;
	}
	
	
	for (uint32_t pages=0; pages<PAGES_PER_BLOCK; pages++)
	{
		// Execute "ER" Erase Row
		NVMCTRL->ADDR.reg = sysex_address + pages*PAGE_SIZE;
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
		while (NVMCTRL->INTFLAG.bit.READY == 0);
		
		// Set automatic page write
		NVMCTRL->CTRLB.bit.MANW = 0;

		// Do writes in pages
		volatile uint32_t *dst_addr = (uint32_t *)(sysex_address + pages*PAGE_SIZE);
		volatile uint32_t *src_addr = (uint32_t *)(sysex_data + pages*PAGE_SIZE);
		
		// Execute "PBC" Page Buffer Clear
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
		while (NVMCTRL->INTFLAG.bit.READY == 0);
		
		// Fill page buffer
		
		for (int i=0; i<16; i++)
		{
			dst_addr[i] = src_addr[i];
		}
		/*
		dst_addr[0] = src_addr[0];
		dst_addr[1] = src_addr[1];
		dst_addr[2] = src_addr[2];
		dst_addr[3] = src_addr[3];
		dst_addr[4] = src_addr[4];
		dst_addr[5] = src_addr[5];
		dst_addr[6] = src_addr[6];
		dst_addr[7] = src_addr[7];
		dst_addr[8] = src_addr[8];
		dst_addr[9] = src_addr[9];
		dst_addr[10] = src_addr[10];
		dst_addr[11] = src_addr[11];
		dst_addr[12] = src_addr[12];
		dst_addr[13] = src_addr[13];
		dst_addr[14] = src_addr[14];
		dst_addr[15] = src_addr[15];
		
	
			
	// Fill page buffer
	for (uint32_t i=0; i<(PAGE_SIZE/4); i++)
	{
		uint32_t index = i + pages*(PAGE_SIZE/4);
		dst_addr[i] = src_addr[i];
	}
*/

		// Execute "WP" Write Page
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
		while (NVMCTRL->INTFLAG.bit.READY == 0);
	}
	
	
	MIDI.sendSysEx(sizeof(ack_msg),ack_msg);
	//sam_ba_putdata(ptr_monitor_if,ack_msg,sizeof(ack_msg));

	
	return 1;
}

void handleSystemExclusive(byte *message, unsigned size)
{
	uint8_t res = 0;
	
	uint8_t data[256];

	sysex_cnt = size - 2;
	
	memcpy(data,message,sysex_cnt);
	
	if ( sysex_cnt < 4)
	{
		return;
	}
	else
	{
		stream = message + 1;
			
		if (stream[3] == CMD_START_BOOTLOADER)
		{
			res = 1;
			pagesCnt = 0;
			MIDI.sendSysEx(sizeof(ack_msg),ack_msg);
			//sam_ba_putdata(ptr_monitor_if,ack_msg,sizeof(ack_msg));
		}
		else if (stream[3] == CMD_MAIN_PROGRAM)
		{
			rstFlg = 1;
				
			return;
		}
		else if (stream[3] == CMD_BOOT_DATA_BLOCK)
		{
			res = write_block();
			//animation
			if(res)
			{
					
			}
		}
	}
}

static void jump_to_application(void) {

  /* Rebase the Stack Pointer */
  __set_MSP( (uint32_t)(__sketch_vectors_ptr) );

  /* Rebase the vector table base address */
  SCB->VTOR = ((uint32_t)(&__sketch_vectors_ptr) & SCB_VTOR_TBLOFF_Msk);

  /* Jump to application Reset Handler in the application */
  asm("bx %0"::"r"(*pulSketch_Start_Address));
}

static volatile bool main_b_cdc_enable = false;

static volatile bool jump_to_app = false;

/**
 * \brief Check the application startup condition
 *
 */
static void check_start_application(void)
{
  /*
   * Test sketch stack pointer @ &__sketch_vectors_ptr
   * Stay in SAM-BA if value @ (&__sketch_vectors_ptr) == 0xFFFFFFFF (Erased flash cell value)
   */
  if (__sketch_vectors_ptr == 0xFFFFFFFF)
  {
    /* Stay in bootloader */
    return;
  }

  /*
   * Load the sketch Reset Handler address
   * __sketch_vectors_ptr is exported from linker script and point on first 32b word of sketch vector table
   * First 32b word is sketch stack
   * Second 32b word is sketch entry point: Reset_Handler()
   */
  pulSketch_Start_Address = &__sketch_vectors_ptr ;
  pulSketch_Start_Address++ ;

  /*
   * Test vector table address of sketch @ &__sketch_vectors_ptr
   * Stay in SAM-BA if this function is not aligned enough, ie not valid
   */
  if ( ((uint32_t)(&__sketch_vectors_ptr) & ~SCB_VTOR_TBLOFF_Msk) != 0x00)
  {
    /* Stay in bootloader */
    return;
  }

#if defined(BOOT_DOUBLE_TAP_ADDRESS)
  #define DOUBLE_TAP_MAGIC 0x07738135
  if (PM->RCAUSE.bit.POR)
  {
    /* On power-on initialize double-tap */
    BOOT_DOUBLE_TAP_DATA = 0;
  }
  else
  {
    if (BOOT_DOUBLE_TAP_DATA == DOUBLE_TAP_MAGIC)
    {
      /* Second tap, stay in bootloader */
      BOOT_DOUBLE_TAP_DATA = 0;
      return;
    }

    /* First tap */
    BOOT_DOUBLE_TAP_DATA = DOUBLE_TAP_MAGIC;

    /* Wait 0.5sec to see if the user tap reset again.
     * The loop value is based on SAMD21 default 1MHz clock @ reset.
     */
    delay(500);

    /* Timeout happened, continue boot... */
    BOOT_DOUBLE_TAP_DATA = 0;
  }
#endif

  jump_to_app = true;
}

void setup() {
    SerialUSB.begin(115200);
	
    //while (!Serial);
	check_start_application();
	
	/* Jump in application if condition is satisfied */

	if (jump_to_app) {
		jump_to_application();
	}
	
	uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
	PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ];
	PAGES = NVMCTRL->PARAM.bit.NVMP;
	MAX_FLASH = PAGE_SIZE * PAGES;
	
    // Begin MIDI USB port and set handler for Sysex Messages
    MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
    MIDI.setHandleSystemExclusive(handleSystemExclusive);
    MIDI.turnThruOff();

	// EEPROM INITIALIZATION
	uint8_t eepStatus = eep.begin(extEEPROM::twiClock400kHz); //go fast!
    
	// STATUS LED
	statusLED.begin();
	statusLED.setBrightness(STATUS_LED_BRIGHTNESS);
}
uint8_t R=0,G=85,B=190;
uint32_t preescaler=0;


void loop() {
	
    MIDI.read();
	
	if(SerialUSB.available())
	{
		uint8_t data = SerialUSB.read();
	
	}
	
	if(++preescaler>=1000)
	{
		preescaler=0;
		
		statusLED.clear(); // Set all pixel colors to 'off'
		statusLED.setPixelColor(0,R,G,B);
		statusLED.show();
	
		if(++R>=255)
			R=0;
		if(++G>=255)
			G=0;
		if(++B>=255)
			B=0;
	}
	
	if(rstFlg)
	{
		NVIC_SystemReset();
	}
}


/*
	// Disable all interrupts
	__disable_irq();
	uint32_t sysex_address = 0x8000;
	
	NVMCTRL->ADDR.reg = sysex_address;
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
	while (NVMCTRL->INTFLAG.bit.READY == 0);
	
	// Set automatic page write
	NVMCTRL->CTRLB.bit.MANW = 0;

	// Do writes in pages
	uint32_t *dst_addr = (sysex_address);
	uint32_t *src_addr = (sysex_data);
	
	// Execute "PBC" Page Buffer Clear
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
	while (NVMCTRL->INTFLAG.bit.READY == 0);
	
	// Fill page buffer
	
	for (int i=0; i<16; i++)
	{
		*(dst_addr + i*4) = *(src_addr + i*4);
	}
	// Disable all interrupts
	__enable_irq();
*/