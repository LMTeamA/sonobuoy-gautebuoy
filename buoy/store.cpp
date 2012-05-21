/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-02-05
 *
 * SD store
 *
 */

# include "buoy.h"

# include <string.h>

# include "wirish.h"
# include "HardwareSPI.h"

# include "store.h"
# include "ads1282.h"

# if HASRF
  # include "rf.h"
# endif
# if HASGPS
  # include "gps.h"
# endif

namespace Buoy {
  Store::Store () {
    logf_id = 0;
    card    = NULL;
    volume  = NULL;
    root    = NULL;
    sd_data = NULL;

    s_id = 0;
    s_samples = 0;
    s_nrefs = 0;
    s_lastbatch = 0;
    send_i = NULL;
    send_d = NULL;

  }

  void Store::setup (BuoyMaster *b) // {{{
  {
# if DEBUG_VERB
    SerialUSB.println ("[SD] Setup.");
# endif
# if HASRF
    rf  = b->rf;
# endif
    ad  = b->ad;

    spi = new HardwareSPI(SD_SPI);

    lastsd    = millis ();
    lastbatch = 0;
    continuous_write = false;
# if HASRF
    s_lastbatch = 0;
# endif

    /* Initialize card */
    init ();
  } // }}}

  void Store::init () // {{{
  {
# if DEBUG_VERB
    SerialUSB.println ("[SD] Init SD card.");
# endif

    card    = new Sd2Card ();
    volume  = new SdVolume ();
    root    = new SdFile ();
    sd_data = new SdFile ();

    /* Initialize SD card */
    spi->begin (SPI_281_250KHZ, MSBFIRST, 0);
    SD_AVAILABLE = card->init (spi, SD_CS);

    /*
    if (SD_AVAILABLE)
      SD_AVAILABLE = card->cardSize() > 0;
    */

    if (SD_AVAILABLE)
      SD_AVAILABLE = (card->errorCode () == 0);

    /* Beef up SPI speed after init is finished */
    if (SD_AVAILABLE)
      spi->begin (SPI_4_5MHZ, MSBFIRST, 0);

    /* Initialize FAT volume */
    if (SD_AVAILABLE)
      SD_AVAILABLE = volume->init (card, 1);

    /* Open root directory */
    if (SD_AVAILABLE)
      SD_AVAILABLE = root->openRoot (volume);

    if (SD_AVAILABLE)
    {
# if DEBUG_VERB
      SerialUSB.println ("[SD] Card initialized.");
# endif

      open_index ();
      open_data ();

      //open_next_log ();

    } else {
# if DEBUG_VERB
      SerialUSB.println ("[SD] Could not initialize SD card.");
# endif
      current_index.id = 0;
    }
  } // }}}

  void Store::open_index () // {{{
  {
    if (!SD_AVAILABLE) return;

    uint32_t n = 0;

    uint32_t i;
    SdFile fl;

    if (fl.open (root, "LASTID.LON", O_READ)) {

      n = fl.read (reinterpret_cast<char*>(&i), sizeof(uint32_t));
      if (n < sizeof(i)) i = 0;
      fl.close ();

    } else {
      i = 0;
    }

# if DEBUG_VERB
    SerialUSB.print ("[SD] Last id: ");
    SerialUSB.println (i);
# endif
    SD_AVAILABLE &= (card->errorCode () == 0);
    next_index (i + 1);
  } // }}}

  /* Log to file {{{ */
# if 0
  void Store::open_next_log ()
  {
    sprintf (buf, "%lu.LOG", logf_id);

    while (logf.open (root, buf, O_READ)) {
      logf.close ();
      logf_id++;
      sprintf (buf, "%lu.LOG", logf_id);
    }
    logf.close ();

    logf.open (root, buf, O_READ | O_CREAT | O_WRITE);
    rf_send_debug_f ("[SD] Opened log: %lu.LOG", logf_id);
  }

