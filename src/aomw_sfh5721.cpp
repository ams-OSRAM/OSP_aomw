// aomw_sfh5721.cpp - driver for ams-OSRAM SFH 5721 ambient light sensor
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
#include <aomw_sfh5721.h> // own


// This driver assumes an ams-OSRAM SFH5721 light sensor is connected 
// to a SAID I2C bus. This driver is not multi-instance. It can only control 
// one SFH5721; it must have I2C device address AOMW_SFH5721_DADDR7_SAIDSENSE. 
// In other words; this driver is associated with one light sensor. 
// The association is established with a call to aomw_sfh5721_init(addr).

// This driver is not very generic; it just configures the sensor for 
// continuous ALS measurements.


// Address of the SAID with I2C bridge connected to the light sensor
static uint16_t aomw_sfh5721_saidaddr;   


// Registers and values of SFH5721
#define AOMW_SFH5721_R00_OPSEL                    0x00
#define   AOMW_SFH5721_R00_OPSEL_RESET              0b10000000
#define   AOMW_SFH5721_R00_OPSEL_SMODE_SYNC         0b00100000
#define   AOMW_SFH5721_R00_OPSEL_SMODE_ASYNC        0b00000000 
#define   AOMW_SFH5721_R00_OPSEL_OMODE_SINGLE       0b00010000
#define   AOMW_SFH5721_R00_OPSEL_OMODE_CONT         0b00000000
#define   AOMW_SFH5721_R00_OPSEL_EN_ALS             0b00000100
#define   AOMW_SFH5721_R00_OPSEL_EN_IR              0b00000010
#define   AOMW_SFH5721_R00_OPSEL_EN_DARK            0b00000001
#define   AOMW_SFH5721_R00_OPSEL_EN_NONE            0b00000000
#define AOMW_SFH5721_R01_STAT                     0x01
#define   AOMW_SFH5721_R01_STAT_DRY_DARK            0b00000001
#define   AOMW_SFH5721_R01_STAT_DRY_IR              0b00000010
#define   AOMW_SFH5721_R01_STAT_DRY_ALS             0b00000100
#define AOMW_SFH5721_R02_IEN                      0x02
#define   AOMW_SFH5721_R02_IEN_IENL_DARK            0b00000001
#define   AOMW_SFH5721_R02_IEN_IENH_DARK            0b00000010
#define   AOMW_SFH5721_R02_IEN_IENL_IR              0b00000100
#define   AOMW_SFH5721_R02_IEN_IENH_IR              0b00001000
#define   AOMW_SFH5721_R02_IEN_IENL_ALS             0b00010000
#define   AOMW_SFH5721_R02_IEN_IENH_ALS             0b00100000
#define AOMW_SFH5721_R03_ICONF                    0x03
#define   AOMW_SFH5721_R03_ICONF_IPERS(n)           (n) // 0..15
#define   AOMW_SFH5721_R03_ICONF_IOA_AND            0b00010000
#define   AOMW_SFH5721_R03_ICONF_IOA_OR             0b00000000
#define   AOMW_SFH5721_R03_ICONF_ICC_DATA           0b00100000
#define   AOMW_SFH5721_R03_ICONF_ICC_IFLAG          0b00000000
#define AOMW_SFH5721_R04_ILTL                     0x04
#define AOMW_SFH5721_R05_ILTH                     0x05
#define AOMW_SFH5721_R06_IHTL                     0x06
#define AOMW_SFH5721_R07_IHTH                     0x07
#define AOMW_SFH5721_R08_IFLAG                    0x08
#define   AOMW_SFH5721_R08_IFLAG_IFLT_DARK          0b00000001
#define   AOMW_SFH5721_R08_IFLAG_IFHT_DARK          0b00000010
#define   AOMW_SFH5721_R08_IFLAG_IFLT_IR            0b00000100
#define   AOMW_SFH5721_R08_IFLAG_IFHT_IR            0b00001000
#define   AOMW_SFH5721_R08_IFLAG_IFLT_ALS           0b00010000
#define   AOMW_SFH5721_R08_IFLAG_IFHT_ALS           0b00100000
#define AOMW_SFH5721_R09_MCONFA                   0x09
#define   AOMW_SFH5721_R09_MCONFA_IT_0M2            0b00000000 // integration time in ms
#define   AOMW_SFH5721_R09_MCONFA_IT_0M4            0b00001000
#define   AOMW_SFH5721_R09_MCONFA_IT_0M8            0b00010000
#define   AOMW_SFH5721_R09_MCONFA_IT_1M6            0b00011000
#define   AOMW_SFH5721_R09_MCONFA_IT_3M1            0b00100000
#define   AOMW_SFH5721_R09_MCONFA_IT_6M3            0b00101000
#define   AOMW_SFH5721_R09_MCONFA_IT_12M5           0b00110000
#define   AOMW_SFH5721_R09_MCONFA_IT_25M            0b00111000
#define   AOMW_SFH5721_R09_MCONFA_IT_50M            0b01000000
#define   AOMW_SFH5721_R09_MCONFA_IT_100M           0b01001000
#define   AOMW_SFH5721_R09_MCONFA_IT_200M           0b01010000
#define   AOMW_SFH5721_R09_MCONFA_IT_400M           0b01011000
#define   AOMW_SFH5721_R09_MCONFA_IT_800M           0b01100000
#define   AOMW_SFH5721_R09_MCONFA_IT_1600M          0b01101000
#define   AOMW_SFH5721_R09_MCONFA_AGAIN_1           0b00000000
#define   AOMW_SFH5721_R09_MCONFA_AGAIN_4           0b00000001
#define   AOMW_SFH5721_R09_MCONFA_AGAIN_16          0b00000010
#define   AOMW_SFH5721_R09_MCONFA_AGAIN_128         0b00000011
#define   AOMW_SFH5721_R09_MCONFA_AGAIN_256         0b00000100
#define AOMW_SFH5721_R0A_MCONFB                   0x0A
#define   AOMW_SFH5721_R0A_MCONFB_DGAIN_1           0b00000000
#define   AOMW_SFH5721_R0A_MCONFB_DGAIN_2           0b00100000
#define   AOMW_SFH5721_R0A_MCONFB_DGAIN_4           0b01000000
#define   AOMW_SFH5721_R0A_MCONFB_DGAIN_8           0b01100000
#define   AOMW_SFH5721_R0A_MCONFB_DGAIN_16          0b10000000
#define   AOMW_SFH5721_R0A_MCONFB_SUB_DISABLED      0b00000000
#define   AOMW_SFH5721_R0A_MCONFB_SUB_ENABLED       0b00010000
#define   AOMW_SFH5721_R0A_MCONFB_WAIT(n)           (n) // n=0 -> 0ms, n=1..15 -> 2**(n+6)*6.1us
#define AOMW_SFH5721_R0B_MCONFC                   0x0B
#define   AOMW_SFH5721_R0B_MCONFC_EN_DARK            0b00000001
#define   AOMW_SFH5721_R0B_MCONFC_EN_IR              0b00000010
#define   AOMW_SFH5721_R0B_MCONFC_EN_ALS             0b00000100
#define   AOMW_SFH5721_R0B_MCONFC_EN_NONE            0b00000000
#define   AOMW_SFH5721_R0B_MCONFC_LPFDEPTH_2         0b00000000
#define   AOMW_SFH5721_R0B_MCONFC_LPFDEPTH_3         0b00010000
#define   AOMW_SFH5721_R0B_MCONFC_LPFDEPTH_4         0b00100000
#define   AOMW_SFH5721_R0B_MCONFC_LPFDEPTH_5         0b00110000
#define AOMW_SFH5721_R0C_DATA1DARK                0x0C
#define AOMW_SFH5721_R0E_DATA2IR                  0x0E
#define AOMW_SFH5721_R10_DATA3ALS                 0x10
#define AOMW_SFH5721_R14_DEVID                    0x14


