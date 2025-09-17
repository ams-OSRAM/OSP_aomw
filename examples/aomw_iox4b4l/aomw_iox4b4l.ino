// aomw_iox4b4l.ino - demonstrates controlling the IOX (on SAIDsense, connected to 4 buttons and 4 LEDs)
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
#include <aospi.h>         // aospi_init()
#include <aoosp.h>         // aoosp_exec_resetinit()
#include <aomw.h>          // aomw_iox4b4l_led_set()


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and 
checks whether there is a "selector". A selector is an I/O-expander (IOX),
an I2C device that exposes a set of GPIO pins, in the case of a selector
connected to 4 buttons and 4 indicator LEDs.
If the selector is found (there is on on the SAIDbasic board and one on 
the SAIDsense board), the demo plays a light show on the connected 
indicator LEDs, which can be interrupted by pressing a button connected 
to the IOX.

HARDWARE
The demo needs the SAIDbasic board to be connected to the OSP32 board.
Change the #if below to run it using the SAIDsense board instead.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
It shows a running LED using the four indicator LEDs on the SAIDbasic board
(associated with the four buttons). When a button is pressed, the running 
stops, all indicator LEDs switch on except the one associated with the pressed
button.

OUTPUT
Welcome to aomw_iox4b4l.ino
version: result 0.4.6 spi 1.0.0 osp 0.8.0 mw 0.5.0
cfg: 005 20 FDB96420
spi: init(MCU-B)
osp: init
mw: init
demo: init
*/


#if 0 // Demo on SAIDsense
  #define ADDR   0x003 
  #define DADDR7 AOMW_IOX4B4L_DADDR7_SAIDSENSEV2
  #define PINCFG AOMW_IOX4B4L_PINCFG_SAIDSENSEV2
#endif

#if 1 // Demo on SAIDbasic
  #define ADDR   0x005
  #define DADDR7 AOMW_IOX4B4L_DADDR7_SAIDBASIC
  #define PINCFG AOMW_IOX4B4L_PINCFG_SAIDBASIC
#endif


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


void demo_init() {
  // (1) initialize the OSP chain
  aoresult_t result;
  result = aoosp_exec_resetinit(); PRINT_ERROR();

  // (2) check if SAID has I2C bridge
  int enable;
  result= aoosp_exec_i2cenable_get(ADDR, &enable); PRINT_ERROR();
  if( !enable ) result= aoresult_dev_noi2cbridge; PRINT_ERROR();

  // (3) power the I2C bridge in a SAID
  result= aoosp_exec_i2cpower(ADDR); PRINT_ERROR();

  // (4) check whether there is an IOX
  result= aomw_iox4b4l_present(ADDR, DADDR7); PRINT_ERROR();

  // (5) init IOX for SAIDsense board
  result= aomw_iox4b4l_init(ADDR, DADDR7, PINCFG); PRINT_ERROR();

  Serial.printf("demo: init\n");
}


int led;
uint32_t last;
void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_iox4b4l.ino\n");
  Serial.printf("version: result %s spi %s osp %s mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );
  Serial.printf("cfg: %03X %02X %08X\n",ADDR,DADDR7,PINCFG);

  aospi_init();
  aoosp_init();
  aomw_init();
  demo_init();
  Serial.printf("\n");

  led= 0;
  aoresult_t result= aomw_iox4b4l_led_on( AOMW_IOX4B4L_LED(led) ); PRINT_ERROR();
  last= millis();
}

void loop() {
  aoresult_t result;

  // Scan button state to capture transitions ("wentdown")
  result= aomw_iox4b4l_but_scan(); PRINT_ERROR();

  // Some button is pressed, compose mask of all LEDs except pressed one
  if( aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUTALL) ) {
    uint8_t leds=AOMW_IOX4B4L_LEDALL;
    if( aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUT0) ) leds^=AOMW_IOX4B4L_LED0;
    if( aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUT1) ) leds^=AOMW_IOX4B4L_LED1;
    if( aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUT2) ) leds^=AOMW_IOX4B4L_LED2;
    if( aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUT3) ) leds^=AOMW_IOX4B4L_LED3;
    result= aomw_iox4b4l_led_set(leds); PRINT_ERROR();
  }

  // Last button released, back to animation
  if( aomw_iox4b4l_but_wentup(AOMW_IOX4B4L_BUTALL) && !aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUTALL) ) {
    result= aomw_iox4b4l_led_set(AOMW_IOX4B4L_LED(led)); PRINT_ERROR();
    last= millis();
  }

  // Animate indicator LEDs when no button pressed
  if( !aomw_iox4b4l_but_isdown(AOMW_IOX4B4L_BUTALL) && millis()-last>200 ) {
    result= aomw_iox4b4l_led_off(AOMW_IOX4B4L_LED(led)); PRINT_ERROR();
    led= (led+1)%4;
    result= aomw_iox4b4l_led_on(AOMW_IOX4B4L_LED(led)); PRINT_ERROR();
    last= millis();
  }

}