  void Store::log (const char *s)
  {
    if (SD_AVAILABLE) {
      /* Open next log file */
      if (logf.curPosition () > MAX_LOG_SIZE) {
        logf.close ();
        open_next_log ();
      }

      if (!SD_AVAILABLE) return;

      sprintf (buf, "[%lu]", (uint32_t) gps->lastsecond);
      logf.write (buf);
      logf.write (s);
      logf.write ('\n');
    }
  }
# endif // }}}

  void Store::next_index (uint32_t i) // {{{
  {
    /* Open next index file */
    if (!SD_AVAILABLE) return;
    // i is LASTID or LASTID + 1

    // TODO: Handle better MAXID

    if (i > MAXID ) {
# if DEBUG_VERB
    SerialUSB.print ("[SD] Reached maximum ID:");
    SerialUSB.println (i);
    SerialUSB.println (MAXID);
# endif
      i = 1;  // DEBUG
    }

    /* Walk through subsequent indexes above lastid and take next free */
    bool newi = false;

# if DEBUG_VERB
    SerialUSB.print ("[SD] Checking for subseq. indexes from: ");
    SerialUSB.println (i);
# endif

    SdFile fi;

    while (!newi)
    {
      int p = itoa (i, 10, buf);
      strcpy (&(buf[p]), ".IND");
      //sprintf (buf, "%lu.IND", i);

      if (!fi.open(root, buf, O_READ)) {
        newi = true; /* Found new index file at id I */

# if DEBUG_VERB
        SerialUSB.print ("[SD] Found free index at: ");
        SerialUSB.println (i);
# endif
# if HASRF
        rf_send_debug_f ("[SD] Next index: %lu", i);
# endif

      } else {
        fi.close ();
      }
      i++;

      // TODO: Check if trying to open non-existent file sets errorCode()
      SD_AVAILABLE &= (card->errorCode () == 0);
      if (!SD_AVAILABLE) return;
    }
    i--;

    // Open new index
    current_index.version = STORE_VERSION;
    current_index.id = i;
    current_index.sample_l = SAMPLE_LENGTH;
    current_index.samples = 0;
    current_index.samples_per_reference = BATCH_LENGTH;
    current_index.nrefs = 0;

    SD_AVAILABLE &= (card->errorCode () == 0);
    write_index ();
  } // }}}

  void Store::write_index ()  // {{{
  {
    if (!SD_AVAILABLE) return;

# if DEBUG_VERB
    SerialUSB.print   ("[SD] Writing index: ");
    SerialUSB.println (current_index.id);
# endif

    if (current_index.id != 0) {
      int p = itoa (current_index.id, 10, buf);
      strcpy (&(buf[p]), ".IND");
      //sprintf (buf, "%lu.IND", current_index.id);

      SdFile fi;
      fi.open (root, buf, O_CREAT | O_WRITE | O_TRUNC);
      fi.write (reinterpret_cast<char*>(&current_index.version), sizeof(current_index.version));
      fi.write (reinterpret_cast<char*>(&current_index.id), sizeof(current_index.id));
      fi.write (reinterpret_cast<char*>(&current_index.sample_l), sizeof(current_index.sample_l));
      fi.write (reinterpret_cast<char*>(&current_index.samples), sizeof(current_index.samples));
      fi.write (reinterpret_cast<char*>(&current_index.samples_per_reference), sizeof(current_index.samples_per_reference));
      fi.write (reinterpret_cast<char*>(&current_index.nrefs), sizeof(current_index.nrefs));

      for (uint32_t i = 0; i < current_index.nrefs; i++)
        fi.write (reinterpret_cast<char*>(&(current_index.refpos[i])), sizeof(uint32_t));

      for (uint32_t i = 0; i < current_index.nrefs; i++)
        fi.write (reinterpret_cast<char*>(&(current_index.refs[i])), sizeof(uint64_t));

      fi.sync ();
      fi.close ();

      /* Write back last index */
      SdFile fl;
      fl.open (root, "LASTID.LON", O_CREAT | O_WRITE | O_TRUNC);
      fl.write (reinterpret_cast<char*>(&(current_index.id)), sizeof(current_index.id));
      fl.sync ();
      fl.close ();
    }

    SD_AVAILABLE &= (card->errorCode () == 0);
  } // }}}

