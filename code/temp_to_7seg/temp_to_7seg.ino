#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library


int toggle = 0;

int ones, tens, temp_int;

OneWire  ds(2);  // on pin 10

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

void setup(void) {
  
  ones = 0;
  tens = 0;
  temp_int = 0;
  
  Serial.begin(9600);
  for(int i = 0; i < 7; i++) {
    pinMode(SEV_SEG_PINS[i], OUTPUT);
    digitalWrite(SEV_SEG_PINS[i], LOW);
  }
  
  
  
    cli();//stop interrupts

//set timer2 interrupt at 2kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 8 prescaler
  TCCR2B |= (1 << CS11); // | (1 << CS10);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);


  sei();//allow interrupts
}


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
    if(!sev_bit) {
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
  cli();
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
  sei();
}


void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  int digit;
  
  delay(500);
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // convert the data to actual temperature

  unsigned int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  temp_int = fahrenheit + 0.5
  tens = temp_int / 10;
  ones = temp_int % 10;
  SET_SEVEN_SEG( digit, SEV_SEG, SEV_SEG_PINS);
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
}

