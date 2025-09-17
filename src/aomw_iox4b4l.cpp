// aomw_iox4b4l.cpp - driver for an I/O-expander connected to 4 buttons and 4 LEDs
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
#include <aoosp.h>         // aoosp_exec_i2cwrite8()
#include <aomw_iox4b4l.h>  // own



// ==========================================================================
// This driver assumes an I/O-expander is connected to a SAID I2C bus. 
// Furthermore, it assumes 4 ports have a button connected to it (input),
// and that 4 ports have an indicator LED connected to it (output).
// The I/O-expander with 4 buttons and 4 LEDs is also known as a "selector".
// It is possible the configure which pins have a button or a LED, and 
// whether the button or LED is low active.

// I/O-expanders that are compatible with e.g. PCA6408ABSHP (as on SAIDbasic), 
// or PCA9554ABS (as on SAIDsense) are supported by this driver.

// This driver is not multi-instance. It can only control one IOX and only on 
// one I2C bus - that is one SAID. After a call to aomw_iox4b4l_init() the 
// driver is tied to that IOX.


// Address of the SAID with I2C bridge with IOX (I/O-expander)
static uint16_t aomw_iox4b4l_saidaddr; 
// I2C address of the I/O-expander
static uint8_t  aomw_iox4b4l_ioxdaddr7; 


// Registers of the I/O-expander
#define AOMW_IOX4B4L_REGINVAL       0x00 // Input port register reflects the incoming logic levels of all pins (read)
#define AOMW_IOX4B4L_REGOUTVAL      0x01 // The Output port register contains the outgoing logic levels of the pins defined as outputs (read/write)
#define AOMW_IOX4B4L_REGINPINV      0x02 // The Polarity inversion register allows polarity inversion of pins defined as inputs (read/write)
#define AOMW_IOX4B4L_REGCFGINP      0x03 // The Configuration register configures the direction of the I/O pins. If a bit is 1, the pin is input (read/write)


// === Indicator LED ========================================================


// Non uniform pin-out is supported.
// `leds` is a 4-bit sw mask indicating which LEDs should be on (bit=1).
// The 4 states are first mapped to physical pins (0..7).
// Next, they are inverted for the low-active pins.
static uint8_t aomw_iox4b4l_ledpins[4];
static uint8_t aomw_iox4b4l_ledinv;
static uint8_t aomw_iox4b4l_ledsw2hw( uint8_t leds ) {
  uint8_t reordered = 0;
  for( uint8_t i=0; i<4; i++ ) {
    if( leds & (1<<i) ) reordered |= 1 << aomw_iox4b4l_ledpins[i];
  }
  return reordered ^ aomw_iox4b4l_ledinv;
}


// Current state of the indicator LEDs (format: software mask, i.e. 1=on)
static uint8_t  aomw_iox4b4l_led_states; 


/*!
    @brief  Turns on the indicator LEDs that have a bit set in `leds`.
    @param  leds
            A bit mask, typically composed of AOMW_IOX4B4L_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be configured with aomw_iox4b4l_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   The LEDs that have a 0 bit in `leds` are not changed.
*/
aoresult_t aomw_iox4b4l_led_on( uint8_t leds ) {
  aomw_iox4b4l_led_states |= leds;
  uint8_t hwmask = aomw_iox4b4l_ledsw2hw(aomw_iox4b4l_led_states);
  return aoosp_exec_i2cwrite8(aomw_iox4b4l_saidaddr, aomw_iox4b4l_ioxdaddr7, AOMW_IOX4B4L_REGOUTVAL, &hwmask, 1);
}


/*!
    @brief  Turns off the indicator LEDs that have a bit set in `leds`.
    @param  leds
            A bit mask, typically composed of AOMW_IOX4B4L_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be configured with aomw_iox4b4l_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   The LEDs that have a 0 bit in `leds` are not changed.
*/
aoresult_t aomw_iox4b4l_led_off( uint8_t leds ) {
  aomw_iox4b4l_led_states &= ~leds;
  uint8_t hwmask = aomw_iox4b4l_ledsw2hw(aomw_iox4b4l_led_states);
  return aoosp_exec_i2cwrite8(aomw_iox4b4l_saidaddr, aomw_iox4b4l_ioxdaddr7, AOMW_IOX4B4L_REGOUTVAL, &hwmask, 1);
}


