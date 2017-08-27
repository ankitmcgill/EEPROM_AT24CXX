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

#include "EEPROM_AT24CXX.h"

//LOCAL LIBRARY VARIABLES/////////////////////////////////////
//DEBUG RELATRED
static uint8_t _eeprom_at24cxx_debug;

//EEPROM RELATED
static uint8_t _eeprom_at24cxx_model;
static uint8_t _eeprom_at24cxx_i2c_address;

//I2C FUNCTION POINTERS
static void (*_eeprom_at24cxx_i2c_init)(void);
static void (*_eeprom_at24cxx_i2c_writebyte)(uint8_t, uint32_t, uint8_t, uint8_t);
static void (*_eeprom_at24cxx_writebyte_multiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t);
static uint8_t (*_eeprom_at24cxx_i2c_readbyte)(uint8_t, uint32_t, uint8_t);
static void (*_eeprom_at24cxx_readbyte_multiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t);

void PUTINFLASH EEPROM_AT24CXX_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _eeprom_at24cxx_debug = debug_on;
}

void PUTINFLASH EEPROM_AT24CXX_SetI2CFunctions(void (*i2c_init)(void),
                                            void (*i2c_writebyte)(uint8_t, uint32_t, uint8_t, uint8_t),
                                            void (*i2c_writebytemultiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t),
                                            uint8_t (*i2c_readbyte)(uint8_t, uint32_t, uint8_t),
                                            void (*i2c_readbytemultiple)(uint8_t, uint32_t, uint8_t, uint8_t*, uint8_t)
                                          )
{
    //SET I2C DATA TRANSFER FUNCTIONS POINTERS

    _eeprom_at24cxx_i2c_init = i2c_init;
    _eeprom_at24cxx_i2c_writebyte = i2c_writebyte;
    _eeprom_at24cxx_writebyte_multiple = i2c_writebytemultiple;
    _eeprom_at24cxx_i2c_readbyte = i2c_readbyte;
    _eeprom_at24cxx_readbyte_multiple = i2c_readbytemultiple;

    if(_eeprom_at24cxx_debug)
    {
        PRINTF("EEPROM : AT24CXX : i2c operation functions set\n");
    }
}

//GET PARAMETER FUNCTIONS
uint8_t PUTINFLASH EEPROM_AT24CXX_GetI2CAddress(void)
{
    //RETURN THE CALCULATED I2C ADDRESS

    return _eeprom_at24cxx_i2c_address;
}

void PUTINFLASH EEPROM_AT24CXX_Initialize(EEPROM_MODEL_TYPE model, uint8_t a2, uint8_t a1, uint8_t a0)
{
    //INTIALIZE DS1307 RTC
    //CHECK FOR CH BIT, IF DISABLED, ENABLE IT

    //VALIDATE EEPROM MODEL
    if(model >= EEPROM_MODEL_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid EEPROM model !\n");
        return;
    }

    //INIT BACKEND I2C
    #if defined(ESP8266)
        ESP8266_I2C_SetDebug(_eeprom_at24cxx_debug);
        ESP8266_I2C_Init();
    #endif

    //SET EEPROM MODEL
    _eeprom_at24cxx_model = model;

    //CALCULATE I2C ADDRESS
    _eeprom_at24cxx_i2c_address = EEPROM_AT24CXX_I2C_ADDRESS | (a2 << 2) | (a1 << 1) | a0;

    if(_eeprom_at24cxx_debug)
    {
        PRINTF("EEPROM : AT24CXX : Initialized. I2C address = 0x%02X\n", _eeprom_at24cxx_i2c_address);
    }
}

void PUTINFLASH EEPROM_AT24CXX_Write8(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t data)
{
    //WRITE UINT8_T AT SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    //CHECK VALIDITY OF ADDRESS
    //DO WRITE OPERATION
    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_i2c_writebyte)(_eeprom_at24cxx_i2c_address, address, 2, data);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at address %u\n", data, address);
                      }
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_i2c_writebyte)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, data);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at page %u\n", data, address);
                      }
                      break;
    }
}

void PUTINFLASH EEPROM_AT24CXX_Write16(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint16_t data)
{
    //WRITE UINT16_T AT SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    uint8_t* byte = (uint8_t*)ZALLOC(2);
    byte[0] = (uint8_t)((data & 0xFF00) >> 8);
    byte[1] = (uint8_t)data;

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, byte, 2);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at address %u\n", data, address);
                      }
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, byte, 2);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at page %u\n", data, address);
                      }
                      break;
    }
    FREE(byte);
}

void PUTINFLASH EEPROM_AT24CXX_Write32(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint32_t data)
{
    //WRITE UINT32_T AT SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    uint8_t* byte = (uint8_t*)ZALLOC(4);
    byte[0] = (uint8_t)((data & 0xFF000000) >> 24);
    byte[1] = (uint8_t)((data & 0x00FF0000) >> 16);
    byte[2] = (uint8_t)((data & 0x0000FF00) >> 8);
    byte[3] = (uint8_t)data;

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, byte, 4);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at address %u\n", data, address);
                      }
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, byte, 4);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u at page %u\n", data, address);
                      }
                      break;
    }
    FREE(byte);
}

