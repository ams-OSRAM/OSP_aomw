// aomw_topo.h - compute a topological map of all nodes in the OSP chain
/*****************************************************************************
 * Copyright 2024,2025 by ams OSRAM AG                                       *
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
#ifndef _AOMW_TOPO_H_
#define _AOMW_TOPO_H_


#include <stdint.h>     // uint16_t
#include <aoresult.h>   // aoresult_t


// Returns if the current OSP chain has direction Loop (or BiDir).
int aomw_topo_loop();
// Returns the number of nodes in the scanned chain.
uint16_t aomw_topo_numnodes();
// Returns the identity of OSP node `addr`; 1<=addr<=aomw_topo_numnodes().
uint32_t aomw_topo_node_id( uint16_t addr );
// Returns the number of triplets (RGB modules) of OSP node `addr`; 1<=addr<= aomw_topo_numnodes().
uint8_t aomw_topo_node_numtriplets( uint16_t addr );
// Returns the index of the first triplet (RGB module) driven by OSP node `addr`; 1<=addr<=aomw_topo_numnodes().
uint16_t aomw_topo_node_triplet1( uint16_t addr );
// Returns the number of triplets (RGB modules) in the scanned chain.
uint16_t aomw_topo_numtriplets();
// Returns the address of the OSP node that drives triplet `tix`; 0<=tix<aomw_topo_numtriplets().
uint16_t aomw_topo_triplet_addr( uint16_t tix );
// Returns 1 if triplet `tix` is driven by an OSP node with channels; 0<=tix<aomw_topo_numtriplets().
int aomw_topo_triplet_onchan( uint16_t tix );
// Returns the channel triplet `tix` is attached to in case the triplet is driven by an OSP node with channels, 0<=tix<aomw_topo_numtriplets(). Only defined when aomw_topo_triplet_onchan(tix).
uint8_t aomw_topo_triplet_chan( uint16_t tix );
// Returns the number of I2C bridges in the scanned chain.
uint16_t aomw_topo_numi2cbridges();
// Returns the address of the OSP node that has I2C bridge `bix`; 0<=bix<aomw_topo_numi2cbridges().
uint16_t aomw_topo_i2cbridge_addr( uint16_t bix );


// Prints on Serial a summary of the "topology map".
void aomw_topo_dump_summary();
// Prints on Serial a list of nodes from the "topology map".
void aomw_topo_dump_nodes();
// Prints on Serial a list of triplets from the "topology map".
void aomw_topo_dump_triplets();
// Prints on Serial a list of I2C bridges from the "topology map".
void aomw_topo_dump_i2cbridges();
// Prints on Serial the max power consumption of the nodes in the "topology map".
void aomw_topo_dump_power();


// topo build in one run
aoresult_t aomw_topo_build();
// This function is part of the topology builder. Call this once, then follow up with aomw_topo_build_step().
void aomw_topo_build_start();
// This function is part of the topology builder. Call this until aomw_topo_build_done(), but after aomw_topo_build_start().
aoresult_t aomw_topo_build_step();
// This function is part of the topology builder. Call this after aomw_topo_build_step(), to determine if another step() is needed.
int aomw_topo_build_done();


// The topo module uses colors of type aomw_topo_rgb_t, their value should 
// be 0..AOMW_TOPO_BRIGHTNESS_MAX. This is the "topo brightness range"; 
// the actual pwm setting depends on the physical device and their current 
// settings (both abstracted away by this topo module).
#define AOMW_TOPO_BRIGHTNESS_MAX 0x7FFF
// The data type
typedef struct aomw_topo_rgb_s { uint16_t r; uint16_t g; uint16_t b; const char * name; } aomw_topo_rgb_t; // Each 0..AOMW_TOPO_BRIGHTNESS_MAX
// Some predefined color constants
extern const aomw_topo_rgb_t aomw_topo_red;
extern const aomw_topo_rgb_t aomw_topo_yellow;
extern const aomw_topo_rgb_t aomw_topo_green;
extern const aomw_topo_rgb_t aomw_topo_cyan;
extern const aomw_topo_rgb_t aomw_topo_blue;
extern const aomw_topo_rgb_t aomw_topo_magenta;
extern const aomw_topo_rgb_t aomw_topo_white;
extern const aomw_topo_rgb_t aomw_topo_off;
// Sets the color for triplet `tix` to `rgb` - this hides RGBI vs SAID qua current and triplet count
aoresult_t aomw_topo_settriplet( uint16_t tix, const aomw_topo_rgb_t*rgb ); 
// Sets the flags for node addr (if it is a SAID; r/g/b current settings as per topo standard)
aoresult_t aomw_topo_node_setcurrents(uint16_t addr, uint8_t flags);


// Default dim level in "prokibi": 100 is at 100/1024 or ~10% of max PWM. 
// Note that (SAID) current setting is 12mA which is at 12/24 or 1/2 of max current. 
// Effective brightness is thus 1/10 * 1/2 = 1/20 of max.
#define AOMW_TOPO_DIM_DEFAULT 100 
// Sets the global dim-level for aomw_topo_settriplet. Function clips to 0..1024.
void aomw_topo_dim_set( int dim );
// Gets the global dim-level
int aomw_topo_dim_get();


// Searches the entire OSP chain for SAIDs with an I2C bridge, and on the associated I2C bus searches for an I2C device with address `daddr7`.
aoresult_t aomw_topo_i2cfind( int daddr7, uint16_t * addr );


//Registers the "topo" command with the command interpreter.
int aomw_topo_cmd_register();


#endif




