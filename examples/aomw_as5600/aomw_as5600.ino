// aomw_as5600.ino - demonstrates reading a magnetic rotary sensor AS5600
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
#include <aospi.h>
#include <aoosp.h>
#include <aomw.h>


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID and checks 
whether there is a AS5600 rotary sensor - as on the SAIDsense board. 
If there is, the demo configures the sensor and starts reading and printing 
angles at a steady pace.

HARDWARE
The demo needs the SAIDsense board to be connected to the OSP32 board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Nothing to be seen on the OSP chain; check the Serial out 
(or even serial plotter, see screenshot in SerialPlotter.png).

OUTPUT
Welcome to aomw_as5600.ino
version: result 0.4.6, spi 1.0.0, osp 0.8.0, mw 0.5.0
spi: init(MCU-B)
osp: init
mw: init
demo: init
force(agc):27 angle(deg):169
force(agc):27 angle(deg):170
force(agc):27 angle(deg):174
force(agc):27 angle(deg):188
force(agc):27 angle(deg):202
force(agc):27 angle(deg):215
force(agc):27 angle(deg):220
force(agc):27 angle(deg):228
force(agc):27 angle(deg):245
force(agc):27 angle(deg):253
force(agc):27 angle(deg):261
force(agc):27 angle(deg):289
force(agc):27 angle(deg):303
force(agc):27 angle(deg):318
*/


// The address of the SAID that has the I2C bridge
#define ADDR 0x003 // SAIDsense connected to OSP32 (with 1 SAID); SAISsense has two SAIDs; second has I2C bridge


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


// Poll time
#define POLL_MS 100 // slower then AS5600


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

  // (4) check whether there is an AS5600 connected to the SAID
  result= aomw_as5600_present(ADDR); PRINT_ERROR();

  // (5) init the AS5600 magnetic rotary sensor
  result= aomw_as5600_init(ADDR); PRINT_ERROR();

  // (6) setup poll timer
  lastms= millis() - POLL_MS;
  Serial.printf("demo: init\n");
}


void demo_step() {
  if( millis()-lastms>POLL_MS ) {
    aoresult_t result;
    // Get AS5600 angle and force
    int angle_raw, angle_deg, agc;
    result= aomw_as5600_angle_get(&angle_raw); PRINT_ERROR();
    result= aomw_as5600_force_get(&agc); PRINT_ERROR();
    angle_deg= angle_raw*360/AOMW_AS5600_ANGLE_MAX;
    // Print
    Serial.printf( "force(agc):%d angle(deg):%d\n", agc, angle_deg ); 
    lastms= millis();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_as5600.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  demo_init();
}

void loop() {
  demo_step();
}

