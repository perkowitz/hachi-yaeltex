///////////////////////////////////////////////////////////////////////////////
// BOSSA
//
// Copyright (c) 2015, Arduino LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <iostream>
#include <exception>

#include "Samba.h"
#include "WordCopyApplet.h"
#include "NvmFlash.h"

// The base address of the NVM module in
// main memory + offset to the CTRLA register
#define NVM_COMMAND_REG                 (_regs+0x00)
#define NVM_CONTROL_REG                 (_regs+0x04)
#define NVM_CONTROL_MANUAL_WRITE_BIT    (0x1 << 7)

#define NVM_D51_COMMAND_REG                 (_regs+0x04)
#define NVM_D51_CONTROL_REG                 (_regs+0x00)
#define NVM_D51_CONTROL_MANUAL_WRITE_MASK   (0x0030)

// The NVM register that stores lock status
#define NVM_LOCK_REG                    (_regs+0x20)
#define NVM_D51_UNLOCK_REG              (_regs+0x18)

// NVM security bit
#define NVM_SECURITY_BIT_REG            (_regs+0x18)
#define NVM_D51_SECURITY_BIT_REG        (0x41002000ul+0x2)
#define NVM_SECURITY_BIT_POS            (8)
#define NVM_D51_SECURITY_BIT_POS        (0)

// NVM status
#define NVM_STATUS_REG              (_regs+0x14)
#define NVM_D51_STATUS_REG          (_regs+0x12)
#define NVM_STATUS_READY_MASK       0x1

// NVM input register to some of the CMDEX commands.
#define NVM_ADDR_REG                    (_regs+0x1c)
#define NVM_D51_ADDR_REG                (_regs+0x14)

// NVM error
#define NVM_ERROR_REG                  (_regs+0x18)
#define NVM_D51_ERROR_REG              (_regs+0x10)
#define NVM_ERROR_MASK                 (0xFFEBu)
#define NVM_D51_ERROR_MASK             (0xF84Fu)
#define NVM_D51_ERROR_DONE_MASK        (0x1 << 0)
#define NVM_D51_ERROR_PROGE_MASK       (0x1 << 2)

// CMDEX field should be 0xA5 to allow execution of any command.
#define CMDEX_KEY             (0xa500u)

// List of NVM Commands.//as per datasheet prefix CMDEX
#define CMD_LOCK_REGION       (CMDEX_KEY | 0x0040u)
#define CMD_UNLOCK_REGION     (CMDEX_KEY | 0x0041u)
#define CMD_ERASE_ROW         (CMDEX_KEY | 0x0002u)
#define CMD_WRITE_PAGE        (CMDEX_KEY | 0x0004u)
#define CMD_CLEAR_PAGE_BUFFER (CMDEX_KEY | 0x0044u)
#define CMD_SET_SECURITY_BIT  (CMDEX_KEY | 0x0045u)

#define CMD_D51_LOCK_REGION       (CMDEX_KEY | 0x0011u)
#define CMD_D51_UNLOCK_REGION     (CMDEX_KEY | 0x0012u)
#define CMD_D51_ERASE_BLOCK       (CMDEX_KEY | 0x0001u)
#define CMD_D51_WRITE_PAGE        (CMDEX_KEY | 0x0003u)
#define CMD_D51_CLEAR_PAGE_BUFFER (CMDEX_KEY | 0x0015u)
#define CMD_D51_SET_SECURITY_BIT  (CMDEX_KEY | 0x0016u)
#define CMD_D51_RESET_CONTROLLER  (CMDEX_KEY | 0x0010u)

// This is the word size allowed by the communication layer of this bossa client, not the NVM word size which is 2 bytes
#define SAMBA_API_WORD_SIZE (4ul) // bytes

