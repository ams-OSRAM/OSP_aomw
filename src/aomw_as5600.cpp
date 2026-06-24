// aomw_as5600.cpp - driver for ams-OSRAM AS5600 magnetic rotary sensor
/*****************************************************************************
 * Copyright 2025, 2026 by ams OSRAM AG                                      *
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
#include <aomw_as5600.h>   // own


// This driver assumes an ams-OSRAM AS5600 rotary sensor is connected 
// to a SAID I2C bus. This driver is not multi-instance. It can only control 
// one AS5600; it must have I2C device address AOMW_AS5600_DADDR7_SAIDSENSE. In
// other words; this driver is associated with one rotary sensor. 
// The association is established with a call to aomw_as5600_init(addr).

// This driver is not very generic; it just configures the sensor for 
// continuous angle measurements.


// Address of the SAID with I2C bridge connected to the rotary sensor
static uint16_t aomw_as5600_saidaddr;   


// Registers of AS5600 (datasheet order, most not used)
// Registers tagged as 1R are 8 bit, 2R are 16 bit.
#define AOMW_AS5600_1R00_ZMCO                0x00
#define AOMW_AS5600_2R01_ZPOS                0x01
#define AOMW_AS5600_2R03_MPOS                0x03
#define AOMW_AS5600_2R05_MANG                0x05
#define AOMW_AS5600_2R07_CONF                0x07
#define   AOMW_AS5600_2R07_CONF_PM_NOM         0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_PM_LPM1        0b0000000000000001
#define   AOMW_AS5600_2R07_CONF_PM_LPM2        0b0000000000000010
#define   AOMW_AS5600_2R07_CONF_PM_LPM3        0b0000000000000011
#define   AOMW_AS5600_2R07_CONF_HYST_OFF       0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_HYST_1LSB      0b0000000000000100
#define   AOMW_AS5600_2R07_CONF_HYST_2LSB      0b0000000000001000
#define   AOMW_AS5600_2R07_CONF_HYST_3LSB      0b0000000000001100
#define   AOMW_AS5600_2R07_CONF_OUTS_ANAFR     0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_OUTS_ANARR     0b0000000000010000
#define   AOMW_AS5600_2R07_CONF_OUTS_DIG       0b0000000000100000
#define   AOMW_AS5600_2R07_CONF_PWMF_115HZ     0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_PWMF_230HZ     0b0000000001000000
#define   AOMW_AS5600_2R07_CONF_PWMF_460HZ     0b0000000010000000
#define   AOMW_AS5600_2R07_CONF_PWMF_920HZ     0b0000000011000000
#define   AOMW_AS5600_2R07_CONF_SF_16X         0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_SF_8X          0b0000000100000000
#define   AOMW_AS5600_2R07_CONF_SF_4X          0b0000001000000000
#define   AOMW_AS5600_2R07_CONF_SF_2X          0b0000001100000000
#define   AOMW_AS5600_2R07_CONF_FTH_SLOW       0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_FTH_6LSB       0b0000010000000000
#define   AOMW_AS5600_2R07_CONF_FTH_7LSB       0b0000100000000000
#define   AOMW_AS5600_2R07_CONF_FTH_9LSB       0b0000110000000000
#define   AOMW_AS5600_2R07_CONF_FTH_18LSB      0b0001000000000000
#define   AOMW_AS5600_2R07_CONF_FTH_21LSB      0b0001010000000000
#define   AOMW_AS5600_2R07_CONF_FTH_24LSB      0b0001100000000000
#define   AOMW_AS5600_2R07_CONF_FTH_10LSB      0b0001110000000000
#define   AOMW_AS5600_2R07_CONF_WD_OFF         0b0000000000000000
#define   AOMW_AS5600_2R07_CONF_WD_ON          0b0010000000000000
#define AOMW_AS5600_2R0C_RAWANGLE            0x0C
#define AOMW_AS5600_2R0E_ANGLE               0x0E
#define AOMW_AS5600_1R0B_STATUS              0x0B
#define   AOMW_AS5600_1R0B_STATUS_MD           0b00100000
#define   AOMW_AS5600_1R0B_STATUS_ML           0b00010000
#define   AOMW_AS5600_1R0B_STATUS_MH           0b00001000
#define AOMW_AS5600_1R1A_AGC                 0x1A
#define   AOMW_AS5600_1R1A_AGC_MID             0x80
#define AOMW_AS5600_2R1B_MAGNITUDE           0x1B
#define AOMW_AS5600_1RFF_BURN                0xFF


// Performs a hardwired configuration
static aoresult_t aomw_as5600_conf() {
  aoresult_t result;
  uint8_t buf[2];

  // Clear ZPOS
  buf[0]= 0x00; buf[1]= 0x00;
  result= aoosp_exec_i2cwrite8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_2R01_ZPOS, buf, 2 );
  if( result!=aoresult_ok ) return result;
  
  // Clear MPOS
  buf[0]= 0x00; buf[1]= 0x00;
  result= aoosp_exec_i2cwrite8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_2R03_MPOS, buf, 2 );
  if( result!=aoresult_ok ) return result;
  
  // Clear MANG
  buf[0]= 0x00; buf[1]= 0x00;
  result= aoosp_exec_i2cwrite8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_2R05_MANG, buf, 2 );
  if( result!=aoresult_ok ) return result;

  // CONF to default
  uint16_t conf = 
    AOMW_AS5600_2R07_CONF_WD_OFF | AOMW_AS5600_2R07_CONF_FTH_SLOW | AOMW_AS5600_2R07_CONF_SF_16X |
    AOMW_AS5600_2R07_CONF_PWMF_115HZ | AOMW_AS5600_2R07_CONF_OUTS_ANAFR | AOMW_AS5600_2R07_CONF_HYST_OFF | AOMW_AS5600_2R07_CONF_PM_NOM;
  buf[0]= (conf >> 8 ) & 0xFF;
  buf[1]= (conf >> 0 ) & 0xFF; 
  result= aoosp_exec_i2cwrite8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_2R07_CONF, buf, 2 );
  if( result!=aoresult_ok ) return result;
  
  // AGC to default
  buf[0]= AOMW_AS5600_1R1A_AGC_MID;
  result= aoosp_exec_i2cwrite8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_1R1A_AGC, buf, 1 );
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}


// === main =================================================================


/*!
    @brief  Reads and returns the magnet angle measured by the AS5600 rotary sensor.
    @param  *angle
            Out parameter for the read angle.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a rotary sensor is associated with this
            library via `aomw_as5600_init()`.
    @note   The angle is a raw 12 bit value, mapped to one full rotation.
*/
aoresult_t aomw_as5600_angle_get(int*angle) {
  aoresult_t result;
  uint8_t buf[2];
  
  // Use processed angle AOMW_AS5600_2R0E_ANGLE 
  // but conf() cleared offset so should be same as RAWANGLE
  result= aoosp_exec_i2cread8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_2R0E_ANGLE,buf,2);
  if( result!=aoresult_ok ) return result;

  *angle= buf[0]*256 + buf[1];
  return aoresult_ok;
}


