// aomw_sfh5721.ino - demonstrates reading the ams-OSRAM SFH 5721 ambient light sensor
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
#include <aospi.h>         // aospi_init()
#include <aoosp.h>         // aoosp_exec_resetinit()
#include <aomw.h>          // aomw_sfh5721_als_get()


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and 
checks whether there is a SFH5721 light sensor - as on the SAIDsense board. 
If there is, the demo configures the sensor and starts reading and printing 
light levels at a steady pace.

HARDWARE
The demo needs the SAIDsense board to be connected to the OSP32 board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Nothing seen on the OSP chain; check serial out (or even serial plotter,
see screenshot in SerialPlotter.png).

OUTPUT
Welcome to aomw_sfh5721.ino
version: result 0.4.6 spi 1.0.0 osp 0.8.0 mw 0.5.0
spi: init(MCU-B)
osp: init
mw: init
demo: init
SFH5721(lux):557
SFH5721(lux):555
SFH5721(lux):559
SFH5721(lux):581
SFH5721(lux):601
SFH5721(lux):613
SFH5721(lux):617
SFH5721(lux):609
SFH5721(lux):574
SFH5721(lux):476
SFH5721(lux):360
SFH5721(lux):224
SFH5721(lux):139
SFH5721(lux):99
SFH5721(lux):247
SFH5721(lux):372
SFH5721(lux):480
SFH5721(lux):544
SFH5721(lux):577
*/


// The address of the SAID that has the I2C bridge
#define ADDR 0x003 // SAIDsense connected to OSP32 (with 1 SAID); SAISsense has two SAIDs; second has i2C bridge


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


// Poll time
#define POLL_MS 100 // slower then SFH5721


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

  // (4) check whether there is an SFH5721 connected to the SAID
  result= aomw_sfh5721_present(ADDR); PRINT_ERROR();

  // (5) init the SFH5721 light sensor
  result= aomw_sfh5721_init(ADDR); PRINT_ERROR();

  // Wait for first measurement to complete
  delay(25);
  
  // (6) setup poll timer
  lastms= millis() - POLL_MS;
  Serial.printf("demo: init\n");
}


void demo_step() {
  if( millis()-lastms>POLL_MS ) {
    aoresult_t result;
    // Get SFH5721 light level (=lux)
    int als;
    result= aomw_sfh5721_als_get(&als); PRINT_ERROR();
    // Print
    Serial.printf( "SFH5721(lux):%d\n", als );
    lastms= millis();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_sfh5721.ino\n");
  Serial.printf("version: result %s spi %s osp %s mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  demo_init();
}


void loop() {
  demo_step();
}