/* This class is designed specifically for SAM Dxx architecture in mind */
NvmFlash::NvmFlash(Samba& samba,
                   const std::string& name,
                   uint32_t addr,
                   uint32_t pages,
                   uint32_t size,
                   uint32_t planes,
                   uint32_t lockRegions,
                   uint32_t user,
                   uint32_t stack,
                   uint32_t regs,
                   uint32_t bodreg,
                   uint32_t bodresetbit,
                   uint32_t bodenablebit,
                   uint32_t pagesPerRow,
                   bool isD51,
                   bool canBrownout)
    : Flash(samba, name, addr, pages, size, planes, lockRegions, user, stack),
      _regs(regs), _bodreg(bodreg), _bodresetbit(bodresetbit), _bodenablebit(bodenablebit), _pagesPerRow(pagesPerRow), _isD51(isD51), _canBrownout(canBrownout)
{
    // Upon power up the NVM controller goes through a power up sequence.
    // During this time, access to the NVM controller is halted. Upon power up the
    // the NVM controller is operational without any need for user configuration.
}

NvmFlash::~NvmFlash()
{
}

void
NvmFlash::eraseAll()
{
    // Leave the first 8KB or 4KB, where bootloader resides, erase the rest.
    // Row is a concept used for convinence. When writing you have to write
    // page(s). When erasing you have to erase row(s) (block(s) for the D51).

    if (_samba.isChipEraseAvailable())
    {
        // If extended chip erase is available...

        _samba.chipErase(_addr);
        return;
    }

    // ...otherwise go with the legacy slow erase...

    // Calculate the number of rows (blocks with D51) that samba occupies.
      uint32_t starting_row = _addr / _size / _pagesPerRow;
      uint32_t total_rows = _pages / _pagesPerRow;

    for (uint32_t row=starting_row; row<total_rows; row++)
    {
        uint32_t addr_in_flash = (row * _pagesPerRow * pageSize());
        // The address is byte address, so convert it to word address (except with D51).
        if (_isD51 == false) {
          addr_in_flash = addr_in_flash / 2;
        }

        // Safe wait. Check and see if this is needed all the time
        while (!nvmIsReady())
        {
        }

        // Clear error bits
        if (_isD51 == true) {
          uint16_t status_reg = _samba.readWord(NVM_D51_ERROR_REG) & 0xffff;
          _samba.writeWord(NVM_D51_ERROR_REG, status_reg & NVM_D51_ERROR_MASK);
        } else {
          uint16_t status_reg = _samba.readWord(NVM_ERROR_REG) & 0xffff;
          _samba.writeWord(NVM_ERROR_REG, status_reg | NVM_ERROR_MASK);
        }

        // Issue erase command
        if (_isD51 == true) {
          _samba.writeWord(NVM_D51_ADDR_REG, addr_in_flash);
          executeNvmCommand(CMD_D51_ERASE_BLOCK);
        } else {
          _samba.writeWord(NVM_ADDR_REG, addr_in_flash);
          executeNvmCommand(CMD_ERASE_ROW);
        }
    }
}

bool
NvmFlash::nvmIsReady()
{
    uint8_t int_flag;

    if (_isD51 == true) {
      int_flag = _samba.readByte(NVM_D51_STATUS_REG) & NVM_STATUS_READY_MASK;
    } else {
      int_flag = _samba.readByte(NVM_STATUS_REG) & NVM_STATUS_READY_MASK;
    }

    return int_flag == 1;
}

void
NvmFlash::eraseAuto(bool enable)
{
    // Useless for SAMD, the flash controller doesn't have the auto erase function
}

bool
NvmFlash::isLocked()
{
    return getLockRegion(0);
}

// Returns true if locked, false otherwise.
bool
NvmFlash::getLockRegion(uint32_t region)
{
    uint32_t value;

    if (region >= _lockRegions)
    {
        throw FlashRegionError();
    }

    if (_isD51 == true) {
      value = _samba.readWord(NVM_D51_UNLOCK_REG);
    } else {
      value = _samba.readWord(NVM_LOCK_REG);
    }

    return ((value & (1 << region)) == 0); // 0 -> locked, 1 -> unlocked,
}