  void Store::roll_data_file () // {{{
  {
    /* Open new index and data file */
    if (!SD_AVAILABLE) return;
# if DEBUG_VERB
    SerialUSB.println ("[SD] Syncing index and data and rolling..");
# endif

    /* Truncate data file to actual size */
    //sd_data.truncate (sd_data.curPosition ());

    sd_data->sync ();
    sd_data->close ();
    SD_AVAILABLE &= (card->errorCode () == 0);

    /* Write current index */
    write_index ();

    /* Open new index */
    next_index (current_index.id + 1);

    /* Open new data file */
    open_data ();
  } // }}}

  void Store::write_batch () // {{{
  {
# if DEBUG_VERB
    SerialUSB.println ("[SD] Writing batch..");
# endif
    /* Write new batch of samples */
    if (!SD_AVAILABLE) {
      return;
    }

    uint32_t s =  lastbatch * BATCH_LENGTH;

    /* Check if we have room for samples in store */
    /*
    if (current_index.samples > (MAX_SAMPLES_PER_FILE - BATCH_LENGTH))
    {
      roll_data_file ();
    }
    */

    /* Check if we have room in size */
    if (sd_data->curPosition () > (SD_DATA_FILE_SIZE - (BATCH_LENGTH * (SAMPLE_LENGTH))))
    {
# if DEBUG_INFO
      SerialUSB.println ("[SD] Rolling because of SD_DATA_FILE_SIZE.");
# endif
      roll_data_file ();
    }

    write_reference (ad->references[lastbatch], ad->reference_status[lastbatch]);

    if (!SD_AVAILABLE) return;

    /* Writing entries */
    /*
# if DEBUG_VERB
    SerialUSB.println ("[SD] Writing entries to data file.");
# endif
    */

    int r = sd_data->write (reinterpret_cast<char*>((uint32_t*) &(ad->values[s])), sizeof(uint32_t) * BATCH_LENGTH);

    if (r != sizeof(uint32_t) * BATCH_LENGTH) {
# if DEBUG_VERB
      SerialUSB.println ("[SD] [Error] Failed while writing samples.");
# endif

      SD_AVAILABLE = false;
    }

    current_index.samples += BATCH_LENGTH;

    sd_data->sync ();
    SD_AVAILABLE &= (card->errorCode () == 0);

    /* Ready for next batch if successful write */
    if (SD_AVAILABLE)
      lastbatch  =  (lastbatch + 1) % BATCHES;

    if (lastbatch != ad->batch) {
# if DEBUG_VERB
      SerialUSB.println ("[SD] [Error] Out of sync with AD, might not finish writing batch before it is swapped.");
# endif
    }
  } // }}}

  void Store::open_data () // {{{
  {
    /* Open data file */
    if (!SD_AVAILABLE) return;

    char fname[13];
    int p = itoa (current_index.id, 10, fname);
    strcpy (&(fname[p]), ".DAT");
    //sprintf (fname, "%lu.DAT", current_index.id);

    SerialUSB.print ("[SD] Opening: ");
    SerialUSB.println (fname);

    SD_AVAILABLE &= sd_data->open (root, fname, O_CREAT | O_WRITE | O_TRUNC);
    SD_AVAILABLE &= (card->errorCode () == 0);
  } // }}}

