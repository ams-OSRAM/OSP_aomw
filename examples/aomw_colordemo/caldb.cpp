// aomw_caldb.cpp - calibration data base
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
#include <aoresult.h> // AORESULT_ASSERT()
#include <aomw.h>     // aomw_color_cxcyiv3_t
#include "caldb.h"    // own header


// The DMC color calibration data has 4 significant digits.
// EPSILON is used as a margin in floating point comparisons.
#define EPSILON          0.00005 


// This calibration database is for a reel of 250 RGBs.
// In this reel, each LED is calibrated for two currents: 20 and 50 mA.
#define CALDB_CURRENTS_COUNT       2
#define CALDB_CURRENTS_LO    (20E-3)
#define CALDB_CURRENTS_HI    (50E-3)
static float caldb_currents[CALDB_CURRENTS_COUNT]={ CALDB_CURRENTS_LO, CALDB_CURRENTS_HI }; 


// Reference temperature for the calibration data.
#define CALDB_TREF               25 


// The _color_ calibration data of all triplets.
typedef struct caldb_colors_s {
  aomw_color_cxcyiv3_t cur[CALDB_CURRENTS_COUNT];
} caldb_colors_t;
static caldb_colors_t caldb_colors[] = {
  #define TRIPLET(index,calweek,pocket,DMC,CX_BLU_20,CX_BLU_50,CX_GRN_20,CX_GRN_50,CX_RED_20,CX_RED_50,CY_BLU_20,CY_BLU_50,CY_GRN_20,CY_GRN_50,CY_RED_20,CY_RED_50,IV_BLU_20,IV_BLU_50,IV_GRN_20,IV_GRN_50,IV_RED_20,IV_RED_50,UF_BLU_20,UF_BLU_50,UF_GRN_20,UF_GRN_50,UF_RED_20,UF_RED_50,LDOM_RED_20,LDOM_GRN_20,LDOM_BLU_20,LDOM_RED_50,LDOM_GRN_50,LDOM_BLU_50,CX_A_RED_20,CX_B_RED_20,CY_A_RED_20,CY_B_RED_20,IV_A_RED_20,IV_B_RED_20,CX_A_GRN_20,CX_B_GRN_20,CY_A_GRN_20,CY_B_GRN_20,IV_A_GRN_20,IV_B_GRN_20,CX_A_BLU_20,CX_B_BLU_20,CY_A_BLU_20,CY_B_BLU_20,IV_A_BLU_20,IV_B_BLU_20) \
    { .cur={  \
      {.r={CX_RED_20,CY_RED_20,IV_RED_20},.g={CX_GRN_20,CY_GRN_20,IV_GRN_20},.b={CX_BLU_20,CY_BLU_20,IV_BLU_20}},  \
      {.r={CX_RED_50,CY_RED_50,IV_RED_50},.g={CX_GRN_50,CY_GRN_50,IV_GRN_50},.b={CX_BLU_50,CY_BLU_50,IV_BLU_50}},  \
    }},
  #include "caldb.hpp" // link-in 18 floats per triplet
  #undef TRIPLET
};
#define CALDB_COLORS_COUNT  (sizeof(caldb_colors)/sizeof(caldb_colors[0]))