/*!
    @brief  Toggles the indicator LEDs that have a bit set in `leds`.
    @param  leds
            A bit mask, typically composed of AOMW_IOX4B4L_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be configured with aomw_iox4b4l_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   The LEDs that have a 0 bit in `leds` are not changed.
*/
aoresult_t aomw_iox4b4l_led_tgl( uint8_t leds ) {
  aomw_iox4b4l_led_states ^= leds;
  uint8_t hwmask = aomw_iox4b4l_ledsw2hw(aomw_iox4b4l_led_states);
  return aoosp_exec_i2cwrite8(aomw_iox4b4l_saidaddr, aomw_iox4b4l_ioxdaddr7, AOMW_IOX4B4L_REGOUTVAL, &hwmask, 1);
}


/*!
    @brief  The bits set in `leds` indicate which indicator LEDs to turn on, 
            the clear bits, which to turn off.
    @param  leds
            A bit mask, typically composed of AOMW_IOX4B4L_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be configured with aomw_iox4b4l_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   All indicator LEDs are controlled.
*/
aoresult_t aomw_iox4b4l_led_set( uint8_t leds ) {
  aomw_iox4b4l_led_states = leds;
  uint8_t hwmask = aomw_iox4b4l_ledsw2hw(aomw_iox4b4l_led_states);
  return aoosp_exec_i2cwrite8(aomw_iox4b4l_saidaddr, aomw_iox4b4l_ioxdaddr7, AOMW_IOX4B4L_REGOUTVAL, &hwmask, 1);
}


// === Button ===============================================================


// Non uniform pin-out is supported.
// `hwmask` is a 8-bit register value indicating the status of the pins.
// First, status is inverted for the low-active pins, to produce a sw view (1=pressed).
// Next, the 8 bits are reduced to 4 belonging to the buttons, and reordered to bits 3-0.
static uint8_t aomw_iox4b4l_butpins[4]; 
static uint8_t aomw_iox4b4l_butinv;
static uint8_t aomw_iox4b4l_buthw2sw( uint8_t hwmask ) {
  hwmask ^= aomw_iox4b4l_butinv;
  uint8_t buts = 0;
  for( uint8_t i=0; i<4; i++ ) {
    if( hwmask & (1<<aomw_iox4b4l_butpins[i]) ) buts |= (1<<i);
  }
  return buts;
}


// State of the buttons (format: software mask), previous and current
static uint8_t  aomw_iox4b4l_but_prvstates; 
static uint8_t  aomw_iox4b4l_but_curstates;


/*!
    @brief  This function scans all buttons for their state (up, down) 
            and records that in a global variable. It also maintains
            the previous state. Use functions aomw_iox4b4l_but_isup()/down(), 
            and/or aomw_iox4b4l_but_wentup()/wentdown() to determine which 
            buttons are respectively went up/down.
    @return aoresult_ok           if button status read was successful
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one, must first be configured with aomw_iox4b4l_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   Call aoui32_but_scan() frequently, but also with delays 
            in between (1ms) to mitigate contact bounce of the buttons.
*/
aoresult_t aomw_iox4b4l_but_scan( ) {
  uint8_t hwmask;
  aoresult_t result;
  result=  aoosp_exec_i2cread8(aomw_iox4b4l_saidaddr, aomw_iox4b4l_ioxdaddr7, AOMW_IOX4B4L_REGINVAL, &hwmask, 1);

  aomw_iox4b4l_but_prvstates= aomw_iox4b4l_but_curstates;
  aomw_iox4b4l_but_curstates= aomw_iox4b4l_buthw2sw(hwmask);
  return result;
}


