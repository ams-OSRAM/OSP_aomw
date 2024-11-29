// aomw_tscript.cpp - tiny script to animate a series of rgb triplets using (tiny) instruction
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
#include <Arduino.h>       // Serial.printf()
#include <aomw_topo.h>     // aomw_topo_settriplet
#include <aomw_tscript.h>  // own


/*
A "tiny script" consist of a number of instructions. One instruction sets a 
region, that is, a consecutive series of RGB triplets, to one color. For 
example, one instruction could set triplets 1, 2, 3 to red. Another 
instruction could set triplets 4, 5, 6 to white and yet another could set 
7, 8, 9 to blue. Instruction have a flag "with-previous". So if, in the 
above example, the second instruction (white) and the third instruction (blue)
do have the with-previous flag set, the three instructions together make 
one frame drawing the Red/White/Blue flag on triplets 1 to 9.

A script runs on a chain of any length (any number of triplets). Triplets 
are not addressed individually, but the chain is partitioned in regions. 
Instructions set the color of a region as a whole. For this instructions 
contain a start-of-region and an end-of-region index. These start and end 
index need to be _mapped_ to physical triplets (depending on chain length). 

A script needs to be stored in a 256 bytes EEPROM, so everything about this
script is "tiny". Each instruction is 16 bit, and the red, green, and blue 
brightness levels, as well as the start-of-region and end-of-region index 
are only three bits each. In other words, there are only eight brightness 
levels and only eight regions. An instruction is coded as follows:

  +----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | 15 | 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  +----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  |with| start of  |  end of   |    red    |   green   |   blue    |
  |prev|  region   |  region   | brightness| brightness| brightness|
  +----+-----------+-----------+-----------+-----------+-----------+

The regions are linearly distributed over all triplets, and the brightness
levels are exponentially scaled.

Note that each field of an instruction is three bits, so instructions are 
relatively readable when using octal notation. Let's have a look at an
example:

  0007007,
  0166100,
  0070000,

The first instruction starts with 0, C-syntax for octal. We strip that and
break the rest in pieces 0 07 007. The last three digits are 007 so red 0,
green 0 and blue 7, so brightest blue. The two digits before that 07 denote
the region, here regions 0 to 7, which means the whole chain. The leading 0
means not with-previous, so this is/starts a frame.

The second instruction is 1 66 100 (dropping the leading octal 0). Here the 
color is lowest red (100), and the region is only number 6 (6..6). The
with-previous is set, so this instruction belongs to the same frame as the
first instruction. The third instruction starts a new frame; it has
with-previous not set.

If the chain would be 16 long it would look as follows
  0 1 2 3 4 5 6 7 8 9101112131415  triplet index
  0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7  region index
  B B B B B B B B B B B B r r B B  resulting frame

The third instruction is special. The region runs from 7 to 0. This is 
normally not a legal instruction. Instruction with start-of-region greater
than end of region mean end-of-script.
*/


// ==========================================================================
// Brightness lookup table: for i in range(7) : print( hex(int(0x3C0*1.8**i)) )
// Maps a brightness level from an instruction to a brightness level for topo.
static uint16_t aomw_tscript_brightness[8] = {
  0x0000, //0
  0x03c0, //1
  0x06c0, //2
  0x0c26, //3
  0x15de, //4
  0x275d, //5
  0x46db, //6
  0x7f8b, //7
};


// === Iterator ==============================================================


static const uint16_t *    aomw_tscript_insts;      // List of instructions ("the script")
static int                 aomw_tscript_cursor;     // Index of first instruction to play
static aomw_tscript_inst_t aomw_tscript_inst;       // Decoded instruction under the cursor
static int                 aomw_tscript_numtriplets;// Multiplier to go from triplet index 0..7 in instruction to triplet index in actual chain


