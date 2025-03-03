// aomw_topo.cpp - compute a topological map of all nodes in the OSP chain
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
#include <Arduino.h>    // Serial.printf
#include <aoosp.h>      // aoosp_send_identify()
#include <aocmd.h>      // aocmd_cint_register()
#include <aomw_topo.h>  // own


// === data model database ==================================================


// Terminology: a _node_ is an element in an OSP chain. Such an element has
// an _identity_ ("id"), which is either an RGBI or a SAID. At the moment of
// writing this, there are no other OSP node types (chips) available. A node
// has an _address_ usually abbreviated to "addr" and addresses start at 1.
//
// A _triplet_ is a group of three LEDs: typically one red, one green, and
// one blue LED. An RGBI embeds one triplet, a SAID has 3 _channels_, each
// channel drives one external triplet. However, it is possible that a SAID
// is configured to use its third channel for I2C. In that case the SAID
// only drives two triplets.
//
// A _triplet_ has a triplet index usually abbreviated to "tix" and unlike
// addresses triplet indices start at 0. An _i2cbridge_ is a node (of type
// SAID) whose third channel is configured for I2C.
//
// An OSP chain needs to be RESET and INITed, then scanned. This is done 
// by calling first aomw_topo_build_start(), then continuously calling 
// aomw_topo_build_step(), until aomw_topo_build_done(). There is a 
// shorthand that performs all these steps, aomw_topo_build(), but
// the start/step is better in combination with a command interpreter,
// because of the liveliness/responsiveness.
//
// The build creates a topological map of all triplets, nodes and I2C bridges.
// Once the scan is completed, it can be printed for debug with 
// aomw_topo_dump(), but in normal applications, the topological map is 
// inspected via the observers aomw_topo_node_xxx(), aomw_topo_triplet_xxx(), 
// and aomw_topo_i2cbridge_xxx().
//
// The aomw_topo_build also configures the chain: clearing error flags,
// enable crc checking, powering i2C bridges, and last but not least setting 
// the drive current and going active. This makes all the nodes in the OSP 
// chain ready for pwm telegrams via aomw_topo_settriplet().


// ESP32 has large RAM, go for max
#define AOMW_TOPO_MAXNODES       AOOSP_ADDR_UNICASTMAX     
#define AOMW_TOPO_MAXTRIPLETS    (3*AOOSP_ADDR_UNICASTMAX)
#define AOMW_TOPO_MAXI2CBRIDGES  AOOSP_ADDR_UNICASTMAX     


#define AOMW_TOPO_CHAN_NONE     0xFF // channel id used internally when there are no channels (ie for RGBI)


static int      aomw_topo_loop_;                                   // Chain has direction loop (1) or bidir (0)
static uint16_t aomw_topo_last_;                                   // The address of the last node (response from INIT telegram)

static uint16_t aomw_topo_numnodes_;                               // The number of nodes in the chain (at the end of scan must be equal to aomw_topo_last_)
static uint32_t aomw_topo_node_id_[AOMW_TOPO_MAXNODES];            // The identity reported by the node
static uint8_t  aomw_topo_node_numtriplets_[AOMW_TOPO_MAXNODES];   // Number of triplets in that node (RGBI: 1, SAID: 3 or 2)
static uint16_t aomw_topo_node_triplet1_[AOMW_TOPO_MAXNODES];      // The triplet index of the first triplet of this node

static uint16_t aomw_topo_numtriplets_;                            // Number of triplets in the chain
static uint16_t aomw_topo_triplet_addr_[AOMW_TOPO_MAXTRIPLETS];    // The address of the node this triplet belongs to
static uint8_t  aomw_topo_triplet_chan_[AOMW_TOPO_MAXTRIPLETS];    // The channel of the node this triplet is connected to (AOMW_TOPO_CHAN_NONE for RGBI)

static uint16_t aomw_topo_numi2cbridges_;                          // Number of I2C bridges in the chain (SAIDs with OTP flag)
static uint16_t aomw_topo_i2cbridge_addr_[AOMW_TOPO_MAXI2CBRIDGES];// The address of the node this i2c bridge belongs to


// === data model observers =================================================


/*!
    @brief  Returns if the current OSP chain has direction Loop (or BiDir).
    @return 1  if the current OSP chain has direction Loop.
            0  if the current OSP chain has direction BiDir.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   This is part of what is known as the OSP chain "topology map".
*/
int aomw_topo_loop() {
  return aomw_topo_loop_;
}


