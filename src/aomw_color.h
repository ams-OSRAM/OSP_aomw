// aomw_color.h - color conversion, mixing and temperature corrections
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
#ifndef _AOMW_COLOR_H_
#define _AOMW_COLOR_H_


#include <stdint.h>


// === Calibration DB ========================================================
// Format for storing _color_ calibration values


/*!
    @typedef aomw_color_cxcyiv1_t
    @brief CIE x,y coordinates and luminous intensity Cx,Cy,Iv ("color point") 
           for one single LED (of an RGB triplet).
    @note  The Cx,Cy,Iv color space matches the human eye, and is used to 
           calibrate LED colors; it is less suited for color computations. 
           Typically, this struct is used in the "calibration database"
           to record the color points of all LEDs in an OSP chain.
    @note  Cx,Cy,Iv is what is depicted in the CIE 1931 xyY chromaticity 
           diagram at https://en.wikipedia.org/wiki/CIE_1931_color_space
           We prefer Cx,Cy,Iv over x,y,Y to distinguish it from X,Y,Z as
           used in `aomw_color_xyz_t`.
    @note  This is uncompressed format, typically a real application would 
           compress the data in the "calibration database".
    @note  The DMC (dot matrix code) data for ams OSRAM side lookers is in
           the Cx,Cy,Iv format; the data for RGBI's comes in a different 
           format, namely u',v',Iv.
*/
typedef struct aomw_color_cxcyiv1_s {
  float Cx;
  float Cy;
  float Iv;
} aomw_color_cxcyiv1_t;


// Converts a single Cx,Cy,Iv color point to a string of the form "0.0000,0.0000,0.0000"
const char * aomw_color_cxcyiv1_to_str( aomw_color_cxcyiv1_t * cxcyiv );


/*!
    @typedef aomw_color_cxcyiv3_t
    @brief CIE Cx,Cy,Iv color points for an entire RGB triplet (3 LEDs).
    @note  See `aomw_color_cxcyiv1_t` for notes on this data type.
*/
typedef struct aomw_color_cxcyiv3_s {
  aomw_color_cxcyiv1_t r;
  aomw_color_cxcyiv1_t g;
  aomw_color_cxcyiv1_t b;
} aomw_color_cxcyiv3_t;


// Converts a triple Cx,Cy,Iv color point to a string of the form "R:0.0000,0.0000,0.0000 G:0.0000,0.0000,0.0000 B:0.0000,0.0000,0.0000"
const char * aomw_color_cxcyiv3_to_str( aomw_color_cxcyiv3_t * cxcyiv3 );


// === Temperature correction ================================================
// Format for storing _temperature correction_ calibration values;
// polynomials for temperature behavior of a (Cx Cy Iv) color point


/*!
    @typedef aomw_color_poly_t
    @brief The coefficients of one second degree polynomial (1+ax+bx²).
    @note  These polynomials are used to correct a aomw_color_cxcyiv1_t color
           point when temperature changes. They describe the temperature
           dependencies around a reference (calibration) temperature Tref:
             Cx_Tact = Cx_Tref * ( 1 + a*(Tact-Tref) + b*(Tact-Tref)^2 ) 
    @note  This is uncompressed format, typically a real application would 
           compress the data in the "calibration database".
    @note  Observe that the coefficient of x^0 is not stored; it is assumed
           to be 1, which makes sense for the temperature correction 
           application.
*/
typedef struct aomw_color_poly_s  { 
  float a;
  float b; 
} aomw_color_poly_t;


/*!
    @typedef aomw_color_poly1_t
    @brief The (Cx, Cy, Iv) temperature dependency polynomials for one 
           single LED (of an RGB triplet).
    @note  See `aomw_color_poly_t` for notes on this data type.
*/
typedef struct aomw_color_poly1_s { 
  aomw_color_poly_t Cx; 
  aomw_color_poly_t Cy; 
  aomw_color_poly_t Iv; 
} aomw_color_poly1_t;


/*!
    @typedef aomw_color_poly3_t
    @brief The (Cx, Cy, Iv) temperature dependency polynomials for an 
           entire RGB triplet (3 LEDs).
    @note  See `aomw_color_poly_t` for notes on this data type.
*/
typedef struct aomw_color_poly3_s { 
  aomw_color_poly1_t r; 
  aomw_color_poly1_t g; 
  aomw_color_poly1_t b; 
} aomw_color_poly3_t;



// === Computations =========================================================
// Format for mixing colors


/*!
    @typedef aomw_color_xyz_t
    @brief Tristimulus X, Y, Z color point for one single LED.
    @note  See the CIE XYZ color space at
           https://en.wikipedia.org/wiki/CIE_1931_color_space
    @note  The X,Y,Z color space is used by the color computations in this
           library. When mixing two colors in this space, the resulting color 
           is on a straight line between the two source colors.
*/
typedef struct aomw_color_xyz1_s {
  float X;
  float Y;
  float Z;
} aomw_color_xyz1_t;


