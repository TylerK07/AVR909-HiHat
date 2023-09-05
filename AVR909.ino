#include "r909hh.h"



/*******************************************
* Main Setup Function                      *
*******************************************/
//#define PIN_ISC_DISABLE 0x00C0  // DISABLED

#define NUM_SEGMENTS 2
uint16_t sample_segments[2] = {18444, 24576}; // Last index for open high-hat and closed high-hat
uint8_t  current_segment = 0;                 // The current segment being played

uint16_t sampCounter = 0;                     // Position inside of the sample array
uint8_t  sampOffset = 0;                      // Bit-position inside of the current sample

void setup() {
  //Set Up the Timer
  takeOverTCA0();                            // Override the timer
  TCA0.SINGLE.PER = 0x0271;                  // This is the sample period (20 MHz / this value) 
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;   // Enable overflow interrupt
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;  // Enable the timer with no prescaler

  PORTD.DIRSET = 0xFF;                       // Set all pins on PORTD to output

  pinMode( PIN_PA6, INPUT_PULLUP );          // Input pin for Open HH Trigger
  pinMode( PIN_PA7, INPUT_PULLUP );          // Input pin for Closed HH Trigger
  pinMode( PIN_PC0, OUTPUT );                // Output pin for actirity LED
}




/*******************************************
* Main Loop Function                       *
*******************************************/
void loop() {
  TCA0.SINGLE.PER = analogRead(PIN_PF0);     // Set Timer Period based on value of Speed control 

  // Currently these just see if a button is pressed and as long as the selected sample isn't playing, it moves the sample
  // counter to the correct location and starts playing the sample. In practice, this means that you can't retrigger the
  // current sample until it has completed playing (but you can swap between samples). I'm not sure if I like this behavior
  // and will probbly end up checking if the button gets released so a sample can be retriggered.

  if( (digitalRead( PIN_PA6 ) == 0) && (sampCounter >= sample_segments[0]) ){
    current_segment = 0;
    sampCounter = 0;
    sampOffset  = 0;
    digitalWrite( PIN_PC0, 1 );
  }
  if( (digitalRead( PIN_PA7 ) == 0) && ((sampCounter >= sample_segments[1]) || (sampCounter <= sample_segments[0])) ){
    current_segment = 1;
    sampCounter = sample_segments[0]+3;
    sampOffset  = 0;
    digitalWrite( PIN_PC0, 1 );
  }
}


// Timer Interrupt Function gets run every time the hardware counter hits value in TCA0.SINGLE.PER
ISR(TCA0_OVF_vect) {
  if (sampCounter < sample_segments[current_segment]){            // See if we hit the end of the current sample
    uint8_t s1 = pgm_read_byte_near( sample + sampCounter );      // Grab the current byte in the sample
    uint8_t s2 = pgm_read_byte_near( sample + sampCounter + 1 );  // Grab the next byte in the sample
    uint8_t s = 0;                                                // This will store our final value

    // Extract the next six bits and put them into an 8-bit byte (s)
    // There are four cases depending on the bit offset. I handle them individually to keep the formulas fast
    // though there may be ways to do this faster?
    switch (sampOffset) {
      case 0: s = s1 & 0xFC;                      sampOffset = 6;                break; // Sample Counter doesn't increment in this case
      case 2: s = s1 << 2;                        sampOffset = 0; sampCounter++; break;
      case 4: s = (s1 << 4) | ((s2 >> 4) & 0xFC); sampOffset = 2; sampCounter++; break;
      case 6: s = (s1 << 6) | ((s2 >> 2) & 0xFC); sampOffset = 4; sampCounter++; break;
    }
    PORTD.OUT = s;                           // Assign all bits to PORTD at the same time 

  } else {
    digitalWrite( PIN_PC0, 0 );              // Turn off the activity LED if nothing is playing
  }
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;  // Reset the interrupt!!
}