/*!
    @brief  Returns which of the buttons in `buts` were down (depressed)
            during the last aomw_iox4b4l_but_scan() call, but where up 
            (released) during the call to aomw_iox4b4l_but_scan() before that; 
            the button "went down".
    @param  buts
            A mask, formed by OR-ing AOMW_IOX4B4L_BUTxxx macros.
    @return Returns a mask, subset of buts, with a 1 for buttons that went down.
    @note   Must aomw_iox4b4l_but_scan() before using this function.
*/
uint8_t aomw_iox4b4l_but_wentdown( uint8_t buts ) {
  // Buttons are low active
  // Return those that were 1 (up), are now 0 (down), and are part of `buts`
  return ~aomw_iox4b4l_but_prvstates & aomw_iox4b4l_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` was down (depressed)
            during the last aomw_iox4b4l_but_scan() call.
    @param  buts
            A mask, formed by OR-ing AOMW_IOX4B4L_BUTxxx macros.
    @return Returns a mask, subset of buts, with a 1 for buttons that is down.
    @note   Must aomw_iox4b4l_but_scan() before using this function.
*/
uint8_t aomw_iox4b4l_but_isdown( uint8_t buts ) {
  // Buttons are low active
  // Return those that are now 0 (down), and are part of `buts`
  return aomw_iox4b4l_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` were up (released)
            during the last aomw_iox4b4l_but_scan() call, but where down
            (depressed) during the call to aomw_iox4b4l_but_scan() before that; 
            the button "went up".
    @param  buts
            A mask, formed by OR-ing AOMW_IOX4B4L_BUTxxx macros.
    @return Returns a mask, subset of buts, with a 1 for buttons that went up.
    @note   Must aomw_iox4b4l_but_scan() before using this function.
*/
uint8_t aomw_iox4b4l_but_wentup( uint8_t buts ) {
  // Buttons are low active
  // Return those that were 0 (down), are now 1 (up), and are part of `buts`
  return aomw_iox4b4l_but_prvstates & ~aomw_iox4b4l_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` was up (released)
            during the last aomw_iox4b4l_but_scan() call.
    @param  buts
            A mask, formed by OR-ing AOMW_IOX4B4L_BUTxxx macros.
    @return Returns a mask, subset of buts, with a 1 for buttons that is up.
    @note   Must aomw_iox4b4l_but_scan() before using this function.
*/
uint8_t aomw_iox4b4l_but_isup( uint8_t buts ) {
  // Buttons are low active
  // Return those that are now 1 (up), and are part of `buts`
  return ~aomw_iox4b4l_but_curstates & buts;
}


// === main =================================================================


/*!
    @brief  Tests if an I/O-expander (IOX) is connected to the I2C bus of  
            OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @param  daddr7
            The 7 bits I2C device address used for accessing the 
            I/O-expander, typically AOMW_IOX4B4L_DADDR7_SAIDSENSE or 
            AOMW_IOX4B4L_DADDR7_SAIDBASIC.
    @return aoresult_ok              if IOX is present to the I2C bus of addr
            aoresult_dev_noi2cdev    if IOX is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            other                    OSP (communication) errors
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   Sends I2C telegrams, so OSP must be initialized, e.g. via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, e.g. 
            via a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_iox4b4l_present(uint16_t addr, uint8_t daddr7) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  // Check if we can read from 00
  uint8_t buf;
  result = aoosp_exec_i2cread8(addr, daddr7, 0x00, &buf, 1);
  // Real error
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the I/O-expander (IOX) with 
            I2C device address `daddr7` connected to the I2C bus of the OSP 
            node (SAID) with address `addr`. This I/O-expander has 4 buttons 
            and 4 indicator LEDs connected to it, the pin-out is configured 
            with `pincfg`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with an I/O-expander.
    @param  daddr7
            The 7 bits I2C device address used for accessing the I/O-expander.
    @param  pincfg
            A 32 bits integer, partitioned in 8 times 4 bits. Each 4-bit part 
            denotes configures how a button or indicator LED is connected 
            to the I/O-expander.
            Bits 0-3 configure button 0, bits 4-7, 8-11 and 12-15 configure 
            buttons 1, 2 and 3 respectively. Bits 16-19, 20-23, 24-27 and 28-31 
            configure LED 0, 1, 2 and 3 respectively.
            The 4 bits for a button LED are split in an MSB which indicates the 
            polarity (0 is low-active, 1 is high-active), the three LSB bits 
            define the pin number (0-7) used for that button/LED.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   As an example, the indicator LEDs on SAIDsense are connected as 
            follows (numbering base 0, and from MSB to LSB):
            LED3@7, BUT3@6, BUT2@5, LED2@4, BUT1@3, LED1@2, BUT0@1 LED0@0.
            The led pins are thus at 7420, and button pins at 6531.
            The LEDs are high active, so the pincfg is 0x74206532|0x88880000.
    @note   As an example, the indicator LEDs on SAIDbasic are connected as 
            follows (numbering base 0, and from MSB to LSB):
            LED3@7, BUT3@6, LED2@5, BUT2@4, LED1@3, BUT1@2, LED0@1, BUT0@0.
            The led pins are thus at 7531 and the button pins at 6420.
            The LEDs are high active, so the pincfg is 0x75316420|0x88880000.
    @note   This routine support different I/O-expanders, like the one on the
            SAIDsense board with daddr7 AOMW_IOX4B4L_DADDR7_SAIDSENSE or the
            one on the SAIDbasic board with AOMW_IOX4B4L_DADDR7_SAIDBASIC.
    @note   It is allowed to call this function again, to associate this
            driver with a different I/O-expander.
    @note   Sends I2C telegrams, so OSP must be initialized, e.g. via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, e.g. 
            via a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_iox4b4l_init(uint16_t addr, uint8_t daddr7, uint32_t pincfg) {
  aoresult_t result;

  // Record address of the SAID with I2C bridge
  aomw_iox4b4l_saidaddr= addr;
  // Record address of the I/O-expander on the bridge
  aomw_iox4b4l_ioxdaddr7= daddr7;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  // Process pincfg
  int pins=0; // For completeness check
  aomw_iox4b4l_butinv=0;
  for( int but=0; but<4; but++ ) {
    // Extract pin number and hi-active from `pincfg`
    int pin= pincfg & 0x07; 
    int act= pincfg & 0x08;
    pincfg >>=4;
    // Record pin number for button `but`
    aomw_iox4b4l_butpins[but]= pin;
    // If button is low active, register it for inverting
    if( act==0 ) aomw_iox4b4l_butinv |= 1<<pin;
    // For completeness check
    pins |= 1<<pin;
  }
  uint8_t inputpins = pins; // record which bits are inputs (ie for buttons)
  aomw_iox4b4l_ledinv=0;
  for( int led=0; led<4; led++ ) {
    // Extract pin number and hi-active from `pincfg`
    int pin= pincfg & 0x07;
    int act= pincfg & 0x08;
    pincfg >>=4;
    // Record pin number for LED `led`
    aomw_iox4b4l_ledpins[led]= pin;
    // If LED is low active, register it for inverting
    if( act==0 ) aomw_iox4b4l_ledinv |= 1<<pin; 
    // For completeness check
    pins |= 1<<pin;
  }
  AORESULT_ASSERT( pins==255 ); // all 8 pins must be in use

  // Switch indicator LEDs off
  result= aomw_iox4b4l_led_set(AOMW_IOX4B4L_LEDNONE);
  if( result!=aoresult_ok ) return result;
  // Configure button pins as input - and thus the led pins as output
  result = aoosp_exec_i2cwrite8(addr, daddr7, AOMW_IOX4B4L_REGCFGINP, &inputpins, 1);
  if( result!=aoresult_ok ) return result;
  // Initialize "prev" state of buttons
  result= aomw_iox4b4l_but_scan();
  if( result!=aoresult_ok ) return result;
  
  return aoresult_ok;
}