// The _temperature correction_ calibration data of all triplets.
static aomw_color_poly3_t caldb_polys[] = {
// Individual compensation parameters for every device calculated using the model parameters.
  #define TRIPLET(index,calweek,pocket,DMC,CX_BLU_20,CX_BLU_50,CX_GRN_20,CX_GRN_50,CX_RED_20,CX_RED_50,CY_BLU_20,CY_BLU_50,CY_GRN_20,CY_GRN_50,CY_RED_20,CY_RED_50,IV_BLU_20,IV_BLU_50,IV_GRN_20,IV_GRN_50,IV_RED_20,IV_RED_50,UF_BLU_20,UF_BLU_50,UF_GRN_20,UF_GRN_50,UF_RED_20,UF_RED_50,LDOM_RED_20,LDOM_GRN_20,LDOM_BLU_20,LDOM_RED_50,LDOM_GRN_50,LDOM_BLU_50,CX_A_RED_20,CX_B_RED_20,CY_A_RED_20,CY_B_RED_20,IV_A_RED_20,IV_B_RED_20,CX_A_GRN_20,CX_B_GRN_20,CY_A_GRN_20,CY_B_GRN_20,IV_A_GRN_20,IV_B_GRN_20,CX_A_BLU_20,CX_B_BLU_20,CY_A_BLU_20,CY_B_BLU_20,IV_A_BLU_20,IV_B_BLU_20) \
    { \
      .r={.Cx={.a=CX_A_RED_20,.b=CX_B_RED_20},.Cy={.a=CY_A_RED_20,.b=CY_B_RED_20},.Iv={.a=IV_A_RED_20,.b=IV_B_RED_20}}, \
      .g={.Cx={.a=CX_A_GRN_20,.b=CX_B_GRN_20},.Cy={.a=CY_A_GRN_20,.b=CY_B_GRN_20},.Iv={.a=IV_A_GRN_20,.b=IV_B_GRN_20}}, \
      .b={.Cx={.a=CX_A_BLU_20,.b=CX_B_BLU_20},.Cy={.a=CY_A_BLU_20,.b=CY_B_BLU_20},.Iv={.a=IV_A_BLU_20,.b=IV_B_BLU_20}}  \
    },
  #include "caldb.hpp" // link-in 18 floats per triplet
  #undef TRIPLET
};
#define CALDB_POLYS_COUNT  (sizeof(caldb_polys)/sizeof(caldb_polys[0]))


// Generic compensation parameters obtained from training data.
static aomw_color_poly3_t caldb_poly_generictrain = {
  .r={.Cx={.a= 9.895365e-05,.b= 7.549378e-07},.Cy={.a=-3.039617e-04,.b= 1.274861e-06},.Iv={.a=-6.507311e-03,.b= 6.882696e-06}},
  .g={.Cx={.a= 2.022753e-03,.b= 2.466239e-06},.Cy={.a=-2.389899e-04,.b=-7.784895e-07},.Iv={.a=-1.323083e-03,.b=-1.756017e-06}},
  .b={.Cx={.a=-3.082683e-04,.b=-4.286760e-07},.Cy={.a= 2.459377e-03,.b= 6.967915e-06},.Iv={.a= 1.199001e-03,.b=-3.254902e-06}}
};


// Generic compensation parameters for the provided samples calculated using the model parameters.
static aomw_color_poly3_t caldb_poly_genericmodel = {
  .r={.Cx={.a= 1.317620e-04,.b=-8.334195e-07},.Cy={.a=-3.513584e-04,.b= 1.080907e-06},.Iv={.a=-6.423312e-03,.b= 5.239551e-06}},
  .g={.Cx={.a= 2.185336e-03,.b= 2.644325e-06},.Cy={.a=-2.013109e-04,.b=-8.194112e-07},.Iv={.a=-1.240255e-03,.b=-2.126623e-06}},
  .b={.Cx={.a=-3.076502e-04,.b=-3.747002e-07},.Cy={.a= 2.436524e-03,.b= 6.648076e-06},.Iv={.a= 1.193414e-03,.b=-3.707391e-06}}
};



// The pre-mixing temperature correction has three methods. Store which is active
caldb_tempmethod_t caldb_tempmethod= caldb_tempmethod_individual;


// Sets the pre-mixing temperature correction method used in caldb_get().
void caldb_tempmethod_set(caldb_tempmethod_t method) {
  caldb_tempmethod = method;
}


// Gets the pre-mixing temperature correction method used in caldb_get().
caldb_tempmethod_t caldb_tempmethod_get() {
  return caldb_tempmethod;
}
 

// Returns the number of triplets with calibration data in the caldb.
int caldb_count() {
  return CALDB_COLORS_COUNT;
}


