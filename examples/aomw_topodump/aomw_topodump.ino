// aomw_topodump.ino - dumps tables build by the topo scanner (which OSP nodes at which address)
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
This demo scans the OSP chain using the topo builder from the middleware to 
form a topology map of all nodes of the OSP chain. Next, it prints out the 
chain configuration: nodes, triplets, i2cbridges.
It does feature some time measurement code. See the comment at
the end of this sketch for an analysis of the measurements.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached, but 
the output shown below is when the SAIDbasic board is connected.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
Only scans the change, does not activate LEDs.

OUTPUT
Welcome to aomw_topodump.ino
version: result 0.4.1, spi 0.5.1, osp 0.4.1, mw 0.4.0
spi: init
osp: init
mw: init

time 5502us commands 42 responses 15
nodes(N) 1..9, triplets(T) 0..16, i2cbridges(I) 0..1, dir loop
N001 (00000040) T0 T1 I0
N002 (00000040) T2 T3 T4
N003 (00000040) T5 T6 T7
N004 (00000000) T8
N005 (00000040) T9 T10 I1
N006 (00000000) T11
N007 (00000000) T12
N008 (00000000) T13
N009 (00000040) T14 T15 T16
*/


uint32_t time0us,time1us;
void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aomw_topodump.ino\n");
  Serial.printf("version: result %s, spi %s, osp %s, mw %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOMW_VERSION );

  aospi_init();
  aoosp_init();
  aomw_init();
  Serial.printf("\n");

  aospi_txcount_reset();
  aospi_rxcount_reset();
  // aoosp_loglevel_set(aoosp_loglevel_tele); // enabling log, breaks time measurement

  time0us= micros();
  aomw_topo_build_start();
}


int report;
void loop() {
  if( !aomw_topo_build_done() ) aomw_topo_build_step(); 

  if( aomw_topo_build_done() ) {
    if( !report ) { 
      time1us= micros();
      Serial.printf("time %luus commands %d responses %d\n",time1us-time0us,aospi_txcount_get(), aospi_rxcount_get() );
      aomw_topo_dump_summary(); aomw_topo_dump_nodes(); 
      report=1; }
  }
}


