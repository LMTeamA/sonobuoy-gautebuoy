/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2011-09-07
 *
 * Communication protocol to Zero manager over RF.
 *
 */

# ifndef RF_C
# define RF_C

# include "buoy.h"
# include "store.h"
# include "rf.h"
# include "ads1282.h"

void rf_setup ()
{
  /* Setting up Serial interface to RF */
  RF_Serial.begin(RF_BAUDRATE);

  /* Send greeting */
  rf_send_debug (GREETING);
}

/* Protocol
 *
 * Telegram consists of:
 * $Type,values,values,values*Checksum
 *
 * Type is one of:
 *  - AD    AD data and status messages
 *  - GPS   GPS position and time data
 *  - DBG   Debug message
 *
 * After * checksum is computed as XOR of all values
 * between, and not including, $ and *. Two hexadecimal digits.
 *
 */


void rf_loop ()
{

}

void rf_send_status ()
{
  rf_ad_message (AD_STATUS);
  rf_gps_message (GPS_STATUS);
}

void rf_send_debug (const char * msg)
{
  /* Format:
   * $DBG,[msg]*CS
   *
   */

  char buf[RF_BUFLEN];
  sprintf(buf, "$DBG,%s*", msg);
  APPEND_CSUM (buf);

  RF_Serial.println (buf);
}

void rf_ad_message (RF_AD_MESSAGE messagetype)
{
  char buf[RF_BUFLEN];

  switch (messagetype)
  {
    case AD_STATUS:
      // $AD,S,[queue position], [queue fill time],[value],[config]*CS
      sprintf (buf, "$AD,S,%u,%lu,0x%02X%02X%02X,0x%02X%02X%02X*", ad_qposition, ad_queue_time, ad_value[0], ad_value[1], ad_value[2], ad_config[0], ad_config[1], ad_config[2]);
      APPEND_CSUM (buf);

      RF_Serial.println (buf);

      break;

    case AD_DATA_BATCH:
      /* Send AD_DATA_BATCH_LEN samples */
      # define AD_DATA_BATCH_LEN (AD_QUEUE_LENGTH / 2)

      /* Format:

       * 1. Initiate binary data stream:

       $AD,D,[k = number of samples],[reference]*CC

       * 2. Send one $ to indicate start of data

       * 3. Send k number of samples: 3 bytes * k

       * 4. Send k number of timestamps: 4 bytes * k

       * 4. Send end of data with checksum

       */
      {
        int start = (batchready == 1 ? 0 : AD_DATA_BATCH_LEN);

        int n = sprintf (buf, "$AD,D,%d,%lu*", AD_DATA_BATCH_LEN, referencesecond);
        APPEND_CSUM (buf);
        RF_Serial.println (buf);

        delayMicroseconds (100);

        byte csum = 0;

        /* Write '$' to signal start of binary data */
        RF_Serial.write ('$');

        sample lasts;
        sample s;

        for (int i = 0; i < AD_DATA_BATCH_LEN; i++)
        {
          memcpy (s, (const void *) ad_queue[start + i], 3);
          /* MSB first (big endian), means concatenating bytes on RX will
           * result in LSB first; little endian. */
          RF_Serial.write (s, 3);

          csum = csum ^ s[0];
          csum = csum ^ s[1];
          csum = csum ^ s[2];

          memcpy (lasts, s, 3);

          delayMicroseconds (100);
        }

        /* Send time stamps */
        ulong t = 0;
        for (int i = 0; i < AD_DATA_BATCH_LEN; i++)
        {
          t = ad_time[start + i];

          /* Writes MSB first */
          RF_Serial.write ((byte*)(&t), 4);

          csum = csum ^ ((byte*)&t)[0];
          csum = csum ^ ((byte*)&t)[1];
          csum = csum ^ ((byte*)&t)[2];
          csum = csum ^ ((byte*)&t)[3];
        }

        /* Send end of data with Checksum */
        sprintf (buf, "$AD,DE," F_CSUM "*", csum);
        APPEND_CSUM (buf);
        RF_Serial.println (buf);
        delayMicroseconds (100);
      }
      break;

    default:
      return;
  }
}

void rf_gps_message (RF_GPS_MESSAGE messagetype)
{
  char buf[RF_BUFLEN];

  switch (messagetype)
  {
    case GPS_STATUS:
      // $GPS,S,[lasttype],[telegrams received],[lasttelegram],Lat,Lon,unixtime,time,date,Valid,HAS_TIME,HAS_SYNC,HAS_SYNC_REFERENCE*CS
      // Valid: Y = Yes, N = No
      sprintf (buf, "$GPS,S,%d,%d,%s,%c,%s,%c,%lu,%lu,%02d%02d%02d,%c,%c,%c,%c*", gps_data.lasttype, gps_data.received, gps_data.latitude, (gps_data.north ? 'N' : 'S'), gps_data.longitude, (gps_data.east ? 'E' : 'W'), lastsecond, gps_data.time, gps_data.day, gps_data.month, gps_data.year, (gps_data.valid ? 'Y' : 'N'), (HAS_TIME ? 'Y' : 'N'), (HAS_SYNC ? 'Y' : 'N'), (HAS_SYNC_REFERENCE ? 'Y' : 'N'));

      break;

    default:
      return;
  }

  APPEND_CSUM (buf);
  RF_Serial.println (buf);
}


byte gen_checksum (char *buf)
{
/* Generate checksum for NULL terminated string
 * (skipping first and last char) */

  byte csum = 0;
  int len = strlen(buf);

  for (int i = 1; i < (len-1); i++)
    csum = csum ^ ((byte)buf[i]);

  return csum;
}

bool test_checksum (char *buf)
{
  /* Input: String including $ and * with HEX decimal checksum
   *        to test. NULL terminated.
   */
  int len = strlen(buf);

  uint csum = 0;
  sscanf (&(buf[len-2]), F_CSUM, &csum);

  ulong tsum = 0;
  for (int i = 1; i < (len - 3); i++)
    tsum = tsum ^ (uint)buf[i];

  return tsum == csum;
}

# endif

/* vim: set filetype=arduino :  */

