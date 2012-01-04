#include "ax25encoder.h"

// @export class_definition
AX25Encoder::AX25Encoder() {
  phase_state = false;
  buf = 0;
  bufpos = 0;
  bitpos = 0;
  
  n_ones = 0;
  flagrem = 0;
};


// @export "foo"

#define HEAD_FLAGS 3
#define TAIL_FLAGS 2

#define XXXSTATE_MARK 1
#define XXXSTATE_SPACE 2

// @export "bar"

// @export enqueue
void AX25Encoder::enqueue(uint8_t *inbuf, uint16_t blen) {
  buf = inbuf;
  bufpos = 0;
  buflen = blen;
  bitpos = 0;
  flagrem = HEAD_FLAGS;
}

// @export nextstate
uint8_t AX25Encoder::nextState() {
  uint8_t curbit;

  if (bufpos == buflen && flagrem == 0) return 0;

  if (flagrem) {
    curbit = 0x7E & (1<<bitpos);

    if (bitpos == 7) {
      bitpos = 0;
      flagrem--;
    } else {
      bitpos++;
    }
	
  } else {

    if (n_ones == 5) { // bit-stuffing: engage.
      curbit = 0;
    } else {
      curbit = buf[bufpos] & (1<<bitpos);
	  
      if (bitpos == 7) {
	bitpos = 0;
	bufpos++;

	if (bufpos == buflen) {
	  flagrem = TAIL_FLAGS;
	}
      } else {
	bitpos++;
      }
    }
  }

  if (!curbit) {
    phase_state = !phase_state;
    n_ones = 0;
  } else {
    n_ones++;
  }

  return phase_state ? XXXSTATE_MARK : XXXSTATE_SPACE;
}