  void Store::write_reference (uint64_t ref, uint32_t refstat) // {{{
  {
    if (!SD_AVAILABLE) {
      return;
    }

    if (current_index.nrefs >= MAX_REFERENCES) {
# if DEBUG_VERB
      SerialUSB.println ("[SD] Max references reached.");
# endif
      roll_data_file ();
    }

    /* Update index */
    current_index.refpos[current_index.nrefs] = sd_data->curPosition ();
    current_index.refs[current_index.nrefs]   = ref;

    /* Pad with 0 */
    for (uint32_t i = 0; i < (SD_REFERENCE_PADN * (SAMPLE_LENGTH)); i++)
      sd_data->write ((byte)0);

    sd_data->write (reinterpret_cast<char*>(&(current_index.nrefs)), sizeof(uint32_t));
    sd_data->write (reinterpret_cast<char*>(&(ref)), sizeof(uint64_t));
    sd_data->write (reinterpret_cast<char*>(&(refstat)), sizeof(uint32_t));

    /* Pad with 0 */
    for (uint32_t i = 0; i < (SD_REFERENCE_PADN * (SAMPLE_LENGTH)); i++)
      sd_data->write ((byte)0);

    current_index.nrefs++;

    SD_AVAILABLE &= (card->errorCode () == 0);
  } // }}}

  void Store::loop () // {{{
  {
    /* Try to set up SD card, 5 sec delay  */
    if (!SD_AVAILABLE && (millis () - lastsd) > 5000) {
# if DEBUG_VERB
      SerialUSB.println ("[SD] [Error] Trying to reset.");
      SerialUSB.println (card->errorCode ());
# endif

      /* Reset SPI */
      spi->end ();

      /* Clean up stale file references */
# if DEBUG_INFO
      SerialUSB.println ("[SD] Cleaning up..");
# endif
      if (card != NULL) {
        delete card;
        card = NULL;
      }

      if (volume != NULL) {
        delete volume;
        volume = NULL;
      }

      if (root != NULL) {
        delete root;
        root = NULL;
      }

      if (sd_data != NULL) {
        delete sd_data;
        sd_data = NULL;
      }

# if HASRF
      s_id = 0;
      s_samples = 0;
      s_nrefs   = 0;

      if (send_i != NULL) {
        delete send_i;
        send_i  = NULL;
      }

      if (send_d != NULL) {
        delete send_d;
        send_d  = NULL;
      }
# endif

      /* Reinitiate SD card */
# if DEBUG_INFO
      SerialUSB.println ("[SD] Re-init..");
# endif
      init ();

      lastsd = millis ();
    }

    /* Check if new batch is ready */
    /* Loop must run at least 2x speed (Nyquist) of batchfilltime */
    if (SD_AVAILABLE && continuous_write) {
      if (ad->batch != lastbatch) {
        write_batch ();
      }
    }
  } // }}}

  void Store::start_continuous_write () {
    continuous_write = true;
  }

  void Store::stop_continuous_write () {
    continuous_write = false;
  }

# if HASRF
  void Store::send_indexes (uint32_t start, uint32_t length) { // {{{
    if (!SD_AVAILABLE) {
      rf->send_error (RF::E_SDUNAVAILABLE);
      return;
    }

    /* Search on disk through indexes from start up to length nos */
    SdFile fi;
    while (length > 0) {
      int p = itoa (start, 10, buf);
      strcpy (&(buf[p]), ".IND");
      //sprintf (buf, "%lu.IND", start);
      RF_Serial.print ("$IDS,");
      RF_Serial.print (start);
      if (!!fi.open (root, buf, O_READ)) {
        fi.close ();
        RF_Serial.println (",1*NN");
        /*
        sprintf (rf->buf, "$IDS,%lu*", start);
        APPEND_CSUM(rf->buf);
        RF_Serial.println (rf->buf);
        */
      } else {
        RF_Serial.println (",0*NN");
      }

      start++;
      length--;
    }
  } // }}}

  void Store::_reset_index () { // {{{
    if (send_i != NULL) {
      delete send_i;
      send_i = NULL;
    }

    s_id      = 0;
    s_samples = 0;
    s_nrefs   = 0;
    s_lastbatch = 0;

    if (send_d != NULL) {
      send_d->close ();
      delete send_d;
      send_d = NULL;
    }
  } // }}}