// Locks a given region number.
void
NvmFlash::setLockRegion(uint32_t region, bool enable)
{
    if (region >= _lockRegions)
    {
        throw FlashRegionError();
    }

    if (enable != getLockRegion(region))
    {
        if (enable)
        {
            // To lock a region you have to pass an address to the
            // ADDR register, and then execute "lock region" cmd
            // on the NVM controller.
            uint32_t addr_to_lock = getAddressByRegion(region);
            //addr_to_lock = addr_to_lock & 0x1fffff;
            if (_isD51 == true) {
              _samba.writeWord(NVM_D51_ADDR_REG, addr_to_lock);
              executeNvmCommand(CMD_D51_LOCK_REGION);
            } else {
              _samba.writeWord(NVM_ADDR_REG, addr_to_lock);
              executeNvmCommand(CMD_LOCK_REGION);
            }
        }
        else
        {
            uint32_t addr_to_unlock = getAddressByRegion(region);
            //addr_to_unlock = addr_to_unlock & 0x1fffff;
            if (_isD51 == true) {
              _samba.writeWord(NVM_D51_ADDR_REG, addr_to_unlock);
              executeNvmCommand(CMD_D51_UNLOCK_REGION);
            } else {
              _samba.writeWord(NVM_ADDR_REG, addr_to_unlock);
              executeNvmCommand(CMD_UNLOCK_REGION);
            }
        }
    }
}


// Read the security bit, returns true if set, false otherwise.
bool
NvmFlash::getSecurity()
{
    if (_isD51 == true) {
      uint16_t status_reg_value = _samba.readWord(NVM_D51_SECURITY_BIT_REG) & 0xffff;
      return (((status_reg_value >> NVM_D51_SECURITY_BIT_POS) & 0x1) == 1);
    } else {
      uint16_t status_reg_value = _samba.readWord(NVM_SECURITY_BIT_REG) & 0xffff;
      return (((status_reg_value >> NVM_SECURITY_BIT_POS) & 0x1) == 1);
    }
}

// Set's the security bit.
void
NvmFlash::setSecurity()
{
    if (!getSecurity())
    {
        if (_isD51 == true) {
          // Clear error bits
          uint16_t status_reg = _samba.readWord(NVM_D51_ERROR_REG) & 0xffff;
          _samba.writeWord(NVM_D51_ERROR_REG, status_reg & NVM_D51_ERROR_MASK);

          // Set security bit, wait for completion
          executeNvmCommand(CMD_D51_SET_SECURITY_BIT);
          while ((_samba.readByte(NVM_D51_ERROR_REG) & NVM_D51_ERROR_DONE_MASK) == 0);

          // Check for error
          status_reg = _samba.readWord(NVM_D51_ERROR_REG) & 0xffff;
          if (status_reg & NVM_D51_ERROR_PROGE_MASK)
          {
            throw NvmFlashCmdError("Error when setting security bit");
          } else {
            // Reset NVM controller so that the security status bit is updated
            // executeNvmCommand(CMD_D51_RESET_CONTROLLER);
          }
        } else {
          executeNvmCommand(CMD_SET_SECURITY_BIT);

          if (!getSecurity())
          {
            throw NvmFlashCmdError("Error when setting security bit");
          }
        }
    }
}

// Enable/disable the Bod. The values are lost on target reset.
void
NvmFlash::setBod(bool enable)
{
    uint32_t bod33_ctrl_reg = _samba.readWord(_bodreg);

    if(enable)
    {
        bod33_ctrl_reg |= _bodenablebit; // Enable the bod control
        _samba.writeWord(_bodreg, bod33_ctrl_reg);
    }
    else
    {
        bod33_ctrl_reg &= ~(_bodenablebit);
        _samba.writeWord(_bodreg, bod33_ctrl_reg);
    }
}

bool
NvmFlash::getBod()
{
    uint32_t value = _samba.readWord(_bodreg);
    bool bod_set = (value & _bodenablebit) ? true : false;

    return bod_set;
}

bool
NvmFlash::getBor()
{
    uint32_t bod33_ctrl_reg = _samba.readWord(_bodreg);
    bool reset_enabled = (bod33_ctrl_reg & _bodresetbit) ? true : false;

    return reset_enabled;
}

void
NvmFlash::setBor(bool enable)
{
    uint32_t bod33_ctrl_reg = _samba.readWord(_bodreg);

    if (enable)
    {
        bod33_ctrl_reg |= _bodresetbit;
        _samba.writeWord(_bodreg, bod33_ctrl_reg);
    }
    else
    {
        bod33_ctrl_reg &= ~(_bodresetbit);
        _samba.writeWord(_bodreg, bod33_ctrl_reg);
    }
}


bool
NvmFlash::getBootFlash()
{
    // Always boots from flash. No ROM boot available.
    return true;
}