/*!
    @brief  Returns the number of nodes in the scanned OSP chain.
    @return Number of OSP nodes in the scanned chain.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_numnodes() {
  return aomw_topo_numnodes_;
}


/*!
    @brief  Returns the identity of OSP node at address `addr`.
    @param  addr
            The address of the OSP node.
    @return The identity (as reported by telegram 07/IDENTIFY).
    @note   Only available after aomw_topo_build() - or start/step.
    @note   addr is 1-based, so 1 <= addr <= aomw_topo_numnodes().
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint32_t aomw_topo_node_id( uint16_t addr ) {
  AORESULT_ASSERT( 1<=addr && addr<=aomw_topo_numnodes_ );
  return aomw_topo_node_id_[addr]; // skip slot 0
}


/*!
    @brief  Returns the number of triplets (RGB modules) connected to 
            the OSP node at address `addr`.
    @param  addr
            The address of the OSP node.
    @return The number of triplets. Typically 1 for RGBI's and 3 for SAID's
            (but 2 if the SAID has an I2C bridge).
    @note   Only available after aomw_topo_build() - or start/step.
    @note   addr is 1-based, so 1 <= addr <= aomw_topo_numnodes().
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint8_t aomw_topo_node_numtriplets( uint16_t addr ) {
  AORESULT_ASSERT( 1<=addr && addr<=aomw_topo_numnodes_ );
  return aomw_topo_node_numtriplets_[addr]; // skip slot 0
}


/*!
    @brief  Returns the index of the first triplet (RGB module) driven by 
            OSP node at address `addr`.
    @param  addr
            The address of the OSP node.
    @return Index of the first triplet.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   addr is 1-based, so 1 <= addr <= aomw_topo_numnodes().
    @note   If a node has more than 1 triplet, see aomw_topo_node_numtriplets(),
            the next ones are consecutively numbered.
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_node_triplet1( uint16_t addr ) {
  AORESULT_ASSERT( 1<=addr && addr<=aomw_topo_numnodes_ );
  return aomw_topo_node_triplet1_[addr]; // skip slot 0
}


/*!
    @brief  Returns the number of triplets (RGB modules) in the scanned 
            OSP chain.
    @return Number of triplets in the scanned chain.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_numtriplets() {
  return aomw_topo_numtriplets_;
}


/*!
    @brief  Returns the address of the OSP node that drives triplet `tix`.
    @param  tix
            The index of the triplet.
    @return The OSP address of the triplet.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   tix is 0-based, so , 0 <= tix < aomw_topo_numtriplets().
    @note   Since some OSP nodes (eg SAID) drive multiple RGB modules, 
            there are typically more triplets than OSP nodes. Also, 
            because triplets are the visible "actuators" of the OSP chain, 
            animations operate in the triplet (tix) domain, not in the node 
            (addr) domain. However, in order to send the correct telegram, 
            the addr and channel (see aomw_topo_triplet_chan) are needed.
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_triplet_addr( uint16_t tix ) {
  AORESULT_ASSERT( tix<aomw_topo_numtriplets_ );
  return aomw_topo_triplet_addr_[tix];
}


/*!
    @brief  Returns 1 if triplet `tix` is driven by an OSP node with channels.
    @param  tix
            The index of the triplet.
    @return 1 iff the the triplet is on a channel of an OSP node.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   tix is 0-based, so , 0 <= tix < aomw_topo_numtriplets().
    @note   Some OSP node (eg RGBIs) have no notion of channels, so a
            SETPWM(r,g,b) telegram needs to be sent. Other OSP nodes 
            (eg SAIDs) do have multiple channels, and they require a 
            SETPWMCHN(chan,r,g,b). 
            In the latter case, this function returns 1.
    @note   This is part of what is known as the OSP chain "topology map".
*/
int aomw_topo_triplet_onchan( uint16_t tix ) {
  AORESULT_ASSERT( tix<aomw_topo_numtriplets_ );
  return aomw_topo_triplet_chan_[tix] != AOMW_TOPO_CHAN_NONE;
}


/*!
    @brief  Returns the channel triplet `tix` is attached to in case the
            triplet is driven by an OSP node with channels.
    @param  tix
            The index of the triplet.
    @return The channel of a node.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   tix is 0-based, so , 0 <= tix < aomw_topo_numtriplets().
    @note   Some OSP node (eg RGBIs) have no notion of channels, so a
            SETPWM(r,g,b) telegram needs to be sent. Other OSP nodes 
            (eg SAIDs) do have multiple channels, and they require a 
            SETPWMCHN(chan,r,g,b). 
            In the latter case this function returns the channel.
    @note   Only defined when aomw_topo_triplet_onchan(tix).       
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint8_t aomw_topo_triplet_chan( uint16_t tix ) {
  AORESULT_ASSERT( tix<aomw_topo_numtriplets_ );
  AORESULT_ASSERT( aomw_topo_triplet_chan_[tix] != AOMW_TOPO_CHAN_NONE );
  return aomw_topo_triplet_chan_[tix];
}


/*!
    @brief  Returns the number of I2C bridges in the scanned chain.
    @return Number of I2C bridges in the scanned OSP chain.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   SAIDs can have an I2C bridge, if configured as such in their OTP.
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_numi2cbridges() {
  return aomw_topo_numi2cbridges_;
}


/*!
    @brief  Returns the address of the OSP node that has I2C bridge 
            with index `iix`.
    @param  iix
            The index of the I2C bridge.
    @return The OSP address of the I2C bridge.
    @note   Only available after aomw_topo_build() - or start/step.
    @note   iix is 0-based, so , 0 <= iix < aomw_topo_numi2cbridges().
    @note   This is part of what is known as the OSP chain "topology map".
*/
uint16_t aomw_topo_i2cbridge_addr( uint16_t iix ) {
  AORESULT_ASSERT( iix<aomw_topo_numi2cbridges_ );
 return aomw_topo_i2cbridge_addr_[iix];
}


