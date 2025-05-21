// aomw_eeprom.cpp - driver for an I2C EEPROM (AT24C02C) connected to a SAID
/*****************************************************************************
 * Copyright 2024,2025 by ams OSRAM AG                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************/
#include <Arduino.h>      // delay()
#include <aoosp.h>        // aoosp_exec_i2cwrite8()
#include <aomw_eeprom.h>  // own
#include <string.h>       // memcpy()


// This is a driver for EEPROM memories.
// Each memory location stores one byte.
// The memory must have 8 bit addresses, so max memory size is 256 bytes.
// The EEPROM is connected with I2C to a SAID and with OSP to the MCU.


// Maximum read size is dictated by telegrams size
#define AOMW_EEPROM_MAXREADCHUNK 8
// The size of a page inside the EEPROM
#define AOMW_EEPROM_PAGESIZE     8


/*!
    @brief  Checks if an EEPROM with the 7-bit device address `daddr7` is
            connected to (the I2C bridge of) OSP node with address `addr`.
    @param  addr
            The address of the OSP node (with the I2C bridge).
    @param  daddr7
            The I2C device address of the EEPROM.
    @return aoresult_ok           if EEPROM found
            aoresult_dev_noi2cdev if EEPROM not found
            other error code      if there is a (communications) error
    @note   The test is a one-byte read from register 0 of device daddr7,
            so this might lead to false positives.
    @note   The OSP chain must be initialized (eg with aoosp_exec_resetinit(),
            or aomw_topo_build()) and I2C bridge of `addr` must be powered
            (eg with aoosp_exec_i2cpower()).
    @note   Typical values for `daddr7` are AOMW_EEPROM_DADDR7_XXX.
*/
aoresult_t aomw_eeprom_present(uint16_t addr, uint8_t daddr7 ) {
  uint8_t buf;
  aoresult_t result;
  result = aoosp_exec_i2cread8(addr, daddr7, 0x00, &buf, 1);
  // Real error
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  return aoresult_ok;
}


/*!
    @brief  Reads `count` bytes into buffer `buf` from an EEPROM with the
            7-bit I2C device address `daddr7` connected to (the I2C bridge
            of) the OSP node with address `addr`. The EEPROM will be read
            from (register) address `raddr` and further.
    @param  addr
            The address of the OSP node (with the I2C bridge).
    @param  daddr7
            The I2C device address of the EEPROM.
    @param  raddr
            The address of the register in the EEPROM from where to read).
    @param  buf
            The (address of the) buffer to store the read bytes into.
    @param  count
            The number of bytes to read (`buf` must have at least this size).
    @return aoresult_ok           if read was successful
            other error code      if there is a (communications) error
    @note   The OSP chain must be initialized (eg with aoosp_exec_resetinit(),
            or aomw_topo_build()) and I2C bridge of `addr` must be powered
            (eg with aoosp_exec_i2cpower()).
    @note   Typical values for `daddr7` are AOMW_EEPROM_DADDR7_XXX.
*/
aoresult_t aomw_eeprom_read(uint16_t addr, uint8_t daddr7, uint8_t raddr, uint8_t *buf, int count ) {
  if( raddr+count>256 ) return aoresult_outofmem;
  aoresult_t result;
  while( count>0 ) {
    uint8_t chunk= count > AOMW_EEPROM_MAXREADCHUNK  ?  AOMW_EEPROM_MAXREADCHUNK  :  count;
    result= aoosp_exec_i2cread8(addr, daddr7, raddr, buf, chunk);
    if( result!=aoresult_ok ) return result;
    raddr+= chunk;
    buf+= chunk;
    count-= chunk;
  }
  return aoresult_ok;
}


