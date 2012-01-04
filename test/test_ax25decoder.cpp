#include <stdio.h>
#include <unittest++/UnitTest++.h>

#include "ax25decoder.h"
#include "ax25encoder.h"



SUITE(AX25Decoder) {
  class DummyModulator {
  public:
    AX25Encoder ax25e;

    double phase;
    uint32_t pos;
    bool at_boundary;

    uint8_t curv;

    DummyModulator() {
      ax25e = AX25Encoder();
      setup();
    }

    void setup() {
      phase = 0.0;
      pos = 0;
      curv = 0;
      at_boundary = true;
    }

    void enqueue(uint8_t *buf, uint16_t buflen) {
      ax25e.enqueue(buf, buflen);      
      at_boundary = true;
    }

#define XXXPERIOD_MARK  206
#define XXXPERIOD_SPACE 112
#define XXXPERIOD_BAUD  206

    // returns the next zero offset, or zero at boundaries.  Returns
    // zero when out of data.

    bool getSpeed() {
	uint8_t v = ax25e.nextState();

	if (v == XXXSTATE_MARK) {
	  curv = XXXPERIOD_MARK;
	} else if (v == XXXSTATE_SPACE) {
	  curv = XXXPERIOD_SPACE;
	} else if (v == 0) {
	  return false; // out of data
	}
	return true;
    }

    // Eg: 103,103,0,103,103,0,55,55,55,0,0
    //             |         |          | $
    uint32_t nextZeroCrossing() {

      if (at_boundary) {

	if (!getSpeed()) return 0;

	at_boundary = false;
	return 0;
      } 

      uint8_t halfv = curv/2;

      uint32_t nextpos;

      if (phase) {
	double partial = (phase/180.0)*(double)halfv;
	uint8_t fixup = (uint8_t)partial;
	nextpos = pos + fixup;
	phase = 0;
      } else {
	nextpos = pos + halfv;
      }

      if ((nextpos / XXXPERIOD_BAUD) > (pos / XXXPERIOD_BAUD)) {
	phase = (double)(360.0*(nextpos % XXXPERIOD_BAUD))/(double)curv;
	if (phase < 1) phase = 0;

	if (phase == 0) {
	  at_boundary = true;
	  pos = nextpos;
	  return pos;
	} else {
	  nextpos -= (nextpos % XXXPERIOD_BAUD);
	  pos = nextpos;
	  if (!getSpeed()) at_boundary = true;
	  return 0;
	}
      }

      pos = nextpos;
      return nextpos;      
    }

  };


  struct AX25DecoderFixture {
    AX25Decoder ax25d;
    DummyModulator dm;

    AX25DecoderFixture() { 
      ax25d = AX25Decoder(); 
      dm = DummyModulator();
    }

  };





  TEST_FIXTURE(AX25DecoderFixture, DCDNotSetOnBogusData)
    {
      uint8_t obs[8] = { 0,0,0,0,0,0,0};

      CHECK(!ax25d.dcd);

      // Make sure we don't actually do anything with zero inputs.
      ax25d.submitObservations(NULL, 0);
      CHECK(!ax25d.dcd);

      ax25d.submitObservations(obs, 1);
      CHECK(!ax25d.dcd);

      // Go ahead and set DCD, to unset it with short data
      ax25d.submitObservations(obs, 2);
      CHECK(ax25d.dcd);

      ax25d.submitObservations(obs, 1);
      CHECK(!ax25d.dcd);



      // Go ahead and set DCD, to unset it with long data
      ax25d.submitObservations(obs, 5);
      CHECK(ax25d.dcd);

      ax25d.submitObservations(obs, 6);
      CHECK(!ax25d.dcd);

      
    }


  TEST_FIXTURE(AX25DecoderFixture, TestFlagSync) {
    uint8_t obs[2] = { 0,0 };

    CHECK(ax25d.retbyte == 0);


    // A quick-and-dirty kludge, not the prettiest, but effective.
#define SEND_AND_CHECK(dt, expbyte, byte) { obs[0] = dt; obs[1] = dt; bool SEND_AND_CHECK_RET = ax25d.submitObservations(obs, 2); CHECK(!expbyte || SEND_AND_CHECK_RET); CHECK(!expbyte || byte == ax25d.retbyte); CHECK(ax25d.dcd);}

    SEND_AND_CHECK( 55, false, 0x00); // Send a 0: 0x00 @ 1
    SEND_AND_CHECK( 55, false, 0x02); // Send a 1: 0x02 @ 2
    SEND_AND_CHECK(105, false, 0x02); // Send a 0: 0x02 @ 3
    SEND_AND_CHECK( 55, false, 0x02); // Send a 0: 0x02 @ 4
    SEND_AND_CHECK( 55, false, 0x12); // Send a 1: 0x12 @ 5
    SEND_AND_CHECK( 55, false, 0x32); // Send a 1: 0x32 @ 6
    SEND_AND_CHECK( 55, false, 0x72); // Send a 1: 0x72 @ 7
    SEND_AND_CHECK( 55,  true, 0xF2); // Send a 1: 0xF2 @ 8
    SEND_AND_CHECK( 55, false, 0x01); // Send a 1: 0x01 @ 1
    SEND_AND_CHECK( 55, false, 0x03); // Send a 1: 0x03 @ 2
    SEND_AND_CHECK(105,  true, 0x7E); // Send a 0 <-- triggers 0x7E
				      // flag detect

    // Make sure we're back in sync
    SEND_AND_CHECK( 55, false, 0x00); // Send a 0: 0x00 @ 1
    SEND_AND_CHECK( 55, false, 0x02); // Send a 1: 0x02 @ 2
    SEND_AND_CHECK(105, false, 0x02); // Send a 0: 0x02 @ 3
    SEND_AND_CHECK( 55, false, 0x02); // Send a 0: 0x02 @ 4
    SEND_AND_CHECK( 55, false, 0x12); // Send a 1: 0x12 @ 5
    SEND_AND_CHECK( 55, false, 0x32); // Send a 1: 0x32 @ 6
    SEND_AND_CHECK( 55, false, 0x72); // Send a 1: 0x72 @ 7
    SEND_AND_CHECK( 55,  true, 0xF2); // Send a 1: 0xF2 @ 8


#undef SEND_AND_CHECK
  }


  TEST_FIXTURE(AX25DecoderFixture, TestBasicModulation) {
    uint8_t data[] = { 0xFF, 0x00, 0xAA, 0x55, 0xFF, 0x00 };
    uint8_t datalen = sizeof(data)/sizeof(uint8_t);

    dm.enqueue(data, datalen);


    uint32_t last_xc = dm.nextZeroCrossing();
    uint32_t cur_xc;

    bool have_data = true;

    uint8_t obs[8];
    uint8_t obs_i = 0;

    uint8_t output[32];
    uint8_t output_i = 0;

    while (have_data) {
      cur_xc = dm.nextZeroCrossing();


      if (cur_xc != 0) {
	obs[obs_i] = cur_xc - last_xc;
	obs_i++;
	last_xc = cur_xc;
      } else {
	if (0 == obs_i) {
	  have_data = false;
	} else {
	  if (ax25d.submitObservations(obs, obs_i)) {
	    output[output_i] = ax25d.retbyte;
	    output_i++;
	  }
	  obs_i = 0;
	}
      }

    }

    uint8_t expected[] = { 0x7E, 0x7E, 0x7E, 0xFF, 0x00, 0xAA, 0x55, 0xFF, 0x00, 0x7E, 0x7E };
    uint8_t expected_len = sizeof(expected)/sizeof(uint8_t);

    CHECK_EQUAL((int)expected_len, (int)output_i);

    for (int i = 0; i < expected_len; i++) {
      CHECK_EQUAL((int)expected[i], (int)output[i]);
    }
  }


  TEST_FIXTURE(AX25DecoderFixture, TestShiftedModulation) {
    for (double psi = 0.0; psi < 360.0; psi += 5.0) {

      dm = DummyModulator();
      dm.phase = psi;

      uint8_t data[] = { 0xFF, 0x00, 0xAA, 0x55, 0xFF, 0x00 };
      uint8_t datalen = sizeof(data)/sizeof(uint8_t);

      dm.enqueue(data, datalen);


      uint32_t last_xc = dm.nextZeroCrossing();
      uint32_t cur_xc;

      bool have_data = true;

      uint8_t obs[8];
      uint8_t obs_i = 0;

      uint8_t output[32];
      uint8_t output_i = 0;

      while (have_data) {
	cur_xc = dm.nextZeroCrossing();

	if (cur_xc != 0) {
	  obs[obs_i] = cur_xc - last_xc;
	  obs_i++;
	  last_xc = cur_xc;
	} else {
	  if (0 == obs_i) {
	    have_data = false;
	  } else {
	    if (ax25d.submitObservations(obs, obs_i)) {
	      output[output_i] = ax25d.retbyte;
	      output_i++;
	    }
	    obs_i = 0;
	  }
	}

      }

      uint8_t expected[] = { 0x7E, 0x7E, 0x7E, 0xFF, 0x00, 0xAA, 0x55, 0xFF, 0x00, 0x7E, 0x7E };
      uint8_t expected_len = sizeof(expected)/sizeof(uint8_t);

      CHECK_EQUAL((int)expected_len, (int)output_i);

      for (int i = 0; i < expected_len; i++) {
	CHECK_EQUAL((int)expected[i], (int)output[i]);
      }
    }
  }


}

int main(int argc, char *argv[]) {
  return UnitTest::RunAllTests();
}