  bool Store::_check_index (uint32_t id) { // {{{
    if (!SD_AVAILABLE) {
      rf->send_error (RF::E_SDUNAVAILABLE);
      return false;
    }

    // Should not happen (index is always closed at the end of this function)
    if (send_i != NULL) {
      _reset_index ();
    }

    if (s_id != id) {
      _reset_index ();

      if (current_index.id == id) {

        s_id      = id;
        s_samples = current_index.samples;
        s_nrefs   = current_index.nrefs;

      } else {
        // Not working on current index, read index file

        int p = itoa (id, 10, buf);
        strcpy (&(buf[p]), ".IND");
        //sprintf (buf, "%lu.IND", id);

        send_i = new SdFile ();

        if (!send_i->open (root, buf, O_READ)) {
          delete send_i;
          send_i = NULL;
          s_id = 0;

          rf->send_error (RF::E_NOSUCHID);
          return false;
        }

        /* Reading first part of Index */
        send_i->seekCur (  sizeof(Index::version)
                         + sizeof(Index::id)
                         + sizeof(Index::sample_l));
        send_i->read (reinterpret_cast<char*>(&s_samples), sizeof(s_samples));
        send_i->seekCur (sizeof(Index::samples_per_reference));
        send_i->read (reinterpret_cast<char*>(&s_nrefs), sizeof(s_nrefs));

        send_i->close ();
        delete send_i;
        send_i = NULL;
      }
    }

    return true;
  } // }}}

  void Store::send_index (uint32_t id) { // {{{
    if (!_check_index (id)) return;

    // format: $IND,id,samples,nrefs*CS
    RF_Serial.print ("$IND,");
    RF_Serial.print (id);
    RF_Serial.print (",");
    RF_Serial.print (s_samples);
    RF_Serial.print (",");
    RF_Serial.print (s_nrefs);
    RF_Serial.println ("*NN");

    /*
    sprintf(rf->buf, "$IND," STRINGIFY(STORE_VERSION) ",%lu," STRINGIFY(SAMPLE_LENGTH) ",%lu," STRINGIFY(BATCH_LENGTH) ",%lu*", id, s_samples, s_nrefs);
    APPEND_CSUM (rf->buf);
    RF_Serial.println (rf->buf);
    */
  } // }}}

