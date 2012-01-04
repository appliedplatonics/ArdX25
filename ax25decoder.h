#ifndef AX25DECODER_H
#define AX25DECODER_H
#include <inttypes.h>

class AX25Decoder {
 public:
  AX25Decoder();
  
  bool submitObservations(uint8_t *deltas, uint8_t n);

  bool dcd;
  uint8_t retbyte;

  void clearState();

 private:
  uint8_t curbyte;
  uint8_t curbyte_i;

  static const uint8_t THRESHOLD_SHORT = 60;
  static const uint8_t THRESHOLD_LONG  = 95;

  bool last_state;

  uint8_t n_ones;
};

#endif