// Dissects a 16-bit instruction (see top of this file) into the fields of aomw_tscript_inst_t.
// It maps region indices from the instruction (0..7) to triplet indices spread over the OSP chain.
// It maps brightness levels from the instruction (0..7) to brightness levels used by topo (0..32767).
static void aomw_tscript_decode( ) {
  // Helpers to slice bits form an instruction
  #define BITS_MASK(n)                  ((1<<(n))-1)                           // number of bits set BITS_MASK(3)=0b111 (max n=31)
  #define BITS_SLICE(v,lo,hi)           ( ((v)>>(lo)) & BITS_MASK((hi)-(lo)) ) // including lo, excluding hi
  // Get current instruction
  uint16_t code = aomw_tscript_insts[aomw_tscript_cursor];
  uint16_t tix0 = BITS_SLICE(code,12,15);
  uint16_t tix1 = BITS_SLICE(code, 9,12);
  // Get the instruction parts
  aomw_tscript_inst.cursor   = aomw_tscript_cursor;
  aomw_tscript_inst.code     = code;
  aomw_tscript_inst.atend    = tix0>tix1;
  aomw_tscript_inst.withprev = BITS_SLICE(code,15,16);
  aomw_tscript_inst.tix0     = (  tix0    * aomw_tscript_numtriplets + 4 ) / 8;
  aomw_tscript_inst.tix1     = ( (tix1+1) * aomw_tscript_numtriplets + 4 ) / 8;
  if( aomw_tscript_inst.tix1>aomw_tscript_numtriplets ) aomw_tscript_inst.tix1= aomw_tscript_numtriplets;
  aomw_tscript_inst.rgb.r    = aomw_tscript_brightness[ BITS_SLICE(code,6,9) ];
  aomw_tscript_inst.rgb.g    = aomw_tscript_brightness[ BITS_SLICE(code,3,6) ];
  aomw_tscript_inst.rgb.b    = aomw_tscript_brightness[ BITS_SLICE(code,0,3) ];
}


/*!
    @brief  Sets the script cursor to the first instruction of the script.
    @note   A script must have been installed with aomw_tscript_install().
    @note   See aomw_tscript_get() for details of the instruction under 
            the cursor.
*/
void aomw_tscript_gotofirst() {
  aomw_tscript_cursor= 0;
  aomw_tscript_decode();
}


/*!
    @brief  Moves the script cursor to the next instruction of the script.
    @note   The script cursor must be set once with aomw_tscript_gotofirst(),
            then multiple calls to aomw_tscript_gotonext() until 
            aomw_tscript_atend() holds.
    @note   If aomw_tscript_atend() holds, the cursor is not moved.
    @note   See aomw_tscript_get() for details of the instruction under 
            the cursor.
*/
void aomw_tscript_gotonext() {
  if( !aomw_tscript_atend() ) {
    aomw_tscript_cursor++;
    aomw_tscript_decode();
  }
}


/*!
    @brief  Indicates if the script cursor is at the end of the script or
            pointing at a valid instruction.
    @return Returns true iff the script cursor is at the end-of-script instruction.
    @note   A script must have been installed with aomw_tscript_install().
    @note   The end-of-script instruction is an instruction, acting as sentinel.
            Any instruction with start-of-region greater than end-of-region 
            indicates end-of-script, it is suggested to use 0070000 (octal).
*/
bool aomw_tscript_atend() {
  return aomw_tscript_inst.atend;
}


/*!
    @brief  Returns the (details of the) instruction under the cursor.
    @return (Pointer to a struct with the details of the) instruction under
            the cursor.
    @note   A script must have been installed with aomw_tscript_install().
    @note   A script consists of a series of animation instructions.
            There is also a script cursor that points at the current 
            instruction. The cursor is controlled through an iterator API: 
            gotofirst, gotonext and atend; the get function returns the 
            instruction under the script cursor.
    @note   An instruction has only 3 bits to denote the start- and end- 
            triplet that need to be colored, but also only three bits for
            the brightness level of the red, green and blue colors.
            The get() maps region indices from the instruction (0..7) to 
            triplet indices spread over the OSP chain. It also maps the 
            brightness levels from the instruction (0..7) to brightness 
            levels used by topo (0..32767).
    @note   It is possible for the caller to use the instruction, however
            the caller typically uses aomw_tscript_playframe().
*/
const aomw_tscript_inst_t * aomw_tscript_get() {
  AORESULT_ASSERT( aomw_tscript_insts!=NULL ); // forgot aomw_tscript_install()?
  return &aomw_tscript_inst;
}