/*!
    @brief  Reads and returns the magnet magnitude and agc.
            They are combined into a force level.
    @param  *agc
            Out parameter for the automatic gain control chosen by the sensor.
    @param  *mag
            Out parameter for the magnetic magnitude.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a rotary sensor is associated with this
            library via `aomw_as5600_init()`.
    @note   The `mag` is a raw 12 bit value, the `agc` a raw 7 bit value (at 3V3).
    @note   An output parameter may be passed as NULL.
*/
aoresult_t aomw_as5600_force_get(int*agc, int*mag) {
  aoresult_t result;
  uint8_t buf[3];
  
  // Use processed angle AOMW_AS5600_2R0E_ANGLE 
  // but conf() cleared offset so should be same as RAWANGLE
  result= aoosp_exec_i2cread8(aomw_as5600_saidaddr,AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_1R1A_AGC,buf,3);
  if( result!=aoresult_ok ) return result;

  // Assign to out parameters
  if( agc   ) *agc  = buf[0];
  if( mag   ) *mag  = buf[1]*256 + buf[2];
  
  return aoresult_ok;
}


/*!
    @brief  Tests if an AS5600 rotary sensor is connected to the I2C 
            bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @return aoresult_ok              if sensor is present on the I2C bus of addr
            aoresult_dev_noi2cdev    if sensor is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            aoresult_comparefail     if magnet is absent
            aoresult_other           if magnet is too weak or too strong
            other                    OSP (communication) errors
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the rotary sensor has I2C device address
            AOMW_AS5600_DADDR7_SAIDSENSE.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
    @note   Since the AS5600 does not have an ID register, this function 
            reads STATUS and checks if the magnet is in range.
*/
aoresult_t aomw_as5600_present(uint16_t addr ) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  // Read AOMW_AS5600_1R0B_STATUS
  uint8_t status;
  result = aoosp_exec_i2cread8(addr, AOMW_AS5600_DADDR7_SAIDSENSE, AOMW_AS5600_1R0B_STATUS,&status, 1);
  // Real error
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  // Magnet present
  if( ! (status & AOMW_AS5600_1R0B_STATUS_MD) ) return aoresult_comparefail;
  if(    status & AOMW_AS5600_1R0B_STATUS_ML  ) return aoresult_other;
  if(    status & AOMW_AS5600_1R0B_STATUS_MH  ) return aoresult_other;
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the AS5600 rotary sensor 
            connected to the I2C bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with a rotary sensor.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the rotary sensor has I2C device address
            AOMW_AS5600_DADDR7_SAIDSENSE.
    @note   It is allowed to call this function again, to associate this
            driver with a different sensor.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_as5600_init(uint16_t addr) {
  aoresult_t result;

  // Record address of the SAID with I2C bridge
  aomw_as5600_saidaddr= addr;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  result= aomw_as5600_conf();
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}


