// aomw_tscript.ino - demonstrates playing a script
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
This demo uses topo to initialize the OSP chain, then installs one of the
animation scripts. The main program continuously loops over all script
instructions to draw the frames.

Note that the scripts need to be packed in small memories (256 bytes), as
a result, the color steps are quite coarse.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached, but 
for better animation script rendering connect eg the SAIDbasic board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Depends on the animation script
- blink: five LEDs on the SAIDbasic blink yellow and blue.
- walk: block move left to right, in the middle its half highlights

OUTPUT
Welcome to aomw_tscript.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init
*/


static const uint16_t blink[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB 
  0034660,  // ---YY---
  0034007,  // ---BB---
  0070000,  // end
};


static const uint16_t walk[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB  oPLURGB  oPLURGB
  0000400,                   // r-------
  0000000, 0111200,          // -r------
  0011000, 0122200,          // --r-----
  0022000, 0133700, 0144200, // ---Rr---
  0033200, 0144700,          // ---rR---
  0034000, 0155200,          // -----r--
  0055000, 0166200,          // ------r-
  0066000, 0177200,          // -------r
  0077000,                   // --------
  0070000,                   // end
};


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_tscript.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  aoresult_t result= aomw_topo_build();
  if( result!=aoresult_ok ) Serial.printf(aoresult_to_str(result));

  // Pick a script from this demo (simple), or one of the stock ones in aomw_tscript.
  const uint16_t * anim= blink; // walk, aomw_tscript_rainbow(), aomw_tscript_bouncingblock(), aomw_tscript_colormix(), aomw_tscript_heartbeat()
  aomw_tscript_install( anim, aomw_topo_numtriplets() );
}


void loop() {
  // Show one frame
  aoresult_t result= aomw_tscript_playframe();
  if( result!=aoresult_ok ) Serial.printf(aoresult_to_str(result));
  // Wait for next frame: 500ms give a frame rate of 2FPS
  delay(500);
}

