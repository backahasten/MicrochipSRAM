/*******************************************************************************************************************
** Class definition header for the MicrochipSRAM class. This library allows reading from and writing to any of    **
** Microchip family of Serial SRAM and Serial NVSRAM memory chips. The list can be found at Microchip's page      **
** at http://www.microchip.com/design-centers/memory/serial-sram-serial-nvsram/overview.                          **
**                                                                                                                **
** At the time of coding this library, the following memories can be used: 23x640 (64Kbit), 23x256 (256Kbit),     **
**                                                                                                                **
** 23x512 (512Kbit), 23xx1024 (1Mbit), 23LCV512 (512Kbit) and 23LCV1024 (1Mbit).                                  **
**                                                                                                                **
** The memory has 3 operating modes: byte, (32-byte) Page and sequential. All read or write command are started   **
** by pulling the SPI CS/SS pin low and sending 2 or 3 address bytes (depending upon which memory is being used)  **
** and then either the data is sent to the memory or read from it starting at that addessr.  The default power-on **
** mode is the byte mode, where only one byte is transmitted. The next mode, page, will transmit data within the  **
** 32-Byte page, wrapping around to the beginning of the page when the end is reached. The last mode, sequential, **
** ignores page boundaries and reads/writes sequentially until the last memory position is reached, whereupon it  **
** wraps around to the beginning again.                                                                           **
**                                                                                                                **
** This library switches the memory to use sequential mode in order to simplify and speed up data transfer        **
**                                                                                                                **
** Instantiating the class involves turning on SPI for the CS/SS pin passed in, and then performing some read and **
** write operations to determine which memory is actually being used. First, a test to see whether 2 or 3 address **
** bytes are used. Only the 1Mbit chips use 3 address bytes, but if 2 address bytes are in use then two bytes are **
** written to the last address, and if the first byte of the chip has a changed value then we know we've gotten a **
** wraparound and have determined the memory size. If the first byte hasn't changed, then the actual memory has   **
** to be a larger one, and the next possible size is tested                                                       **
**                                                                                                                **
** Although programming for the Arduino and in c/c++ is new to me, I'm a professional programmer and have learned,**
** over the years, that it is much easier to ignore superfluous comments than it is to decipher non-existent ones;**
** so both my comments and variable names tend to be verbose. The code is written to fit in the first 80 spaces   **
** and the comments start after that and go to column 117 - allowing the code to be printed in A4 landscape mode. **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer           Comments                                                                 **
** ====== ========== =================== ======================================================================== **
** 1.0.3  2017-04-25 Arnd@SV-Zanshin.Com Removed default value definition from clearMemory function definition    **
** 1.0.2  2016-11-20 Arnd@SV-Zanshin.Com Changed constructor to only use CS/SS pin, moved "SRAMBytes" to public   **
** 1.0.1  2016-11-19 Arnd@SV-Zanshin.Com Added method "clearMemory"                                               **
** 1.0.0  2016-11-19 Arnd@SV-Zanshin.Com Cleaned up, published  https://github.com/SV-Zanshin/MicrochipSRAM       **
** 1.0.b1 2016-11-16 Arnd@SV-Zanshin.Com Created class                                                            **
**                                                                                                                **
*******************************************************************************************************************/
#include "Arduino.h"                                                          // Arduino data type definitions    //
#include <SPI.h>                                                              // SPI (Serial Peripheral Interface)//
#ifndef MicrochipSRAM_h                                                       // Guard code definition            //
  #define MicrochipSRAM_h                                                     // Define the name inside guard code//
    /***************************************************************************************************************
    ** Declare constants used in the class                                                                        **
    ***************************************************************************************************************/
    const uint8_t  SRAM_WRITE_MODE_REG =      0x01;                           // Write the mode register          //
    const uint8_t  SRAM_READ_MODE_REG  =      0x05;                           // Read the mode register           //
    const uint8_t  SRAM_BYTE_MODE      = B00000000;                           // 2MSB 00 is Byte mode             //
    const uint8_t  SRAM_PAGE_MODE      = B10000000;                           // 2MSB 10 is page mode             //
    const uint8_t  SRAM_SEQ_MODE       = B11000000;                           // 2MSB 11 is sequential mode       //
    const uint32_t SRAM_1024           =    131072;                           // Equates to 1mbit of storage      //
    const uint32_t SRAM_512            =     65536;                           // Equates to 512kbit of storage    //
    const uint32_t SRAM_256            =     32768;                           // Equates to 256kbit of storage    //
    const uint32_t SRAM_128            =     16384;                           // Equates to 128kbit of storage    //
    const uint32_t SRAM_64             =      8192;                           // Equates to 64kbit of storage     //
    const uint8_t  SRAM_WRITE_CODE     =         2;                           // Write                            //
    const uint8_t  SRAM_READ_CODE      =         3;                           // Read                             //
  class MicrochipSRAM {                                                       // Class definition                 //
    public:                                                                   // Publicly visible methods         //
      MicrochipSRAM(const uint8_t SSPin);                                     // Class constructor                //
      ~MicrochipSRAM();                                                       // Class destructor                 //
      void clearMemory(const uint8_t clearValue = 0);                         // Clear all memory to one value    //
      /*************************************************************************************************************
      ** Declare the get and put methods as template functions here in the header file. This allows any type of   **
      ** variable or structure to be used rather than having to make one function for each datatype used. Note    **
      ** that due to the sequential mode being active, reads and writes that go past the last existing address    **
      ** will automatically wrap back to the beginning of the memory                                              **
      *************************************************************************************************************/
      template< typename T > uint32_t &get(const uint32_t addr,T &value) {    // method to write a structure      //
        uint8_t* bytePtr       = (uint8_t*)&value;                            // Pointer to structure beginning   //
        uint32_t returnAddress = (addr+sizeof(T))%SRAMBytes;                  // compute the return address       //
        digitalWrite(_SSPin,LOW);                                             // Pull CS/SS low to select device  //
        SPI.transfer(SRAM_READ_CODE);                                         // Send the command for WRITE mode  //
        if (SRAMBytes==SRAM_1024) SPI.transfer((uint8_t)(addr>>16)&0xFF);     // Send the MSB of the 24bit address//
        SPI.transfer((uint8_t)(addr>>8) & 0xFF);                              // Send the 2nd byte of the address //
        SPI.transfer((uint8_t)addr);                                          // Send the LSB of the address      //
        for (uint32_t i=0;i<sizeof(T);i++) *bytePtr++ = SPI.transfer(0x00);   // loop for each byte to be read    //
        digitalWrite(_SSPin,HIGH);                                            // Pull the SS/CS high to deselect  //
        return(returnAddress);                                                // Return the computed new address  //
      } // of method get                                                      //----------------------------------//
      template<typename T> uint32_t &put(const uint32_t addr,const T &value){ // method to get a structure        //
        const uint8_t* bytePtr = (const uint8_t*)&value;                      // Pointer to structure beginning   //
        uint32_t returnAddress = (addr+sizeof(T))%SRAMBytes;                  // compute the return address       //
        digitalWrite(_SSPin,LOW);                                             // Pull CS/SS low to select device  //
        SPI.transfer(SRAM_WRITE_CODE);                                        // Send the command for WRITE mode  //
        if (SRAMBytes==SRAM_1024) SPI.transfer((uint8_t)(addr>>16)&0xFF);     // Send the MSB of the 24bit address//
        SPI.transfer((uint8_t)(addr>>8) & 0xFF);                              // Send the 2nd byte of the address //
        SPI.transfer((uint8_t)addr);                                          // Send the LSB of the address      //
        for (uint32_t i=0;i<sizeof(T);i++) SPI.transfer(*bytePtr++);          // loop for each byte to be written //
        digitalWrite(_SSPin,HIGH);                                            // Pull the SS/CS high to deselect  //
        return(returnAddress);                                                // Return the computed new address  //
      } // of method put                                                      //----------------------------------//
      template< typename T > &fillMemory( uint32_t addr, T &value ) {         // method to fill memory with values//
        while(addr<(SRAMBytes-sizeof(T))) addr = put(addr,value);             // loop until we reach end of memory//
      } // of method fillMemory                                               //----------------------------------//
      uint32_t SRAMBytes = 0;                                                 // Number of bytes available on chip//
    private:                                                                  // Private variables and methods    //
      uint8_t  _SSPin    = 0;                                                 // The CS/SS pin attached           //
  }; // of MicrochipSRAM class definition                                     //                                  //
#endif                                                                        //----------------------------------//