// === data model dump ======================================================


/*!
    @brief  Prints on Serial a summary of the "topology map".
    @note   Only available after aomw_topo_build() - or start/step.
*/
void aomw_topo_dump_summary() {
  Serial.printf("nodes(N) 1..%d, ", aomw_topo_numnodes_ );
  Serial.printf("triplets(T) 0..%d, ", aomw_topo_numtriplets_ - 1 );
  if( aomw_topo_numi2cbridges_ == 0 ) 
    Serial.printf("i2cbridges(I) none, " );
  else
    Serial.printf("i2cbridges(I) 0..%d, ", aomw_topo_numi2cbridges_-1 );
  Serial.printf("dir %s\n", aomw_topo_loop()?"loop":"bidir");
}


/*!
    @brief  Prints on Serial a list of nodes from the "topology map".
    @note   Only available after aomw_topo_build() - or start/step.
*/
void aomw_topo_dump_nodes() {
  uint16_t iix = 0;
  for( uint16_t addr=1; addr<=aomw_topo_numnodes_; addr++ ) {
    Serial.printf("N%03X (%08lX)", addr,aomw_topo_node_id(addr) );
    for( uint16_t tix=aomw_topo_node_triplet1(addr); tix<aomw_topo_node_triplet1(addr)+aomw_topo_node_numtriplets(addr); tix++ )
      Serial.printf(" T%d",tix);
    if( iix<aomw_topo_numi2cbridges_ && aomw_topo_i2cbridge_addr(iix)==addr ) { Serial.printf(" I%d",iix); iix++; }
    Serial.printf("\n");
  }
}


/*!
    @brief  Prints on Serial a list of triplets from the "topology map".
    @note   Only available after aomw_topo_build() - or start/step.
*/
void aomw_topo_dump_triplets() {
  for( uint16_t tix=0; tix<aomw_topo_numtriplets_; tix++ ) {
    uint16_t addr = aomw_topo_triplet_addr_[tix];
    Serial.printf("T%d N%03X", tix, addr );
    if( aomw_topo_triplet_onchan(tix) ) Serial.printf(".C%d", aomw_topo_triplet_chan(tix) );
    Serial.printf("\n");
  }
}


/*!
    @brief  Prints on Serial a list of I2C bridges from the "topology map".
    @note   Only available after aomw_topo_build() - or start/step.
*/
void aomw_topo_dump_i2cbridges() {
  for( uint16_t iix=0; iix<aomw_topo_numi2cbridges_; iix++ ) {
    Serial.printf("I%d N%03X\n", iix,aomw_topo_i2cbridge_addr(iix) );
  }
}


// === topo build helpers ===================================================