/*!
    @brief  Installs a new script.
    @param  insts
            A pointer to the first instruction of an animation script.
    @param  numtriplets
            Number of RGB triplets in the OPS chain.
    @note   The animation script may be arbitrarily long, this module
            only records the pointer to the script. The script must have
            an end-of-script instruction.
    @note   The `numtriplets` tells the instruction interpreter how to 
            map region indices from the instruction (0..7) to triplet 
            indices spread over the OSP chain.
    @note   This module only supports one (active) animation script at a 
            time. It also support only one iterator on that script.
    @note   This function also calls gotofirst().
*/
void aomw_tscript_install(const uint16_t *insts, uint16_t numtriplets) {
  aomw_tscript_insts= insts;
  aomw_tscript_numtriplets= numtriplets;
  aomw_tscript_gotofirst();
}


// === play ==================================================================


/*!
    @brief  Plays the the instruction under the cursor.
    @return aoresult_assert       if atend() holds
            aoresult_ok           if triplets are set successfully
            other error code      if there is a (communications) error
    @note   Playing the instruction means that the begin- and end-of-region 
            from the instruction are mapped to triplet indices in the OSP 
            chain, and that all triplets in that region are set to the RGB 
            color levels from the instruction.
    @note   A script must have been installed with aomw_tscript_install().
    @note   The play instruction uses aomw_topo_settriplet(), so the topo
            map must have been build, eg with aomw_topo_build().
    @note   This function does not move the cursor (use the iterator API).
    @note   This function should not be called when aomw_tscript_atend() holds.
*/
aoresult_t aomw_tscript_playinst() {
  if( aomw_tscript_atend() ) return aoresult_assert;
  // Using internal `aomw_tscript_inst` instead of public `aomw_tscript_get()`.
  // Serial.printf("#%d 0o%06o : %d [%d,%d) %04x.%04x.%04x\n", aomw_tscript_cursor, aomw_tscript_insts[aomw_tscript_cursor], aomw_tscript_inst.withprev, aomw_tscript_inst.tix0, aomw_tscript_inst.tix1, aomw_tscript_inst.rgb.r, aomw_tscript_inst.rgb.g, aomw_tscript_inst.rgb.b );
  for( uint16_t tix=aomw_tscript_inst.tix0; tix<aomw_tscript_inst.tix1; tix++ ) {
    aoresult_t result= aomw_topo_settriplet(tix, &aomw_tscript_inst.rgb );
    if( result!=aoresult_ok ) return result;
  }
  return aoresult_ok;
}


/*!
    @brief  Plays the the instruction under the cursor and uses the iterator
            to move to the next instruction. If next instruction has the 
            with-previous flag asserted, also executes it, and so on. 
    @return aoresult_assert       if script only has end-of-script instruction.
            aoresult_outofmem     if there are too many with-previous in a row
            aoresult_ok           if triplets are set successfully
            other error code      if there is a (communications) error
    @note   "Playing" is in the sense of aomw_tscript_playinst().
    @note   This function does move the cursor (using the iterator API).
    @note   This function wraps when atend() holds, but it does this before
            playing the instruction, not after playing. This allows the caller
            to check atend().
*/
aoresult_t aomw_tscript_playframe() {
  if( aomw_tscript_atend() ) aomw_tscript_gotofirst();
  int n=1;
  do {
    if( n>8 ) return aoresult_outofmem; // can not have more then 8 with-previous, because there are only 8 segments
    aoresult_t result= aomw_tscript_playinst();
    if( result!=aoresult_ok ) return result;
    aomw_tscript_gotonext();
    n++;
  } while( aomw_tscript_get()->withprev );
  return aoresult_ok;
}


// ==========================================================================
// Stock animation scripts


