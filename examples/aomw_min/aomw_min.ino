// aomw_min.ino - minimal example using topo to blink an LED
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
#include <aoresult.h>
#include <aospi.h>
#include <aoosp.h>
#include <aomw.h>


/*
DESCRIPTION
This demo scans the OSP chain using the topo builder from the middleware to 
form a topology map of all nodes of the OSP chain. Next, it toggles
the first triplet between magenta and yellow. Note that topo abstracts
away that a SAID has three RGB triplets and an RGBI one, and that they
need different telegrams for the same results.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached, but
a terminator in OUT or a loop back cable from OUT to IN is needed.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
The first RGB (L1.0 aka OUT0) of SAID OUT blinks magenta and yellow,
while printing this to Serial.

OUTPUT
Welcome to aomw_min.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init

magenta (ok)
yellow (ok)

magenta (ok)
yellow (ok)

magenta (ok)
yellow (ok)
*/


uint32_t time0us,time1us;
void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_min.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  Serial.printf("\n");

  aomw_topo_build();     // Determine how many and which OSP nodes are connected
  aomw_topo_dim_set(64); // 64/1024 (~6%) of max brightness
}


void loop() {
  // Blink triplet 0 in two colors.
  aoresult_t result;
  result= aomw_topo_settriplet( 0, &aomw_topo_magenta );
  Serial.printf("magenta (%s)\n", aoresult_to_str(result) );
  delay(1000);

  result= aomw_topo_settriplet( 0, &aomw_topo_yellow );
  Serial.printf("yellow (%s)\n", aoresult_to_str(result) );
  delay(1000);

  Serial.printf("\n");
}

