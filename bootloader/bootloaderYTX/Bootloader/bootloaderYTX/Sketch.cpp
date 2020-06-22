
#include <Arduino.h>

#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_UsbTransport.h>

#include <extEEPROM.h>

#include <Adafruit_NeoPixel.h>

#include <stdint.h>

#include "board_definitions.h"

#include "sam_ba_monitor.h"

volatile uint8_t rstFlg = 0;

uint32_t PAGE_SIZE, PAGES, MIN_FLASH, MAX_FLASH, ENCODED_BLOCK_SIZE;



/* Variable to let the main task select the appropriate communication interface */
volatile uint8_t b_sharp_received;

/* RX and TX Buffers + rw pointers for each buffer */
volatile uint8_t buffer_rx_usart[USART_BUFFER_SIZE];

volatile uint8_t idx_rx_read;
volatile uint8_t idx_rx_write;

volatile uint8_t buffer_tx_usart[USART_BUFFER_SIZE];

volatile uint8_t idx_tx_read;
volatile uint8_t idx_tx_write;

/* Test for timeout in AT91F_GetChar */
uint8_t error_timeout;
uint16_t size_of_data;
uint8_t mode_of_transfer;

static const uint16_t crc16Table[256]=
{
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};



//*----------------------------------------------------------------------------
//* \brief Compute the CRC
//*----------------------------------------------------------------------------
unsigned short serial_add_crc(char ptr, unsigned short crc)
{
	return (crc << 8) ^ crc16Table[((crc >> 8) ^ ptr) & 0xff];
}

/**
 * \brief Puts a byte on usart line
 * The type int is used to support printf redirection from compiler LIB.
 *
 * \param value      Value to put
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
int cdc_putc(/*P_USB_CDC pCdc,*/ int value)
{
	/* Send single byte on USB CDC */
	SerialUSB.write((const char *)&value);

	return 1;
}

int cdc_getc(/*P_USB_CDC pCdc*/void)
{
	uint8_t rx_char;

	/* Read singly byte on USB CDC */
	rx_char = SerialUSB.read();

	return (int)rx_char;
}

bool cdc_is_rx_ready(/*P_USB_CDC pCdc*/void)
{
	/* Check whether the device is configured */
	//if (  )
	//return 0;

	/* Return transfer complete 0 flag status */
	return SerialUSB.available();
}

uint32_t cdc_write_buf(/*P_USB_CDC pCdc,*/ void const* data, uint32_t length)
{
	/* Send the specified number of bytes on USB CDC */
	uint8_t* ptrData = (uint8_t*) data;
	for(int i = 0; i<length; i++){
		SerialUSB.write(ptrData[i]);
	}
	return length;
}

uint32_t cdc_read_buf(/*P_USB_CDC pCdc,*/ void* data, uint32_t length)
{
	uint8_t* ptrData = (uint8_t*) data;
	uint32_t i = 0;
	while(SerialUSB.available() && i < length){
		ptrData[i] = (uint8_t) SerialUSB.read();
		i++;
	}
	/* Read from USB CDC */
	return i;
}

uint32_t cdc_read_buf_xmd(/*P_USB_CDC pCdc,*/ void* data, uint32_t length)
{

	/* Blocking read till specified number of bytes is received */
	// XXX: USB_Read_blocking is not reliable
	// return USB_Read_blocking(pCdc, (char *)data, length);

	char *dst = (char *)data;
	uint32_t remaining = length;
	while (remaining)
	{
		uint32_t readed = cdc_read_buf((char *)dst, remaining);
		remaining -= readed;
		dst += readed;
	}

	return length;
}


int serial_sharp_received(void)
{
	if (SerialUSB.available())
	{
		if (cdc_getc() == SHARP_CHARACTER)
		return (true);
	}
	return (false);
}