// Converts a single X,Y,Z color point to a string of the form "0.0000,0.0000,0.0000"
const char * aomw_color_xyz1_to_str( aomw_color_xyz1_t * xyz );


/*!
    @typedef aomw_color_xyz3_t
    @brief Tristimulus X, Y, Z color points for an entire RGB triplet (3 LEDs).
    @note  See `aomw_color_xyz_t` for notes on this data type.
*/
typedef struct aomw_color_xyz3_s {
  aomw_color_xyz1_t r;
  aomw_color_xyz1_t g;
  aomw_color_xyz1_t b;
} aomw_color_xyz3_t;


// Converts a triple X,Y,Z color point to a string of the form "R:0.0000,0.0000,0.0000 G:0.0000,0.0000,0.0000 B:0.0000,0.0000,0.0000"
const char * aomw_color_xyz3_to_str( aomw_color_xyz3_t * xyz3 );


// === Result ===============================================================
// Format for computed color configuration (duty cycles)


/*!
    @typedef aomw_color_mix_t
    @brief The mixing ratio for a triplet (to achieve a target color).
    @note  The mixing ratios are roughly in the range [0,1].
           They are typically used as duty cycles for the emitting triplet. 
           If they are outside the range [0,1] the color is out of reach;
           in this case the function `aomw_color_mix_to_pwm()` clips.
*/
typedef struct aomw_color_mix_s {
  float r;
  float g;
  float b;
} aomw_color_mix_t; 


// Converts a mixing ratio to a string of the form "0.0000,0.0000,0.0000"
const char * aomw_color_mix_to_str( aomw_color_mix_t * mix );


/*!
    @typedef aomw_color_pwm_t
    @brief The PWM settings for a triplet, e.g. to be used in SETPWM telegrams.
*/
typedef struct aomw_color_pwm_s {
  uint16_t r;
  uint16_t g;
  uint16_t b;
} aomw_color_pwm_t; 


// Converts a PWM setting to a string of the form "0x0000,0x0000,0x0000"
const char * aomw_color_pwm_to_str( aomw_color_pwm_t * pwm );


// ==========================================================================
// Color transformation functions
// See OSP_aomw/readme.md and OSP_aomw/extras/aomw_color.drawio.png


// Interpolate for a single LED, between color `clo` ("color lo" emitted at current `ilo`) and color `chi` ("color hi" emitted at current ihi) given an actual current i. Outputs the interpolated color via `c`.
void aomw_color_interpolate1( float ilo, /*in*/ aomw_color_cxcyiv1_t * clo, float ihi, /*in*/ aomw_color_cxcyiv1_t * chi, float i, /*out*/ aomw_color_cxcyiv1_t * c);


// Interpolate for an RGB triplet, between color `clo` ("color lo" emitted at current `ilo`) and color `chi` ("color hi" emitted at current ihi) given an actual current i. Outputs the interpolated color via `c`.
void aomw_color_interpolate3( float ilo, /*in*/ aomw_color_cxcyiv3_t * clo, float ihi, /*in*/ aomw_color_cxcyiv3_t * chi, float i, /*out*/ aomw_color_cxcyiv3_t * c);


// Applies the temperature corrections described in `poly1` to color `cxcyiv1`. Actual temperature is dtemp above reference temperature of poly1 (deg C).
void aomw_color_poly_apply1( /*in/out*/ aomw_color_cxcyiv1_t * cxcyiv1, aomw_color_poly1_t *poly1, float dtemp);


// Applies the temperature corrections described in `poly3` to color `cxcyiv3`. Actual temperature is dtemp above reference temperature of poly1 (deg C).
void aomw_color_poly_apply3( /*in/out*/ aomw_color_cxcyiv3_t * cxcyiv3, aomw_color_poly3_t *poly3, float dtemp);


// Converts a single Cx,Cy,Iv (calibrated) color point to a tristimulus X,Y,Z color point for computations.
void aomw_color_cxcyiv1_to_xyz1( /*in*/ aomw_color_cxcyiv1_t * cxcyiv1, /*out*/ aomw_color_xyz1_t * xyz1);


// Converts a triple Cx,Cy,Iv (calibrated color point to a triple tristimulus X,Y,Z color point for computations.
void aomw_color_cxcyiv3_to_xyz3(/*in*/ aomw_color_cxcyiv3_t * cxcyiv3, /*out*/ aomw_color_xyz3_t * xyz3);


// Given a `source` triplet with its X,Y,Z color points, computes their mixing ratio `mix` to reach color `target`.
void aomw_color_computemix( aomw_color_xyz3_t * source, aomw_color_xyz1_t * target, aomw_color_mix_t * mix );


// Maps a mixing ratio to a PWM setting [0..pwmmax]. Returns false if clipping was applied.
bool aomw_color_mix_to_pwm( /*in*/ aomw_color_mix_t * mix, int pwmmax, /*out*/ aomw_color_pwm_t * pwm );


#endif


