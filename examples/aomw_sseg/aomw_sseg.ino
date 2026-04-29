// aomw_sseg.ino - demonstrates the quad 7-segment display
/*****************************************************************************
 * Copyright 2025-2026 by ams OSRAM AG                                       *
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
#include <aomw.h>          // aomw_sseg_printf()


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and checks 
whether there is a quad 7-segment display - as on the SAIDsense board. If there 
is, the demo configures the display and starts showing increasing numbers.

HARDWARE
The demo needs the SAIDsense board to be connected to the OSP32 board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
The 7-segment display on the SAIDsense board shall first show SSEG
(see demo.jpg), then a counter from -99.9 to 999.9 and then wrap around.

OUTPUT
Welcome to aomw_sseg.ino
version: result 0.4.6, spi 1.0.0, osp 0.8.0, mw 0.5.0
spi: init(MCU-B)
osp: init
mw: init
demo: init
*/


// The address of the SAID that has the I2C bridge
#define ADDR 0x003 // SAIDsense connected to OSP32 (with 1 SAID); SAISsense has two SAIDs; second has display


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


// Poll time
#define POLL_MS 75 


// Last poll timestamp
int      num;
uint32_t lastms;


void demo_init() {
  // (1) initialize the OSP chain
  aoresult_t result;
  result = aoosp_exec_resetinit(); PRINT_ERROR();

  // (2) check if SAID has I2C bridge
  int enable;
  result= aoosp_exec_i2cenable_get(ADDR, &enable); PRINT_ERROR();
  if( !enable ) result= aoresult_dev_noi2cbridge; PRINT_ERROR();

  // (3) power the I2C bridge in the SAID
  result= aoosp_exec_i2cpower(ADDR); PRINT_ERROR();

  // (4) check whether there is a quad 7-segment display connected to the SAID
  result= aomw_sseg_present(ADDR); PRINT_ERROR();

  // (5) init the quad 7-segment display
  result= aomw_sseg_init(ADDR); PRINT_ERROR();

  // (6) print text on display
  result= aomw_sseg_printf("SSEG"); PRINT_ERROR(); 
  delay(2000);

  // (7) setup poll timer
  lastms= millis() - POLL_MS; // force print now
  num=-999;
  Serial.printf("demo: init\n");
}


void demo_step() {
  if( millis()-lastms>POLL_MS ) {
    aoresult_t result;
    result= aomw_sseg_printf("%5.1f",num/10.0); PRINT_ERROR(); 
    num++;
    if( num>=9999 ) num=-999;
    lastms= millis();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_sseg.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  demo_init();
}


void loop() {
  demo_step();
}