static const uint16_t aomw_tscript_rainbow_[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB

  // From all black to all white
  0007000,
  0007111,
  0007222,
  0007333,
  0007444,
  0007555,
  0007666,
  0007777,

  // All bands up
  // Segment 1 from white to red
  0011766,
  0011755,
  0011744,
  0011733,
  0011722,
  0011711,
  0011700,
  // Segment 2 from white to yellow
  0022776,
  0022775,
  0022774,
  0022773,
  0022772,
  0022771,
  0022770,
  // Segment 3 from white to green
  0033676,
  0033575,
  0033474,
  0033373,
  0033272,
  0033171,
  0033070,
  // Segment 4 from white to cyan
  0044677,
  0044577,
  0044477,
  0044377,
  0044277,
  0044177,
  0044077,
  // Segment 5 from white to blue
  0055667,
  0055557,
  0055447,
  0055337,
  0055227,
  0055117,
  0055007,
  // Segment 6 from white to purple
  0066767,
  0066757,
  0066747,
  0066737,
  0066727,
  0066717,
  0066707,

  // All bands down
  // Segment 0 from white to black
  0000666,
  0000555,
  0000444,
  0000333,
  0000222,
  0000111,
  0000000,
  // Segment 1 from red to black
  0011600,
  0011500,
  0011400,
  0011300,
  0011200,
  0011100,
  0011000,
  // Segment 2 from yellow to black
  0022660,
  0022550,
  0022440,
  0022330,
  0022220,
  0022110,
  0022000,
  // Segment 3 from green to black
  0033060,
  0033050,
  0033040,
  0033030,
  0033020,
  0033010,
  0033000,
  // Segment 4 from cyan to black
  0044066,
  0044055,
  0044044,
  0044033,
  0044022,
  0044011,
  0044000,
  // Segment 5 from blue to black
  0055006,
  0055005,
  0055004,
  0055003,
  0055002,
  0055001,
  0055000,
  // Segment 6 from purple to black
  0066606,
  0066505,
  0066404,
  0066303,
  0066202,
  0066101,
  0066000,
  // Segment 7 from white to black
  0077666,
  0077555,
  0077444,
  0077333,
  0077222,
  0077111,
  0077000,

  // End
  0070000,
};


/*!
    @brief  The rainbow animation script.
    @return (a pointer to the first instruction of) the animation script.
    @note   See aomw_tscript_rainbow_bytes().
    @note   This script resides in rom/flash.
    @note   This script is intended to be stored in the EEPROM of the 
            SAIDbasic board - so that the board has a script available.
    @note   The animation script starts with dimming the hole strip 
            from black up to white. Then segment 1 becomes more and more red, 
            then segment 2 becomes more and more yellow, then 3 green, 
            4 cyan, 5 blue, 6 purple. Segment 0 and 7 stay white. 
            Then one by one the segments dim down to black.
*/
const uint16_t * aomw_tscript_rainbow() {
  return aomw_tscript_rainbow_;
}


/*!
    @brief  The size of the rainbow animation script.
    @return The script size in bytes (each script instruction is two bytes).
    @note   See aomw_tscript_rainbow().
*/
int aomw_tscript_rainbow_bytes() {
  return sizeof(aomw_tscript_rainbow_);
}


