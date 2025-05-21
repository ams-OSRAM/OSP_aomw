// aomw_caldb.h - calibration data base
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
#ifndef _AOMW_CALDB_H_
#define _AOMW_CALDB_H_


#include <aomw.h>     // aomw_color_mix_t


// The pre-mixing temperature correction has three methods.
typedef enum caldb_tempmethod_e {
  caldb_tempmethod_individual,   // use per LED coefficients
  caldb_tempmethod_generictrain, // use generic (LED type) coefficients from ML training
  caldb_tempmethod_genericmodel, // use generic (LED type) coefficients from model
} caldb_tempmethod_t;
// Set and get the pre-mixing temperature correction method used in caldb_get().
void               caldb_tempmethod_set(caldb_tempmethod_t method);
caldb_tempmethod_t caldb_tempmethod_get();


// Returns the number of triplets in the calibration database.
int caldb_count();
// Returns the calibration data for RGB triplet `tix` when driven with current `cur` (in A) at temperature `tempc` (in C). The data is returned as tristimulus vectors.
void caldb_get(int tix, float cur, float tempc, aomw_color_xyz3_t * triplet);


// A routine to perform some consistency checks on the calibration data.
void caldb_check();
// Computes the average color over the entire calibration database.
void caldb_average(aomw_color_cxcyiv3_t * avg);
// Prints the average color of red, green and blue in the calibration database and the max deviation.
void caldb_printstats();


#endif


