// aomw_topodemo.ino - uses the topology map to create a running led animation
/*****************************************************************************
 * Copyright 2024-2026 by ams OSRAM AG                                       *
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
This demo first creates a topology map of all nodes of the OSP chain. 
Next it animates a running led animation, constantly updating the triplets.
The topo map creation and running led animation is driven from a state
machine. This would allow running commands from serial or scanning for 
presses of UI buttons.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached, but 
the output shown below is when the SAIDbasic board is connected.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
A running LED, first blue then white, first from left to right, then
from right to left.

OUTPUT
Welcome to aomw_topodemo.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init

Starting animation on 17 RGBs
Brightness 64/1024
*/


// === The RUNLED state machine =============================================


// Time (in ms) between two LED updates
#define RUNLED_MS 25


// The colors in the runled loop
static const aomw_topo_rgb_t * const runled_rgbs[] = { &aomw_topo_blue, &aomw_topo_white };
#define RUNLED_RGBS_SIZE ( sizeof(runled_rgbs)/sizeof(runled_rgbs[0]) )


// The state of the runled state machine
static int      runled_tix;
static int      runled_colorix;
static int      runled_dir;
static uint32_t runled_ms;


// Start of the runled state machine
static void runled_start() {
  runled_tix= 0;
  runled_colorix= 0;
  runled_dir= +1;
  runled_ms= millis();
}


// Step of the runled state machine
static aoresult_t runled_step() {
  if( millis()-runled_ms < RUNLED_MS ) return aoresult_ok; // not yet time to update
  runled_ms = millis();

  // Set triplet tix to color cix
  aoresult_t result= aomw_topo_settriplet(runled_tix, runled_rgbs[runled_colorix] );
  if( result!=aoresult_ok ) return result;

  // Go to next triplet
  int new_tix = runled_tix + runled_dir;
  if( 0<=new_tix && new_tix<aomw_topo_numtriplets() ) {
    runled_tix= new_tix;
  } else  { // hit either end
    // reverse direction and step color
    runled_dir = -runled_dir;
    runled_colorix += 1;
    if( runled_colorix==RUNLED_RGBS_SIZE ) {
      runled_colorix= 0;
    }
    // Just in case there was and error (under voltage error) in some node, broadcast clear all and broadcast switch back on
    result= aoosp_send_clrerror(0x000);
    if( result!=aoresult_ok ) return result;
    result=aoosp_send_goactive(0x000);
    if( result!=aoresult_ok ) return result;
  }
  return aoresult_ok;
}


// === The application ======================================================


// The application states
#define APPSTATE_TOPOBUILD  1
#define APPSTATE_RUNLED     2
#define APPSTATE_ERROR      3
#define APPSTATE_DONE       4


int appstate;


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_topodemo.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  Serial.printf("\n");

  aomw_topo_dim_set(64); // 64/1024 (~6%) of max brightness

  appstate= APPSTATE_TOPOBUILD;
  aomw_topo_build_start();
}


aoresult_t result;
void loop() {
  switch( appstate ) {

    case APPSTATE_TOPOBUILD:
      if( aomw_topo_build_done() ) {
        Serial.printf("Starting animation on %d RGBs\n", aomw_topo_numtriplets() );
        Serial.printf("Brightness %d/1024\n", aomw_topo_dim_get() );
        runled_start();
        appstate= APPSTATE_RUNLED;
      }
      result= aomw_topo_build_step(); 
      if( result!=aoresult_ok ) appstate= APPSTATE_ERROR;
    break;

    case APPSTATE_RUNLED:
      result= runled_step(); 
      if( result!=aoresult_ok ) appstate= APPSTATE_ERROR;
    break;

    case APPSTATE_ERROR:
      Serial.printf("ERROR %s\n", aoresult_to_str(result,1));
      appstate= APPSTATE_DONE;
    break;

    case APPSTATE_DONE:
      // spin (after error is printed)
    break;
  }

}