const char RomBOOT_Version[] = SAM_BA_VERSION;
//const char RomBOOT_ExtendedCapabilities[] = "[Arduino:XYZ]";
const char RomBOOT_ExtendedCapabilities[] = "[Yaeltex Kilomux: V2]";
/* Provides one common interface to handle both USART and USB-CDC */
/* Provides one common interface to handle both USART and USB-CDC */
typedef struct
{
  /* send one byte of data */
  int (*put_c)(int value);
  /* Get one byte */
  int (*get_c)(void);
  /* Receive buffer not empty */
  bool (*is_rx_ready)(void);
  /* Send given data (polling) */
  uint32_t (*putdata)(void const* data, uint32_t length);
  /* Get data from comm. device */
  uint32_t (*getdata)(void* data, uint32_t length);
  /* Send given data (polling) using xmodem (if necessary) */
  uint32_t (*putdata_xmd)(void const* data, uint32_t length);
  /* Get data from comm. device using xmodem (if necessary) */
  uint32_t (*getdata_xmd)(void* data, uint32_t length);
} t_monitor_if;


const t_monitor_if uart_if =
{
  .put_c =       NULL,
  .get_c =       NULL,
  .is_rx_ready = NULL,
  .putdata =     NULL,
  .getdata =     NULL,
  .putdata_xmd = NULL,
  .getdata_xmd = NULL
};

/* The pointer to the interface object use by the monitor */
t_monitor_if * ptr_monitor_if;

/* b_terminal_mode mode (ascii) or hex mode */
volatile bool b_terminal_mode = false;
volatile bool b_sam_ba_interface_usart = false;



/*
 * Central SAM-BA monitor putdata function using the board LEDs
 */
static uint32_t sam_ba_putdata(t_monitor_if* pInterface, void const* data, uint32_t length)
{
	uint32_t result ;

	result=cdc_write_buf(data, length);

	return result;
}

/*
 * Central SAM-BA monitor getdata function using the board LEDs
 */
static uint32_t sam_ba_getdata(t_monitor_if* pInterface, void* data, uint32_t length)
{
	uint32_t result ;

	result=cdc_read_buf(data, length);

	if (result)
	{
		
	}

	return result;
}

/*
 * Central SAM-BA monitor putdata function using the board LEDs
 */
static uint32_t sam_ba_putdata_xmd(t_monitor_if* pInterface, void const* data, uint32_t length)
{
	uint32_t result ;

	result=cdc_write_buf(data, length);

	return result;
}

/*
 * Central SAM-BA monitor getdata function using the board LEDs
 */
static uint32_t sam_ba_getdata_xmd(t_monitor_if* pInterface, void* data, uint32_t length)
{
	uint32_t result ;

	result=cdc_read_buf_xmd(data, length);
//
	//if (result)
	//{
//
	//}

	return result;
}

/**
 * \brief This function allows data emission by USART
 *
 * \param *data  Data pointer
 * \param length Length of the data
 */
void sam_ba_putdata_term(uint8_t* data, uint32_t length)
{
  uint8_t temp, buf[12], *data_ascii;
  uint32_t pepe, int_value;

  if (b_terminal_mode)
  {
    if (length == 4)
      int_value = *(uint32_t *) data;
    else if (length == 2)
      int_value = *(uint16_t *) data;
    else
      int_value = *(uint8_t *) data;

    data_ascii = buf + 2;
    data_ascii += length * 2 - 1;

    for (pepe = 0; pepe < length * 2; pepe++)
    {
      temp = (uint8_t) (int_value & 0xf);

      if (temp <= 0x9)
        *data_ascii = temp | 0x30;
      else
        *data_ascii = temp + 0x37;

      int_value >>= 4;
      data_ascii--;
    }
    buf[0] = '0';
    buf[1] = 'x';
    buf[length * 2 + 2] = '\n';
    buf[length * 2 + 3] = '\r';
    sam_ba_putdata(ptr_monitor_if, buf, length * 2 + 4);
  }
  else
    sam_ba_putdata(ptr_monitor_if, data, length);
  return;
}

volatile uint32_t sp;
void call_applet(uint32_t address)
{
  uint32_t app_start_address;

  __disable_irq();

  sp = __get_MSP();

  /* Rebase the Stack Pointer */
  __set_MSP(*(uint32_t *) address);

  /* Load the Reset Handler address of the application */
  app_start_address = *(uint32_t *)(address + 4);

  /* Jump to application Reset Handler in the application */
  asm("bx %0"::"r"(app_start_address));
}