static aoresult_t aomw_topo_node_identify(uint16_t addr) {
  // Get the id of the node
  uint32_t id;
  aoresult_t result = aoosp_send_identify( addr, &id );
  if( result!=aoresult_ok ) return result;
  // Record the node's id (if there is still space)
  aomw_topo_numnodes_++; // 1-based, so pre-increment
  AORESULT_ASSERT(addr==aomw_topo_numnodes_);
  if( aomw_topo_numnodes_>=AOMW_TOPO_MAXNODES ) return aoresult_outofmem;
  aomw_topo_node_id_[aomw_topo_numnodes_] = id;
  aomw_topo_node_triplet1_[aomw_topo_numnodes_] = aomw_topo_numtriplets_;
  // Register the triplets of the node
  if( AOOSP_IDENTIFY_IS_RGBI(id) ) { // RGBI: one triplet, no channel.
    // Record the triplet's address and channel (if there is still space)
    if( aomw_topo_numtriplets_>=AOMW_TOPO_MAXTRIPLETS ) return aoresult_outofmem;
    aomw_topo_triplet_addr_[aomw_topo_numtriplets_] = addr;
    aomw_topo_triplet_chan_[aomw_topo_numtriplets_] = AOMW_TOPO_CHAN_NONE;
    aomw_topo_numtriplets_++;
    aomw_topo_node_numtriplets_[aomw_topo_numnodes_] = 1;
  } else if( AOOSP_IDENTIFY_IS_SAID(id) ) { // SAID: three triplets, or two plus I2C bridge
    // Record the channel 0 triplet's address and channel (if there is still space)
    if( aomw_topo_numtriplets_>=AOMW_TOPO_MAXTRIPLETS ) return aoresult_outofmem;
    aomw_topo_triplet_addr_[aomw_topo_numtriplets_] = addr;
    aomw_topo_triplet_chan_[aomw_topo_numtriplets_] = 0;
    aomw_topo_numtriplets_++;
    // Record the channel 1 triplet's address and channel (if there is still space)
    if( aomw_topo_numtriplets_>=AOMW_TOPO_MAXTRIPLETS ) return aoresult_outofmem;
    aomw_topo_triplet_addr_[aomw_topo_numtriplets_] = addr;
    aomw_topo_triplet_chan_[aomw_topo_numtriplets_] = 1;
    aomw_topo_numtriplets_++;
    // Is channel 2 of this SAID wired for I2C?
    // todo: also inspect other config bits to skip channels (haptic, sync, star, clustering?)
    int isbridge;
    result = aoosp_exec_i2cenable_get(addr, &isbridge );
    if( result!=aoresult_ok ) return result;
    if( isbridge ) {
      // Record the I2C bridge's address (if there is still space)
      if( aomw_topo_numi2cbridges_>=AOMW_TOPO_MAXI2CBRIDGES ) return aoresult_outofmem;
      aomw_topo_i2cbridge_addr_[aomw_topo_numi2cbridges_] = addr;
      aomw_topo_numi2cbridges_ ++;
      aomw_topo_node_numtriplets_[aomw_topo_numnodes_] = 2;
    } else {
      // Record the channel 2 triplet's address and channel (if there is still space)
      if( aomw_topo_numtriplets_>=AOMW_TOPO_MAXTRIPLETS ) return aoresult_outofmem;
      aomw_topo_triplet_addr_[aomw_topo_numtriplets_] = addr;
      aomw_topo_triplet_chan_[aomw_topo_numtriplets_] = 2;
      aomw_topo_numtriplets_++;
      aomw_topo_node_numtriplets_[aomw_topo_numnodes_] = 3;
    }
  } else { // Unknown id
    return aoresult_sys_id; // Or shall we ignore the node, instead of giving error
  }
  return aoresult_ok;
}


static aoresult_t aomw_topo_node_enablecrc(uint16_t addr) {
  aoresult_t result;
  if( AOOSP_IDENTIFY_IS_RGBI(aomw_topo_node_id_[addr]) ) {
    result= aoosp_send_setsetup(addr, AOOSP_SETUP_FLAGS_RGBI_DFLT | AOOSP_SETUP_FLAGS_CRCEN );
  } else if( AOOSP_IDENTIFY_IS_SAID(aomw_topo_node_id_[addr]) ) {
    result= aoosp_send_setsetup(addr, AOOSP_SETUP_FLAGS_SAID_DFLT | AOOSP_SETUP_FLAGS_CRCEN );
  } else {
    result= aoresult_sys_id; // Or shall we ignore the node, instead of giving error
  }
  return result;
}


static aoresult_t aomw_topo_i2cbridge_power(int iix) {
  // Supply current to I2C pads (channel 2)
  return aoosp_send_setcurchn( aomw_topo_i2cbridge_addr_[iix], /*chan*/2, AOOSP_CURCHN_FLAGS_DEFAULT,  4, 4, 4);
}


/*!
    @brief  The topology builder also sets the current of all triplet drivers
            in all OSP nodes (to ~10mA), not to be changed by client code. 
            However SAIDs have some flags in the CURRENT register, like
            AOOSP_CURCHN_FLAGS_DITHER, that can be changed by this function.
    @param  addr
            The address of the OSP node.
    @param  flags
            Combination of AOOSP_CURCHN_FLAGS_xxx.
    @return aoresult_ok      if successful
            aoresult_sys_id  if not an RGBI or SAID
            other error code if there is a (communications) error
    @note   Only available after aomw_topo_build() - or start/step.
    @note   addr is 1-based, so 1 <= addr <= aomw_topo_numnodes().
*/
aoresult_t aomw_topo_node_setcurrents(uint16_t addr, uint8_t flags) {
  aoresult_t result;
  // To make all triplets have the same brightness, we select a "base current"
  // with which all channels of all nodes are driven. The base current topo
  // selected is 12 mA. This fits withing the capabilities of SAID channel 0,
  // SAID channel 1 and 2, and is close for RGBI (night-mode, 10mA).
  //
  // This function will set SAIDs their channel to 12 mA. For RGBI, the current
  // is part of PWM setting, their night mode will be selected.
  //   cur   0    1    2    3    4
  //   chn0  3mA  6mA 12mA 24mA 48mA
  //   chn1 1.5mA 3mA  6mA 12mA 24mA
  //   chn2 1.5mA 3mA  6mA 12mA 24mA

  if(   AOOSP_IDENTIFY_IS_RGBI(aomw_topo_node_id_[addr]) ) return aoresult_ok;     // Skip RGBI's
  if( ! AOOSP_IDENTIFY_IS_SAID(aomw_topo_node_id_[addr]) ) return aoresult_sys_id; // Or shall we ignore the node, instead of giving error

  // Channel 0 is high power, so we select current level 2 (3x12mA)
  result= aoosp_send_setcurchn(addr, 0, flags, 2, 2, 2);
  if( result!=aoresult_ok) return result;

  // Channel 1 is low power, so we select current level 3 (3x12mA)
  result= aoosp_send_setcurchn(addr, 1, flags, 3, 3, 3);
  if( result!=aoresult_ok) return result;

  // Is channel 2 in use for a triplet? If it is used for I2C bridge, bail out
  if( aomw_topo_node_numtriplets_[addr]==2 ) return aoresult_ok;

  // Channel 2 is low power, so we select current level 3 (3x12mA)
  result= aoosp_send_setcurchn(addr, 2, flags, 3, 3, 3);
  if( result!=aoresult_ok) return result;

  return aoresult_ok;
}