static const uint16_t aomw_tscript_bouncingblock_[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB

  // Red block moving left to right (1) on blue background (7)
  0007007,
  0177100,

  0007007,
  0166100,

  0007007,
  0155100,

  0007007,
  0144100,

  0007007,
  0133100,

  0007007,
  0122100,

  0007007,
  0111100,

  0007007,
  0100100,

  // script was too long so skipping red=2,bg=6

  // Red block moving left to right (3) on blue background (5)
  0007005,
  0100300,

  0007005,
  0111300,

  0007005,
  0122300,

  0007005,
  0133300,

  0007005,
  0144300,

  0007005,
  0155300,

  0007005,
  0166300,

  0007005,
  0177300,

  // Red block moving back, more red (4), less blue (4)
  0007004,
  0177400,

  0007004,
  0166400,

  0007004,
  0155400,

  0007004,
  0144400,

  0007004,
  0133400,

  0007004,
  0122400,

  0007004,
  0111400,

  0007004,
  0100400,

  // Red block moving left to right (5) on blue background (3)
  0007003,
  0100500,

  0007003,
  0111500,

  0007003,
  0122500,

  0007003,
  0133500,

  0007003,
  0144500,

  0007003,
  0155500,

  0007003,
  0166500,

  0007003,
  0177500,

  // Red block moving back, more red (6), less blue (2)
  0007002,
  0177600,

  0007002,
  0166600,

  0007002,
  0155600,

  0007002,
  0144600,

  0007002,
  0133600,

  0007002,
  0122600,

  0007002,
  0111600,

  0007002,
  0100600,

  // Red block moving left to right (7) on blue background (1)
  0007001,
  0100700,

  0007001,
  0111700,

  0007001,
  0122700,

  0007001,
  0133700,

  0007001,
  0144700,

  0007001,
  0155700,

  0007001,
  0166700,

  0007001,
  0177700,

  // Red block moving back, more red (7) erasing blue
  0007000,
  0177700,

  0007000,
  0166700,

  0007000,
  0155700,

  0007000,
  0144700,

  0007000,
  0133700,

  0007000,
  0122700,

  0007000,
  0111700,

  0007000,
  0100700,

  // End
  0070000,
};


/*!
    @brief  The bouncingblock animation script.
    @return (a pointer to the first instruction of) the animation script.
    @note   See aomw_tscript_bouncingblock_bytes().
    @note   This script resides in rom/flash.
    @note   This script is intended to be stored on I2C EEPROM stick nr 1,
            which comes with the SAIDbasic board.
    @note   The animation script has the background blue with a red block 
            (one segment) moving. The script starts with a bright blue 
            background and a dim red block moving from segment 7 (right) 
            to segment 0 (left). Then the block moves back, the blue is 
            dimmed down and the red is dimmed up. Back again, less blue, 
            more red, and so on.
*/
const uint16_t * aomw_tscript_bouncingblock() {
  return aomw_tscript_bouncingblock_;
}


/*!
    @brief  The size of the bouncingblock animation script.
    @return The script size in bytes (each script instruction is two bytes).
    @note   See aomw_tscript_bouncingblock().
*/
int aomw_tscript_bouncingblock_bytes() {
  return sizeof(aomw_tscript_bouncingblock_);
}


static const uint16_t aomw_tscript_colormix_[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB


  // white bg, red from left, green from right
  0007777, // 01234567
  0100700, // r-------

  0007777,
  0100700, // 01234567
  0177070, // r------g

  0007777,
  0101700, // 01234567
  0177070, // rr-----g

  0007777,
  0101700, // 01234567
  0167070, // rr----gg

  0007777,
  0112700, // 01234567
  0167070, // -rr---gg

  0007777,
  0112700, // 01234567
  0156070, // -rr--gg-

  0007777,
  0123700, // 01234567
  0156070, // --rr-gg-

  0007777,
  0123700, // 01234567
  0145070, // --rrgg--

  0007777,
  0133700, // 01234567
  0144770, // ---ryg--
  0155070,

  0007777, // 01234567
  0134770, // ---yy---

  0007777,
  0155700, // 01234567
  0144770, // ---gyr--
  0133070,

  0007777,
  0145700, // 01234567
  0123070, // --ggrr--

  0007777,
  0156700, // 01234567
  0123070, // --gg-rr-

  0007777,
  0156700, // 01234567
  0112070, // -gg--rr-

  0007777,
  0167700, // 01234567
  0112070, // -gg---rr

  0007777,
  0167700, // 01234567
  0101070, // gg----rr

  0007777,
  0177700, // 01234567
  0101070, // gg-----r

  0007777,
  0177700, // 01234567
  0100070, // g------r

  0007777, // 01234567
  0100070, // g-------

  0007777, // 01234567

  // back
  0007777, // 01234567
  0100070, // g-------

  0007777,
  0177700, // 01234567
  0100070, // g------r

  0007777,
  0177700, // 01234567
  0101070, // gg-----r

  0007777,
  0167700, // 01234567
  0101070, // gg----rr

  0007777,
  0167700, // 01234567
  0112070, // -gg---rr

  0007777,
  0156700, // 01234567
  0112070, // -gg--rr-

  0007777,
  0156700, // 01234567
  0123070, // --gg-rr-

  0007777,
  0145700, // 01234567
  0123070, // --ggrr--

  0007777,
  0155700, // 01234567
  0144770, // ---gyr--
  0133070,

  0007777, // 01234567
  0134770, // ---yy---

  0007777,
  0133700, // 01234567
  0144770, // ---ryg--
  0155070,

  0007777,
  0123700, // 01234567
  0145070, // --rrgg--

  0007777,
  0123700, // 01234567
  0156070, // --rr-gg-

  0007777,
  0112700, // 01234567
  0156070, // -rr--gg-

  0007777,
  0112700, // 01234567
  0167070, // -rr---gg

  0007777,
  0101700, // 01234567
  0167070, // rr----gg

  0007777,
  0101700, // 01234567
  0177070, // rr-----g

  0007777,
  0100700, // 01234567
  0177070, // r------g

  0007777, // 01234567
  0100700, // r-------

  0007777, // 01234567

  // End
  0070000,
};


