// aomw_as6212.ino - demonstrates reading an AS6212 temperature sensor
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
#include <aomw.h>          // aomw_as6212_temp_get()


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and checks 
whether there is an AS6212 temperature sensor - as on the SAIDsense board. 
If there is, the demo configures the sensor and starts reading and printing 
temperatures at a steady pace.

HARDWARE
The demo needs the SAIDsense board to be connected to the OSP32 board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Nothing to be seen on the OSP chain; check the Serial out 
(or even serial plotter, see screenshot in SerialPlotter.png).

OUTPUT
Welcome to aomw_as6212.ino
version: result 0.4.6, spi 1.0.0, osp 0.8.0, mw 0.5.0
spi: init(MCU-B)
osp: init
mw: init
demo: init
AS6212-C:24.8    SAID-C:27
AS6212-C:24.8    SAID-C:26
AS6212-C:24.8    SAID-C:26
AS6212-C:24.9    SAID-C:26
AS6212-C:25.0    SAID-C:27
AS6212-C:25.0    SAID-C:26
AS6212-C:25.0    SAID-C:27
AS6212-C:25.1    SAID-C:26
AS6212-C:25.1    SAID-C:27
AS6212-C:25.2    SAID-C:26
AS6212-C:25.2    SAID-C:26
AS6212-C:25.2    SAID-C:27
*/


// The address of the SAID that has the I2C bridge
#define ADDR 0x003 // SAIDsense connected to OSP32 (with 1 SAID); SAISsense has two SAIDs; second has i2C bridge


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


// Poll time
#define POLL_MS 100 // too fast for AS6212 so we get some duplicates


// Last poll timestamp
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

  // (4) check whether there is an AS6212 connected to the SAID
  result= aomw_as6212_present(ADDR); PRINT_ERROR();

  // (5) init the AS521x temperature sensor
  result= aomw_as6212_init(ADDR); PRINT_ERROR();

  // (6) set conversion rate for the temperature sensor
  result= aomw_as6212_convrate_set(125); PRINT_ERROR();
  
  // (7) setup poll timer
  lastms= millis() - POLL_MS;
  Serial.printf("demo: init\n");
}


void demo_step() {
  if( millis()-lastms>POLL_MS ) {
    aoresult_t result;
    // Get AS6212 temp    
    int millicelsius;
    result= aomw_as6212_temp_get(&millicelsius); PRINT_ERROR();
    // Get SAID temp    
    uint8_t rawtemp;
    result= aoosp_send_readtemp(ADDR, &rawtemp); PRINT_ERROR();
    // Print both
    Serial.printf( "AS6212-C:%.1f    SAID-C:%d\n", millicelsius/1000.0f, aoosp_prt_temp_said(rawtemp) );
    lastms= millis();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_as6212.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  demo_init();
}


void loop() {
  demo_step();
}

