/*
  SmartEEPROM library for ATSAME/D51 microcontrollers.
  Written by Tommy / Deezums

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

#include <Arduino.h>
#include "SmartEEPROM.h"

SmartEEPROM::SmartEEPROM()
{
  _status = SEE_NO_INT;
}

uint8_t SmartEEPROM::read(int address)
{
  waitBusy();	
  return _SmartEEPROM8[address];
}

void SmartEEPROM::update(int address, uint8_t value)
{
  waitBusy();
  if (_SmartEEPROM8[address] != value) { 
    _SmartEEPROM8[address] = value;
  }
  if (!isDirty()) {
	commit();
  }
}

void SmartEEPROM::write(int address, uint8_t value)
{
  update(address, value);
}

bool SmartEEPROM::isDirty()
{
  return (bool)(NVMCTRL->SEESTAT.reg & NVMCTRL_SEESTAT_LOAD);
}

bool SmartEEPROM::isBusy()
{
  return (bool)(NVMCTRL->SEESTAT.reg & NVMCTRL_SEESTAT_BUSY_Msk);
}

void SmartEEPROM::waitBusy()
{
  while ((NVMCTRL->SEESTAT.reg & NVMCTRL_SEESTAT_BUSY) != 0);
}

bool SmartEEPROM::isLocked()
{
  return (bool)(NVMCTRL->SEESTAT.reg & NVMCTRL_SEESTAT_LOCK_Msk);  
}

void SmartEEPROM::commit()
{
  NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_SEEFLUSH;
 return;
}

uint8_t SmartEEPROM::init(void)
{
  _status = checkFuses(EEPROM_EMULATION_SIZE);
  if (isLocked())
    _status = SEE_LOCKED;
  NVMCTRL->SEECFG.reg = (uint32_t)(NVMCTRL_SEECFG_WMODE_BUFFERED);
  return _status;
}

uint8_t SmartEEPROM::checkFuses(int s) {
  uint8_t userPage[128*4];    
  memcpy(userPage, (uint8_t *const)NVMCTRL_USER, sizeof(userPage));   
  int si = sizeof(SEEConverter) / sizeof(SEEConverter[0]);            
  while (--si >= 0 && SEEConverter[si].size != s)                     
    ;
  if (si < 0) {                                                       	// Incompatible eeprom size requested                         
    return SEE_ERR;
  }
  uint8_t newSEEFuses = MAKESEEFUSES(SEEConverter[si].psz, SEEConverter[si].sblk);  
  if (newSEEFuses == userPage[SEEFUSESINDEX]) {   						// EEPROM Fuses already set       
    return SEE_OK;                                                                         
  }
  else {  																// Set new EEPROM fuses
    while (!NVMCTRL->STATUS.bit.READY) { asm(""); }                   
    NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;               
    const bool format = ((newSEEFuses ^ userPage[SEEFUSESINDEX]) & newSEEFuses);
    if (format)
      exec_cmd(NVMCTRL_CTRLB_CMD_EP);
    exec_cmd(NVMCTRL_CTRLB_CMD_PBC);
    userPage[SEEFUSESINDEX] = newSEEFuses;
    const int ei = format ? (int)sizeof(userPage) : (SEEFUSESINDEX + 1);
    for (int i = 0; i < ei; i += 16) {
      uint8_t *const qwBlockAddr = (uint8_t *const)(NVMCTRL_USER + i);
      memcpy (qwBlockAddr, &userPage[i], 16);
      exec_cmdaddr(NVMCTRL_CTRLB_CMD_WQW, qwBlockAddr);
    }
	NVIC_SystemReset(); 	// Reset for fuses to take effect, should not get here next time
	return SEE_OK;			// nope...
  }
}
