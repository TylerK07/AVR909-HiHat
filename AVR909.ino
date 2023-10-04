#include "r909hh.h"



/*******************************************
* Global Definitions & Variables           *
*******************************************/

//OUTPUT PINS:
#define PIN_TRIGGER     0b00000010                                   // PIN C1 - Turns on for a split second to initiate the envelope 
#define PIN_PLAYING     0b00000100                                   // PIN C2 - Indicates that either sample is playing (turns VCA on)
#define PIN_CLOSED_HAT  0b00001000                                   // PIN C3 - Indicates that the hat is closed (determines which envelope / VCA input to use)

#define PIN_LED_OPEN    0b00000001                                   // PIN A0 - Indicates whether the Open HiHat is playing
#define PIN_LED_CLOSED  0b00000010                                   // PIN A1 - Indicates whether the Closed HiHat is playing

//INPUT PINS:
#define PIN_BTN_OPEN    0b00010000                                   // PIN A4 - Connects to Open HiHat Trigger Button 
#define PIN_BTN_CLOSED  0b00100000                                   // PIN A5 - Connects to Closed HiHat Trigger Button 
#define PIN_TRIG_OPEN   0b01000000                                   // PIN A6 - Connects to Open HiHat External Trigger 
#define PIN_TRIG_CLOSED 0b10000000                                   // PIN A7 - Connects to Closed HiHat External Trigger 


#define NUM_SEGMENTS 2                                               // Number of sample segments
uint16_t sample_segments[2] = {18444, 24576};                        // Last index for open high-hat and closed high-hat
uint8_t  current_segment = 0;                                        // The current segment being played

uint16_t sample_counter   = 0;                                       // Position inside of the sample array
uint8_t  sample_direction = 0;                                       // Direction to play the sample (to be implemented...)

uint8_t  sample_buffer[4] = {0,0,0,0};                               // Contains the current 4 decompressed bytes
uint8_t  buffer_index = 0;                                           // contains the index of the current decompressed byte in buffer

#define  RETRIGGER_TIME 5;                                           // Number of milliseconds to wait before a sample can be retriggered
unsigned long retrig = 0;                                            // Retriggering counter
uint8_t  current_button = 0;                                         // Tracks which button is currently being pressed / triggered



/*******************************************
* Main Setup Function                      *
*******************************************/

void setup() {
  // Set Up the Timer
  takeOverTCA0();                                                    // Override the timer
  TCA0.SINGLE.PER = 0x0271;                                          // This is the sample period (20 MHz / this value) 
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;                           // Enable overflow interrupt
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;                          // Enable the timer with no prescaler

  // Set up pins
  PORTD.DIRSET = 0xFF;                                               // Set all pins on PORTD to output
  PORTC.DIRSET = PIN_CLOSED_HAT | PIN_PLAYING | PIN_TRIGGER;         // Set the control pins to be outputs
  PORTA.DIRSET = PIN_LED_OPEN | PIN_LED_CLOSED;                      // Set the LED indicators to be outputs
  PORTA.DIRCLR = PIN_BTN_OPEN | PIN_BTN_CLOSED | PIN_TRIG_OPEN | PIN_TRIG_CLOSED; // Set tigger/button pins to be inputs

  // Configure buttons as pullups:
  pinMode( PIN_PA4, INPUT_PULLUP );                                  // Input pin for Open HH Button
  pinMode( PIN_PA5, INPUT_PULLUP );                                  // Input pin for Closed HH Button
  pinMode( PIN_PA6, INPUT_PULLUP );                                  // Input pin for Open HH Trigger
  pinMode( PIN_PA7, INPUT_PULLUP );                                  // Input pin for Closed HH Trigger

}



/*******************************************
* Sample Trigger Functions                 *
*******************************************/

void playOpenHH(){
  current_segment = 0;                                               // Set the current sample segment to be zero (OpenHH)
  sample_counter = 0;                                                // Move the sample counter to the beginning of the segment
  buffer_index = 4;                                                  // Set the buffer index to the end so it grabs the next segment 
  PORTA.OUTCLR = PIN_LED_OPEN | PIN_LED_CLOSED;                      // Turn off the LEDs
  PORTC.OUTCLR = PIN_PLAYING | PIN_TRIGGER | PIN_CLOSED_HAT;         // Turn off the control signals
  delay(1);                                                          // Wait a split second
  PORTA.OUTSET = PIN_LED_OPEN;                                       // Turn on the OpenHH LED
  PORTC.OUTSET = PIN_PLAYING | PIN_TRIGGER;                          // Turn on the playing / trigger control signals
}