void
NvmFlash::setBootFlash(bool enable)
{
    printf("Ignoring set boot from flash flag.\n");
}

void
NvmFlash::writePage(uint32_t page)
{
    if (page >= _pages)
    {
        throw FlashPageError();
    }

    // Clear page buffer
    if (_isD51 == true) {
      executeNvmCommand(CMD_D51_CLEAR_PAGE_BUFFER);
    } else {
      executeNvmCommand(CMD_CLEAR_PAGE_BUFFER);
    }

    // Clear error flags in target
    if (_isD51 == true) {
      uint16_t status_reg = _samba.readWord(NVM_D51_ERROR_REG) & 0xffff;
      _samba.writeWord(NVM_D51_ERROR_REG, status_reg & NVM_D51_ERROR_MASK);
    } else {
      uint16_t status_reg = _samba.readWord(NVM_ERROR_REG) & 0xffff;
      _samba.writeWord(NVM_ERROR_REG, status_reg | NVM_ERROR_MASK);
    }

    // Configure manual page write. This is critical for the write to work
    // irrespective of full page or partial page.
    if (_isD51 == true) {
      uint32_t ctrlb_reg = _samba.readWord(NVM_D51_CONTROL_REG);
      _samba.writeWord(NVM_D51_CONTROL_REG, ctrlb_reg & ~NVM_D51_CONTROL_MANUAL_WRITE_MASK); // clear WMODE bits
    } else {
      uint32_t ctrlb_reg = _samba.readWord(NVM_CONTROL_REG);
      _samba.writeWord(NVM_CONTROL_REG, ctrlb_reg | NVM_CONTROL_MANUAL_WRITE_BIT);
    }

    // Compute the start address.
    uint32_t addr = _addr + (page * _size );
    uint32_t addr_cached = addr;

    // Whole page data has been loaded via Flash::loadBuffer before the call
    // to this function in Flasher::write
    _wordCopy.setDstAddr(addr);
    _wordCopy.setSrcAddr(_onBufferA ? _pageBufferA : _pageBufferB);
    _onBufferA = !_onBufferA;
    while (!nvmIsReady());
    _wordCopy.runv();

    while (!nvmIsReady());

    if (_isD51 == true) {
      // Reset the buffer, so that subsequent reads are clear
      _samba.writeWord(NVM_D51_ADDR_REG, addr_cached);
      executeNvmCommand(CMD_D51_WRITE_PAGE);
    } else {
      // Reset the buffer, so that subsequent reads are clear
      _samba.writeWord(NVM_ADDR_REG, addr_cached >> 1); // Convert byte address to word addres
      executeNvmCommand(CMD_WRITE_PAGE);
    }
}

void
NvmFlash::readPage(uint32_t page, uint8_t* buf)
{
    if (page >= _pages)
    {
        throw FlashPageError();
    }

    // Convert page number into physical address.
    // The flash base should be defined as starting after bootloader (ie at 0x00002000 => page number 128)
    // flash_base_address + page.no * page_size
    uint32_t addr = _addr + (page * pageSize());
    _samba.read(addr, buf, pageSize());
}

// Returns the start address of a specified region number
// based on the flash specifications. The returned address is
// word address (or byte address for D51).
uint32_t
NvmFlash::getAddressByRegion(uint32_t region_num)
{
    if (region_num >= _lockRegions)
    {
        throw FlashRegionError();
    }

    uint32_t size_of_region = (pageSize() * _pages) / _lockRegions; // Flash Size / no of lock regions
    uint32_t addr = address() + (region_num * size_of_region);
    if (_isD51 == false) {
      addr = addr / 2; // Convert byte address to word address if not D51
    }

    return addr;
}

void
NvmFlash::executeNvmCommand(uint32_t cmd)
{
    // Wait for the nvm controller to be ready
    while (!nvmIsReady())
    {
    }

    // Send the comamnd to nvm controller.
    if (_isD51 == true) {
      _samba.writeWord(NVM_D51_COMMAND_REG, cmd);
    } else {
      _samba.writeWord(NVM_COMMAND_REG, cmd);
    }

    // Wait till the operation completes.
    while (!nvmIsReady())
    {
    }

    //TODO : check and return return error status from nvm status flag.
}
