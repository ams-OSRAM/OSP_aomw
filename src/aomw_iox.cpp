// aomw_iox.cpp - driver for NXP PCA6408ABSHP I/O-expander connected to a SAID, assuming 4 LEDs and 4 buttons are connected
/*****************************************************************************
 * Copyright 2024 by ams OSRAM AG                                            *
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
#include <aoosp.h>     // aoosp_exec_i2cwrite8()
#include <aomw_iox.h>  // own



// ==========================================================================
// This driver assumes an NXP PCA6408ABSHP I/O-expander is connected to a SAID 
// I2C bus. Furthermore, it assumes ports 1, 3, 5, and 7 have a button 
// connected to it. The buttons are low active (0 when pushed). It also 
// assumes that ports 2, 4, 6, and 8 have a indicator LED connected to it.
// The indicator LEDs are high active (1 switches them on).
//
// This driver is not multi-instance. It can only control an IOX with I2C
// address AOMW_IOX_DADDR7, and only one I2C bus - that is one SAID.
// After a call to aomw_iox_init(addr) the driver is tied to that IOX.


// Address of the SAID with I2C bridge with IOX
static uint16_t aomw_iox_saidaddr;   


// Registers of the I/O-expander
#define AOMW_IOX_REGINVAL       0x00 // Input port register reflects the incoming logic levels of all pins (read)
#define AOMW_IOX_REGOUTVAL      0x01 // The Output port register contains the outgoing logic levels of the pins defined as outputs (read/write)
#define AOMW_IOX_REGINPINV      0x02 // The Polarity inversion register allows polarity inversion of pins defined as inputs (read/write)
#define AOMW_IOX_REGCFGINP      0x03 // The Configuration register configures the direction of the I/O pins. If a bit is 1, the pin is input (read/write)


// === Indicator LED ========================================================


// Current state of the indicator LEDs (shadow of the IOX register)
static uint8_t  aomw_iox_led_states; 


/*!
    @brief  Turns on the indicator LEDs that have a bit set in `led`.
    @param  leds
            A bit mask, typically composed of AOMW_IOX_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be established with aomw_iox_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   The LEDs that have a 0 bit in `led` are not changed.
*/
aoresult_t aomw_iox_led_on( uint8_t leds ) {
  aomw_iox_led_states |= leds;
  return aoosp_exec_i2cwrite8(aomw_iox_saidaddr, AOMW_IOX_DADDR7, AOMW_IOX_REGOUTVAL, &aomw_iox_led_states, 1);
}


/*!
    @brief  Turns off the indicator LEDs that have a bit set in `led`.
    @param  leds
            A bit mask, typically composed of AOMW_IOX_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be established with aomw_iox_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   The LEDs that have a 0 bit in `led` are not changed.
*/
aoresult_t aomw_iox_led_off( uint8_t leds ) {
  aomw_iox_led_states &= ~leds;
  return aoosp_exec_i2cwrite8(aomw_iox_saidaddr, AOMW_IOX_DADDR7, AOMW_IOX_REGOUTVAL, &aomw_iox_led_states, 1);
}


/*!
    @brief  The bits set in `leds` indicate which indicator LEDs to turn on, 
            the clear bits, which to turn off.
    @param  leds
            A bit mask, typically composed of AOMW_IOX_LEDxxx flags.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one must first be established with aomw_iox_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   All indicator LEDs are controlled.
*/
aoresult_t aomw_iox_led_set( uint8_t leds ) {
  aomw_iox_led_states = leds;
  return aoosp_exec_i2cwrite8(aomw_iox_saidaddr, AOMW_IOX_DADDR7, AOMW_IOX_REGOUTVAL, &aomw_iox_led_states, 1);
}


// === Button ===============================================================


// State of the buttons (shadow of the IOX register), previous and current
static uint8_t  aomw_iox_but_prvstates; 
static uint8_t  aomw_iox_but_curstates;


/*!
    @brief  This function scans all buttons for their state (up, down) 
            and records that in a global variable. It also maintains
            the previous state. Use functions aomw_iox_but_isup()/down(), 
            and/or aomw_iox_but_wentup()/wentdown() to determine which 
            buttons are respectively went up/down.
    @return aoresult_ok           if button status read was successful
            other error code      if there is a (communications) error
    @note   This module controls (LEDs and buttons on) one I/O-expander;
            which one, must first be established once, with aomw_iox_init().
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   Call aoui32_but_scan() frequently, but also with delays 
            in between (1ms) to mitigate contact bounce of the buttons.
*/
aoresult_t aomw_iox_but_scan( ) {
  aomw_iox_but_prvstates= aomw_iox_but_curstates;
  return aoosp_exec_i2cread8(aomw_iox_saidaddr, AOMW_IOX_DADDR7, AOMW_IOX_REGINVAL, &aomw_iox_but_curstates, 1);
}