/*!
    @brief Returns the calibration data for RGB triplet `tix` when driven with
           current `cur` at temperature `tempc`. The data is returned as 
           tristimulus vectors.
    @param tix
           triplet index 0 <= tix < db_count().
    @param cur
           drive current in A.
    @param tempc
           temperature in degrees C.
    @param triplet
           caller allocated output parameter for the tristimulus.
    @note  This function includes pre-mixing temperature correction for each 
           individual LED.
    @note  This function returns the tristimulus vector, as needed by the
           color transformation functions. It is up to the implementation
           of the calibration data store to decide in what format to store 
           the calibration data (here Cx/Cy/Iv), if/how to compress
           it (here stored uncompressed), and how to handle multiple drive 
           currents (here:interpolate).
*/
void caldb_get(int tix, float cur, float tempc, aomw_color_xyz3_t * triplet) {
  // Get the color points for triple tx at its two driver currents
  #if CALDB_CURRENTS_COUNT!=2
    #error This function is hardwired for two drive currents
  #endif
  // Get the triplet color data for low drive current and high drive current.
  aomw_color_cxcyiv3_t * clo= & caldb_colors[tix].cur[0];
  aomw_color_cxcyiv3_t * chi= & caldb_colors[tix].cur[1];

  // Step 1: for requested drive current cur, interpolate the color points.
  aomw_color_cxcyiv3_t source;
  aomw_color_interpolate3(CALDB_CURRENTS_LO, clo, CALDB_CURRENTS_HI, chi, cur, &source );

  // Select temperature compensation polynomial (for evaluation; normal app hard wires one).
  aomw_color_poly3_t * poly;
  if( caldb_tempmethod==caldb_tempmethod_genericmodel ) {
    poly= & caldb_poly_genericmodel;
  } else if( caldb_tempmethod==caldb_tempmethod_generictrain ) {
    poly= & caldb_poly_generictrain;
  } else {
    poly= & caldb_polys[tix];
  }

  // Step 2: apply the (pre-mixing) temperature correction.
  aomw_color_poly_apply3( & source, poly, tempc - CALDB_TREF );

  // Step 3: translate from CxCyIv to XYZ.
  aomw_color_cxcyiv3_to_xyz3( & source, triplet );
}


// ==========================================================================
// The functions below are for quality assurance (checking DB consistency)
// ==========================================================================


// A routine to perform some consistency checks on the instance data
void caldb_check() {
  AORESULT_ASSERT(CALDB_COLORS_COUNT==CALDB_POLYS_COUNT); // something is wrong in tables
  AORESULT_ASSERT(CALDB_CURRENTS_COUNT==2);               // for interpolate

  for( int tix=0; tix<CALDB_COLORS_COUNT; tix++ ) {
    float cx_req; 
    float cy_req;

    // Check IV increasing over current
    for( int cix=1; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      float factor = 0.5 * caldb_currents[cix] / caldb_currents[cix-1];
      AORESULT_ASSERT( caldb_colors[tix].cur[cix-1].r.Iv * factor < caldb_colors[tix].cur[cix].r.Iv );
      AORESULT_ASSERT( caldb_colors[tix].cur[cix-1].g.Iv * factor < caldb_colors[tix].cur[cix].g.Iv );
      AORESULT_ASSERT( caldb_colors[tix].cur[cix-1].b.Iv * factor < caldb_colors[tix].cur[cix].b.Iv );
    }

    // check CX/CY is indeed REDish (and within range)
    cx_req= 0.6973;
    cy_req= 0.3024;
    #define REDDISTMAX 0.00005
    for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      float cx= caldb_colors[tix].cur[cix].r.Cx;
      float cy= caldb_colors[tix].cur[cix].r.Cy;
      float red_dist= (cx - cx_req)*(cx - cx_req) + (cy - cy_req)*(cy - cy_req);
      AORESULT_ASSERT( red_dist<REDDISTMAX);
      AORESULT_ASSERT( 0+EPSILON<cx && cx<1-EPSILON);
      AORESULT_ASSERT( 0+EPSILON<cy && cy<1-EPSILON);
    }

    // check CX/CY is indeed GREENish (and within range)
    cx_req= 0.1332;
    cy_req= 0.7310;
    #define GREENDISTMAX 0.0005 // greater fluctuations than red&blue
    for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      float cx= caldb_colors[tix].cur[cix].g.Cx;
      float cy= caldb_colors[tix].cur[cix].g.Cy;
      float green_dist= (cx - cx_req)*(cx - cx_req) + (cy - cy_req)*(cy - cy_req);
      AORESULT_ASSERT( green_dist<GREENDISTMAX);
      AORESULT_ASSERT( 0+EPSILON<cx && cx<1-EPSILON);
      AORESULT_ASSERT( 0+EPSILON<cy && cy<1-EPSILON);
    }

    // check CX/CY is indeed BLUEish (and within range)
    cx_req= 0.1543;
    cy_req= 0.0229;
    #define BLUEDISTMAX 0.00005
    for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      float cx= caldb_colors[tix].cur[cix].b.Cx;
      float cy= caldb_colors[tix].cur[cix].b.Cy;
      float blue_dist= (cx - cx_req)*(cx - cx_req) + (cy - cy_req)*(cy - cy_req);
      AORESULT_ASSERT( blue_dist<BLUEDISTMAX);
      AORESULT_ASSERT( 0+EPSILON<cx && cx<1-EPSILON);
      AORESULT_ASSERT( 0+EPSILON<cy && cy<1-EPSILON);
    }

  }
}