uint32_t current_number;
uint32_t i, length;
uint8_t command, *ptr_data, *ptr, data[SIZEBUFMAX];
uint8_t j;
uint32_t u32tmp;

// Prints a 32-bit integer in hex.
static void put_uint32(uint32_t n)
{
  char buff[8];
  int i;
  for (i=0; i<8; i++)
  {
    int d = n & 0XF;
    n = (n >> 4);

    buff[7-i] = d > 9 ? 'A' + d - 10 : '0' + d;
  }
  sam_ba_putdata( ptr_monitor_if, buff, 8);
}

static void sam_ba_monitor_loop(void)
{
  length = sam_ba_getdata(ptr_monitor_if, data, SIZEBUFMAX);
  ptr = data;
  
  for (i = 0; i < length; i++, ptr++)
  {
    if (*ptr == 0xff) continue;

    if (*ptr == '#')
    {
      if (b_terminal_mode)
      {
        sam_ba_putdata(ptr_monitor_if, "\n\r", 2);
      }
      if (command == 'S')
      {
        //Check if some data are remaining in the "data" buffer
        if(length>i)
        {
          //Move current indexes to next avail data (currently ptr points to "#")
          ptr++;
          i++;

          //We need to add first the remaining data of the current buffer already read from usb
          //read a maximum of "current_number" bytes
          if ((length-i) < current_number)
          {
            u32tmp=(length-i);
          }
          else
          {
            u32tmp=current_number;
          }

          memcpy(ptr_data, ptr, u32tmp);
          i += u32tmp;
          ptr += u32tmp;
          j = u32tmp;
        }
        //update i with the data read from the buffer
        i--;
        ptr--;
        //Do we expect more data ?
        if(j<current_number)
          sam_ba_getdata_xmd(ptr_monitor_if, ptr_data, current_number-j);

        __asm("nop");
      }
      else if (command == 'R')
      {
        sam_ba_putdata_xmd(ptr_monitor_if, ptr_data, current_number);
      }
      else if (command == 'O')
      {
        *ptr_data = (char) current_number;
      }
      else if (command == 'H')
      {
        *((uint16_t *) ptr_data) = (uint16_t) current_number;
      }
      else if (command == 'W')
      {
        *((int *) ptr_data) = current_number;
      }
      else if (command == 'o')
      {
        sam_ba_putdata_term(ptr_data, 1);
      }
      else if (command == 'h')
      {
        current_number = *((uint16_t *) ptr_data);
        sam_ba_putdata_term((uint8_t*) &current_number, 2);
      }
      else if (command == 'w')
      {
        current_number = *((uint32_t *) ptr_data);
        sam_ba_putdata_term((uint8_t*) &current_number, 4);
      }
      else if (command == 'G')
      {
        call_applet(current_number);
        /* Rebase the Stack Pointer */
        __set_MSP(sp);
        __enable_irq();
        //if (b_sam_ba_interface_usart) {
          //ptr_monitor_if->put_c(0x6);
        //}
      }
      else if (command == 'T')
      {
        b_terminal_mode = 1;
        sam_ba_putdata(ptr_monitor_if, "\n\r", 2);
      }
      else if (command == 'N')
      {
        if (b_terminal_mode == 0)
        {
          sam_ba_putdata( ptr_monitor_if, "\n\r", 2);
        }
        b_terminal_mode = 0;
      }
      else if (command == 'V')
      {
        sam_ba_putdata( ptr_monitor_if, "v", 1);
        sam_ba_putdata( ptr_monitor_if, (uint8_t *) RomBOOT_Version, strlen(RomBOOT_Version));
        sam_ba_putdata( ptr_monitor_if, " ", 1);
        sam_ba_putdata( ptr_monitor_if, (uint8_t *) RomBOOT_ExtendedCapabilities, strlen(RomBOOT_ExtendedCapabilities));
        sam_ba_putdata( ptr_monitor_if, " ", 1);
        ptr = (uint8_t*) &(__DATE__);
        i = 0;
        while (*ptr++ != '\0')
          i++;
        sam_ba_putdata( ptr_monitor_if, (uint8_t *) &(__DATE__), i);
        sam_ba_putdata( ptr_monitor_if, " ", 1);
        i = 0;
        ptr = (uint8_t*) &(__TIME__);
        while (*ptr++ != '\0')
          i++;
        sam_ba_putdata( ptr_monitor_if, (uint8_t *) &(__TIME__), i);
        sam_ba_putdata( ptr_monitor_if, "\n\r", 2);
      }
      else if (command == 'X')
      {
        // Syntax: X[ADDR]#
        // Erase the flash memory starting from ADDR to the end of flash.

        // Note: the flash memory is erased in ROWS, that is in block of 4 pages.
        //       Even if the starting address is the last byte of a ROW the entire
        //       ROW is erased anyway.

        uint32_t dst_addr = current_number; // starting address

        while (dst_addr < MAX_FLASH)
        {
          // Execute "ER" Erase Row
          NVMCTRL->ADDR.reg = dst_addr / 2;
          NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
          while (NVMCTRL->INTFLAG.bit.READY == 0)
            ;
          dst_addr += PAGE_SIZE * 4; // Skip a ROW
        }

        // Notify command completed
        sam_ba_putdata( ptr_monitor_if, "X\n\r", 3);
      }
      else if (command == 'Y')
      {
        // This command writes the content of a buffer in SRAM into flash memory.

        // Syntax: Y[ADDR],0#
        // Set the starting address of the SRAM buffer.

        // Syntax: Y[ROM_ADDR],[SIZE]#
        // Write the first SIZE bytes from the SRAM buffer (previously set) into
        // flash memory starting from address ROM_ADDR

        static uint32_t *src_buff_addr = NULL;

        if (current_number == 0)
        {
          // Set buffer address
          src_buff_addr = (uint32_t*)ptr_data;
        }
        else
        {
          // Write to flash
          uint32_t size = current_number/4;
          uint32_t *src_addr = src_buff_addr;
          uint32_t *dst_addr = (uint32_t*)ptr_data;


          // Set automatic page write
          NVMCTRL->CTRLB.bit.MANW = 0;

          // Do writes in pages
          while (size)
          {
            // Execute "PBC" Page Buffer Clear
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
            while (NVMCTRL->INTFLAG.bit.READY == 0)
              ;

            // Fill page buffer
            uint32_t i;
            for (i=0; i<(PAGE_SIZE/4) && i<size; i++)
            {
              dst_addr[i] = src_addr[i];
            }

            // Execute "WP" Write Page
            //NVMCTRL->ADDR.reg = ((uint32_t)dst_addr) / 2;
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
            while (NVMCTRL->INTFLAG.bit.READY == 0)
              ;

            // Advance to next page
            dst_addr += i;
            src_addr += i;
            size     -= i;
          }
        }

end:

        // Notify command completed
        sam_ba_putdata( ptr_monitor_if, "Y\n\r", 3);
      }
      else if (command == 'Z')
      {
        // This command calculate CRC for a given area of memory.
        // It's useful to quickly check if a transfer has been done
        // successfully.

        // Syntax: Z[START_ADDR],[SIZE]#
        // Returns: Z[CRC]#

        uint8_t *data;
        uint32_t size = current_number;
        uint16_t crc = 0;
        uint32_t i = 0;

        data = (uint8_t *)ptr_data;


        for (i=0; i<size; i++)
          crc = serial_add_crc(*data++, crc);

        // Send response
        sam_ba_putdata( ptr_monitor_if, "Z", 1);
        put_uint32(crc);
        sam_ba_putdata( ptr_monitor_if, "#\n\r", 3);
      }

      command = 'z';
      current_number = 0;

      if (b_terminal_mode)
      {
        sam_ba_putdata( ptr_monitor_if, ">", 1);
      }
    }
    else
    {
      if (('0' <= *ptr) && (*ptr <= '9'))
      {
        current_number = (current_number << 4) | (*ptr - '0');
      }
      else if (('A' <= *ptr) && (*ptr <= 'F'))
      {
        current_number = (current_number << 4) | (*ptr - 'A' + 0xa);
      }
      else if (('a' <= *ptr) && (*ptr <= 'f'))
      {
        current_number = (current_number << 4) | (*ptr - 'a' + 0xa);
      }
      else if (*ptr == ',')
      {
        ptr_data = (uint8_t *) current_number;
        current_number = 0;
      }
      else
      {
        command = *ptr;
        current_number = 0;
      }
    }
  }
}



