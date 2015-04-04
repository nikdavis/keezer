

//storage variables
int toggle = 0;

int SEV_SEG[] = {	0x3F,
			0x06,
			0x5B,
			0x4F,
			0x66,
			0x6D,
			0x7D,
			0x07,
			0x7F,
			0x67 };

// Currently pins 4-10
// pin wise, top right to top left, bottom right to bottom left
int SEV_SEG_PINS[] =
		{	5,	// a
			4,	// b
			7,	// c
			9,	// d
			10,	// e
			6,	// f
			8 };	// g

int tens, ones;

void setup(){
  tens = 1;
  ones = 3;
  
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  for(int i = 0; i < 7; i++) {
    pinMode(SEV_SEG_PINS[i], OUTPUT);
    digitalWrite(SEV_SEG_PINS[i], HIGH);
  }

  cli();//stop interrupts

//set timer2 interrupt at 2kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 124;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 8 prescaler
  TCCR2B |= (1 << CS11) | (1 << CS10);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);


sei();//allow interrupts

}//end setup




// For now number < 100
int SET_SEVEN_SEG( int number, int OUT[], int PINS[]) {
  // Find constituent numbers
  int i, sev_hex, sev_bit;
  sev_bit = 0;
  
  // Check that number can fit on one seven segment display
  if (number > 10 || number < 0) {
    return -1;
  }
  
  // Grab the pin configuration for this
  // digit, i.e: gfedcba (bits)
  sev_hex = OUT[number];
  
  // Set each pin
  for(i = 0; i < 7; i++) {
    sev_bit = sev_hex & 1;      // Grab lowest bit, ie state of next segment
    if(sev_bit == 0) {
      digitalWrite(PINS[i], HIGH);
    }
    else {
      digitalWrite(PINS[i], LOW);
    }
    sev_hex = sev_hex >> 1;     // Shift segments
  }
  
  return 1;
}

ISR(TIMER2_COMPA_vect){//timer1 interrupt 8kHz toggles pin 9
//generates pulse wave of frequency 8kHz/2 = 4kHz (takes two cycles for full wave- toggle high then toggle low)
  if (toggle){
    digitalWrite(11, LOW);
    SET_SEVEN_SEG(tens, SEV_SEG, SEV_SEG_PINS);
    digitalWrite(12, HIGH);
    toggle = 0;
  }
  else{
    digitalWrite(12, LOW);
    SET_SEVEN_SEG(ones, SEV_SEG, SEV_SEG_PINS);
    digitalWrite(11, HIGH);
    toggle = 1;
  }
}


void loop() {
  while(1) {
  }
}