// === topo build top-level state machine ===================================



typedef enum aomw_topo_build_state_e {
  AOMW_TOPO_BUILD_STATE_START,
  AOMW_TOPO_BUILD_STATE_IDENTIFYING,
  AOMW_TOPO_BUILD_STATE_CONFIGCLRERROR,
  AOMW_TOPO_BUILD_STATE_CONFIGENABLECRC,
  AOMW_TOPO_BUILD_STATE_CONFIGI2CPOWER,
  AOMW_TOPO_BUILD_STATE_CONFIGSETCURRENT,
  AOMW_TOPO_BUILD_STATE_CONFIGGOACTIVE,
  AOMW_TOPO_BUILD_STATE_DONE,
} aomw_topo_build_state_t;


static aomw_topo_build_state_t aomw_topo_build_state;    // current state
static aoresult_t              aomw_topo_build_result;   // persistent storage of last result (when state==AOMW_TOPO_BUILD_STATE_DONE)
static int                     aomw_topo_build_substate; // Some states iterate over all nodes or all I2C bridges, this is used to keep track of which
#define ADDR                   aomw_topo_build_substate  // an alias to make more clear what is iterated over in a state
#define BIX                    aomw_topo_build_substate  // an alias to make more clear what is iterated over in a state


/*!
    @brief  This function is part of the topology builder.
            Call this once, then follow up with aomw_topo_build_step().
    @note   Building might be redone, as long as it begins with start().
    @note   To build the topology map (fill the topo data model), several 
            telegrams need to be send to the chain, to individual nodes or 
            even to channels: reset, init, identify, readotp (for i2c bridge), 
            clrerror, setsetup (enable crc), setcurrent, goactive.
            If all those telegrams would be sent in one function, the runtime
            of that function would be rather long (for long OSP chains). 
            Therefore this API offers start/step/done, each sending 
            approximately one telegram per call. If the long run-time is of 
            no concern, call the convenience function aomw_topo_build().
*/
void aomw_topo_build_start() {
  aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_START;
}


/*!
    @brief  This function is part of the topology builder.
            Call this until aomw_topo_build_done(), but after 
            aomw_topo_build_start().
    @return aoresult_ok      if successful
            other error code if there is a (communications) error
    @note   Send telegrams, approximately one per step() call.
*/
#define ON_ERROR_RETURN() do { if( result!=aoresult_ok ) { aomw_topo_build_result=result; aomw_topo_build_state=AOMW_TOPO_BUILD_STATE_DONE; return result; } } while(0)
aoresult_t aomw_topo_build_step() {
  aoresult_t result;

  switch( aomw_topo_build_state ) {

    case AOMW_TOPO_BUILD_STATE_START:
      // reset & init entire chain
      result= aoosp_exec_resetinit(&aomw_topo_last_, &aomw_topo_loop_); ON_ERROR_RETURN();
      // prep next state (clear database)
      aomw_topo_numnodes_ = 0;
      aomw_topo_numtriplets_ = 0;
      aomw_topo_numi2cbridges_ = 0;
      ADDR=1; // nodes to scan: 1<=ADDR<=aomw_topo_last_
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_IDENTIFYING;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_IDENTIFYING:
      // Scan node (get its id, get number of triplets)
      if( ADDR<=aomw_topo_last_ ) { // nodes to scan: 1<=ADDR<=aomw_topo_last_
        result= aomw_topo_node_identify(ADDR++); ON_ERROR_RETURN();
        return aoresult_ok; // loop
      }
      AORESULT_ASSERT( aomw_topo_last_==aomw_topo_numnodes_);
      // prep next state
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_CONFIGCLRERROR;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_CONFIGCLRERROR:
      // Broadcast clear error (to clear the over voltage flag of all SAIDs), must have, otherwise SAID will not go ACTIVE
      result= aoosp_send_clrerror(0); ON_ERROR_RETURN();
      // prep next state
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_CONFIGENABLECRC;
      ADDR=1; // nodes to enable CRC checking for: 1<=ADDR<=aomw_topo_last_
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_CONFIGENABLECRC:
      // Enable CRC for all nodes (could be skipped)
      if( ADDR <= aomw_topo_last_ ) { // nodes to enable CRC checking for: 1<=ADDR<=aomw_topo_last_
        result= aomw_topo_node_enablecrc(ADDR++); ON_ERROR_RETURN();
        return aoresult_ok; // loop
      }
      // prep next state
      BIX=0; // I2C bridges to power: 0<=BIX<aomw_topo_numi2cbridges_
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_CONFIGI2CPOWER;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_CONFIGI2CPOWER:
      // Every I2C bridge needs its pads powered
      if( BIX < aomw_topo_numi2cbridges_ ) { // I2C bridges to power: 0<=BIX<aomw_topo_numi2cbridges_
        result= aomw_topo_i2cbridge_power(BIX++); ON_ERROR_RETURN();
        return aoresult_ok; // loop
      }
      // prep next state
      ADDR=1; // nodes to set PWM current: 1<=ADDR<=aomw_topo_last_
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_CONFIGSETCURRENT;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_CONFIGSETCURRENT:
      // Set the current level of the PWM drivers
      if( ADDR <= aomw_topo_last_ ) { // nodes to set PWM current: 1<=ADDR<=aomw_topo_last_
        result= aomw_topo_node_setcurrents(ADDR++,AOOSP_CURCHN_FLAGS_DITHER); ON_ERROR_RETURN();
        return aoresult_ok; // loop
      }
      // prep next state
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_CONFIGGOACTIVE;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_CONFIGGOACTIVE:
      // Switch all nodes to active (LEDs on)
      result= aoosp_send_goactive(0); ON_ERROR_RETURN();
      // prep next state
      aomw_topo_build_result= aoresult_ok;
      aomw_topo_build_state= AOMW_TOPO_BUILD_STATE_DONE;
      return aoresult_ok;

    case AOMW_TOPO_BUILD_STATE_DONE:
      return aomw_topo_build_result;
  }

  return aoresult_assert; // should never reach this
}


