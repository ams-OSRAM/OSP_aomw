// aomw_as6212.h - driver for ams-OSRAM AS6212 temperature sensor
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
#ifndef _AOMW_AS6212_H_
#define _AOMW_AS6212_H_


#include <stdint.h>     // uint16_t
#include <aoresult.h>   // aoresult_t


// I2C address of the temperature sensor
#define AOMW_AS6212_DADDR7_SAIDSENSE    0x48


// Configures the conversion rate of the AS6212 temperature sensor.
aoresult_t aomw_as6212_convrate_set(int ms);
// Reads and returns the conversion rate of the AS6212 temperature sensor.
aoresult_t aomw_as6212_convrate_get(int *ms);
// Reads and returns the temperature measured by the AS6212 temperature sensor.
aoresult_t aomw_as6212_temp_get(int*millicelsius);


// Tests if an AS6212 is connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_as6212_present(uint16_t addr);
// Associates this software driver to the AS6212 connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_as6212_init(uint16_t addr);


#endif







