#ifndef AX25ENCODER_H
#define AX25ENCODER_H

#include <inttypes.h>

#define XXXSTATE_MARK  1
#define XXXSTATE_SPACE 2


class AX25Encoder {
public:
  AX25Encoder();
  void enqueue(uint8_t*, uint16_t);
  uint8_t nextState();

private:
  uint8_t *buf;
  uint16_t buflen;
  uint16_t bufpos;
  uint8_t bitpos;

  uint8_t n_ones;
  uint8_t flagrem;

  bool phase_state;
};



#endif
