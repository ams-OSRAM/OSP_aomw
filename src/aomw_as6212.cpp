// aomw_as6212.cpp - driver for ams-OSRAM AS6212 temperature sensor
/*****************************************************************************
 * Copyright 2025 by ams OSRAM AG                                            *
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
#include <Arduino.h>       // Serial hidden in AORESULT_ASSERT
#include <aoosp.h>         // aoosp_exec_i2cwrite8()
#include <aomw_as6212.h>   // own


// This driver assumes an ams-OSRAM AS6212 temperature sensor is connected 
// to a SAID I2C bus. This driver is not multi-instance. It can only control 
// one AS6212; it must have I2C device address AOMW_AS6212_DADDR7_SAIDSENSE. 
// In other words; this driver is associated with one temperature sensor. 
// The association is established with a call to aomw_as6212_init(addr).

// This driver does not (yet) support the thresholds of the AS6212.
// This means that the CONFIG fields AL, IM, POL, CF[] are not supported.

// This driver does not (yet) support sleep mode of the AS6212.
// This means that the CONFIG fields SM and SS are not supported.


// Fields of the CONFIG register      S__CCPISCCA_____    
#define AOMW_AS6212_CONFIG_DFLT     0b0100000010100000
#define AOMW_AS6212_CONFIG_CR_MASK  0b1111111100111111
#define AOMW_AS6212_CONFIG_CR_SHIFT 6


// Map 16 bits to hi and lo byte
#define  HI8(b16) ((uint8_t) (((b16)>>8)&0xFF) )
#define  LO8(b16) ((uint8_t) (((b16)>>0)&0xFF) )


// TVAL scale
#define AOMW_AS6212_TVAL_SCALE      128 


// Address of the SAID with I2C bridge connected to the temperature sensor
static uint16_t aomw_as6212_saidaddr;   


// Registers of the AS6212
#define AOMW_AS6212_TVAL   0x00 // Temperature register; contains the temperature value
#define AOMW_AS6212_CONFIG 0x01 // Configuration register; configuration settings of the temperature sensor
#define AOMW_AS6212_TLOW   0x02 // TLOW register; low temperature threshold value
#define AOMW_AS6212_THIGH  0x03 // THIGH register; high temperature threshold value


/*!
    @brief  Configures the conversion rate of the AS6212 temperature sensor.
    @param  ms
            The requested configuration rate.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a temperature sensor is associated with this
            library via `aomw_as6212_init()`.
    @note   Only some fixed conversion rates are supported (4000, 1000, 250, 
            and 125ms). A supported rate close to the passed `ms` is picked.
*/
aoresult_t aomw_as6212_convrate_set(int ms) {
  AORESULT_ASSERT(aomw_as6212_saidaddr!=0); // Forgot aomw_as6212_init()?
  aoresult_t result;
  // Round ms to conversion rate
  uint16_t cr;
  // CR ms
  // 00 4000
  // 01 1000
  // 10 250
  // 11 125
  if(      ms>2000 ) cr=0b00; // 4000
  else if( ms>500  ) cr=0b01; // 1000
  else if( ms>150  ) cr=0b10; // 250
  else               cr=0b11; // 125
  // Mask conversion rate into default config
  uint16_t config= AOMW_AS6212_CONFIG_DFLT & ~AOMW_AS6212_CONFIG_CR_MASK;
  config |= cr<<AOMW_AS6212_CONFIG_CR_SHIFT;
  // Ensure endianess
  uint8_t buf[2] = {HI8(config), LO8(config)}; // AS6212 is big endian
  result = aoosp_exec_i2cwrite8(aomw_as6212_saidaddr, AOMW_AS6212_DADDR7_SAIDSENSE, AOMW_AS6212_CONFIG, buf, 2);
  return result;
}


/*!
    @brief  Reads and returns the conversion rate of the AS6212 temperature sensor.
    @param  *ms
            Out parameter for the read configuration rate.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a temperature sensor is associated with this
            library via `aomw_as6212_init()`.
*/
aoresult_t aomw_as6212_convrate_get(int *ms) {
  AORESULT_ASSERT(aomw_as6212_saidaddr!=0); // Forgot aomw_as6212_init()?
  aoresult_t result;
  *ms= -1;
  // Get CONFIG register
  uint8_t buf[2];
  result = aoosp_exec_i2cread8(aomw_as6212_saidaddr, AOMW_AS6212_DADDR7_SAIDSENSE, AOMW_AS6212_CONFIG, buf, 2);
  if( result!=aoresult_ok ) return result;
  uint16_t config=buf[1]*256+buf[0];
  // Extract conversion rate
  int cr = (config & AOMW_AS6212_CONFIG_CR_MASK)>>AOMW_AS6212_CONFIG_CR_SHIFT;
  if( cr==0 ) *ms=4000;
  else if( cr==1 ) *ms=1000;
  else if( cr==2 ) *ms=250;
  else  *ms=125;
  return aoresult_ok;
}


/*!
    @brief  Reads and returns the temperature measured by the AS6212 temperature sensor.
    @param  *millicelsius
            Out parameter for the read temperature.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a temperature sensor is associated with this
            library via `aomw_as6212_init()`.
    @note   To stay in the integer domain, the temperature is returned
            multiplied by 1000 ("milli Celsius").
*/
aoresult_t aomw_as6212_temp_get(int*millicelsius) {
  AORESULT_ASSERT(aomw_as6212_saidaddr!=0); // Forgot aomw_as6212_init()?
  aoresult_t result;
  *millicelsius= -1;
  // Get TVAL register
  uint8_t buf[2];
  result = aoosp_exec_i2cread8(aomw_as6212_saidaddr, AOMW_AS6212_DADDR7_SAIDSENSE, AOMW_AS6212_TVAL, buf, 2);
  if( result!=aoresult_ok ) return result;
  // Convert buffer (big endian) to milli Celsius
  uint16_t tval=buf[0]*256+buf[1];
  *millicelsius= 1000*(int16_t)tval/AOMW_AS6212_TVAL_SCALE;
  return aoresult_ok;
}


// === main =================================================================


/*!
    @brief  Tests if an AS6212 temperature sensor is connected to the I2C 
            bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @return aoresult_ok              if sensor is present on the I2C bus of addr
            aoresult_dev_noi2cdev    if sensor is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            other                    OSP (communication) errors
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the temperature sensor has I2C device address
            AOMW_AS6212_DADDR7_SAIDSENSE.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_as6212_present(uint16_t addr ) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  // Check if we can read from 00
  uint8_t buf[1];
  result = aoosp_exec_i2cread8(addr, AOMW_AS6212_DADDR7_SAIDSENSE, 0x00, buf, 1);
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  // Real error
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the temperature sensor 
            connected to the I2C bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with a temperature sensor.
    @return aoresult_ok           if init was successful
            other error code      if there is a (communications) error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the temperature sensor has I2C device address
            AOMW_AS6212_DADDR7_SAIDSENSE.
    @note   It is allowed to call this function again, to associate this
            driver with a different sensor.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_as6212_init(uint16_t addr) {
  aoresult_t result;

  // Record address of the SAID with I2C bridge
  aomw_as6212_saidaddr= addr;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  // Reset CONFIG; AS6212 is big endian
  uint8_t buf[2] = {HI8(AOMW_AS6212_CONFIG_DFLT), LO8(AOMW_AS6212_CONFIG_DFLT)}; 
  result = aoosp_exec_i2cwrite8(addr, AOMW_AS6212_DADDR7_SAIDSENSE, AOMW_AS6212_CONFIG, buf, 2);
  
  return result;
}


