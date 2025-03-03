// aomw_iox.ino - demonstrates controlling the IOX (with 4 buttons and 4 indicator LEDs)
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
#include <aospi.h>
#include <aoosp.h>
#include <aomw.h>


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and 
checks whether there is an I/O-expander (IOX). An I/O-expander is an 
I2C device that exposes a set of GPIO pins. If there is an IOX, the demo 
plays a light show on the connected indicator LEDs, which can be 
interrupted by pressing a button connected to the IOX.

HARDWARE
The demo needs the SAIDbasic to beconnected to the OSP32 board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
It shows a running LED using the four indicator LEDs on the SAID basic board
(associated with the four buttons). When a button is pressed, the running 
stops, all indicator LEDs swithc on except the one associated with the pressed
button.

OUTPUT
Welcome to aomw_iox.ino
version: result 0.4.1 spi 0.5.1 osp 0.4.1 mw 0.4.0
spi: init
osp: init
mw: init
demo: init
*/


// The address of the SAID that has the I2C bridge
#define ADDR 0x005 // SAID basic has IOX on SAID with addr 005


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
  result= aomw_iox_present(ADDR); PRINT_ERROR();

  // (5) init IOX
  result= aomw_iox_init(ADDR); PRINT_ERROR();

  Serial.printf("demo: init\n");
}


int led;
uint32_t last;
void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_iox.ino\n");
  Serial.printf("version: result %s spi %s osp %s mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  demo_init();
  Serial.printf("\n");

  led= 0;
  aoresult_t result= aomw_iox_led_on( AOMW_IOX_LED(led) ); PRINT_ERROR();
  last= millis();
}

void loop() {
  aoresult_t result;

  result= aomw_iox_but_scan(); PRINT_ERROR();
  if( aomw_iox_but_isdown(AOMW_IOX_BUTALL) ) {
    // A button is pressed, compose mask of all LEDs on except pressed one
    uint8_t leds=AOMW_IOX_LEDALL;
    if( aomw_iox_but_isdown(AOMW_IOX_BUT0) ) leds^=AOMW_IOX_LED0;
    if( aomw_iox_but_isdown(AOMW_IOX_BUT1) ) leds^=AOMW_IOX_LED1;
    if( aomw_iox_but_isdown(AOMW_IOX_BUT2) ) leds^=AOMW_IOX_LED2;
    if( aomw_iox_but_isdown(AOMW_IOX_BUT3) ) leds^=AOMW_IOX_LED3;
    result= aomw_iox_led_set(leds); PRINT_ERROR();
    return;
  }

  // animate indicator LEDs
  if( millis()-last>200 ) {
    last= millis();
    result= aomw_iox_led_off(AOMW_IOX_LED(led)); PRINT_ERROR();
    led= (led+1)%4;
    result= aomw_iox_led_on(AOMW_IOX_LED(led)); PRINT_ERROR();
  }

}

