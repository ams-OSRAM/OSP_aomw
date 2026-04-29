// aomw_toposkip.ino - uses the topology map for light effects, skipping some SAID channels
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
This demos the SKIPCHN feature: the topo builder skips channel x of a SAID if
the SAID has the SKIPCHNx bit set in OTP. The demo begins with no channels
skipped, but each step adds one channel to skip.

HARDWARE
The demo runs on the OSP32 board, in loop back mode.
In Arduino select board "ESP32S3 Dev Module".
Since the demo writes to the OTP (mirror), it requires the SAID password,
see aoosp.cpp line 30 or aoosp_said_testpw_get().

BEHAVIOR
All five LEDs on OSP32 will light up, every step one is skipped, so
after 6 steps all LEDs are off. Every step the color toggles between
cyan and green. This is done, because when N001.C2 is skipped no
change is visible because that one is always skipped since it is
allocated to I2C.

OUTPUT
Welcome to aomw_toposkip.ino
version: result 0.4.6, spi 1.0.0, osp 0.8.0, mw 0.5.0
spi: init(MCU-B)
osp: init
mw: init

skip N001 none
skip N002 none
triplets 5
T0 N001.C0
T1 N001.C1       << N001.C2 is always I2C
T2 N002.C0
T3 N002.C1
T4 N002.C2

skip N002 C2
triplets 4
T0 N001.C0
T1 N001.C1
T2 N002.C0
T3 N002.C1

skip N002 C2 C1
triplets 3
T0 N001.C0
T1 N001.C1
T2 N002.C0

skip N002 C2 C1 C0
triplets 2
T0 N001.C0
T1 N001.C1

skip N001 C2      << N001.C2 was skipped; it has I2C
triplets 2
T0 N001.C0
T1 N001.C1

skip N001 C2 C1
triplets 1
T0 N001.C0

skip N001 C2 C1 C0
triplets 0

skip N001 none    << restarts
skip N002 none
triplets 5
T0 N001.C0
T1 N001.C1
T2 N002.C0
T3 N002.C1
T4 N002.C2
*/


// === The RUNLED state machine =============================================


static void skipchns(int addr, int chns) {
  aoresult_t result;

  // Reset and initialize all nodes (so that skipchns can be called)
  result= aoosp_exec_resetinit(); 
  if( result!=aoresult_ok ) Serial.printf("resetinit %s\n", aoresult_to_str(result) );

  // Configure which channels (chns) to skip from node (addr)
  result= aoosp_exec_skipchns_set(addr,chns); 
  if( result!=aoresult_ok ) Serial.printf("skipchns_set %s\n", aoresult_to_str(result) );

  Serial.printf("skip N%03X", addr );
  if( chns==0 ) Serial.printf(" none");
  if( (chns&(1<<2))!=0 ) Serial.printf(" C2");
  if( (chns&(1<<1))!=0 ) Serial.printf(" C1");
  if( (chns&(1<<0))!=0 ) Serial.printf(" C0");
  Serial.printf("\n");
}


int count;
static void all_on() {
  aoresult_t result;

  // Build topo map (OTP is persistent over RESET)
  result= aomw_topo_build();
  if( result!=aoresult_ok ) Serial.printf("topo_build %s\n", aoresult_to_str(result) );
  Serial.printf("triplets %d\n", aomw_topo_numtriplets());
  aomw_topo_dump_triplets();

  // Illuminate all LEDs
  const aomw_topo_rgb_t * color = count++%2==0 ? &aomw_topo_cyan : &aomw_topo_green;
  for( int tix=0; tix<aomw_topo_numtriplets(); tix++ ) {
    aoresult_t result= aomw_topo_settriplet(tix, color );
    if( result!=aoresult_ok ) Serial.printf("topo_build %s\n", aoresult_to_str(result) );
  }

  Serial.printf("\n");
  delay(2000);
}


// === The application ======================================================


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_toposkip.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();

  aomw_topo_dim_set(64); // 64/1024 (~6%) of max brightness
  Serial.printf("\n");
}


void loop() {
  skipchns(0x001, 0b000);
  skipchns(0x002, 0b000);
  all_on();

  skipchns(0x002, 0b100);
  all_on();

  skipchns(0x002, 0b110);
  all_on();

  skipchns(0x002, 0b111);
  all_on();

  skipchns(0x001, 0b100);
  all_on();

  skipchns(0x001, 0b110);
  all_on();

  skipchns(0x001, 0b111);
  all_on();
}