/*!
    @brief  The colormix animation script.
    @return (a pointer to the first instruction of) the animation script.
    @note   See aomw_tscript_colormix_bytes().
    @note   This script resides in rom/flash.
    @note   This script is intended to be stored on I2C EEPROM stick nr 2,
            which comes with the SAIDbasic board.
    @note   On a white background a red block (two segments) comes from the
            left a green block (two segments). Step by step the approach.
            Where they overlap, the color is yellow. The block continue to
            move until the are at the other end, then they reverse.
*/
const uint16_t * aomw_tscript_colormix() {
  return aomw_tscript_colormix_;
}


/*!
    @brief  The size of the colormix animation script.
    @return The script size in bytes (each script instruction is two bytes).
    @note   See aomw_tscript_colormix().
*/
int aomw_tscript_colormix_bytes() {
  return sizeof(aomw_tscript_colormix_);
}


static const uint16_t aomw_tscript_heartbeat_[] = {
// Octal 0, 0 or 1 for with-previous, 0..7 for lower index, 0..7 for upper index, 0..7 for red, 0..7 for green and 0..7 for blue
//oPLURGB

  // first heart beat
  0007100,
  0007100,
  0007100,
  0007300,
  0007500,
  0007700,
  0007700,
  0007500,
  0007300,
  0007100,
  // second heart beat
  0007100,
  0007300,
  0007500,
  0007700,
  0007700,
  0007700,
  0007700,
  0007700,
  0007700,
  0007500,
  0007300,
  0007100,

  // fade
  0007100,
  0007100,
  // long pause
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  0007010,
  // fade
  0007100,
  0007100,

  // End
  0070000,
};


/*!
    @brief  The heartbeat animation script.
    @return (a pointer to the first instruction of) the animation script.
    @note   See aomw_tscript_heartbeat_bytes().
    @note   This script resides in rom/flash.
    @note   This script is intended to be stored with the app that plays
            animation scripts (eg aoapps_aniscript) so that it has an 
            animation script, even if no EEPROM is found.
    @note   In this animation red dims up then down (full strip).
            Next, red dims up stronger and then down, followed by a 
            long pause in green.
*/
const uint16_t * aomw_tscript_heartbeat() {
  return aomw_tscript_heartbeat_;
}


/*!
    @brief  The size of the heartbeat animation script.
    @return The script size in bytes (each script instruction is two bytes).
    @note   See aomw_tscript_heartbeat().
*/
int aomw_tscript_heartbeat_bytes() {
  return sizeof(aomw_tscript_heartbeat_);
}