// Computes the average Cx, Cy, IV over the calibration database
void caldb_average(aomw_color_cxcyiv3_t * avg) {
  *avg = { {0,0,0}, {0,0,0}, {0,0,0} };

  for( int tix=0; tix<CALDB_COLORS_COUNT; tix++ ) {
    for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      avg->r.Cx += caldb_colors[tix].cur[cix].r.Cx;
      avg->r.Cy += caldb_colors[tix].cur[cix].r.Cy;
      avg->r.Iv += caldb_colors[tix].cur[cix].r.Iv;

      avg->g.Cx += caldb_colors[tix].cur[cix].g.Cx;
      avg->g.Cy += caldb_colors[tix].cur[cix].g.Cy;
      avg->g.Iv += caldb_colors[tix].cur[cix].g.Iv;

      avg->b.Cx += caldb_colors[tix].cur[cix].b.Cx;
      avg->b.Cy += caldb_colors[tix].cur[cix].b.Cy;
      avg->b.Iv += caldb_colors[tix].cur[cix].b.Iv;
    }
  }
  
  avg->r.Cx /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT); 
  avg->r.Cy /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
  avg->r.Iv /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
  
  avg->g.Cx /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT); 
  avg->g.Cy /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
  avg->g.Iv /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
  
  avg->b.Cx /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT); 
  avg->b.Cy /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
  avg->b.Iv /= (CALDB_COLORS_COUNT*CALDB_CURRENTS_COUNT);
}


// Prints the average color of red, green and blue in the calibration database and the max deviation
void caldb_printstats() {
  // Compute average
  aomw_color_cxcyiv3_t avg;
  caldb_average(&avg);
  
  // Compute biggest outliers
  float red_dist_max= 0; 
  float grn_dist_max= 0; 
  float blu_dist_max= 0;

  for( int tix=0; tix<CALDB_COLORS_COUNT; tix++ ) {
    for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) {
      float red_cx = caldb_colors[tix].cur[cix].r.Cx;
      float red_cy = caldb_colors[tix].cur[cix].r.Cy;
      float red_dist= (red_cx - avg.r.Cx)*(red_cx - avg.r.Cx) + (red_cy - avg.r.Cy)*(red_cy - avg.r.Cy);
      if( red_dist > red_dist_max ) red_dist_max= red_dist;

      float grn_cx = caldb_colors[tix].cur[cix].g.Cx;
      float grn_cy = caldb_colors[tix].cur[cix].g.Cy;
      float grn_dist= (grn_cx - avg.g.Cx)*(grn_cx - avg.g.Cx) + (grn_cy - avg.g.Cy)*(grn_cy - avg.g.Cy);
      if( grn_dist > grn_dist_max ) grn_dist_max= grn_dist;

      float blu_cx = caldb_colors[tix].cur[cix].b.Cx;
      float blu_cy = caldb_colors[tix].cur[cix].b.Cy;
      float blu_dist= (blu_cx - avg.b.Cx)*(blu_cx - avg.b.Cx) + (blu_cy - avg.b.Cy)*(blu_cy - avg.b.Cy);
      if( blu_dist > blu_dist_max ) blu_dist_max= blu_dist;
    }
  }

  // Print stats
  Serial.printf("CALDB instances %d currents %d:", CALDB_COLORS_COUNT, CALDB_CURRENTS_COUNT);
  for( int cix=0; cix<CALDB_CURRENTS_COUNT; cix++ ) Serial.printf(" %.3f",caldb_currents[cix]);
  Serial.printf("\n");

  Serial.printf("average R %s (max dist2 %0.6f)\n", aomw_color_cxcyiv1_to_str(&avg.r), red_dist_max);
  Serial.printf("average G %s (max dist2 %0.6f)\n", aomw_color_cxcyiv1_to_str(&avg.g), grn_dist_max);
  Serial.printf("average B %s (max dist2 %0.6f)\n", aomw_color_cxcyiv1_to_str(&avg.b), blu_dist_max);

  Serial.printf("\n");
}