// Issue startup sequence from datasheet, and checks DEVID.
static aoresult_t aomw_sfh5721_startup() {
  aoresult_t result;
  uint8_t buf;

  // Datasheet has recommended startup sequence 
  // step 1: power up or reset
  // If the host was reset, the SFH5721 might still be measuring and then the startup sequence doesn't ACK. So reset SFH5721
  buf=AOMW_SFH5721_R00_OPSEL_RESET;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R00_OPSEL,&buf,1);
  if( result!=aoresult_ok ) return result;
  
  // step 2: Give time to read OTP
  delay(5); 

#if 0 // Step 4 does not work on SAID
  // step 3: single byte to register
  buf=0x55;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, 0x62,&buf,1);
  if( result!=aoresult_ok ) return result;

  // step 4: the 00 byte should not be done, but SAID forces to send a "register value"
  buf=0x00;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, 0xEC,&buf,1);
  if( result!=aoresult_ok ) return result;
  
  // step 5: Give time to read OTP
  delay(5); 
#endif

  // step 6 IC ready for use: read and check DEVID
  result= aoosp_exec_i2cread8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R14_DEVID,&buf,1);
  if( result!=aoresult_ok ) return result;
  if( buf!=0x01 ) return aoresult_dev_noi2cdev;

  return aoresult_ok;
}


