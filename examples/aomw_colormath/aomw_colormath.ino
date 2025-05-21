// aomw_colormath.ino - demonstrates color computations
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
#include <aomw_color.h>


/*
DESCRIPTION
This demo shows how to compute the duty cycles for the R, G, and B LEDs of
a calibrated triplet in order to reach a target color point. A triplet is 
said to be calibrated when the exact color point is known for each of its 
three LEDs; for a given drive current and LED temperature. 
This demo also includes a (post-mixing) temperature correction.
The demo is "mathematical" in that it shows _how_ the computations are done,
printing intermediate results. It uses real calibration figures, but it is 
very unlikely that those match your actual hardware. 
On top of that, this demo does not _control_ triplets, or use actual sensor 
data (temperature), it spoofs inputs and only prints outputs to Serial.

HARDWARE
This is a pure mathematical example. It does not require any specific
OSP related hardware. It can be run on the OSP32 board, then in Arduino,
select board "ESP32S3 Dev Module".

BEHAVIOR
No external behavior, except for serial prints (see below).

OUTPUT
Welcome to aomw_colormath.ino

Using this source triplet to reach target colors
CxCyIv R:0.6970,0.3026,1.1155 G:0.1446,0.7410,2.7169 B:0.1535,0.0237,0.4067
XYZ    R:2.5694,1.1155,0.0015 G:0.5302,2.7169,0.4195 B:2.6341,0.4067,14.1195

TEST   TARGET = REDs XYZ @ 20.0 mA and 25 ºC
TARGET 2.5694,1.1155,0.0015
MIX    1.0000,-0.0000,0.0000
TCORR  1.0073,-0.0000,0.0000
PWM    0x7FFF,0x0000,0x0000 - clipped

TEST   TARGET = GREENs XYZ * 0.95 @ 20.0 mA and 25 ºC
TARGET 0.5037,2.5811,0.3985
MIX    -0.0000,0.9500,-0.0000
TCORR  -0.0000,0.9505,-0.0000
PWM    0x0000,0x79A8,0x0000 - reached

TEST   TARGET = BLUEs XYZ * 0.50 @ 20.0 mA and 25 ºC
TARGET 1.3171,0.2033,7.0598
MIX    0.0000,0.0000,0.5000
TCORR  0.0000,0.0000,0.4999
PWM    0x0000,0x0000,0x3FFC - reached

TEST   TARGET = brightest WHITE @ 20.0 mA and 25 ºC
TARGET 3.6996,3.8924,4.2390
MIX    0.9564,0.9995,0.2704
TCORR  0.9633,1.0000,0.2704
PWM    0x7B4C,0x7FFF,0x229B - reached

TEST   TARGET = medium WHITE (hot) @ 20.0 mA and 100 ºC
TARGET 1.8498,1.9462,2.1195
MIX    0.4782,0.4998,0.1352
TCORR  0.8887,0.6022,0.1605
PWM    0x71BF,0x4D13,0x148C - reached
*/


// === Calibration database ==================================================
// Color points of the triplets instances, and temperature correction of the triplet type


// Taking DMC (Dot Matrix Code) numbers from some side looker batch as an example 
// (only 5 side lookers, only 20 mA drive current). 
// The following array of structs is the resulting calibration database.
static aomw_color_cxcyiv3_t caldb_colors[] = {                                         // pocket DMC 
  { .r={0.6973,0.3024,1.0992}, .g={0.1379,0.7401,2.7084}, .b={0.1538,0.0235,0.4054} }, //    1   0ECLTD0510
  { .r={0.6964,0.3031,1.1157}, .g={0.1362,0.7387,2.6487}, .b={0.1540,0.0232,0.4042} }, //    2   0ECJ5C0403
  { .r={0.6970,0.3026,1.1155}, .g={0.1446,0.7410,2.7169}, .b={0.1535,0.0237,0.4067} }, //    3   0ECJTJ0419
  { .r={0.6971,0.3026,1.1138}, .g={0.1383,0.7392,2.6989}, .b={0.1542,0.0229,0.4054} }, //    4   0ECJJZ0310
  { .r={0.6966,0.3030,1.1121}, .g={0.1377,0.7380,2.7580}, .b={0.1535,0.0237,0.4154} }, //    5   0ECJI60212
};
#define CALDB_COUNT  (sizeof(caldb_colors)/sizeof(caldb_colors[0]) )


