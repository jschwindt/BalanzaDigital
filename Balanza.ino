#include <HX711.h>

HX711 scale(A2, A3);

#define SERIAL_DEBUG

#define NUM_DIGITS            4
#define NUM_SEGMENTS          7
#define _Off                  10
#define _Dash                 11
#define _E                    12
#define _r                    13
#define _d_write(d4,d3,d2,d1) digit_data[3] = d4; digit_data[2] = d3; digit_data[1] = d2; digit_data[0] = d1

const byte TARE_BUTTON = 3;

const byte anode[NUM_DIGITS] = { 2,     A0,     13,      10 };
                            // Digit4, Digit3, Digit2, Digit1

const byte cathode[NUM_SEGMENTS] = { 11, A1, 6,  8,  9,  12, 5 };
                                //   A,  B,  C,  D,  E,  F,  G

const byte seven_segments[14][NUM_SEGMENTS] = {
  { 0,0,0,0,0,0,1 },  // = 0
  { 1,0,0,1,1,1,1 },  // = 1
  { 0,0,1,0,0,1,0 },  // = 2
  { 0,0,0,0,1,1,0 },  // = 3
  { 1,0,0,1,1,0,0 },  // = 4
  { 0,1,0,0,1,0,0 },  // = 5
  { 0,1,0,0,0,0,0 },  // = 6
  { 0,0,0,1,1,1,1 },  // = 7
  { 0,0,0,0,0,0,0 },  // = 8
  { 0,0,0,0,1,0,0 },  // = 9
  { 1,1,1,1,1,1,1 },  // = 10 (OFF)
  { 1,1,1,1,1,1,0 },  // = 11 ("-")
  { 0,1,1,0,0,0,0 },  // = 12 ("E")
  { 1,1,1,1,0,1,0 }   // = 13 ("r")
};

byte current_digit, current_segment;
byte digit_data[NUM_DIGITS];
long tare;

void setup_ports() {
  pinMode(TARE_BUTTON, INPUT_PULLUP);

  for (byte a = 0; a < NUM_DIGITS; ++a) {
    pinMode(anode[a], OUTPUT);
    digitalWrite(anode[a], 0);
  }
  for (byte b = 0; b < NUM_SEGMENTS; ++b) {
    pinMode(cathode[b], OUTPUT);
    digitalWrite(cathode[b], 1);
  }
}

void write_number(int number) {
  if (number > 9999) {
    _d_write(_Off, _E, _r, _r);  return;
  }
  if (number < -999) {
    _d_write(_Dash, _E, _r, _r); return;
  }
  byte i = 0;
  int value = number < 0 ? -number : number;
  do {
    digit_data[i++] = value % 10;
  } while ((value /= 10) != 0);

  if (number < 0) {
    digit_data[i++] = _Dash;
  }
  while (i < NUM_DIGITS) {
    digit_data[i++] = _Off;
  }
}

void setup_timer_interrrupt() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 32;               // compare match register 16MHz/256/2000Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  sei();
}

ISR(TIMER1_COMPA_vect) {
  // First turn off current digit/segment
  digitalWrite(anode[current_digit], 0);
  digitalWrite(cathode[current_segment], 1);

  // Find next digit/segment
  if (++current_segment == NUM_SEGMENTS) {
    current_segment = 0;
    if (++current_digit == NUM_DIGITS) {
      current_digit = 0;
    }
  }

  // Turn on new segment
  digitalWrite(anode[current_digit], 1);
  digitalWrite(cathode[current_segment], seven_segments[ digit_data[current_digit] ][current_segment]);
}

void setup() {
  #if defined(SERIAL_DEBUG)
    Serial.begin(115200);
  #endif

  current_digit   = 0;
  current_segment = 0;
  setup_ports();
  setup_timer_interrrupt();

  tare = scale.read_average(10);
}

void loop() {
  long value;
  int weight;

  value = scale.read();
  weight = (value - tare) / 483;

  #if defined(SERIAL_DEBUG)
    Serial.print(value);
    Serial.print("\t");
    Serial.print(weight);
    Serial.println(" gr.");
  #endif

  write_number(weight);

  if (digitalRead(TARE_BUTTON) == LOW) {
    tare = scale.read_average(10);
  }
  delay(300);
}

// (199500-130400)/143