  void Store::send_batch (uint32_t id, uint32_t refno, uint32_t start,  // {{{
                          uint32_t length) {
    if (!_check_index (id)) return;

    // refno is ref number in this id
    if (refno >= s_nrefs) {
      rf->send_error (RF::E_NOSUCHREF);
      return;
    }

    // start is sample no from first sample of this ref no
    if (((refno * BATCH_LENGTH) + start + length) > s_samples) {

      if (send_d != NULL) {
        send_d->close ();
        delete send_d;
        send_d = NULL;
      }


      rf->send_error (RF::E_NOSUCHSAMPLE);
      return;
    }

    /* Open data file, otherwise it must already be the data file for this id */
    if (send_d == NULL) {

      int p = itoa (id, 10, buf);
      strcpy (&(buf[p]), ".DAT");
      //sprintf (buf, "%lu.DAT", id);
      send_d = new SdFile ();
      if (!send_d->open (root, buf, O_READ)) {

        SD_AVAILABLE = (card->errorCode () == 0);
        if (!SD_AVAILABLE) {
          rf->send_error (RF::E_SDUNAVAILABLE);
          return;
        }

        rf->send_error (RF::E_NOSUCHDAT);
        delete send_d;
        send_d = NULL;
        return;
      }
    }

    /* Search to beginning of desired ref + start */
    uint32_t pos = (SD_REFERENCE_LENGTH * refno) + (BATCH_LENGTH * SAMPLE_LENGTH * refno);

    if (start > 0)
      pos += SD_REFERENCE_LENGTH + (SAMPLE_LENGTH * start);

    SerialUSB.println (pos);
    send_d->seekSet (pos);

    SD_AVAILABLE = (card->errorCode () == 0);
    if (!SD_AVAILABLE) {
      rf->send_error (RF::E_SDUNAVAILABLE);
      return;
    }

    /* Format and protocol:

     * 1. Initiate binary data stream:

     if start = 0, send reference
     $AD,D,[k = number of samples],[reference],[reference_status]*CC

     else send empty:
     $AD,D,[k = number of samples],0,0*CC

     * 2. Send one $ to indicate start of data
     * 3. Send k number of samples: 4 bytes * k
     * 4. Send end of data with checksum

     */

    uint32_t _refno   = 0;
    uint64_t ref      = 0;
    uint32_t refstat  = 0;

    if (start == 0) {
      send_d->seekCur (SD_REFERENCE_PADN * SAMPLE_LENGTH); // seek past padding
      send_d->read (reinterpret_cast<char*>(&_refno), sizeof(_refno));
      send_d->read (reinterpret_cast<char*>(&ref), sizeof(ref));
      send_d->read (reinterpret_cast<char*>(&refstat), sizeof(refstat));
      send_d->seekCur (SD_REFERENCE_PADN * SAMPLE_LENGTH); // seek to first sample

      if (_refno != refno) {
        SerialUSB.println ("nrf");
        rf->send_error (RF::E_BADDAT);
      }
    }


    RF_Serial.print   ("$AD,D,");
    RF_Serial.print   (id);
    RF_Serial.print   (",");
    RF_Serial.print   (refno);
    RF_Serial.print   (",");
    RF_Serial.print   (start);
    RF_Serial.print   (",");
    RF_Serial.print   (length);
    RF_Serial.print   (",");
    RF_Serial.print   (ref);
    RF_Serial.print   (",");
    RF_Serial.print   (refstat);
    RF_Serial.println ("*NN");

    /*
    sprintf (buf, "$AD,D,%lu,%llu,%lu*", length, ref, refstat);
    APPEND_CSUM (buf);
    RF_Serial.println (buf);
    */


    /* Write '$' to signal start of binary data */
    RF_Serial.write ('$');

    uint32_t s;
    byte csum = 0;

    int n = 0;
    bool bad = false;

    for (uint32_t i = 0; i < length; i++)
    {
      if (!bad) {
        n = send_d->read (reinterpret_cast<char*>(&s), sizeof (uint32_t));

        if (n != 4) {
          bad = true;
        }

        /* MSB first (big endian), means concatenating bytes on RX will
         * result in LSB first; little endian. */
        RF_Serial.write ((byte*)(&s), 4);

        csum = csum ^ ((byte*)&s)[0];
        csum = csum ^ ((byte*)&s)[1];
        csum = csum ^ ((byte*)&s)[2];
        csum = csum ^ ((byte*)&s)[3];

      } else {
        for (int i = 0; i < 4; i++)
          RF_Serial.write ((byte)0); // write 0's to complete the batch in case of error
      }

    }

    /* Send end of data with Checksum */
    RF_Serial.print   ("$AD,DE,");
    if (bad)
      RF_Serial.print ("XX"); // make sure cheksum does not validate in case of error
    else
      RF_Serial.print   (csum, HEX);
    RF_Serial.println ("*NN");
    /*
    sprintf (buf, "$AD,DE," F_CSUM "*", csum);
    APPEND_CSUM (buf);
    RF_Serial.println (buf);
    */

    if (bad) {
      rf->send_error (RF::E_BADDAT);
    }

    /* If this was the last sample, close file. */
    if (((refno * BATCH_LENGTH) + start + length) > s_samples) {
      if (send_d != NULL) {
        send_d->close ();
        delete send_d;
        send_d = NULL;
      }

      s_id      = 0;
      s_samples = 0;
      s_nrefs   = 0;
    }
  } // }}}

  void Store::send_lastid () { // {{{
    if (!SD_AVAILABLE) {
      rf->send_error (RF::E_SDUNAVAILABLE);
      return;
    }

    RF_Serial.print ("$LID,");
    RF_Serial.print (current_index.id);
    RF_Serial.println ("*NN");
  } // }}}
# endif
}

/* vim: set filetype=arduino :  */