// The A(T) functions that fit through the Tables 4 and 5 of AN078_OSIREE3731i_StartupGuide_4.2-5.3.pdf
// We have separate functions for the colors, but not for the drive current, 
// because those overlap. These functions assume a Tcal of 25C.
static inline void caldb_posttempcorr_r( float * dc, float temp) {
  *dc *=  0.000050f*temp*temp + 0.0051f*temp + 0.8485f ; 
}
static inline void caldb_posttempcorr_g( float * dc, float temp) {
  *dc *=  0.000009f*temp*temp + 0.0016f*temp + 0.9549f ; 
}
static inline void caldb_posttempcorr_b( float * dc, float temp) {
  *dc *=  0.000008f*temp*temp + 0.0015f*temp + 0.9573f ; 
}
static void caldb_posttempcorr( aomw_color_mix_t * dc, float temp ) {
  caldb_posttempcorr_r( & dc->r, temp );
  caldb_posttempcorr_g( & dc->g, temp );
  caldb_posttempcorr_b( & dc->b, temp );
}


// === Verbatim computation  =================================================
// Computations printed step by step; see OSP_aomw/extras/aomw_color.drawio.png


// Print out one source-to-target computation 
void compute(const char * msg,  aomw_color_xyz3_t * source, aomw_color_xyz1_t * target, float cur /*A*/, int temp /*degC*/) {
  aomw_color_mix_t mix; // The computed mixing ratios (duty cycle for R, G, and B PWM).
  aomw_color_pwm_t pwm; // The computed PWM settings
  bool             ok;  // Was target color reached

  Serial.printf("TEST   TARGET = %s @ %.1f mA and %d ºC\n",msg, cur*1000, temp);
  Serial.printf("TARGET %s\n", aomw_color_xyz1_to_str(target) );

  aomw_color_computemix( source, target, &mix );
  Serial.printf("MIX    %s\n", aomw_color_mix_to_str(&mix) );

  caldb_posttempcorr( &mix, temp );
  Serial.printf("TCORR  %s\n", aomw_color_mix_to_str(&mix) );
  
  ok= aomw_color_mix_to_pwm( &mix, 0x7FFF, &pwm ); // SAID has 15 bits at 500Hz, 14 at 1000Hz, both could be +1 for dithering
  Serial.printf("PWM    %s - %s\n",aomw_color_pwm_to_str(&pwm), ok?"reached":"clipped" );

  Serial.printf("\n");
}


// For some source triplet, prints multiple source-to-target computations
void example_target_computations(aomw_color_cxcyiv3_t * cxcyiv3) {
  aomw_color_xyz3_t source;  // Triplet with known colors for its R, G and B.
  aomw_color_xyz1_t target;  // Target color point to be reached by mixing.

  // print color point of selected source triplet in "human" color space
  Serial.printf("Using this source triplet to reach target colors\n" );
  Serial.printf("CxCyIv %s\n", aomw_color_cxcyiv3_to_str(cxcyiv3) );
  // Convert selected source triplet to tristimulus (XYZ) color space and print that
  aomw_color_cxcyiv3_to_xyz3( cxcyiv3, &source);
  Serial.printf("XYZ    %s\n", aomw_color_xyz3_to_str(&source) );
  Serial.printf("\n");


  // As first target color, we pick the native color of the RED LED of the source triplet itself
  target= source.r;
  compute("REDs XYZ", &source, &target, 20E-3/*A*/, 25/*degC*/);

  // Next target, green of the source triplet at 95%
  target= {source.g.X*0.95f, source.g.Y*0.95f, source.g.Z*0.95f};
  compute("GREENs XYZ * 0.95", &source, &target, 20E-3/*A*/, 25/*degC*/);

  // Third target, blue of the source triplet at 50%
  target= {source.b.X*0.5f, source.b.Y*0.5f, source.b.Z*0.5f};
  compute("BLUEs XYZ * 0.50", &source, &target, 20E-3/*A*/, 25/*degC*/);

  // Test four is brightest white point
  // https://www.waveformlighting.com/tech/calculate-color-temperature-cct-from-cie-1931-xy-coordinates
  // Iv is here hand-tuned to have the brightest white without clipping
  aomw_color_cxcyiv1_t cxcyiv1= {0.3127, 0.3290, 3.8924};
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv1, &target );
  compute("brightest WHITE", &source, &target, 20E-3/*A*/, 25/*degC*/);

  // Test five is medium white point at high temperature
  cxcyiv1.Iv /= 2.0;
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv1, &target );
  compute("medium WHITE (hot)", &source, &target, 20E-3/*A*/, 100/*degC*/);
}


// === Main program  =========================================================
// Run a computation


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_colormath.ino\n\n");

  // Use triplet 2 for this example
  aomw_color_cxcyiv3_t * cxcyiv3 = &caldb_colors[2];

  // Show several example computations to reach target colors
  example_target_computations( cxcyiv3 );
}


void loop() {
  delay(5000);
}