void sam_ba_monitor_init(void)
{

	b_sam_ba_interface_usart = true;
	
	ptr_data = NULL;
	command = 'z';
	
	//Initialize flag
	b_sharp_received = false;
	idx_rx_read = 0;
	idx_rx_write = 0;
	idx_tx_read = 0;
	idx_tx_write = 0;

	error_timeout = 0;
	
	SerialUSB.begin(115200);
}

void sam_ba_monitor_run(void)
{
  uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
  PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ];
  PAGES = NVMCTRL->PARAM.bit.NVMP;
  MAX_FLASH = PAGE_SIZE * PAGES;

  ptr_data = NULL;
  command = 'z';
  while (1)
  {
    sam_ba_monitor_loop();
  }
}

#define STATUS_LED_BRIGHTNESS   64
#define STATUS_LED_PIN			26

#define SAMD11_BOOT_MODE_PIN	6
#define SAMD11_RST_PIN			38

static const unsigned sUsbTransportBufferSize = 256;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

struct MySettings : public midi::DefaultSettings
{
  static const bool Use1ByteParsing = false;
  static const unsigned SysExMaxSize = sUsbTransportBufferSize; // Accept SysEx messages up to 256 bytes long.
  static const bool UseRunningStatus = false; // My devices seem to be ok with it.
};

