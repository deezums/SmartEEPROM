/*
  SmartEEPROM library for ATSAME/D51 microcontrollers.
  Written by Tommy / Deezums

  NOTE! SmartEEPROM uses more flash per kb of emulated eeprom
  Fuses must be set to match expected EEPROM size

  EEprom Size   Flash Size  PSZ fuse, SBLK fuse
  512b        = 16k flash   0(4)      1
  1k eeprom   = 16k flash   1(8)      1
  2k eeprom   = 16k flash   2(16)     1
  4k eeprom   = 16k flash   3(32)     1
  8k eeprom   = 32k flash   4(64)     2
  16k eeprom  = 48k flash   5(128)    3
  32k eeprom  = 80k flash   6(256)    5
  64k eeprom  = 160k flash  7(512)    10

  Copyright (c) 2015-2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SMART_EEPROM_h
#define SMART_EEPROM_h
#include <Arduino.h>

#ifndef EEPROM_EMULATION_SIZE
#define EEPROM_EMULATION_SIZE 4096
#endif

#define NVMCTRL_SEESTAT_LOAD_Msk  ((0x1) << NVMCTRL_SEESTAT_LOAD_Pos)               
#define NVMCTRL_SEESTAT_BUSY_Msk  ((0x1) << NVMCTRL_SEESTAT_BUSY_Pos)  
#define NVMCTRL_SEESTAT_LOCK_Msk  ((0x1) << NVMCTRL_SEESTAT_LOCK_Pos)
#define exec_cmdaddr(cmd,a)   do{                                                   \
                                NVMCTRL->ADDR.reg = (uint32_t)a;                    \
                                NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | cmd; \
                                while (!NVMCTRL->STATUS.bit.READY) { asm (""); }    \
                              }while(0)
#define exec_cmd(cmd)         exec_cmdaddr(cmd, NVMCTRL_USER)
#define SEEFUSESINDEX         (NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER)
#define SEEPSZ                ((userPage[SEEFUSESINDEX] & NVMCTRL_FUSES_SEEPSZ_Msk) >> NVMCTRL_FUSES_SEEPSZ_Pos)
#define SEESBLK               ((userPage[SEEFUSESINDEX] & NVMCTRL_FUSES_SEESBLK_Msk) >> NVMCTRL_FUSES_SEESBLK_Pos)
#define MAKESEEFUSES(p,s)     ((userPage[SEEFUSESINDEX] & ~NVMCTRL_FUSES_SEEPSZ_Msk & ~NVMCTRL_FUSES_SEESBLK_Msk) | NVMCTRL_FUSES_SEEPSZ(p) | NVMCTRL_FUSES_SEESBLK(s))

#define SEE_NO_INT    0		// Smart EEPROM not configured
#define SEE_OK        1		// Smart EEPROM OK/Ready
#define SEE_ERR	  	  2		// Smart EEPROM Config error (incompatible size)
#define SEE_LOCKED    3		// Smart EEPROM region fuse locked - update fuses

  const struct {
	size_t size;
	uint8_t psz, sblk;
  } SEEConverter[] = {
	{ 0, 		0, 	0}, 	//16kB flash
	{ 512, 		0, 	1}, 	//16kB flash
	{ 1024, 	1, 	1}, 	//16kB flash
	{ 2048, 	2, 	1}, 	//16kB flash
	{ 4096, 	3, 	1}, 	//16kB flash
	{ 8192, 	4, 	2}, 	//32kB flash
	{ 16384, 	5, 	3}, 	//48kB flash
	{ 32768, 	6, 	5}, 	//80kB flash
	{ 65536, 	7, 	10}		//160kB flash
  };

class SmartEEPROM {

  public:
    SmartEEPROM();
	
	 /**
     * Check if EEprom is ready
	 * @return true, if eeprom is ready, error code if not
     */
    uint8_t init();

    /**
     * Read an eeprom cell
     * @param index
     * @return value
     */
    uint8_t read(int);

    /**
     * Write value to an eeprom cell
     * @param index
     * @param value
     */
    void write(int, uint8_t);

    /**
     * Update a eeprom cell
     * @param index
     * @param value
     */
    void update(int, uint8_t);

    /**
     * Read AnyTypeOfData from eeprom 
     * @param address
     * @return AnyTypeOfData
     */
    template< typename T > T &get( int idx, T &t ){
        uint16_t e = idx;
        uint8_t *ptr = (uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  *ptr++ = read(e);
        return t;
    }

    /**
     * Write AnyTypeOfData to eeprom
     * @param address 
     * @param AnyTypeOfData 
     * @return number of bytes written to flash 
     */
    template< typename T > const T &put( int idx, const T &t ){        
        const uint8_t *ptr = (const uint8_t*) &t;
        uint16_t e = idx;
        for( int count = sizeof(T) ; count ; --count, ++e )  update(e, *ptr++);
        return t;
    }

    /**
     * Check whether the eeprom is busy reading/writing data
     * @return true, if eeprom is busy, false if not
     */
    bool isBusy();  

    /**
     * Return the total length of emulated EEPROM in bytes
     */
    uint16_t length() { return EEPROM_EMULATION_SIZE; }

  private:
  
  	 /**
     * Check if EEprom fuses are configured
	 * @return true, if eeprom fuses are ready, error code if not
     */
    uint8_t checkFuses(int);	
	
     /**
     * Check whether the eeprom buffer has unwritten data
     * @return true, if eeprom data in buffer, false if not
     */
    bool isDirty();

    /**
     * Check if EEprom is locked
     * @return true, if eeprom data is write locked, false if not
     */
    bool isLocked(); 
	
    /**
     * Check if EEprom is busy, wait if so
     */	
	void waitBusy();
	
	/**
     * Write eeprom page buffer to the underlying flash storage if necessary, handled automatically in some cases
     */
    void commit();
	
    /** 
	*  Define a pointer to access SmartEEPROM as bytes 
	*/
    uint8_t *_SmartEEPROM8 = (uint8_t *)SEEPROM_ADDR;
	
    /* 
	*  Define a pointer to access SmartEEPROM as words (32-bits) 
	*/
    uint32_t *_SmartEEPROM32 = (uint32_t *)SEEPROM_ADDR;
	
    /* 
	*  Status of Smart EEPROM, need to check before read/write 
	*/
    uint8_t _status = SEE_NO_INT;
	
};

extern SmartEEPROM EEPROM;

#endif
