// aomw_sfh5721.h - driver for ams-OSRAM SFH 5721 ambient light sensor
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
#ifndef _AOMW_SFH5721_H_
#define _AOMW_SFH5721_H_


#include <stdint.h>     // uint16_t
#include <aoresult.h>   // aoresult_t


// I2C address of the light sensor
#define AOMW_SFH5721_DADDR7_0_SAIDSENSE  0x26 // ADDR pin to GND
#define AOMW_SFH5721_DADDR7_1_SAIDSENSE  0x27 // ADDR pin to VDD
#define AOMW_SFH5721_DADDR7_SAIDSENSE    AOMW_SFH5721_DADDR7_0_SAIDSENSE


// Reads and returns the ambient light level by the SFH5721 light sensor.
aoresult_t aomw_sfh5721_als_get(int*als);


// Tests if an SFH5721 is connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_sfh5721_present(uint16_t addr);
// Associates this software driver to the SFH5721 connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_sfh5721_init(uint16_t addr);


#endif