/* 
When loging is enabled (aoosp_loglevel_set), we get the below output.
The first (hand) prepended number is the tx count, the second (hand) prepended number is the rx count.

 0      reset(0x000) [tele A0 00 00 22]
 1   0  initloop(0x001) [tele A0 04 03 86] -> [resp A0 25 03 00 50 3E] last=0x09=9 temp=0x00=-86 stat=0x50=SLEEP:tV:clou (-126, SLEEP:oL:clou)
 2   1  identify(0x001) [tele A0 04 07 3A] -> [resp A0 06 07 00 00 00 40 AA] id=0x00000040
 3   2  readotp(0x001,0x0D) [tele A0 04 D8 0D E2] -> [resp A0 07 D8 00 00 00 00 00 00 00 08 B7] otp 0x0D: 08 00 00 00 00 00 00 00
 4   3  identify(0x002) [tele A0 08 07 6A] -> [resp A0 0A 07 00 00 00 40 2B] id=0x00000040
 5   4  readotp(0x002,0x0D) [tele A0 08 D8 0D AA] -> [resp A0 0B D8 00 00 00 00 00 00 00 10 3A] otp 0x0D: 10 00 00 00 00 00 00 00
 6   5  identify(0x003) [tele A0 0C 07 BF] -> [resp A0 0E 07 00 00 00 40 54] id=0x00000040
 7   6  readotp(0x003,0x0D) [tele A0 0C D8 0D 92] -> [resp A0 0F D8 00 00 00 00 00 00 00 00 B8] otp 0x0D: 00 00 00 00 00 00 00 00
 8   7  identify(0x004) [tele A0 10 07 CA] -> [resp A0 12 07 00 00 00 00 E0] id=0x00000000
 9   8  identify(0x005) [tele A0 14 07 1F] -> [resp A0 16 07 00 00 00 40 79] id=0x00000040
10   9  readotp(0x005,0x0D) [tele A0 14 D8 0D 02] -> [resp A0 17 D8 00 00 00 00 00 00 00 01 7F] otp 0x0D: 01 00 00 00 00 00 00 00
11  10  identify(0x006) [tele A0 18 07 4F] -> [resp A0 1A 07 00 00 00 00 1E] id=0x00000000
12  11  identify(0x007) [tele A0 1C 07 9A] -> [resp A0 1E 07 00 00 00 00 61] id=0x00000000
13  12  identify(0x008) [tele A0 20 07 A5] -> [resp A0 22 07 00 00 00 00 BA] id=0x00000000
14  13  identify(0x009) [tele A0 24 07 70] -> [resp A0 26 07 00 00 00 40 23] id=0x00000040
15  14  readotp(0x009,0x0D) [tele A0 24 D8 0D 0D] -> [resp A0 27 D8 00 00 00 00 00 00 00 08 F8] otp 0x0D: 08 00 00 00 00 00 00 00
16      clrerror(0x000) [tele A0 00 01 0D]
17      setsetup(0x001,0x33) [tele A0 04 CD 33 93]
18      setsetup(0x002,0x33) [tele A0 08 CD 33 DB]
19      setsetup(0x003,0x33) [tele A0 0C CD 33 E3]
20      setsetup(0x004,0x33) [tele A0 10 CD 33 4B]
21      setsetup(0x005,0x33) [tele A0 14 CD 33 73]
22      setsetup(0x006,0x33) [tele A0 18 CD 33 3B]
23      setsetup(0x007,0x33) [tele A0 1C CD 33 03]
24      setsetup(0x008,0x33) [tele A0 20 CD 33 44]
25      setsetup(0x009,0x33) [tele A0 24 CD 33 7C]
26      setcurchn(0x005,2,AOOSP_CURCHN_FLAGS_DITHER,4,4,4) [tele A0 15 D1 02 04 44 0B]
27      setcurchn(0x001,0,AOOSP_CURCHN_FLAGS_DITHER,2,2,2) [tele A0 05 D1 00 12 22 B4]
28      setcurchn(0x001,1,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 05 D1 01 13 33 D2]
29      setcurchn(0x001,2,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 05 D1 02 13 33 C0]
30      setcurchn(0x002,0,AOOSP_CURCHN_FLAGS_DITHER,2,2,2) [tele A0 09 D1 00 12 22 A5]
31      setcurchn(0x002,1,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 09 D1 01 13 33 C3]
32      setcurchn(0x002,2,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 09 D1 02 13 33 D1]
33      setcurchn(0x003,0,AOOSP_CURCHN_FLAGS_DITHER,2,2,2) [tele A0 0D D1 00 12 22 4F]
34      setcurchn(0x003,1,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 0D D1 01 13 33 29]
35      setcurchn(0x003,2,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 0D D1 02 13 33 3B]
36      setcurchn(0x005,0,AOOSP_CURCHN_FLAGS_DITHER,2,2,2) [tele A0 15 D1 00 12 22 6D]
37      setcurchn(0x005,1,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 15 D1 01 13 33 0B]
38      setcurchn(0x009,0,AOOSP_CURCHN_FLAGS_DITHER,2,2,2) [tele A0 25 D1 00 12 22 29]
39      setcurchn(0x009,1,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 25 D1 01 13 33 4F]
40      setcurchn(0x009,2,AOOSP_CURCHN_FLAGS_DITHER,3,3,3) [tele A0 25 D1 02 13 33 5D]
41      goactive(0x000) [tele A0 00 05 B1]

These counts is confirmed by the first line of the report
  time 5450us commands 42 responses 15

In total 42 commands were sent, and 15 responses received, or 57 telegrams in total.
At 2.4MHz, one 12 bytes (96 bit) telegram takes 40us, so 57 would take 2280us.
The topo build state machine sends these in 5450us, twice longer.

We did overestimate payload size (always 12 bytes), but we did not take into account 
inter telegram time, telegram forwarding time (along the chain) or telegram 
execution time (reset, otp).

We have also tried to send those 42+15 telegrams directly on aoosp level and even 
on aispi level (see aoosp-time.ino). We see that the software overhead can be ignored.
  test_spi  : time 4961us commands 42 responses 15
  test_osp  : time 4993us commands 42 responses 15
  topo build: time 5260us commands 42 responses 15
(the topo build is lower here, this is probably due to ESP caches being filled).

*/
