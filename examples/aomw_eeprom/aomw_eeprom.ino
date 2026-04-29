// aomw_eeprom.ino - demonstrates reading writing eeprom
/*****************************************************************************
 * Copyright 2024-2026 by ams OSRAM AG                                       *
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
#include <aospi.h>
#include <aoosp.h>
#include <aomw.h>


/*
DESCRIPTION
This demo initializes an OSP chain, powers the I2C bridge in a SAID
and checks whether there is an EEPROM. If so, reads and prints the
entire EEPROM contents, then modifies one row and then restores that.
It finalizes by doing a compare of the current EEPROM content with
the values read at the start.

HARDWARE
The demo runs on the OSP32 board. It has a built-in EEPROM.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Manipulates EEPROM, no LED control.
Notice on Serial how the EEPROM is changed (row 20).

OUTPUT
Welcome to aomw_eeprom.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init
demo: init

Using SAID 001 EEPROM 54

Read original content
00: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
10: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
20: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF <<<
30: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
40: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
50: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
60: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
70: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
80: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
a0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
b0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
c0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
d0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
e0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
f0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

Overwrite row 20 with 00..0F
00: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
10: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
20: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F <<<
30: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
40: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
50: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
60: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
70: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
80: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
a0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
b0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
c0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
d0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
e0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
f0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

Restore row 20
00: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
10: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
20: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF <<<
30: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
40: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
50: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
60: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
70: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
80: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
a0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
b0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
c0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
d0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
e0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
f0: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

Successfully restored
*/


// The address of the SAID that has the I2C bridge
#define ADDR   0x001 // The OSP32 board has an built-in EEPROM attached to SAID 001
// The I2C address of EEPROM memory
#define DADDR7 AOMW_EEPROM_DADDR7_OSP32


// Lazy way of error handling
#define PRINT_ERROR() do { if( result!=aoresult_ok ) { Serial.printf("ERROR %s\n", aoresult_to_str(result) ); } } while(0)


void demo_init() {
  // (1) initialize the OSP chain
  aoresult_t result;
  result = aoosp_exec_resetinit(); PRINT_ERROR();

  // (2patch) In case the SAID does not have the I2C_BRIDGE_EN, we could try to override this in RAM
  // result= aoosp_exec_i2cenable_set(ADDR,1); PRINT_ERROR();

  // (2) check if SAID has I2C bridge
  int enable;
  result= aoosp_exec_i2cenable_get(ADDR, &enable); PRINT_ERROR();
  if( !enable ) result= aoresult_dev_noi2cbridge; PRINT_ERROR();

  // (3) power the I2C bridge in a SAID
  result= aoosp_exec_i2cpower(ADDR); PRINT_ERROR();

  // (4) check wether there is an EEPROM
  result= aomw_eeprom_present(ADDR,DADDR7); PRINT_ERROR();

  Serial.printf("demo: init\n");
}


void demo_dump(int row) {
  uint8_t buf[256];
  aoresult_t result= aomw_eeprom_read(ADDR, DADDR7, 0x00, buf, 256 ); PRINT_ERROR();
  for( int i=0; i<256; i+=16 ) Serial.printf("%02x: %s%s\n", i, aoosp_prt_bytes(&buf[i],16),i==row?" <<<":"");
}


#define ROWIX 0x20
#define ROWSZ 0x10
void demo_readwrite() {
  aoresult_t result;

  Serial.printf("\nUsing SAID %03X EEPROM %02x\n",ADDR, DADDR7);

  Serial.printf("\nRead original content\n");
  uint8_t buf1[ROWSZ];
  result= aomw_eeprom_read(ADDR, DADDR7, ROWIX, buf1, ROWSZ ); PRINT_ERROR();
  // Show original contents
  demo_dump(ROWIX);

  Serial.printf("\nOverwrite row %02x with 00..0F\n",ROWIX);
  uint8_t buf2[ROWSZ] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
  result= aomw_eeprom_write(ADDR, DADDR7, ROWIX, buf2, ROWSZ ); PRINT_ERROR();
  demo_dump(ROWIX);

  Serial.printf("\nRestore row %02x\n",ROWIX);
  result= aomw_eeprom_write(ADDR, DADDR7, ROWIX, buf1, ROWSZ ); PRINT_ERROR();
  // Show restored contents
  demo_dump(ROWIX);

  // Compare original with current (prints aoresult_comparefail when not equal)
  result= aomw_eeprom_compare(ADDR, DADDR7, ROWIX, buf1, ROWSZ ); PRINT_ERROR();
  if(result==aoresult_ok) Serial.printf("\nSuccessfully restored\n");

}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_eeprom.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  demo_init();

  demo_readwrite();
}


void loop() {
  delay(5000);
}

