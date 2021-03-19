#ifndef DEFINES_H
#define DEFINES_H

#define FALSE           0
#define TRUE            1

// Platform-dependent sleep routines.
#if defined(__WINDOWS_MM__)
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

//Version
#define LOADER_VERSION     "1.1"

//Port filtering
#define PORT_NAME_FILTER    ""

//Files
#define YTX_FILE_SIZE (sizeof(fileHeader))
#define YTX_INI_FILE_SIZE (YTX_FILE_SIZE + sizeof(context))

#define YTX_MAIN_MIN_SIZE   (32*1024)
#define YTX_MAIN_MAX_SIZE   ((256-32)*1024)

#define YTX_AUX_MIN_SIZE   (2*1024)
#define YTX_AUX_MAX_SIZE   ((32-4)*1024)

//MIDI SYSEX
enum ytxIOStructure
{
    ID1,
    ID2,
    ID3,
    MESSAGE_STATUS,
    WISH,
    MESSAGE_TYPE,
    REQUEST_ID,
    DATA
};
//IDs
#define SYSEX_ID0                   'y'
#define SYSEX_ID1                   't'
#define SYSEX_ID2                   'x'
//Request
#define REQUEST_RST                 0x12
#define REQUEST_BOOT_MODE           0x13
#define REQUEST_UPLOAD_SELF         0x14
#define REQUEST_UPLOAD_OTHER        0x15
#define REQUEST_FIRM_DATA_UPLOAD    0x16
#define REQUEST_EEPROM_ERASE        0x19

//Status
#define STATUS_ACK                  1
#define STATUS_NAK                  2

//Firmware update FSM
#define FIRMWARE_UPDATE_STATE_BEGIN         0
#define FIRMWARE_UPDATE_STATE_BEGIN_WAIT    1
#define FIRMWARE_UPDATE_STATE_SEND          2
#define FIRMWARE_UPDATE_STATE_SEND_WAIT     3
#define FIRMWARE_UPDATE_STATE_JUMP_TO_APP   4

//Firmware update parameters
#define BEGIN_FIRMWARE_UPTADE               10
#define BEGIN_AUX_FIRMWARE_UPTADE           15
#define ACK_FIRMWARE_UPTADE                 11
#define NAK_FIRMWARE_UPTADE                 12
#define SEND_BLOCK_FIRMWARE_UPTADE          13
#define END_BLOCK_FIRMWARE_UPTADE           14

#endif  // DEFINES_H