/*!
    @brief  This function is part of the topology builder.
            Call this after aomw_topo_build_step(), to determine if
            another step() is needed.
    @return 1   if no more step is neededaoresult_ok
            0   if another aomw_topo_build_step() is needed
*/
int aomw_topo_build_done() {
  return aomw_topo_build_state==AOMW_TOPO_BUILD_STATE_DONE;
}


/*!
    @brief  This function is a high level wrapper around the fine grain
            topology builder functions.
    @return aoresult_ok      if successful
            other error code if there is a (communications) error
    @note   Send many telegrams, several per OSP node.
    @note   See aomw_topo_build_start().
*/
aoresult_t aomw_topo_build() {
  aoresult_t result;
  aomw_topo_build_start();
  while( !aomw_topo_build_done() ) {
    result= aomw_topo_build_step();
    if( result!=aoresult_ok ) return result;
  }
  return aoresult_ok;
}


// === color helpers ========================================================


// The over all dim level used in aomw_topo_settriplet (0..1024).
static int aomw_topo_dim = AOMW_TOPO_DIM_DEFAULT;


// We define some standard colors.
extern const aomw_topo_rgb_t aomw_topo_red    = { 0x7FFF,0x0000,0x0000, "red" };
extern const aomw_topo_rgb_t aomw_topo_yellow = { 0x7FFF,0x7FFF,0x0000, "yellow" };
extern const aomw_topo_rgb_t aomw_topo_green  = { 0x0000,0x7FFF,0x0000, "green" };
extern const aomw_topo_rgb_t aomw_topo_cyan   = { 0x0000,0x7FFF,0x7FFF, "cyan" };
extern const aomw_topo_rgb_t aomw_topo_blue   = { 0x0000,0x0000,0x7FFF, "blue" };
extern const aomw_topo_rgb_t aomw_topo_magenta= { 0x7FFF,0x0000,0x7FFF, "magenta" };
extern const aomw_topo_rgb_t aomw_topo_white  = { 0x7FFF,0x7FFF,0x7FFF, "white" };
extern const aomw_topo_rgb_t aomw_topo_off    = { 0x0000,0x0000,0x0000, "off" };


