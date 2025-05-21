// aomw_color.cpp - color conversion, mixing and temperature corrections
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
#include <Arduino.h>     // constrain()
#include <aomw_color.h>  // own


// This library works with 4 significant digits.
// EPSILON is used as a margin in floating point comparisons.
#define EPSILON 0.00005


// === convert struct to string ==============================================


// "Only use one char* returning print function at a time."
// Print functions (xxx_to_str)in this file that return a const char * all 
// use the same global buffer aomw_color_buf[] for returning the string.
// This means that only one such function can be used at a time.
#define AOMW_COLOR_BUF_SIZE  128 // should fit 3*(2+3*7)
static char aomw_color_buf[AOMW_COLOR_BUF_SIZE];


/*!
    @brief  Helper (debugging) function to convert `aomw_color_cxcyiv1_t` 
            (single Cx,Cy,Iv color point) to a human readable string.
    @param  cxcyiv
            a Cx,Cy,Iv color point.
    @return String of the form "0.0000,0.0000,0.0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_cxcyiv1_to_str( aomw_color_cxcyiv1_t * cxcyiv ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "%.4f,%.4f,%.4f", cxcyiv->Cx, cxcyiv->Cy, cxcyiv->Iv );
  return aomw_color_buf;
}


/*!
    @brief  Helper (debugging) function to convert `aomw_color_cxcyiv3_t` 
            (triple Cx,Cy,Iv color point) to a human readable string.
    @param  cxcyiv3
            a triple Cx,Cy,Iv color point.
    @return String of the form "R:0.0000,0.0000,0.0000 G:0.0000,0.0000,0.0000 B:0.0000,0.0000,0.0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_cxcyiv3_to_str( aomw_color_cxcyiv3_t * cxcyiv3 ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "R:%.4f,%.4f,%.4f G:%.4f,%.4f,%.4f B:%.4f,%.4f,%.4f", 
    cxcyiv3->r.Cx, cxcyiv3->r.Cy, cxcyiv3->r.Iv,
    cxcyiv3->g.Cx, cxcyiv3->g.Cy, cxcyiv3->g.Iv,
    cxcyiv3->b.Cx, cxcyiv3->b.Cy, cxcyiv3->b.Iv
  );
  return aomw_color_buf;
}


/*!
    @brief  Helper (debugging) function to convert `aomw_color_xyz1_t` 
            (single X,Y,Z color point) to a human readable string.
    @brief  Converts a X,Y,Z color point to a string.
    @param  xyz
            a X,Y,Z color point.
    @return String of the form "0.0000,0.0000,0.0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_xyz1_to_str( aomw_color_xyz1_t * xyz ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "%.4f,%.4f,%.4f", xyz->X, xyz->Y, xyz->Z );
  return aomw_color_buf;
}


/*!
    @brief  Helper (debugging) function to convert `aomw_color_xyz3_t` 
            (triple X,Y,Z color point) to a human readable string.
    @param  xyz3
            a triple X,Y,Z color point.
    @return String of the form "R:0.0000,0.0000,0.0000 G:0.0000,0.0000,0.0000 B:0.0000,0.0000,0.0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_xyz3_to_str( aomw_color_xyz3_t * xyz3 ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "R:%.4f,%.4f,%.4f G:%.4f,%.4f,%.4f B:%.4f,%.4f,%.4f", 
    xyz3->r.X, xyz3->r.Y, xyz3->r.Z,
    xyz3->g.X, xyz3->g.Y, xyz3->g.Z,
    xyz3->b.X, xyz3->b.Y, xyz3->b.Z
  );
  return aomw_color_buf;
}


/*!
    @brief  Helper (debugging) function to convert `aomw_color_mix_t` 
            (mixing ratio) to a human readable string.
    @param  mix
            a triple of duty cycles.
    @return String of the form "0.0000,0.0000,0.0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_mix_to_str( aomw_color_mix_t * mix ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "%.4f,%.4f,%.4f", mix->r, mix->g, mix->b );
  return aomw_color_buf;
}


/*!
    @brief  Helper (debugging) function to convert `aomw_color_pwm_t` 
            (PWM setting) to a human readable string.
    @param  pwm
            a triple of PWM settings.
    @return String of the form "0x0000,0x0000,0x0000".
    @note   Only use one char* returning print function at a time.
*/
const char * aomw_color_pwm_to_str( aomw_color_pwm_t * pwm ) {
  snprintf( aomw_color_buf, AOMW_COLOR_BUF_SIZE, "0x%04X,0x%04X,0x%04X", pwm->r, pwm->g, pwm->b );
  return aomw_color_buf;
}