MIDI_CREATE_CUSTOM_INSTANCE(UsbTransport, sUsbTransport, MIDI, MySettings);

// Create a 'MIDI' object using MySettings bound to Serial.
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDIHW, MySettings);

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

#define ADDRESS_SIZE		(sizeof(uint32_t))
#define PAGES_PER_BLOCK		2

uint8_t *sysex_data;

#define SYSEX_ID0  				'y'
#define SYSEX_ID1               't'
#define SYSEX_ID2               'x'

#define REQUEST_RST                 0x12
#define REQUEST_BOOT_MODE           0x13
#define REQUEST_UPLOAD_SELF         0x14
#define REQUEST_UPLOAD_OTHER        0x15
#define REQUEST_FIRM_DATA_UPLOAD    0x16

#define STATUS_ACK    1
#define STATUS_NAK    2

enum ytxIOStructure
{
	ID0,
	ID1,
	ID2,
	MESSAGE_STATUS,
	WISH,
	MESSAGE_TYPE,
	REQUEST_ID,
	DATA,
	HEADER_SIZE = DATA
};

uint8_t ack_msg[] =
{
	 SYSEX_ID0, SYSEX_ID1, SYSEX_ID2, STATUS_ACK
};

uint8_t nak_msg[] =
{
	 SYSEX_ID0, SYSEX_ID1, SYSEX_ID2, STATUS_NAK
};



uint8_t *stream;
volatile uint8_t sysex_cnt;
uint8_t pagesCnt;


bool forwarderEnable = false;


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

uint8_t encodedSysExSize(uint8_t length)
{
	uint8_t outLength  = 0;     // Num bytes in output array.
	uint8_t count = 0;     // Num 7bytes in a block.

	for (unsigned i = 0; i < length; ++i)
	{
		if (count++ == 6)
		{
			outLength  += 8;
			count       = 0;
		}
	}
	
	return outLength + count + (count != 0 ? 1 : 0);
}

/*! \brief Decode received block and write to flash 
 \param void
 \return 0 if fails or 1 if success
 */

