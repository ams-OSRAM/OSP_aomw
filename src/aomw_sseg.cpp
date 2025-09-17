// aomw_sseg.cpp - driver for a quad 7-segment display (driven by four IOXs)
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
#include <stdarg.h>        // va_start
#include <aoresult.h>      // aoresult_t
#include <aoosp.h>         // aoosp_exec_i2cwrite8()
#include <aomw_sseg.i>     // aomw_sseg_font_get()
#include <aomw_sseg.h>     // own


// Address of the SAID with I2C bridge connected to 4 IOX chips driving the 7-segment modules
static uint16_t aomw_sseg_saidaddr;


// === IOX driver ===========================================================


// The driver is used to control the 8 output pins of the PCA9554 I/O expander. 
// The output pins drive LEDs in a 7-segment module. The output pins are
// low active (0 is on). Input is not supported in this driver.
// The "states" in the API is _user_ state:
//   if bit i in states (mask) is 0 LED i is off
//   if bit i in states (mask) is 1 LED i is on


class aomw_sseg_iox_t {
  public: 
    /*con*/    aomw_sseg_iox_t(uint8_t daddr7);      // Configures I2C device address on the bus (7 bits, ie without r/w)
    aoresult_t begin();                              // Initializes driver (this object) and the underlying IOX device
    aoresult_t set_states(uint8_t mask);             // states:=mask; then sync IOX hardware
    uint8_t    get_daddr7();                         // Returns I2C device address
  private:     
    aoresult_t write(uint8_t raddr, uint8_t val);    // Write `val` to register `raddr`, on I2C device _daddr7 via bridge in SAID aomw_sseg_saidaddr.
    aoresult_t read (uint8_t raddr, uint8_t* val);   // Read `val` from register `raddr`, on I2C device _daddr7 via bridge in SAID aomw_sseg_saidaddr.
    uint8_t    _states;                              // The state of the 8 LEDs (bit set means segment on)
    uint8_t    _daddr7;                              // Device address on the I2C bus (7 bits, ie without r/w)
};


// Registers and values of IOX
#define AOMW_SSEG_IOX_R00_INPUT                  0x00
#define AOMW_SSEG_IOX_R01_OUTPUT                 0x01 // our outputs are low-active
#define   AOMW_SSEG_IOX_R01_OUTPUT_ALLLOW          0x00
#define   AOMW_SSEG_IOX_R01_OUTPUT_ALLHIGH         0xFF
#define AOMW_SSEG_IOX_R02_POLARITY               0x02 // only for inputs
#define   AOMW_SSEG_IOX_R02_POLARITY_ALLLOACTIVE   0xFF
#define   AOMW_SSEG_IOX_R02_POLARITY_ALLHIACTIVE   0x00
#define AOMW_SSEG_IOX_R03_CONFIG                 0x03
#define   AOMW_SSEG_IOX_R03_CONFIG_ALLINPUT        0xFF
#define   AOMW_SSEG_IOX_R03_CONFIG_ALLOUTPUT       0x00


// Configures device address on the I2C bus (7 bits, ie without r/w)
aomw_sseg_iox_t::aomw_sseg_iox_t(uint8_t daddr7) {
  _daddr7 = daddr7;
  _states = 0x00;   // all off
}


// Returns I2C device address
uint8_t aomw_sseg_iox_t::get_daddr7() {
  return _daddr7;
}


// I2C wrapper, writing `val` to register `raddr`, on I2C device _daddr7 via bridge in SAID aomw_sseg_saidaddr.
aoresult_t aomw_sseg_iox_t::write(uint8_t raddr, uint8_t val) {
  return aoosp_exec_i2cwrite8(aomw_sseg_saidaddr,_daddr7,raddr,&val,1);
}