/*!
    @brief  Sets the color for triplet `tix` to `rgb`.
    @param  tix
            The index of the triplet.
    @param  rgb
            A topo color, each component (red, green, blue) has a brightness 
            level from 0 to 0x7FFF (or AOMW_TOPO_BRIGHTNESS_MAX).
    @return aoresult_ok      if successful
            other error code if there is a (communications) error
    @note   Only available after aomw_topo_build() - or start/step.
    @note   tix is 0-based, so , 0 <= tix < aomw_topo_numtriplets().
    @note   One high level feature of the topo module is to abstract away
            how to drive triplets (if there is a channel, the channel's 
            current settings, and available pwm bits). 
    @note   Each component value (r/g/b) in `rgb` needs to be in the 
            "topo brightness range" from 0 to 0x7FFF 
            (from 0 to AOMW_TOPO_BRIGHTNESS_MAX).
    @note   SAIDs will be driven at 12mA, and use the 15 bit "topo brightness 
            range" as PWM value. RGBIs will be driven at night mode (10mA), 
            and also use the 15 bit "topo brightness range" as PWM value.
    @note   The `rgb` color is dimmed down using the global dim value, 
            set by `aomw_topo_dim_set()`.
*/
aoresult_t aomw_topo_settriplet( uint16_t tix, const aomw_topo_rgb_t *rgb  ) {
  // We dim brightness here to prevent under voltage
  uint16_t r = (rgb->r)*aomw_topo_dim/1024; 
  uint16_t g = (rgb->g)*aomw_topo_dim/1024; 
  uint16_t b = (rgb->b)*aomw_topo_dim/1024; 
  // Select osp node and channel 
  uint16_t addr = aomw_topo_triplet_addr(tix);
  aoresult_t result;
  // This is a bit of a shortcut. When the triplet is "on a channel" we
  // equate that to needing a setpwmchn telegram. In a context of only
  // two kinds of nodes known at the moment (SAID and RGBI) that is enough.
  if( aomw_topo_triplet_onchan(tix) ) {
    // Triplet to configure is an external one driven by a SAID. The PWM 
    // register contains a 15-bit PWM value followed by a 1 bit LSB-dithering
    // control. Use the 15-bits of "topo brightness range" and no dithering (<<1).
    result= aoosp_send_setpwmchn(addr, aomw_topo_triplet_chan(tix), r << 1, g << 1, b << 1 );
  } else {
    // Triplet to configure is an RGBI. The PWM register contains a 1-bit drive 
    // current (0=10mA=nightmode, 1=50mA=daymode) followed by a 15-bit PWM value. 
    // Use drive current nightmode and the 15-bits of "topo brightness range".
    result= aoosp_send_setpwm( addr, r, g, b, 0b000 );
  }
  return result;
}


/*!
    @brief  Sets the global dim-level for aomw_topo_settriplet().
    @param  dim
            The dim level; "pro-kibi": 0 to 1024.
    @note   Can be called even if topo has not been built.
    @note   This function clips to 0..1024.
    @note   Changing the dim level has no effect on the current brightness 
            of the triplets in the chain. Only new aomw_topo_settriplet() 
            calls are effected.
    @note   See also aomw_topo_dim_get().
*/
void aomw_topo_dim_set( int dim ) {
  if( dim<0    ) dim=0;
  if( dim>1024 ) dim=1024;
  aomw_topo_dim = dim;
}


/*!
    @brief  Gets the global dim-level.
    @note   See aomw_topo_dim_set().
*/
int aomw_topo_dim_get() {
  return aomw_topo_dim;
}


// == I2C helpers ===========================================================


/*!
    @brief  Searches the entire OSP chain for SAIDs with an I2C bridge, 
            and on the associated I2C bus searches for an I2C device with 
            address `daddr7`.
    @param  daddr7
            The 7bits I2C device address to be searched for.
    @param  addr
            Out parameter for the OSP address of the SAID with the I2C device.
    @return aoresult_ok           if I2C device found (addr is defined)
            aoresult_dev_noi2cdev if I2C device is not found (addr undefined)
            other error code      if there is a (communications) error
    @note   Only available after aomw_topo_build() - or start/step.
    @note   The search is from upstream (low addr) to downstream (high addr).
*/
aoresult_t aomw_topo_i2cfind( int daddr7, uint16_t * addr ) {
  *addr= 0xFFFF;
  if( addr==0 ) return aoresult_outargnull;
  for( uint16_t iix=0; iix<aomw_topo_numi2cbridges_; iix++ ) {
    uint16_t ad= aomw_topo_i2cbridge_addr_[iix];
    uint8_t buf[8];
    aoresult_t result = aoosp_exec_i2cread8(ad, daddr7, 0x00, buf, 1);
    int i2cfail=  result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout;
    if( result!=aoresult_ok && !i2cfail ) return result;
    if( !i2cfail ) { *addr=ad; return aoresult_ok; }
  }
  return aoresult_dev_noi2cdev;
}



// === command handler =======================================================


// Show the dim level (with brightness)
static void aomw_topo_dim_show(  ) {
  int dim= aomw_topo_dim_get();
  int said = 24/12 * (1024+dim/2)/dim;
  int rgbi = 50/10 * (1024+dim/2)/dim;
  Serial.printf("dim %d/1024 (said %dx, rgbi %dx below max power)\n", dim, said, rgbi );
}