// Performs a hardwired configuration
static aoresult_t aomw_sfh5721_conf() {
  aoresult_t result;
  uint8_t buf[1];

  // IT at 25ms is shortest time with full 16 bit resolution, AGAIN at 4x 
  buf[0]= AOMW_SFH5721_R09_MCONFA_IT_25M | AOMW_SFH5721_R09_MCONFA_AGAIN_4;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R09_MCONFA, buf,1 );
  if( result!=aoresult_ok ) return result;

  // No digital gain, no dark count subtraction, no wait (only for omode continuous)
  buf[0]= AOMW_SFH5721_R0A_MCONFB_DGAIN_1 | AOMW_SFH5721_R0A_MCONFB_SUB_DISABLED | AOMW_SFH5721_R0A_MCONFB_WAIT(0);
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R0A_MCONFB, buf,1 );
  if( result!=aoresult_ok ) return result;

  // Enable low pass filter for ALS depth 3
  buf[0]= AOMW_SFH5721_R0B_MCONFC_EN_ALS | AOMW_SFH5721_R0B_MCONFC_LPFDEPTH_3;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R0B_MCONFC, buf, 1 );
  if( result!=aoresult_ok ) return result;

  // Starts continuous measurements for ALS
  buf[0]= AOMW_SFH5721_R00_OPSEL_SMODE_ASYNC | AOMW_SFH5721_R00_OPSEL_OMODE_CONT | AOMW_SFH5721_R00_OPSEL_EN_ALS;
  result= aoosp_exec_i2cwrite8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R00_OPSEL, buf, 1 );
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}


// === main =================================================================


/*!
    @brief  Reads and returns the ALS level measured by the SFH5721 light sensor.
    @param  *als
            Out parameter for the read level.
    @return aoresult_ok              if successful
            other                    OSP (communication) error or I2C error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes a light sensor is associated with this
            library via `aomw_sfh5721_init()`.
    @note   Check SFH5721 datasheet for units lux=512*als/(DGAIN*AGAIN*2^IT).
            With the default config (AGAIN at 4x, DGAIN at 1x, IT at 25ms)
            the reported value is lux. 
*/
aoresult_t aomw_sfh5721_als_get(int*als) {
  aoresult_t result;
  uint8_t buf[2];
  
  result= aoosp_exec_i2cread8(aomw_sfh5721_saidaddr,AOMW_SFH5721_DADDR7_SAIDSENSE, AOMW_SFH5721_R10_DATA3ALS,buf,2);
  if( result!=aoresult_ok ) return result;

  *als= buf[0] + 256*buf[1];
  return aoresult_ok;
}


/*!
    @brief  Tests if an SFH5721 light sensor is connected to the I2C 
            bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @return aoresult_ok              if sensor is present on the I2C bus of addr
            aoresult_dev_noi2cdev    if sensor is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            other                    OSP (communication) errors
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the light sensor has I2C device address
            AOMW_SFH5721_DADDR7_SAIDSENSE.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_sfh5721_present(uint16_t addr ) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  // Check if we can read from 00
  uint8_t buf[1];
  result = aoosp_exec_i2cread8(addr, AOMW_SFH5721_DADDR7_SAIDSENSE, 0x00, buf, 1);
  // Real error
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the SFH5721 light sensor 
            connected to the I2C bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with a light sensor.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   The sensor is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the light sensor has I2C device address
            AOMW_SFH5721_DADDR7_SAIDSENSE.
    @note   It is allowed to call this function again, to associate this
            driver with a different sensor.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_sfh5721_init(uint16_t addr) {
  aoresult_t result;

  // Record address of the SAID with I2C bridge
  aomw_sfh5721_saidaddr= addr;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  result= aomw_sfh5721_startup();
  if( result!=aoresult_ok ) return result;

  result= aomw_sfh5721_conf();
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}