// I2C wrapper, read `val` from register `raddr`, on I2C device _daddr7 via bridge in SAID aomw_sseg_saidaddr.
aoresult_t aomw_sseg_iox_t::read(uint8_t raddr, uint8_t * val) {
  return aoosp_exec_i2cread8(aomw_sseg_saidaddr,_daddr7,raddr,val,1);
}


// Initializes driver (this object) and the underlying IOX device. LEDs will be off.
aoresult_t aomw_sseg_iox_t::begin() {
  aoresult_t result;
  
  // LEDs are low active, so we set the output register to 0xFF (off)
  result = write(AOMW_SSEG_IOX_R01_OUTPUT, AOMW_SSEG_IOX_R01_OUTPUT_ALLHIGH );
  if( result!=aoresult_ok ) return result;

  // Check hardware presence
  uint8_t output;
  result = read(AOMW_SSEG_IOX_R01_OUTPUT, &output );
  if( result!=aoresult_ok ) return result;
  if( output!=AOMW_SSEG_IOX_R01_OUTPUT_ALLHIGH ) return aoresult_comparefail;

  // Switch all to output, this switches all LEDs off
  result = write(AOMW_SSEG_IOX_R03_CONFIG, AOMW_SSEG_IOX_R03_CONFIG_ALLOUTPUT );
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}


// states:=mask; then sync IOX hardware
aoresult_t aomw_sseg_iox_t::set_states(uint8_t mask) {
  _states = mask;
  return write(AOMW_SSEG_IOX_R01_OUTPUT, ~ _states ); // low active
}


// Array with four IOX drivers
static aomw_sseg_iox_t aomw_sseg_iox[AOMW_SSEG_COUNT] = {
  aomw_sseg_iox_t(AOMW_SSEG_0_DADDR7_SAIDSENSE), // Most significant module (left-most) 
  aomw_sseg_iox_t(AOMW_SSEG_1_DADDR7_SAIDSENSE),
  aomw_sseg_iox_t(AOMW_SSEG_2_DADDR7_SAIDSENSE),
  aomw_sseg_iox_t(AOMW_SSEG_3_DADDR7_SAIDSENSE), // Least significant module (right-most)
};


// === Quad 7-segment driver ================================================


/*!
    @brief  Clears the quad 7-segment display (all segments off).
    @return aoresult_ok if display is updated
            other       OSP (communication) errors
    @note   Asserts if aomw_sseg_init() has not yet been called.
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
*/
aoresult_t aomw_sseg_clr() {
  AORESULT_ASSERT(aomw_sseg_saidaddr!=0); // Forgot aomw_sseg_init()?
  
  for( int i=0; i<AOMW_SSEG_COUNT; i++ ) {
    aoresult_t result= aomw_sseg_iox[i].set_states( 0x00 );
    if( result!=aoresult_ok ) return result;
  }
  return aoresult_ok;
}


/*!
    @brief  Sets the quad 7-segment display to the pattern encoded in segs.
    @param  segs
            segs[i] is a bit mask indicating which segments of 7-segment 
            module i to switch on. 
    @return aoresult_ok if display is updated
            other       OSP (communication) errors
    @note   Asserts if aomw_sseg_init() has not yet been called.
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   Mask segs[i] indicates which segments of the 7-segment module 
            to switch on. The segments are mapped as follows to bits:
            (msb) pgfedcba (lsb)
             pos  76543210
               --a--
              |     |
              f     b
              |     |
               --g--
              |     |
              e     c
              |     |
               --d--  (p)
    @note   Array segs must be AOMW_SSEG_COUNT long, most significant
            (left-most) module first.
*/
aoresult_t aomw_sseg_set(const uint8_t * segs) {
  AORESULT_ASSERT(aomw_sseg_saidaddr!=0); // Forgot aomw_sseg_init()?
  
  for( int i=0; i<AOMW_SSEG_COUNT; i++ ) {
    aoresult_t result= aomw_sseg_iox[i].set_states( segs[i] );
    if( result!=aoresult_ok ) return result;
  }
  return aoresult_ok;
}


