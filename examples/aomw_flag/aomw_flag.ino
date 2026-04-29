// aomw_flag.ino - shows various flags
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
This demo builds a topology map of all nodes of the OSP chain. Next, it 
uses this topo map to paint country flags spread out over an entire OSP chain.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached, but 
for better flag rendering connect eg the SAIDbasic board.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Shows 8 country flags in a sequence, then repeats.

OUTPUT
Welcome to aomw_flag.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init

flag dutch
flag columbia
flag japan
flag mali
flag italy
flag europe
flag usa
flag china

flag dutch
flag columbia
*/


#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_flag.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  Serial.printf("\n");

  // Flag painter rely on topo
  aoresult_t result= aomw_topo_build(); PRINT_ERROR();
}


void loop() {
  aoresult_t result;
  #if 0
    result= aomw_flag_painter_dutch(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_columbia(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_japan(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_mali(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_italy(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_europe(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_usa(); PRINT_ERROR();
    delay(2000);
    result= aomw_flag_painter_china(); PRINT_ERROR();
    delay(2000);
  #else
    for( int pix=0; pix<aomw_flag_count(); pix++ ) {
      result= aomw_flag_painter(pix)(); PRINT_ERROR();
      Serial.printf("flag %s\n", aomw_flag_name(pix) );
      delay(2000);
    }
    Serial.printf("\n");
  #endif
}

