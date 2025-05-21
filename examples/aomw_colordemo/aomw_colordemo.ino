// aomw_colordemo.ino - uniform colors for real OSP chain (with varying temperature)
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
#include <aospi.h>
#include <aoosp.h>
#include <caldb.h>


/*
DESCRIPTION
This demo shows how to get uniform colors over multiple RGB triplets with
varying temperatures. It controls a SAID chain with triplets, using the 
temperature sensor in the SAIDs as an approximation of the triplets' 
temperatures. Note however, the supplied calibration data, although real, 
is very unlikely to match the user's hardware (see [*]).

HARDWARE
This demo runs on the OSP32 board, with zero or more demo boards connected
to it. All nodes must be SAID nodes. The demo comes with calibration data
(see caldb), but that should be replaced with data matching the demo board.
In Arduino,select board "ESP32S3 Dev Module".

BEHAVIOR
This demo picks one target color (purple) and constantly updates the PWM 
settings of all channels of all SAIDs to ensure that the purple stays
the same purple (see [**]) even when temperature changes.

OUTPUT
Welcome to aomw_colordemo.ino

spi: init
osp: init

CALDB instances 250 currents (mA) 20.0 50.0 (2)
average R 0.6973,0.3024,1.8325 (max dist2 0.000004)
average G 0.1332,0.7310,3.8872 (max dist2 0.000380)
average B 0.1543,0.0229,0.6355 (max dist2 0.000005)

TIME 8.32 us per triplet

Updated 150000 triplets
Updated 300000 triplets
Updated 450000 triplets
*/


// This function is the work-horse: it gets the exact color point of `source` 
// triplet `tix` at curret `cur` and triplet temperature `tempc`. Then it
// computes the mixing ratio to reach target color `target`. Finally it clips
// and maps the mixing ratio to PWM settings. 
void compute_pwm(int tix, float cur, float tempc, aomw_color_xyz1_t * target,  aomw_color_pwm_t * pwm ) {
  aomw_color_xyz3_t source; // The source triplet that must be mixed to target color
  aomw_color_mix_t  mix;     // The computed mixing ratios (duty cycle for R, G, and B PWM).
  bool              ok;      // Was target color reached

  // Get source triplet calibration data (for current cur, temperature corrected)
  caldb_get(tix, cur, tempc, &source);
  // Compute mixing ratio
  aomw_color_computemix( &source, target, &mix );
  // Map to PMW settings (SAID has 15 bits at 500Hz, 14 at 1000Hz, both could be +1 for dithering)
  ok= aomw_color_mix_to_pwm( &mix, 0xFFFF, pwm );
  if( !ok ) Serial.printf("WARNING: color not reachable (triplet %d: %s)\n",tix,aomw_color_mix_to_str(&mix));
}


// This function calls the "work=horse" `loopcount` times and prints the
// average computation time (per triplet).
void compute_time(int loopcount) {
  aomw_color_xyz1_t target;
  aomw_color_pwm_t  pwm;

  aomw_color_cxcyiv1_t cxcyiv1= {0.40, 0.15, 1.00};
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv1, &target );

  uint32_t t0=micros();
  for(int loop=0; loop<loopcount; loop++ ) {
    compute_pwm( 0, 24E-3, 25.0, &target, &pwm);
  }
  uint32_t t1=micros();
  float t=(t1-t0) / (float)loopcount;
  Serial.printf("TIME %0.2f us per triplet\n",t);
  Serial.printf("\n");
}


// Lazy way to handle errors in this example
#define CHECKRESULT(tele) do { if( result!=aoresult_ok ) { Serial.printf("ERROR in " tele ": %s\n", aoresult_to_str(result) ); return; } } while(0)


uint16_t           g_addr;
aomw_color_xyz1_t  g_target;
int                progress;


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_colordemo.ino\n\n");

  // Init OSP libs
  aospi_init();
  aoosp_init();
  Serial.printf("\n");

  // Stats on caldb (may all be skipped)
  caldb_check();
  caldb_printstats();
  compute_time(100*1000);

  // Init OSP chain
  aoresult_t result;
  result= aoosp_exec_resetinit(    ); CHECKRESULT("resetinit");
  result= aoosp_send_clrerror(0x000); CHECKRESULT("clrerror");
  result= aoosp_send_goactive(0x000); CHECKRESULT("goactive");

  // Ensure all nodes are SAIDs
  for( uint16_t addr=0x001; addr<=aoosp_exec_resetinit_last(); addr++ ) {
    uint32_t id;
    result= aoosp_send_identify(addr, &id ); CHECKRESULT("identity");
    if( !AOOSP_IDENTIFY_IS_SAID(id) ) Serial.printf("ERROR %03X is not a SAID\n",addr);
  }

  // Drive all channels at 24 mA
  #define DRIVE_CURRENT (24E-3f)
  for( uint16_t addr=0x001; addr<=aoosp_exec_resetinit_last(); addr++ ) {
    result= aoosp_send_setcurchn(addr, 0, AOOSP_CURCHN_CUR_DEFAULT, 3,3,3); CHECKRESULT("setcurchn0");
    result= aoosp_send_setcurchn(addr, 1, AOOSP_CURCHN_CUR_DEFAULT, 4,4,4); CHECKRESULT("setcurchn1");
    result= aoosp_send_setcurchn(addr, 2, AOOSP_CURCHN_CUR_DEFAULT, 4,4,4); CHECKRESULT("setcurchn2");
  }

  // Pick a target color
  aomw_color_cxcyiv1_t cxcyiv1= {0.40, 0.20, 0.10}; // chosen target [**] color: purple 
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv1, &g_target );

  // Init state
  g_addr= 0x001;
  progress= 0;
}


void loop() {
  aoresult_t result;

  // Get SAID's temperature (as estimate for the temperature of its three triplets)
  uint8_t tempr;
  result= aoosp_send_readtemp(g_addr, &tempr ); CHECKRESULT("readtemp");
  // Convert from raw to degrees C
  int tempc= aoosp_prt_temp_said(tempr);

  // Update PWM of all three channels of the SAID (they share the temperature)
  for( int chn=0; chn<3; chn++ ) {
    aomw_color_pwm_t pwm;
    int tix= g_addr*3+chn; // Assume caldb matches g_addr/chn order [*]
    compute_pwm( tix, DRIVE_CURRENT, tempc, &g_target, &pwm); 
    result= aoosp_send_setpwmchn( g_addr, chn, pwm.r, pwm.g, pwm.b); CHECKRESULT("setpwmchn");
    delayMicroseconds(50);
  }

  // Go to next address with wrap around [1..last].
  g_addr = (g_addr % aoosp_exec_resetinit_last()) + 1;

  // Show progress
  progress++;
  if( progress%50000==0 ) Serial.printf("Updated %d triplets\n",progress*3);
}