// === color computations ====================================================


// Returns "y" given `x` for the line that runs through the points `x0`,`y0` and `x1`,`y1`.
// This function also extrapolates: `x` does not have to be between `x0` and `x1`.
// It is not allowed that `x0` equals (is very close to) `x1`.
static inline float aomw_color_interpolate0(float x0, float y0, float x1, float y1, float x) {
  return (y1-y0)/(x1-x0) * (x-x0) + y0;
}


/*!
    @brief  Interpolate for a single LED, between color `clo` ("color low",
            emitted at current `ilo`) and color `chi` ("color high" emitted 
            at current ihi) given an actual current i. Outputs the 
            interpolated color via `c`.
    @param  ilo
            low drive current (in A).
    @param  clo
            input parameter holding the Cx,Cy,Iv color point at current `ilo`.
    @param  ihi
            high drive current (in A).
    @param  clo
            input parameter holding the Cx,Cy,Iv color point at current `ihi`.
    @param  i
            actual drive current (does not have to be in the range ilo..ihi).
    @param  c
            output parameter for the Cx,Cy,Iv color point at current `i`.
    @note   It is not allowed that `ilo` equals (is very close to) `ihi`.
    @note   See aomw_color_interpolate3() for triplet version.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_interpolate1( float ilo, /*in*/ aomw_color_cxcyiv1_t * clo, float ihi, /*in*/ aomw_color_cxcyiv1_t * chi, float i, /*out*/ aomw_color_cxcyiv1_t * c) {
  c->Cx= aomw_color_interpolate0( ilo, clo->Cx, ihi, chi->Cx, i);
  c->Cy= aomw_color_interpolate0( ilo, clo->Cy, ihi, chi->Cy, i);
  c->Iv= aomw_color_interpolate0( ilo, clo->Iv, ihi, chi->Iv, i);
}


/*!
    @brief  Interpolate for an RGB triplet, between color `clo` ("color low",
            emitted at current `ilo`) and color `chi` ("color high" emitted 
            at current ihi) given an actual current i. Outputs the 
            interpolated color via `c`.
    @param  ilo
            low drive current (in A).
    @param  clo
            input parameter holding the Cx,Cy,Iv color point at current `ilo`.
    @param  ihi
            high drive current (in A).
    @param  clo
            input parameter holding the Cx,Cy,Iv color point at current `ihi`.
    @param  i
            actual drive current (does not have to be in the range ilo..ihi).
    @param  c
            output parameter holding the Cx,Cy,Iv color points at current `i`.
    @note   It is not allowed that `ilo` equals (is very close to) `ihi`.
    @note   See aomw_color_interpolate1() for single LED version.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_interpolate3( float ilo, /*in*/ aomw_color_cxcyiv3_t * clo, float ihi, /*in*/ aomw_color_cxcyiv3_t * chi, float i, /*out*/ aomw_color_cxcyiv3_t * c) {
  aomw_color_interpolate1(ilo, & clo->r, ihi, & chi->r, i, & c->r );
  aomw_color_interpolate1(ilo, & clo->g, ihi, & chi->g, i, & c->g );
  aomw_color_interpolate1(ilo, & clo->b, ihi, & chi->b, i, & c->b );
}


/*!
    @brief  Applies the temperature corrections described in `poly1` to single
            color point `cxcyiv1` (a single LED of a triplet). The actual 
            temperature is `dtemp` above reference the temperature of `poly1` 
            (in degrees Celsius).
    @param  cxcyiv1
            input/output parameter holding the Cx,Cy,Iv color point.
    @param  poly1
            input parameter describing the color dependency around
            a reference temperature Tref.
    @param  dtemp
            "delta temperature", i.e. Tact-Tref, where Tact is the actual 
            temperature and Tref a reference temperature for which poly1
            was calibrated.
    @note   See aomw_color_poly_apply3() for triplet version.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_poly_apply1( /*in/out*/ aomw_color_cxcyiv1_t * cxcyiv1, aomw_color_poly1_t *poly1, float dtemp) {
  cxcyiv1->Cx *= 1.0f + poly1->Cx.a*dtemp + poly1->Cx.b*dtemp*dtemp;
  cxcyiv1->Cy *= 1.0f + poly1->Cy.a*dtemp + poly1->Cy.b*dtemp*dtemp;
  cxcyiv1->Iv *= 1.0f + poly1->Iv.a*dtemp + poly1->Iv.b*dtemp*dtemp;
}