uint8_t write_block(void)
{
	uint8_t success = 0;
	
	//take received checksum and remove from size
	uint8_t rcbChecksum = stream[sysex_cnt - 1];
	sysex_cnt--;
	
	//calculate block checksum
	uint8_t calChecksum = 0;
	
	for(uint32_t i = 0; i < sysex_cnt; i++)
	{
		calChecksum ^= stream[i];
	}
	calChecksum &= 0x7f;
	
	//check equal
	if (calChecksum == rcbChecksum)
	{
		//remove header block
		stream += HEADER_SIZE;
		sysex_cnt -= HEADER_SIZE;
	
		if(sysex_cnt == ENCODED_BLOCK_SIZE)
		{
			//decode payload(without headers and checksum)
			uint32_t decodedSize = decodeSysEx(stream,sysex_data,sysex_cnt);
	
			//take destination address and remove from block size
			uint32_t flashAddress;
			memcpy(&flashAddress,sysex_data,sizeof(uint32_t));
			flashAddress += MIN_FLASH;
	
			//check errors
			if ((decodedSize - ADDRESS_SIZE) == PAGES_PER_BLOCK*PAGE_SIZE)
			{
				//write block
				for (uint32_t pages=0; pages<PAGES_PER_BLOCK; pages++)
				{
					uint32_t targetAddress = flashAddress + pages*PAGE_SIZE;
					
					if((targetAddress+PAGE_SIZE)>MAX_FLASH)
						break;
		
					// Set automatic page write
					NVMCTRL->CTRLB.bit.MANW = 0;

					// Do writes in pages
					volatile uint32_t *dst_addr = (uint32_t *)(targetAddress);
					volatile uint32_t *src_addr = (uint32_t *)(sysex_data + pages*PAGE_SIZE + ADDRESS_SIZE);
		
					// Execute "PBC" Page Buffer Clear
					NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
					while (NVMCTRL->INTFLAG.bit.READY == 0);
					
					// Fill page buffer
					for (uint32_t i=0; i<(PAGE_SIZE/4); i++)
					{
						dst_addr[i] = src_addr[i];
					}
					
					// Execute "WP" Write Page
					NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
					while (NVMCTRL->INTFLAG.bit.READY == 0);	
				}
				success = 1;
			}
		}
	}
	
	return success;

}