// The handler for the "topo" command
static void aomw_topo_cmd( int argc, char * argv[] ) {
  if( argc>1 && aocmd_cint_isprefix("build",argv[1]) ) {
    if( argc!=2 ) { Serial.printf("ERROR: 'build' has too many args\n" ); return; }
    aoresult_t result= aomw_topo_build();
    if( result!=aoresult_ok ) { Serial.printf("ERROR: 'build' failed (%s)\n",aoresult_to_str(result,1) ); return; }
    if( argv[0][0]!='@' ) { aomw_topo_dump_summary(); return; };
    return;
  }

  if( aomw_topo_numnodes()==0 ) Serial.printf("WARNING: 'topo build' must be run first\n"); 
  
  if( argc==1 ) {
    if( argv[0][0]!='@' ) aomw_topo_dump_nodes(); 
    aomw_topo_dump_summary();
    return;
  } else if( aocmd_cint_isprefix("enum",argv[1]) ) {
    if( argc!=2 ) { Serial.printf("ERROR: 'enum' has too many args\n" ); return; }
    aomw_topo_dump_nodes();
    aomw_topo_dump_triplets();
    aomw_topo_dump_i2cbridges();
    aomw_topo_dump_summary();
    return;
  } else if( aocmd_cint_isprefix("dim",argv[1]) ) {
    if( argc==2 ) { aomw_topo_dim_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'dim' expects <level>\n" ); return; }
    int level;
    bool ok= aocmd_cint_parse_dec(argv[2],&level) ;
    if( !ok || level<0 || level>1024 ) { Serial.printf("ERROR: 'dim' expects <level> (0..1024), not '%s'\n",argv[2] ); return; }
    aomw_topo_dim_set(level);
    if( argv[0][0]!='@' ) aomw_topo_dim_show();
    return;
  } else if( aocmd_cint_isprefix("pwm",argv[1]) ) {
    if( argc<3 ) { Serial.printf("ERROR: 'pwm' expects <tix>\n" ); return; }
    if( aomw_topo_numtriplets()==0 ) Serial.printf("WARNING: forgot 'topo build'?\n" );
    int tix;
    bool ok= aocmd_cint_parse_dec(argv[2],&tix) ;
    if( !ok || tix<0 || tix>=aomw_topo_numtriplets() ) { Serial.printf("ERROR: 'pwm' expects <tix> 0..%d, not %d\n", aomw_topo_numtriplets()-1, tix ); return; }
    if( argc!=6 ) { Serial.printf("ERROR: expected <red> <green> <blue>\n" ); return; }
    aomw_topo_rgb_t rgb;
    ok= aocmd_cint_parse_hex(argv[3],&rgb.r) ;
    if( !ok || rgb.r > AOMW_TOPO_BRIGHTNESS_MAX ) { Serial.printf("ERROR: 'pwm' expects <red> 0..%04X, not '%s'\n", AOMW_TOPO_BRIGHTNESS_MAX, argv[3] ); return; }
    ok= aocmd_cint_parse_hex(argv[4],&rgb.g) ;
    if( !ok || rgb.g > AOMW_TOPO_BRIGHTNESS_MAX ) { Serial.printf("ERROR: 'pwm' expects <green> 0..%04X, not '%s'\n", AOMW_TOPO_BRIGHTNESS_MAX, argv[4] ); return; }
    ok= aocmd_cint_parse_hex(argv[5],&rgb.b) ;
    if( !ok || rgb.b > AOMW_TOPO_BRIGHTNESS_MAX ) { Serial.printf("ERROR: 'pwm' expects <blue> 0..%04X, not '%s'\n", AOMW_TOPO_BRIGHTNESS_MAX, argv[5] ); return; }
    // send telegram
    aoresult_t result= aomw_topo_settriplet(tix,&rgb);
    if( result!=aoresult_ok ) { Serial.printf("ERROR: 'pwm' failed (%s)\n",aoresult_to_str(result,1) ); return; }
    if( argv[0][0]!='@' ) Serial.printf("pwm T%d: %04X %04X %04X\n",tix,rgb.r, rgb.g, rgb.b);
    return;
  } else {
    Serial.printf("ERROR: 'topo' has unknown argument ('%s')\n", argv[1]); return;
  }
}


// The long help text for the "topo" command.
static const char aomw_topo_cmd_longhelp[] = 
  "SYNTAX: topo build\n"
  "- this resets, inits and scans all nodes on the chain creating the map\n"
  "SYNTAX: topo [enum]\n"
  "- without argument, enumerates nodes (the topology map)\n"
  "- with argument, also enumerates triplets and i2c bridges\n"
  "SYNTAX: topo dim [ <level> ]\n"
  "- without argument, shows current global dim level\n"
  "- with argument sets global dim level (0..1024)\n"
  "- only affects newly controlled triplets\n"
  "SYNTAX: topo pwm <tix>  <red> <green> <blue>\n"
  "- sets the pwm settings of RGB triplet <tix> (decimal)\n"
  "- <red> <green> <blue> are each 15 bits hex (0000..7FFF)\n"
  "- the 'topo dim' level is applied\n"
  "NOTES:\n"
  "- a topology map tells which node types are at which address\n"
  "- the topology map must first be 'build' before any other 'topo' command\n"
  "- it shows how many RGB triplets and I2C bridges are connected to each node\n"
  "- supports @-prefix to suppress output\n"
;


/*!
    @brief  Registers the "topo" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
*/
int aomw_topo_cmd_register() {
  return aocmd_cint_register(aomw_topo_cmd, "topo", "build, query and use topology", aomw_topo_cmd_longhelp);
}

