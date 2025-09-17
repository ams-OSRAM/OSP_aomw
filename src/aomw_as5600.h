// aomw_as5600.h - driver for ams-OSRAM AS5600 magnetic rotary sensor
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
#ifndef _AOMW_AS5600_H_
#define _AOMW_AS5600_H_


#include <stdint.h>     // uint16_t
#include <aoresult.h>   // aoresult_t


// I2C address of the sensor
#define AOMW_AS5600_DADDR7_SAIDSENSE  0x36


// Max values
#define AOMW_AS5600_ANGLE_MAX         4095
#define AOMW_AS5600_AGC_MAX            128 // at 3V3
#define AOMW_AS5600_MAG_MAX           4095


// Reads and returns the angle measured by the AS5600 rotary sensor.
aoresult_t aomw_as5600_angle_get(int*angle);
aoresult_t aomw_as5600_force_get(int*agc, int*mag=0);


// Tests if an AS5600 is connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_as5600_present(uint16_t addr);
// Associates this software driver to the AS5600 connected to the I2C bus of OSP node (SAID) with address `addr`.
aoresult_t aomw_as5600_init(uint16_t addr);


#endif

