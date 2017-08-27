/****************************************************************
* AT24CXX SERIAL EEPROM LIBRARY
* I2C BASED
*
* I2C ADDRESS  : 0101 0(A2)(A1)(A0)
*
* NOTE
* -------
*   (1) A2 A1 A0 PINS ARE INTERNALLY PULLED LOW BY THE IC
*
*   (2) MUTIPLE DATA WRITE IS WRITTEN BIG ENDIAN ORDER (HIGHEST
*       BITS WRITTEN FIRST IE AT THE LOWEST MEMORY LOCATION)
*       EXCEPT THE WRITEBLOCK FUNCTION WHICH DATA IS WRITTEN
*       AS IS
*
*   (3) AT24CXX SERIES EEPROM HAS ADDRESS ROLLOVER FOR MULTIPLE
*       BYTES READ / WRITE
*       WRITE ROLLOVER : THE ADDRESS ROLL OVERS FROM THE LAST BYTE
*                         IN THE CURRENT PAGE TO THE FIRST BYTE
*                         IN THE CURRENT PAGE
*       READ ROLLOVER  : THE ADDRESS ROLL OVERS FROM THE LAST BYTE
*                         OF THE LAST PAGE TO THE FIRST BYTE OF
*                         THE FIRST PAGE
*
*   (4) ESP8266 SPECIFIC : HAD SOME ISSUES THE EEPROM I2C, FIRST I2C OPERATION
*       WORKING BUT THE SECOND FAILED. TURNS OUT ITS SOMETHING TO DO WITH
*       ESP8266 SOFT I2C IMPLEMENTATION IN i2c_master. HAD TO JACK
*       UP THE SPEED TO ALMOST 500KHZ (BY DIVING THE delay PARAMETER
*       BY 3 IN MY i2c_master_wait FUNCTION IN i2c_master FILE TO GET IT
*       TO WORK PROPERLY)
*
* AUGUST 28 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
*
* REFERENCES
*
****************************************************************/

#ifndef _EEPROM_AT24CXX_H_
#define _EEPROM_AT24CXX_H_

#if defined(ESP8266)
  #include "osapi.h"
  #include "mem.h"
  #include "ets_sys.h"
  #include "ip_addr.h"
  #include "espconn.h"
  #include "os_type.h"
  #include "mem.h"

  #define PRINTF      os_printf
  #define PUTINFLASH  ICACHE_FLASH_ATTR
  #define ZALLOC      os_zalloc
  #define FREE        os_free
#endif

#define EEPROM_AT24CXX_I2C_ADDRESS            0x50
#define EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(x)  ((x) * 32)

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
typedef enum
{
    EEPROM_MODEL_AT24C32 = 0,
    EEPROM_MODEL_AT24C64,
    EEPROM_MODEL_MAX
} EEPROM_MODEL_TYPE;

typedef enum
{
    ADDRESS_TYPE_BYTE = 0,
    ADDRESS_TYPE_PAGE,
    ADDRESS_TYPE_MAX
} EEPROM_ADDRESS_TYPE;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////
//CONFIGURATION FUNCTIONS
void PUTINFLASH EEPROM_AT24CXX_SetDebug(uint8_t debug_on);
void PUTINFLASH EEPROM_AT24CXX_SetI2CFunctions(void (*i2c_init)(void),
                                                void (*i2c_writebyte)(uint8_t, uint32_t, uint8_t, uint8_t),
                                                void (*i2c_writebytemultiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t),
                                                uint8_t (*i2c_readbyte)(uint8_t, uint32_t, uint8_t),
                                                void (*i2c_readbytemultiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t));


//GET PARAMETER FUNCTIONS
uint8_t PUTINFLASH EEPROM_AT24CXX_GetI2CAddress(void);

//CONTROL FUNCTIONS
void PUTINFLASH EEPROM_AT24CXX_Initialize(EEPROM_MODEL_TYPE model, uint8_t a2, uint8_t a1, uint8_t a0);
void PUTINFLASH EEPROM_AT24CXX_Write8(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t data);
void PUTINFLASH EEPROM_AT24CXX_Write16(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint16_t data);
void PUTINFLASH EEPROM_AT24CXX_Write32(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint32_t data);
void PUTINFLASH EEPROM_AT24CXX_WriteBlock(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t* data, uint8_t data_len);

uint8_t PUTINFLASH EEPROM_AT24CXX_Read8(uint32_t address, EEPROM_ADDRESS_TYPE address_type);
uint16_t PUTINFLASH EEPROM_AT24CXX_Read16(uint32_t address, EEPROM_ADDRESS_TYPE address_type);
uint32_t PUTINFLASH EEPROM_AT24CXX_Read32(uint32_t address, EEPROM_ADDRESS_TYPE address_type);
void PUTINFLASH EEPROM_AT24CXX_ReadBlock(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t* data, uint8_t data_len);

//INTERNAL FUNCTIONS
static uint8_t PUTINFLASH _eeprom_at24cxx_validate_page_address(uint32_t p_address);
static uint8_t PUTINFLASH _eeprom_at24cxx_validate_byte_address(uint32_t b_address);

//END USER HELPER FUNCTION
//ADD WEAR LEVELING FUNCTION HERE
//END FUNCTION PROTOTYPES/////////////////////////////////
#endif
