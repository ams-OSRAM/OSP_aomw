// aomw_iox4b4l.h - driver for an I/O-expander connected to 4 buttons and 4 LEDs
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
#ifndef _AOMW_IOX4B4L_H_
#define _AOMW_IOX4B4L_H_


#include <stdint.h>     // uint8_t
#include <aoresult.h>   // aoresult_t


// === Indicator LED ========================================================


// Masks for aomw_iox4b4l_led_on/off/set to tell which indicator LED to switch (can be or'ed)
#define AOMW_IOX4B4L_LED0    0x01
#define AOMW_IOX4B4L_LED1    0x02
#define AOMW_IOX4B4L_LED2    0x04
#define AOMW_IOX4B4L_LED3    0x08
#define AOMW_IOX4B4L_LED(n)  (1<<(n)) // n=0..3
#define AOMW_IOX4B4L_LEDALL  (AOMW_IOX4B4L_LED0|AOMW_IOX4B4L_LED1|AOMW_IOX4B4L_LED2|AOMW_IOX4B4L_LED3)
#define AOMW_IOX4B4L_LEDNONE 0x00


// Turns on the indicator LEDs that have a bit set in `led`.
aoresult_t aomw_iox4b4l_led_on( uint8_t leds );
// Turns off the indicator LEDs that have a bit set in `led`.
aoresult_t aomw_iox4b4l_led_off( uint8_t leds );
// Toggles the indicator LEDs that have a bit set in `led`.
aoresult_t aomw_iox4b4l_led_tgl( uint8_t leds );
// The bits set in `leds` indicate which indicator LEDs to turn on, the clear bits indicate which to turn off.
aoresult_t aomw_iox4b4l_led_set( uint8_t leds );


// === Button ===============================================================


// Masks for aomw_iox4b4l_but_went/is/down/up to tell which buttons are queried (can be or'ed)
#define AOMW_IOX4B4L_BUT0    0x01
#define AOMW_IOX4B4L_BUT1    0x02
#define AOMW_IOX4B4L_BUT2    0x04
#define AOMW_IOX4B4L_BUT3    0x08
#define AOMW_IOX4B4L_BUT(n)  (1<<(n)) // n=0..3
#define AOMW_IOX4B4L_BUTALL  (AOMW_IOX4B4L_BUT0|AOMW_IOX4B4L_BUT1|AOMW_IOX4B4L_BUT2|AOMW_IOX4B4L_BUT3)


// This function scans all buttons for their state (up, down) and records that in a global variable. It also maintains the previous state. Use functions aomw_iox4b4l_but_isup()/down(), and/or aomw_iox4b4l_but_wentup()/wentdown() to determine which buttons are respectively went up/down.
aoresult_t aomw_iox4b4l_but_scan( );
// Returns which of the buttons in `buts` were down (depressed) during the last aomw_iox4b4l_but_scan() call, but where up (released) during the call to aomw_iox4b4l_but_scan() before that; the button "went down".
uint8_t aomw_iox4b4l_but_wentdown( uint8_t buts );
// Returns which of the buttons in `buts` was down (depressed) during the last aomw_iox4b4l_but_scan() call.
uint8_t aomw_iox4b4l_but_isdown( uint8_t buts );
// Returns which of the buttons in `buts` were up (released) during the last aomw_iox4b4l_but_scan() call, but where down (depressed) during the call to aomw_iox4b4l_but_scan() before that; the button "went up".
uint8_t aomw_iox4b4l_but_wentup( uint8_t buts );
// Returns which of the buttons in `buts` was up (released) during the last aomw_iox4b4l_but_scan() call.
uint8_t aomw_iox4b4l_but_isup( uint8_t buts );


// === main =================================================================


// I2C address and button/led pin mapping of two I/O-expander boards; needed for aomw_iox4b4l_init()
// the ...SAIDSENSEV2 versions are for an early SAIDsense board
#define AOMW_IOX4B4L_DADDR7_SAIDSENSEV2    0x3F
#define AOMW_IOX4B4L_PINCFG_SAIDSENSEV2    (0x74206531 | 0x88880000) // set high active bit for all 4 LEDs

#define AOMW_IOX4B4L_DADDR7_SAIDSENSE      0x3C
#define AOMW_IOX4B4L_PINCFG_SAIDSENSE      0x75316420 // LEDs and buttons are low active

#define AOMW_IOX4B4L_DADDR7_SAIDBASIC      0x20
#define AOMW_IOX4B4L_PINCFG_SAIDBASIC      (0x75316420 | 0x88880000) // set high active bit for all 4 LEDs


// Tests if an I/O-expander (IOX) is connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_iox4b4l_present( uint16_t addr, uint8_t daddr7 );
// Associates this software driver to the I/O-expander (IOX) connected to the I2C bus of OSP node (SAID) with address `addr`, and config `pincfg`) for the I/O-expander at address `daddr7`.
aoresult_t aomw_iox4b4l_init(uint16_t addr, uint8_t daddr7, uint32_t pincfg);


#endif