/*!
    @brief  Writes `count` bytes from buffer `buf` to an EEPROM with the
            7-bit I2C device address `daddr7` connected to (the I2C bridge
            of) the OSP node with address `addr`. The EEPROM will be written
            at (register) address `raddr` and further.
    @param  addr
            The address of the OSP node (with the I2C bridge).
    @param  daddr7
            The I2C device address of the EEPROM.
    @param  raddr
            The address of the register in the EEPROM from where to write).
    @param  buf
            The (address of the) buffer containing the bytes to write.
    @param  count
            The number of bytes to read (`buf` must have at least this size).
    @return aoresult_ok           if write was successful.
            other error code      if there is a (communications) error
    @note   The OSP chain must be initialized (eg with aoosp_exec_resetinit(),
            or aomw_topo_build()) and I2C bridge of `addr` must be powered
            (eg with aoosp_exec_i2cpower()).
    @note   Typical values for `daddr7` are AOMW_EEPROM_DADDR7_XXX.
*/
aoresult_t aomw_eeprom_write(uint16_t addr, uint8_t daddr7, uint8_t raddr, const uint8_t *buf, int count ) {
  if( raddr+count>256 ) return aoresult_outofmem;
  // There are several issues when writing to EEPROM
  // (1) When an I2C write transaction to an EEPROM is completed (when STOP
  //     received), the EEPROM starts an internal write cycle. The "Self-timed
  //     write cycle" takes 5ms max (AT24C02C), so we want to minimize the
  //     amount of write cycles.
  // (2) Writes are buffered in a 8 byte "page" buffer, and the target address
  //     should not cross a 16 byte boundary. Note, some EEPROMs have a page
  //     size of 16, using 8 is safe in all cases (but slightly slower)
  // (3) aoosp_exec_i2cwrite8() only allows payloads of  1, 2, 4, or 6 bytes
  aoresult_t result;
  while( count>0 ) {
    int fit_in_page   = AOMW_EEPROM_PAGESIZE - (raddr % AOMW_EEPROM_PAGESIZE); // see (2)
    int write_to_page = count > fit_in_page ? fit_in_page : count;
    int chunk;  // see (3)
    if( write_to_page>=6 ) chunk=6;
    else if( write_to_page>=4 ) chunk=4;
    else if( write_to_page>=2 ) chunk=2;
    else chunk=1;
    // Serial.printf("eeprom write %02x %d -> %s\n",raddr, chunk, aoosp_buf_str(buf, chunk) );
    result= aoosp_exec_i2cwrite8(addr, daddr7, raddr, buf, chunk);
    delay(5); // see (1)
    if( result!=aoresult_ok ) return result;
    raddr+= chunk;
    buf+= chunk;
    count-= chunk;
  }
  return aoresult_ok;
}


/*!
    @brief  Reads `count` bytes from an EEPROM with the 7-bit I2C device
            address `daddr7` connected to (the I2C bridge of) the OSP node
            with address `addr` and compares them to `buf`. The EEPROM will
            be read from (register) address `raddr` and further.
    @param  addr
            The address of the OSP node (with the I2C bridge).
    @param  daddr7
            The I2C device address of the EEPROM.
    @param  raddr
            The address of the register in the EEPROM from where to read).
    @param  buf
            The (address of the) buffer to storing the bytes to compare with.
    @param  count
            The number of bytes to read (`buf` must have at least this size).
    @return aoresult_ok           if compare was successful
            aoresult_comparefail  if compare fails (buf differs from EEPROM)
            other error code      if there is a (communications) error
    @note   The OSP chain must be initialized (eg with aoosp_exec_resetinit(),
            or aomw_topo_build()) and I2C bridge of `addr` must be powered
            (eg with aoosp_exec_i2cpower()).
    @note   Typical values for `daddr7` are AOMW_EEPROM_DADDR7_XXX.
*/
aoresult_t aomw_eeprom_compare(uint16_t addr, uint8_t daddr7, uint8_t raddr, const uint8_t *buf, int count ) {
  if( raddr+count>256 ) return aoresult_outofmem;
  aoresult_t result;
  uint8_t tmp[8];
  while( count>0 ) {
    uint8_t chunk= count > AOMW_EEPROM_MAXREADCHUNK  ?  AOMW_EEPROM_MAXREADCHUNK  :  count;
    result= aoosp_exec_i2cread8(addr, daddr7, raddr, tmp, chunk);
    if( result!=aoresult_ok ) return result;
    if( memcmp(tmp,buf,chunk)!=0 ) {
      // Serial.printf("EEPROM %02x: %s\n", raddr,aoosp_prt_bytes(tmp,chunk) );
      // Serial.printf("MCUROM %02x: %s\n", raddr,aoosp_prt_bytes(buf,chunk) );
      return aoresult_comparefail;
    }
    raddr+= chunk;
    buf+= chunk;
    count-= chunk;
  }
  return aoresult_ok;
}

