/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-08-15
 *
 * dtt.cpp: Interface to DTT file
 *
 */

# include <iostream>
# include <fstream>
# include <string>

# include "dtt.h"

using namespace std;

namespace Zero {
  Dtt::Dtt (int _id) {
    bdata.id = _id;
    cout << "Opening id: " << bdata.id << "..";

    /* Load index and samples */
    if (!read_index ()) {
      ready = false;
      return;
    }
    if (!read_batches ()) {
      ready = false;
      return;
    }

    cout << "done, read: " << bdata.batches.size () << " batches with: " << bdata.totalsamples << " samples total." << endl;
    ready = true;
  }

  bool Dtt::read_index () {
    char fname[50];
    sprintf (fname, "%d.ITT", bdata.id);

    //cout << "Opening: " << fname << endl;

    ifstream itt (fname);

    if (itt.bad () || !itt.is_open ()) {
      cerr << "Could not open index." << endl;
      return false;
    }
    itt >> bdata.localversion;
    itt >> bdata.remoteversion;
    itt >> bdata.id;
    itt >> bdata.samplescount;
    itt >> bdata.batchcount;

    /*
    cout << "Local version:  " << localversion << endl;
    cout << "Remote version: " << remoteversion << endl;
    cout << "ID:             " << id << endl;
    cout << "Samples:        " << samplescount << endl;
    cout << "Batches:        " << batchcount << endl;
    */

    itt.close ();
    return true;
  }

  bool Dtt::read_batches () {
    char fname[50];
    sprintf (fname, "%d.DTT", bdata.id);
    //cout << "Reading batches from: " << fname << endl;

    ifstream dtt (fname);

    if (dtt.bad () || !dtt.is_open ()) {
      cerr << "Could not open data file." << endl;
      return false;
    }


    /* Read batches */
    for (int i = 0; i < bdata.batchcount; i++) {
      Batch b;
      b.samples = new int[BATCH_LENGTH];

      /* Read reference */
      string ref;
      dtt >> ref;
      (char) dtt.get(); // skip newline

      /* Parse reference */
      int t = 0;
      int pos = 0;
      string token;
      while (t <= 7 && pos < ref.length()) {
        token = "";
        while (pos < ref.length()) {
          char c = ref[pos];
          if (c == ',') {pos++; break;}
          else token += (char)c;
          pos++;
        }

        switch (t) {
          case 0: break; // R
          case 1: sscanf (token.c_str(), "%u", &(b.length)); break;
          case 2: sscanf (token.c_str(), "%u", &(b.no)); break;
          case 3: sscanf (token.c_str(), "%lu", &(b.ref)); break;
          case 4: sscanf (token.c_str(), "%u", &(b.status)); break;
          case 5: b.latitude  = token;
          case 6: b.longitude = token;
          case 7: sscanf (token.c_str(), "%u", &(b.checksum)); break;
        }

        t++;
      }

      //cout << "Ref: " << b.no << ", " << b.ref << ", status: " << b.status << ", latitude: " << b.latitude << ", longitude: " << b.longitude << ", checksum: " << b.checksum << endl;
      //cout << "Read ref: " << b.no << endl;

      /* Read samples */
      for (int j = 0; j < BATCH_LENGTH; j++) {
        /* The sample is stored as a two's complement 32 bit uint,
         * the last bit indicates whether the full scale of the AD
         * has been exceeded. At upper value a set last bit means overflow,
         * at lower value a unset last bit means underflow.
         */
        uint32_t s;
        dtt >> s;
        (char) dtt.get(); // skip newline

        bool fsclipped = s & 0x1;
        s &= 0xfffffffe;          // mask out to avoid confusion with twos_comp


        /* Assuming architecture stores int32_t as two's complement */
        int32_t ss = s; // cast to int32_t
        ss       >>= 1; // shift down to 31 bits (LSB is, now unset, FS clip bit)

        b.samples[j] = ss;

        bdata.totalsamples++;
      }

      bdata.batches.push_back (b);
    }

    //cout << "Read " << totalsamples << " samples total." << endl;
    return true;
  }

  Dtt::~Dtt () {
  }
}