/*!
    @brief  Applies the temperature corrections described in `poly3` to an 
            entire RGB triplet (3 LEDs). The actual temperature is `dtemp` 
            above reference the temperature of `poly3` (in degrees Celsius).
    @param  cxcyiv3
            input/output parameter holding the Cx,Cy,Iv color points of 3 LEDs.
    @param  poly3
            input parameter describing the color dependency around
            a reference temperature Tref for three LEDs.
    @param  dtemp
            "delta temperature", i.e. Tact-Tref, where Tact is the actual 
            temperature and Tref a reference temperature for which poly3
            was calibrated.
    @note   See aomw_color_poly_apply1() for single LED version.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_poly_apply3( /*in/out*/ aomw_color_cxcyiv3_t * cxcyiv3, aomw_color_poly3_t *poly3, float dtemp) {
  aomw_color_poly_apply1( & cxcyiv3->r, & poly3->r, dtemp );
  aomw_color_poly_apply1( & cxcyiv3->g, & poly3->g, dtemp );
  aomw_color_poly_apply1( & cxcyiv3->b, & poly3->b, dtemp );
}


/*!
    @brief  Converts a single Cx,Cy,Iv (single LED) color point (calibrated) 
            to a tristimulus X,Y,Z color point (for computations).
    @param  cxcyiv1
            input parameter holding the Cx,Cy,Iv color point.
    @param  xyz1
            output parameter returning the tristimulus X,Y,Z color point.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_cxcyiv1_to_xyz1( /*in*/ aomw_color_cxcyiv1_t * cxcyiv1, /*out*/ aomw_color_xyz1_t * xyz1) {
  float f = cxcyiv1->Iv / cxcyiv1->Cy; // Cy!=0 should be part of store check
  xyz1->X = cxcyiv1->Cx * f;
  xyz1->Y = cxcyiv1->Iv;
  xyz1->Z = (1 - cxcyiv1->Cx - cxcyiv1->Cy) * f;
}


/*!
    @brief  Converts a triple Cx,Cy,Iv (RGB triplet of 3 LEDs) color point 
            (calibrated) to a triple tristimulus X,Y,Z color point 
            (for computation).
    @param  cxcyiv3
            input parameter holding the Cx,Cy,Iv color point for R, G and B.
    @param  xyz3
            output parameter returning the tristimulus X,Y,Z color point 
            for R, G and B.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_cxcyiv3_to_xyz3(/*in*/ aomw_color_cxcyiv3_t * cxcyiv3, /*out*/ aomw_color_xyz3_t * xyz3) {
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv3->r, &xyz3->r );
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv3->g, &xyz3->g );
  aomw_color_cxcyiv1_to_xyz1( &cxcyiv3->b, &xyz3->b );
}


// Matrix of 3x3 floats.
// Since it is used to manipulate XYZ vectors, it is convenient to use
// aomw_color_xyz1_t for the columns.
typedef aomw_color_xyz1_t matrix3_t[3];


// Computes the determinant of matrix A, needed in aomw_color_computemix()
static float aomw_color_det(matrix3_t A) {
  float d = A[0].X * A[1].Y * A[2].Z - A[0].X * A[1].Z * A[2].Y
          - A[0].Y * A[1].X * A[2].Z + A[0].Y * A[1].Z * A[2].X
          + A[0].Z * A[1].X * A[2].Y - A[0].Z * A[1].Y * A[2].X ;
  return d;
}