/*!
    @brief  Returns which of the buttons in `buts` were down (depressed)
            during the last aomw_iox_but_scan() call, but where up 
            (released) during the call to aomw_iox_but_scan() before that; 
            the button "went down".
    @param  buts
            A mask, formed by OR-ing AOMW_IOX_BUTxxx macros.
    @return Returns a mask, subset of buts, 
            with a 1 for buttons that went down.
    @note   Must aomw_iox_but_scan() before using this function.
*/
uint8_t aomw_iox_but_wentdown( uint8_t buts ) {
  // Buttons are low active
  // Return those that were 1 (up), are now 0 (down), and are part of `buts`
  return aomw_iox_but_prvstates & ~aomw_iox_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` was down (depressed)
            during the last aomw_iox_but_scan() call.
    @param  buts
            A mask, formed by OR-ing AOMW_IOX_BUTxxx macros.
    @return Returns a mask, subset of buts, 
            with a 1 for buttons that is down.
    @note   Must aomw_iox_but_scan() before using this function.
*/
uint8_t aomw_iox_but_isdown( uint8_t buts ) {
  // Buttons are low active
  // Return those that are now 0 (down), and are part of `buts`
  return ~aomw_iox_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` were up (released)
            during the last aomw_iox_but_scan() call, but where down
            (depressed) during the call to aomw_iox_but_scan() before that; 
            the button "went up".
    @param  buts
            A mask, formed by OR-ing AOMW_IOX_BUTxxx macros.
    @return Returns a mask, subset of buts, 
            with a 1 for buttons that went up.
    @note   Must aomw_iox_but_scan() before using this function.
*/
uint8_t aomw_iox_but_wentup( uint8_t buts ) {
  // Buttons are low active
  // Return those that were 0 (down), are now 1 (up), and are part of `buts`
  return ~aomw_iox_but_prvstates & aomw_iox_but_curstates & buts;
}


/*!
    @brief  Returns which of the buttons in `buts` was up (released)
            during the last aomw_iox_but_scan() call.
    @param  buts
            A mask, formed by OR-ing AOMW_IOX_BUTxxx macros.
    @return Returns a mask, subset of buts, 
            with a 1 for buttons that is up.
    @note   Must aomw_iox_but_scan() before using this function.
*/
uint8_t aomw_iox_but_isup( uint8_t buts ) {
  // Buttons are low active
  // Return those that are now 1 (up), and are part of `buts`
  return aomw_iox_but_curstates & buts;
}


// === main =================================================================


/*!
    @brief  Tests if an I/O-expander (IOX) is connected to the I2C bus of  
            OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge.
    @return aoresult_ok              if IOX is present to the I2C bus of addr
            aoresult_dev_noi2cdev    if IOX is not found on the I2C bus of addr
            aoresult_dev_noi2cbridge if addr has no I2C bridge
            other                    OSP (communication) errors
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   This routine assumes the I/O-expander has I2C device address
            AOMW_IOX_DADDR7.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_iox_present(uint16_t addr ) {
  // Check if addr has I2C bridge
  int        enable;
  aoresult_t result;
  result= aoosp_exec_i2cenable_get(addr, &enable); 
  if( result!=aoresult_ok ) return result;
  if( !enable ) return aoresult_dev_noi2cbridge;
  // Check if we can read from 00
  uint8_t buf;
  result = aoosp_exec_i2cread8(addr, AOMW_IOX_DADDR7, 0x00, &buf, 1);
  // Real error
  if( result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout ) return aoresult_dev_noi2cdev;
  return aoresult_ok;
}


/*!
    @brief  Associates this software driver to the I/O-expander (IOX) 
            connected to the I2C bus of OSP node (SAID) with address `addr`.
    @param  addr
            The OSP address of a SAID with an I2C bridge with an I/O-expander.
    @return aoresult_ok           if LEDs are set successfully
            other error code      if there is a (communications) error
    @note   The I/O-expander is controlled via OSP, hence the possibility 
            for transmission errors.
    @note   This routine assumes the I/O-expander has I2C device address
            AOMW_IOX_DADDR7.
    @note   It is allowed to call this function again, to associate this
            driver with a different I/O-expander.
    @note   Sends I2C telegrams, so OSP must be initialized, eg via a call
            to aoosp_exec_resetinit(), and the I2C bus must be powered, eg via 
            a call to aoosp_exec_i2cpower(). Function aomw_topo_build()
            ensures both.
*/
aoresult_t aomw_iox_init(uint16_t addr) {
  aoresult_t result;

  // Record address of the SAID with I2C bridge
  aomw_iox_saidaddr= addr;

  // default I2C bus speed of 100kHz is ok
  // result= aoosp_send_seti2ccfg(addr, AOOSP_I2CCFG_FLAGS_DEFAULT, AOOSP_I2CCFG_SPEED_DEFAULT);
  // if( result!=aoresult_ok ) return result;

  // Switch indicator LEDs off
  result= aomw_iox_led_set(AOMW_IOX_LEDNONE);
  if( result!=aoresult_ok ) return result;

  // Configure button pins as input
  uint8_t cfg = AOMW_IOX_BUTALL;
  result = aoosp_exec_i2cwrite8(addr, AOMW_IOX_DADDR7, AOMW_IOX_REGCFGINP, &cfg, 1);
  if( result!=aoresult_ok ) return result;

  // Determine "prev" state of buttons
  return aomw_iox_but_scan();
}