void PUTINFLASH EEPROM_AT24CXX_WriteBlock(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t* data, uint8_t data_len)
{
    //WRITE BLOCK AT SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, data, data_len);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u bytes at address %u\n", data_len, address);
                      }
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address write\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_writebyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, data, data_len);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : written %u bytes at page %u\n", data_len, address);
                      }
                      break;
    }
}

uint8_t PUTINFLASH EEPROM_AT24CXX_Read8(uint32_t address, EEPROM_ADDRESS_TYPE address_type)
{
    //READ UINT8_T FROM SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    uint8_t data;
    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                            }
                            return;
                      }
                      data = (*_eeprom_at24cxx_i2c_readbyte)(_eeprom_at24cxx_i2c_address, address, 2);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : read %u from address %u\n", data, address);
                      }
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                          }
                          return;
                      }
                      data = (*_eeprom_at24cxx_i2c_readbyte)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2);
                      if(_eeprom_at24cxx_debug)
                      {
                          PRINTF("EEPROM : AT24CXX : read %u from page %u\n", data, address);
                      }
                      break;
    }
    return data;
}

uint16_t PUTINFLASH EEPROM_AT24CXX_Read16(uint32_t address, EEPROM_ADDRESS_TYPE address_type)
{
    //READ UINT16_T FROM SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    uint16_t data;
    uint8_t* byte = (uint8_t*)ZALLOC(2);

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, byte, 2);
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, byte, 2);
                      break;
    }
    data = (byte[0] << 8) | byte[1];
    if(_eeprom_at24cxx_debug)
    {
        if(address_type == ADDRESS_TYPE_BYTE)
            PRINTF("EEPROM : AT24CXX : read %u from address %u\n", data, address);
        else
            PRINTF("EEPROM : AT24CXX : read %u from page %u\n", data, address);
    }
    FREE(byte);
    return data;
}

uint32_t PUTINFLASH EEPROM_AT24CXX_Read32(uint32_t address, EEPROM_ADDRESS_TYPE address_type)
{
    //READ UINT32_T FROM SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    uint32_t data;
    uint8_t* byte = (uint8_t*)ZALLOC(4);

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, byte, 4);
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, byte, 4);
                      break;
    }
    data = (byte[0] << 24) | (byte[1] << 16) | (byte[2] << 8) | byte[3];
    if(_eeprom_at24cxx_debug)
    {
        if(address_type == ADDRESS_TYPE_BYTE)
            PRINTF("EEPROM : AT24CXX : read %u from address %u\n", data, address);
        else
            PRINTF("EEPROM : AT24CXX : read %u from page %u\n", data, address);
    }
    FREE(byte);
    return data;
}

void PUTINFLASH EEPROM_AT24CXX_ReadBlock(uint32_t address, EEPROM_ADDRESS_TYPE address_type, uint8_t* data, uint8_t data_len)
{
    //READ BLOCK FROM SPECIFIED ADDRESS

    if(address_type >= ADDRESS_TYPE_MAX)
    {
        PRINTF("EEPROM : AT24CXX : Invalid address type !\n");
        return;
    }

    switch(address_type)
    {
        case ADDRESS_TYPE_BYTE:
                      if(!_eeprom_at24cxx_validate_byte_address(address))
                      {
                            if(_eeprom_at24cxx_debug)
                            {
                                PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                            }
                            return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, address, 2, data, data_len);
                      break;
        case ADDRESS_TYPE_PAGE:
                      if(!_eeprom_at24cxx_validate_page_address(address))
                      {
                          if(_eeprom_at24cxx_debug)
                          {
                              PRINTF("EEPROM : AT24CXX : Invalid address read\n");
                          }
                          return;
                      }
                      (*_eeprom_at24cxx_readbyte_multiple)(_eeprom_at24cxx_i2c_address, EEPROM_GET_BYTE_ADDRESS_FROM_PAGE(address), 2, data, data_len);
                      break;
    }
    if(_eeprom_at24cxx_debug)
    {
        if(address_type == ADDRESS_TYPE_BYTE)
            PRINTF("EEPROM : AT24CXX : read %u bytes from address %u\n", data_len, address);
        else
            PRINTF("EEPROM : AT24CXX : read %u bytes from page %u\n", data_len, address);
    }
}

static uint8_t PUTINFLASH _eeprom_at24cxx_validate_page_address(uint32_t p_address)
{
    //CHECK FOR VALIDITY OF PAGE ADDRESS

    switch(_eeprom_at24cxx_model)
    {
        case EEPROM_MODEL_AT24C32:
            if(p_address >= 0 && p_address <= 127)
                return 1;
            else
                return 0;
            break;
        case EEPROM_MODEL_AT24C64:
            if(p_address >= 0 && p_address <= 255)
                return 1;
            else
                return 0;
          break;
    }
}

static uint8_t PUTINFLASH _eeprom_at24cxx_validate_byte_address(uint32_t b_address)
{
    //CHECK FOR VALIDITY OF BYTE ADDRESS

    switch(_eeprom_at24cxx_model)
    {
        case EEPROM_MODEL_AT24C32:
            if(b_address >= 0 && b_address <= 4095)
                return 1;
            else
                return 0;
            break;
        case EEPROM_MODEL_AT24C64:
            if(b_address >= 0 && b_address <= 8191)
                return 1;
            else
                return 0;
          break;
    }
}