void playClosedHH(){
  current_segment = 1;                                               // Set the current sample segment to be one (ClosedHH)
  sample_counter = sample_segments[0]+3;                             // Move the sample counter to the beginning of the segment
  buffer_index = 4;                                                  // Set the buffer index to the end so it grabs the next segment
  PORTA.OUTCLR = PIN_LED_OPEN | PIN_LED_CLOSED;                      // Turn off the LEDs
  PORTC.OUTCLR = PIN_PLAYING | PIN_TRIGGER | PIN_CLOSED_HAT;         // Turn off the control signals
  delay(1);                                                          // Wait a split second
  PORTA.OUTSET = PIN_LED_CLOSED;                                     // Turn on the ClosedHH LED
  PORTC.OUTSET = PIN_PLAYING | PIN_TRIGGER | PIN_CLOSED_HAT;         // Turn on the CLOSED_HAT control signal as well as playing / trigger
}



/*******************************************
* Main Loop Function                       *
*******************************************/
void loop() {
  TCA0.SINGLE.PER = analogRead(PIN_PF0);                             // Set Timer Period based on value of Speed control 
  if( retrig < millis() ){                                           // See if it is time to re-check the buttons
    if( (digitalRead(PIN_PA4)==0) || (digitalRead(PIN_PA6)==0) ){    // If Open HiHat buttons pressed
      if( current_button != 1 ) playOpenHH();                        // And it isn't already pressed, then play the OpenHH sample
      current_button = 1;                                            // Track that the OpenHH button is pressed
      retrig = millis() + RETRIGGER_TIME;                            // Set a time to check things again in the future
    } 
    else if( (digitalRead(PIN_PA5)==0)||(digitalRead(PIN_PA7)==0) ){ // If Open HiHat button pressed
      if( current_button != 2 ) playClosedHH();                      // And it isn't already pressed, then play the ClosedHH sample
      current_button = 2;                                            // Track that the ClosedHH button is pressed
      retrig = millis() + RETRIGGER_TIME;                            // Set a time to check things again in the future
    }
    else current_button = 0;                                         // Track that no buttons are pressed

  } else {
    PORTC.OUTCLR = PIN_TRIGGER;                                      // Reset the trigger control so we can hit it again quickly
  }
}



/*******************************************
* Timer Interrupt Function                 *
*******************************************/

// This function gets run every time the hardware counter hits value in TCA0.SINGLE.PER
// It plays the sample at a constant rate, decompressing the data in PROGMEM and 
// outputting the next value onto the port.

ISR(TCA0_OVF_vect) {
  uint8_t b0, b1, b2;                                                // Temporary variables that hold the next three compressed bytes

  if( sample_counter < sample_segments[current_segment] ){           // See if we are still within the current sample

    if(buffer_index == 4 ){                                          // If we have overrun the buffer, then read the next 3 bytes
      b0 = pgm_read_byte_near( sample + sample_counter++ );          // Read byte 0 from PROGMEM
      b1 = pgm_read_byte_near( sample + sample_counter++ );          // Read byte 1 from PROGMEM
      b2 = pgm_read_byte_near( sample + sample_counter++ );          // Read byte 2 from PROGMEM

      sample_buffer[0] = b0 & 0b11111100;                            // Decompress new byte 0 from byte 0
      sample_buffer[1] = ( (b0 << 6) | (b1 >> 2) ) & 0b11111100;     // Decompress new byte 1 from bytes 0 and 1
      sample_buffer[2] = ( (b1 << 4) | (b2 >> 4) ) & 0b11111100;     // Decompress new byte 2 from bytes 1 and 2
      sample_buffer[3] = (b2 << 2);                                  // Decompress new byte 3 from byte 2

      buffer_index = 0;                                              // Set the buffer index back to zero
    }

    PORTD.OUT = sample_buffer[buffer_index++];                       // Output the current decompressed byte onto PORTD

  } else {                                                           // If outside of the current sample's range
    PORTA.OUTCLR = PIN_LED_OPEN | PIN_LED_CLOSED;                    // Turn off the LEDs
    PORTC.OUTCLR = PIN_PLAYING | PIN_TRIGGER;                        // Turn off the control lines 
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                          // Don't forget to reset the interrupt flag!!
}