/*!
    @brief  Sets the quad 7-segment display to `str`.
            Only shows first AOMW_SSEG_COUNT characters, pads with 
            spaces if `str` is shorter.
    @param  str
            The string to be displayed.
    @return aoresult_ok if display is updated
            other       OSP (communication) errors
    @note   Asserts if aomw_sseg_init() has not yet been called.
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   There are two ways to switch on a dot:
            - using a character with bit 8 set: 0x41 is A, 0xC1 is A with dot
            - adding a dot in `str`: "12.3C" prints 123C with a dot after 2
*/
aoresult_t aomw_sseg_print(const char*str) {
  // segs[] will store the bit masks for the 7-segment units
  uint8_t segs[AOMW_SSEG_COUNT];

  // traverse `str`, lookup in font, and do dot replacement
  int i = 0;
  // If first char is a '.' it is printed as in the font (and not decimal point)
  while( i<AOMW_SSEG_COUNT && *str!=0 ) {
    segs[i] = aomw_sseg_font_get(*str);
    str++;
    if( *str=='.' ) {
      segs[i] |= 0x80; // switch on 'dot' segment on
      str++;
    }
    i++;
  }
  // Pad remaining display modules with blanks
  while( i<AOMW_SSEG_COUNT ) segs[i++]=0;
  
  // Send to display
  aoresult_t result= aomw_sseg_set(segs);
  if( result!=aoresult_ok ) return result;

  return aoresult_ok;
}


/*!
    @brief  Formatted print to the quad 7-segment display.
    @param  fmt, ...
            Format and content as in printf()
    @return aoresult_ok if display is updated
            other       OSP (communication) errors
    @note   Asserts if aomw_sseg_init() has not yet been called.
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
*/
aoresult_t aomw_sseg_printf(const char * fmt, ... ) {
  char str[AOMW_SSEG_COUNT*2+1]; // max len A.B.C.D.\0
  va_list args;
  va_start(args,fmt);
  vsnprintf(str,sizeof str,fmt,args);
  va_end(args);
  // Serial.printf("'%s'\n",str);
  return aomw_sseg_print(str);
}


/*!
    @brief  Tests if an quad 7-segment display is connected to the I2C 
            bus of OSP node (SAID) with address `addr`. This means
            checking for 4 I/O expander devices on the I2C bus.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @return aoresult_ok              if display is present on the I2C bus of addr
            aoresult_dev_noi2cdev    if display is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            other                    OSP (communication) errors
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the display (the underlying I/O expanders)
            have I2C device addresses AOMW_SSEG_IOX#_DADDR7.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_sseg_present(uint16_t addr ) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  
  // Check if addr has four IOXes, by reading CONFIG
  uint8_t buf[1];
  for( int i=0; i<AOMW_SSEG_COUNT; i++ ) {
    result = aoosp_exec_i2cread8(addr, aomw_sseg_iox[i].get_daddr7(), AOMW_SSEG_IOX_R03_CONFIG, buf, 1);
    if( result!=aoresult_ok ) return result;
  }
  
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the quad 7-segment display
            connected to the I2C bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with a quad 7-segment display.
    @return aoresult_ok           if init was successful
            other error code      if there is a (communications) error
    @note   The display is controlled via OSP, hence the possibility 
            for telegram transmission errors.
    @note   This routine assumes the display (the underlying I/O expanders)
            have I2C device addresses AOMW_SSEG_#_DADDR7.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
    @note   It is allowed to call this function again, to associate this
            driver with a different display.
*/
aoresult_t aomw_sseg_init(uint16_t addr) {
  // Record address of the SAID with I2C bridge
  aomw_sseg_saidaddr= addr;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  // Init each 7-segment module
  for( int i=0; i<AOMW_SSEG_COUNT; i++ ) {
    aoresult_t result = aomw_sseg_iox[i].begin();
    if( result!=aoresult_ok ) return result;
  }

  return aoresult_ok;
}

