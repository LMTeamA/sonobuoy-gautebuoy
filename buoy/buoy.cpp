/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-01-18
 *
 * Buoy controller.
 *
 */

# include <stdint.h>
# include <stdlib.h>

# include "buoy.h"

# include "wirish.h"

# include "ads1282.h"
# include "rf.h"
# include "gps.h"
# include "store.h"

namespace Buoy {
  void BuoyMaster::main () {
    setup ();

    uint32_t lasts = 0;

    while (true) {
      gps->loop ();
      ad->loop ();
      store->loop ();
      rf->loop ();

      if (millis () - lasts >= 1000) {
        SerialUSB.print ("V ");
        SerialUSB.println (ad->value);
        //ad->print_status ();
        //gps->print_status ();
        lasts = millis ();
      }
    }
  }

  void BuoyMaster::setup () {
    /* Configure LED */
    //pinMode (BOARD_LED_PIN, OUTPUT);
    //digitalWrite (BOARD_LED_PIN, LOW);

    //SerialUSB.begin ();

    pinMode (13, OUTPUT);
    pinMode (3, OUTPUT);
    digitalWrite (3, LOW);
    /* Count down.. */

    for (int i = 0; i < 3; i++) {
      SerialUSB.print ("Starting soon: ");
      SerialUSB.println (i);
      delay(1000);
      //togglePin (3);
    }


    //SerialUSB.println ("[**] Gautebøye 1 [" BUOY_NAME "] version: " GIT_DESC);

    /* Set up devices */
    rf    = new RF ();
    gps   = new GPS ();
    store = new Store ();
    ad    = new ADS1282 ();

    rf->setup     (this);
    gps->setup    (this);
    ad->setup     (this);
    store->setup  (this);

# if DEBUG_INFO
    /* Start reading data continuously and writing to SD card */
    SerialUSB.println ("[Buoy] Initiating continuous transfer and write.");
# endif

    store->start_continuous_write ();
    ad->start_continuous_read ();
  }

  int itoa (uint32_t n, uint8_t base, char *buf) // {{{
  {
    unsigned long i = 0;

    if (n == 0) {
      buf[i++] = '0';
    }

    while (n > 0) {
      buf[i] = (n % base);
      buf[i] += (buf[i] < 10 ? '0' : 'A');
      n /= base;
      i++;
    }

    /* swap */
    i--;
    char c;
    while (n <= (i/2)) {
      c         = buf[i-n];
      buf[i-n]  = buf[n];
      buf[n]    = c;

      n++;
    }

    buf[++i] = 0;
    return i;
  } // }}}

  /* Checksum {{{ */
  byte gen_checksum (const char *buf)
  {
  /* Generate checksum for NULL terminated string */

    byte csum = 0;
    buf++; // skip $

    while (*buf != '*' && *buf != 0) {
      csum = csum ^ ((byte)*buf);
      buf++;
    }

    return csum;
  }

  bool test_checksum (const char *buf)
  {
    /* Input: String including $ and * with HEX decimal checksum
     *        to test. NULL terminated.
     */
    uint32_t tsum = 0;
    buf++; // skip $
    while (*buf != '*' && *buf != 0) {
      tsum = tsum ^ (uint8_t)*buf;
      buf++;
    }
    buf++;

    uint16_t csum = 0;
    csum = strtoul (buf, NULL, 16); // buf now points to first digit of CS

    return tsum == csum;
  } // }}}
}

/* vim: set filetype=arduino :  */