/*!
    @brief  Given a `source` triplet describing the color points for its 
            three LEDs (red, green, and blue), this function computes their 
            mixing ratio `mix` to reach color `target`.
    @param  source
            input parameter holding the X,Y,Z color points for the 
            R, G and B LEDs in the triplet that has to emit the `target` 
            color.
    @param  target
            input parameter for the X,Y,Z color point that `source` 
            must reach.
    @param  mix
            output parameter indicating the mixing ratios (duty cycles)
            for the R, G, and B of `source` to reach `target`.
    @note   Typically the source is calibrated using Cx,Cy,Iv. 
            As a preparation that must first be converted to X,Y,Z using 
            aomw_color_cxcyiv3_to_xyz3().
    @note   The computed mixing values are roughly in the range [0..1], two 
            "cleanup-steps" are still needed: (1) clip the mixing ratios 
            to [0..1], and (2) multiply them with the PWM range of the driver 
            (e.g. 32767 when using 15 bit PWM).
            Both steps are performed by `aomw_color_mix_to_pwm()`.
    @note   To add temperature corrections there are two either-or options.
            (1) pre-mixing-temperature-correction: 
                before calling this functions, modify `source` by computing 
                the drift of the R,G,B color points due to LED temperature 
                not matching calibration temperature.
            (2) post-mixing-temperature-correction: 
                after calling this function apply a correction on the `mix` 
                due to LED temperature not matching calibration temperature.
            Both corrections need calibration parameters that might be 
            triplet instance specific or triplet type generic.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
void aomw_color_computemix( /*in*/ aomw_color_xyz3_t * source, /*in*/ aomw_color_xyz1_t * target, /*out*/ aomw_color_mix_t * mix ) {
  // https://en.wikipedia.org/wiki/CIE_1931_color_space: A useful application of the CIE XYZ color space is
  // that a mixture of two colors in some proportion lies on the straight line between those two colors.

  matrix3_t mA;
  mA[0]= source->r; //     |  XR XG XB  |
  mA[1]= source->g; // A = |  YR YG YB  |
  mA[2]= source->b; //     |  ZR ZG ZB  |
  // We need to find x, such that A * x = T  
  // A, x and T abbreviate variables `mA`, `mix` respectively `target`
  // Math tells us how to find x: x = A^-1 * T

  // Set up the three matrices (each time a column of A is replaced by T)
  matrix3_t mR;
  mR[0]= *target;   //     | (TX) XG XB |
  mR[1]= source->g; // R = | (TY) YG YB |
  mR[2]= source->b; //     | (TZ) ZG ZB |
  matrix3_t mG;
  mG[0]= source->r; //     | XR (TX) XB |
  mG[1]= *target;   // G = | YR (TY) YB |
  mG[2]= source->b; //     | ZR (TZ) ZB |
  matrix3_t mB;
  mB[0]= source->r; //     | XR XG (TX) |
  mB[1]= source->g; // B = | YR YG (TY) |
  mB[2]= *target;   //     | ZR ZG (TZ) |

  // Compute determinants
  float detA = aomw_color_det(mA);
  float detR = aomw_color_det(mR);
  float detG = aomw_color_det(mG);
  float detB = aomw_color_det(mB);

  // Compute RGB mix values
  mix->r = detR/detA; // detA!=0 because r, g, and b form a triangle
  mix->g = detG/detA;
  mix->b = detB/detA;
}


/*!
    @brief  The `mix` computed by `aomw_color_computemix()` needs two 
            cleanups: clip the mixing ratios to [0..1], and (2) multiply 
            them with the PWM range. This function performs both steps.
    @param  mix
            input parameter indicating the mixing ratios (duty cycles)
            for the R, G, and B, roughly in the range [0..1].
    @param  pwmmax
            input parameter indicating the range of the PWM driver.
            A 15-bit PWM driver has a `pwmmax` of 2^15-1 = 32767.
    @param  pwm
            output parameter, with value `mix*pwmmax` clipped to [0..pwmmax].
    @return true   if no clipping was needed and the target color was reached.
            false  if color was out of reach of the source triplet (clipped).
    @note   In order to reduce the number of clip warnings (returns with 
            false), the no-clip region [0..1] has a band of EPSILON
            [0-EPSILON .. 1+EPSILON]. EPSILON is 0.00005.
    @note   The reason that this function is not integrated with 
            `aomw_color_computemix()` is that the caller might want to do a
            post-mixing-temperature-correction in between.
    @note   See extras/aomw_color.drawio.png for color processing overview.
*/
bool aomw_color_mix_to_pwm( /*in*/ aomw_color_mix_t * mix, int pwmmax, /*out*/ aomw_color_pwm_t * pwm ) {
  // Scaling and clipping of the mixing ratios
  pwm->r = constrain( pwmmax * mix->r, 0, pwmmax );
  pwm->g = constrain( pwmmax * mix->g, 0, pwmmax );
  pwm->b = constrain( pwmmax * mix->b, 0, pwmmax );

  // Was there clipping? Take an extra margin of EPSILON before signaling a problem.
  bool ok= 0-EPSILON < mix->r  &&  mix->r < 1+EPSILON  
       &&  0-EPSILON < mix->g  &&  mix->g < 1+EPSILON  
       &&  0-EPSILON < mix->b  &&  mix->b < 1+EPSILON;

  // Return reached (true) or clipped (false)
  return ok; 
}