void handleSystemExclusiveHW(byte *message, unsigned size)
{
	if(forwarderEnable)
		MIDI.sendSysEx(size-2,&message[1]);
}
void handleSystemExclusiveUSB(byte *message, unsigned size)
{
	uint8_t res = 0;

	sysex_cnt = size - 2;
	
	if (sysex_cnt >= HEADER_SIZE)
	{
		stream = message + 1;
	
		if (stream[REQUEST_ID] == REQUEST_UPLOAD_SELF)
		{
			forwarderEnable = false;
			
			// Erase the flash memory starting from ADDR 0x8000 to the end of flash.

			// Note: the flash memory is erased in ROWS, that is in block of 4 pages.
			//       Even if the starting address is the last byte of a ROW the entire
			//       ROW is erased anyway.
					
			uint32_t dst_addr = MIN_FLASH; // starting address

			while (dst_addr < MAX_FLASH)
			{
				// Execute "ER" Erase Row
				NVMCTRL->ADDR.reg = dst_addr / 2;
				NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
				while (NVMCTRL->INTFLAG.bit.READY == 0);
				dst_addr += PAGE_SIZE * 4; // Skip a ROW
			}
					
			res = 1;
			pagesCnt = 0;
			MIDI.sendSysEx(sizeof(ack_msg),ack_msg);
		}
		else if (stream[REQUEST_ID] == REQUEST_UPLOAD_OTHER)
		{
			forwarderEnable = true;
			stream[REQUEST_ID] = REQUEST_UPLOAD_SELF;
			MIDIHW.sendSysEx(size-2,&message[1]);
		}	
		else if (stream[REQUEST_ID] == REQUEST_RST)
		{
			//reset device
			rstFlg = 1;
		}
		else if (stream[REQUEST_ID] == REQUEST_FIRM_DATA_UPLOAD)
		{
			if(forwarderEnable)
			{
				MIDIHW.sendSysEx(size-2,&message[1]);
			}
			else
			{
				res = write_block();
							
				if(res)
				{
					MIDI.sendSysEx(sizeof(ack_msg),ack_msg);
								
					//animation here//
				}
				else
				{
					MIDI.sendSysEx(sizeof(nak_msg),nak_msg);
				}	
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

    /* Wait 0.5sec to see if the user tap reset again */
    delay(100);

    /* Timeout happened, continue boot... */
    BOOT_DOUBLE_TAP_DATA = 0;
  }
#endif

  jump_to_app = true;
}

void setup()
{		
	check_start_application();
	
	// EEPROM INITIALIZATION
	uint8_t eepStatus = eep.begin(extEEPROM::twiClock400kHz); //go fast!
	
	// YAELTEX BOOT SIGN - IF BOOT SIGNATURE IN EEPROM IS SET TO 'Y' REMAIN IN BOOTLOADER TO ACCEPT SYSEX FIRMWARE UPDATE
	bool bootSignPresent = false;
	// TEST I2C EEPROM
	uint8_t stayInBoot = 0;

	uint16_t bytesR = eep.read(BOOT_SIGN_ADDR, (uint8_t*) &stayInBoot, sizeof(stayInBoot));
		
	if(stayInBoot&0x01)
	{
		bootSignPresent = true;
		stayInBoot &= ~ 0x01;
		eep.write(BOOT_SIGN_ADDR, (uint8_t*) &stayInBoot, sizeof(stayInBoot));
	}

	/* Jump in application if condition is satisfied */

	if (!bootSignPresent && jump_to_app) 
	{
		jump_to_application();
	}
	
	USBDevice.init();
	USBDevice.attach();

	
	sam_ba_monitor_init();
	
	pinMode(SAMD11_BOOT_MODE_PIN,OUTPUT);
	pinMode(SAMD11_RST_PIN,OUTPUT);
	
	digitalWrite(SAMD11_BOOT_MODE_PIN,LOW);
	digitalWrite(SAMD11_RST_PIN,LOW);
	delay(1);
	digitalWrite(SAMD11_RST_PIN,HIGH);
	delay(1);
	

	uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
	PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ];
	PAGES = NVMCTRL->PARAM.bit.NVMP;
	MIN_FLASH = (uint32_t)&__sketch_vectors_ptr;
	MAX_FLASH = PAGE_SIZE * PAGES;
	
	//allocate decoded data array
	sysex_data = new uint8_t[PAGE_SIZE*PAGES_PER_BLOCK + ADDRESS_SIZE];
	
	//calculate encoded data size
	ENCODED_BLOCK_SIZE = encodedSysExSize(PAGE_SIZE*PAGES_PER_BLOCK + ADDRESS_SIZE);
	
    // Begin MIDI USB port and set handler for Sysex Messages
    MIDI.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI por USB.
    MIDI.setHandleSystemExclusive(handleSystemExclusiveUSB);
    MIDI.turnThruOff();
	
	MIDIHW.begin(MIDI_CHANNEL_OMNI); // Se inicializa la comunicación MIDI hacia el SAMD11.
	MIDIHW.setHandleSystemExclusive(handleSystemExclusiveHW);
	MIDIHW.turnThruOff();
	

    
	// STATUS LED
	statusLED.begin();
	statusLED.setBrightness(STATUS_LED_BRIGHTNESS);
	
}
uint8_t R=0,G=85,B=190;
uint32_t preescaler=0;

void loop() 
{
    MIDI.read();
    MIDIHW.read();
	
	//if(serial_is_rx_ready())
	if (serial_sharp_received())
	{
		while(1)
		{
			sam_ba_monitor_loop();
		}
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
	
	//reset system if set
	if(rstFlg)
	{
		USBDevice.detach();
		USBDevice.end();
		delay(500);
		NVIC_SystemReset();
	}
}