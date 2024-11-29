// aomw_tscript.h - driver for an I2C EEPROM connected to a SAID
/*****************************************************************************
 * Copyright 2024 by ams OSRAM AG                                            *
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
#ifndef _AOMW_TSCRIPT_H_
#define _AOMW_TSCRIPT_H_


#include <stdint.h>    // uint16_t
#include <aoresult.h>  // aoresult_t
#include <aomw_topo.h> // aomw_topo_rgb_t


// Installs a new script (and sets cursor at first instruction)
// `numtriplets` is needed to scale the region indices.
void aomw_tscript_install(const uint16_t *insts, uint16_t numtriplets); 
// Plays the instruction under the internal cursor; moves cursor by one. 
// If next instruction has "with-previous" also executes it, and so on. 
// If there is the end marker, wraps around.
// Assumes topo has been built, and uses topo_settriplet for the regions.
aoresult_t aomw_tscript_playframe(); 


// For do-it-yourself, there is an iterator over instructions
typedef struct aomw_tscript_inst_s {
  int             cursor;   // index to instruction in installed script
  uint16_t        code;     // raw code at cursor
  // following fields are decoded from `code`
  bool            atend;    // instruction under cursor is end-of-script
  bool            withprev; // this instruction should be combined with previous (else starts a new frame)
  uint16_t        tix0;     // index of triplet at start-of-region (inclusive)
  uint16_t        tix1;     // index of triplet at end-of-region (exclusive)
  aomw_topo_rgb_t rgb;      // color for the region ("topo brightness range" 0..0x7FFF)
} aomw_tscript_inst_t;


// The cursor is at the instruction that marks the end ("end-of-script")
bool aomw_tscript_atend();     
// Move internal cursor to first instruction
void aomw_tscript_gotofirst(); 
// Move internal cursor to next instruction (except when atend, then cursor does not move)
void aomw_tscript_gotonext();  
// Gets instruction details (all fields decoded)
const aomw_tscript_inst_t * aomw_tscript_get();
// Plays instruction under the cursor; precondition: !atend(); does not gotonext()       
aoresult_t aomw_tscript_playinst();  


// Stock animation scripts
const uint16_t * aomw_tscript_rainbow();
int              aomw_tscript_rainbow_bytes();
const uint16_t * aomw_tscript_bouncingblock();
int              aomw_tscript_bouncingblock_bytes();
const uint16_t * aomw_tscript_colormix();
int              aomw_tscript_colormix_bytes();
const uint16_t * aomw_tscript_heartbeat();
int              aomw_tscript_heartbeat_bytes();


#endif
