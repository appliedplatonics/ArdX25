#include "ax25decoder.h"

AX25Decoder::AX25Decoder() { 
  dcd = false; 
  retbyte = 0;
  curbyte = 0;
  curbyte_i = 0;
}

void AX25Decoder::clearState() {
  dcd = false;
  curbyte_i = 0;
  curbyte = 0;

  last_state = 0;
  n_ones = 0;
}


bool AX25Decoder::submitObservations(uint8_t *deltas, uint8_t n) {
  if (n < 2 || n > 5) {
    clearState();
    return false;
  }

  dcd = true;

  bool got_short = false, got_long = false, got_wacky = false;

  for (uint8_t i = 0; i < n; i++) {
    got_short = got_short || (deltas[i] < THRESHOLD_SHORT);
    got_long = got_long || (deltas[i] > THRESHOLD_LONG);
    got_wacky = got_wacky || (deltas[i] >= THRESHOLD_SHORT && deltas[i] <= THRESHOLD_LONG);
  }

  bool cur_state = got_short;

  bool is_wacky = (got_wacky || (got_short && got_long));

  if (is_wacky) cur_state = !last_state;

  if (cur_state != last_state) {
    if (n_ones >= 6) { // "flag": train the line
      curbyte = 0x7E;
      curbyte_i = 7;
    } else if (n_ones == 5) { // bit-stuffed zero
      n_ones = 0;
      last_state = cur_state;
      return false;
    }
    n_ones = 0;
  } else {
    n_ones++;

    curbyte |= (1 << curbyte_i);
  }

  if (got_short || got_long) last_state = cur_state;

  if (curbyte_i < 7) {
    curbyte_i++;
    return false;
  }

  retbyte = curbyte;
  curbyte = 0;
  curbyte_i = 0;

  // Don't clear n_ones here!  We may be out-of-step, and need it to
  // be contiguous.

  return true;
}
