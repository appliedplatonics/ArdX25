#include <util/delay.h>
#include "ax25encoder.h"
#include "ax25decoder.h"

typedef struct rx_state {
  AX25Decoder ax25d; // Actual decoder state

  uint16_t last_xc; // timestamp of last zero-crossing
  uint8_t obs_i;    // dt observation cursor
  uint8_t obs[5];   // list of dts observed 

  uint8_t buf[300]; // text output buffer
  uint16_t bufpos;  // text output buffer cursor

} rx_state_t;

typedef struct  {
  AX25Encoder ax25e;

  uint8_t sine_i;
  uint8_t sine_count;

  bool tx_dcd_lockout;
} tx_state_t;


#if USE_CLOSE_SINE
static const uint8_t sine[16] = { 7, 10, 13, 14, 15, 14, 13, 10, 
				  8,  5,  2,  1,  0,  1,  2,  5, };
#else

// This is a better fit with "sag" accounted for.
static const uint8_t sine[16] = { 8, 11, 13, 14, 15, 14, 13, 11, 
				  9,  6,  3,  2,  1,  2,  3,  6, };

#endif


tx_state_t tx_state;
rx_state_t rx_state;

void rx_state_setup() {
  rx_state.ax25d = AX25Decoder();
  rx_state.obs_i = 0;

  rx_state.bufpos = 0;
}

void tx_state_setup() {
  tx_state.sine_i = 0;
  tx_state.sine_count = 0;
  tx_state.ax25e = AX25Encoder();

  tx_state.tx_dcd_lockout = false;
}

#define ENABLE_BAUD_CLK  bitWrite(TIMSK1, OCIE1A, 1); // p140
#define DISABLE_BAUD_CLK bitWrite(TIMSK1, OCIE1A, 0);


#define ENABLE_TONE_CLK  bitWrite(TIMSK2, OCIE2A, 0/*XXX 1*/); bitWrite(TIMSK2, TOIE2, 1);
#define DISABLE_TONE_CLK bitWrite(TIMSK2, OCIE2A, 0); bitWrite(TIMSK2, TOIE2, 0);

void tx_interrupt_setup() {
  /* Set up timer2 for tone generation */

  Serial.println("  Setting up Timer2");

  // Turn off timer2 interrupts while we twiddle, p164
  DISABLE_TONE_CLK;
  
  //Timer2 Settings: Timer Prescaler=8, CS22,21,20=0,1,0
  TCCR2A = 0x00;    // See S17.11 Register Description, pp159-61
  TCCR2B = 0x00;    // pp162-3

  // Turn on a div-1 prescaler/divider: p163

  bitWrite (TCCR2B, CS22, 0);
  bitWrite (TCCR2B, CS21, 0);
  bitWrite (TCCR2B, CS20, 1);
  

  // go to mode 2 of the generator, cmp OCRA p161,150
  //bitWrite (TCCR2A, WGM21, 1);

  // go to mode 1 of the generator, phase-correct PWM (p161,153)
  bitWrite(TCCR2A, WGM20, 1);

  // Enable PWM output on B3 (Digital11) 
  bitWrite(DDRB, 3, 1);
  bitWrite(TCCR2A, COM2A1, 1);

  // Set an initial, conservative speed.
  OCR2A = 15; // 25=2400*16

  // End state: ready to generate tones, but not enabled
}

// RX also hooks onto the baud interrupt.
void baud_interrupt_setup() {
  DISABLE_BAUD_CLK;

  Serial.println("  Setting up Timer1");

  // Clear state
  TCCR1A = 0x00;    // See S15.11 Register descriptions p135-137
  TCCR1B = 0x00;    // p137-138
  TCCR1C = 0x00;    // p138-139

  // Go to mode 4/CTC of the generator, phase-correct PWM
  bitWrite(TCCR1B, WGM12, 1);
  //bitWrite(TCCR1A, WGM10, 1);

  // Turn on a div-8 prescaler/divider: p138
  bitWrite (TCCR1B, CS12, 0);
  bitWrite (TCCR1B, CS11, 1);
  bitWrite (TCCR1B, CS10, 0);

  OCR1A = 827; // 1202 Hz.  Nice.

  // This should be ready to go now.
}


ISR(TIMER1_COMPA_vect) {
  PORTB ^= (1<<4);
}


ISR(TIMER2_OVF_vect) {
  OCR2A = sine[tx_state.sine_i] << 4;
  tx_state.sine_i = (tx_state.sine_i+1)%16;
}

ISR(TIMER2_COMPA_vect) {
  if (tx_state.sine_count == 0 || tx_state.sine_count == sine[tx_state.sine_i])
    PORTB ^= (1<<5);
  tx_state.sine_count = (tx_state.sine_count+1) % 16;
  if (0 == tx_state.sine_count) tx_state.sine_i = (tx_state.sine_i+1)%16;

}

void setup() {
  Serial.begin(9600);

  bitWrite(DDRB, 3, 1);
  bitWrite(DDRB, 4, 1);
  bitWrite(DDRB, 5, 1);

  tx_state_setup();
  rx_state_setup();

  tx_interrupt_setup();

  baud_interrupt_setup();

  ENABLE_TONE_CLK;
  ENABLE_BAUD_CLK;
}

void loop() {
  _delay_ms(100);
  _delay_ms(100);
  _delay_ms(100);
  _delay_ms(100);
  _delay_ms(100);
}
