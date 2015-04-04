#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#DEFINE 7SEG_PWM_PIN 10

int toggle = 0;

int ones, tens, temp_int, intensity;

OneWire  ds(3);  // on pin 2

// Currently pins 4-10
// pin wise, top right to top left, bottom right to bottom left
int BCD_PINS[] =
		{	4,	// 0
			5,	// 1
			6,	// 2
			7};	// 3

void setup(void) {
  
  ones = 8;
  tens = 8;
  temp_int = 0;
  intensity = 50;  // 7-segment PWM intesity, out of 255
  
  analogWrite(7SEG_PWM_PIN, intensity)
  
  Serial.begin(9600);
  
  for(int i = 0; i < 4; i++) {
    pinMode(BCD_PINS[i], OUTPUT);
    digitalWrite(BCD_PINS[i], HIGH);
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
int SET_BCD_DRIVER( int number, int PINS[]) {
  // Find constituent numbers
  int i, bcd_bit;
  bcd_bit = 0;
  
  // Check that number can fit on one seven segment display
  if (number > 10 || number < 0) {
    return -1;
  }
  
  // Set each pin based off BCD digit
  for(i = 0; i < 4; i++) {
    bcd_bit = number & 1;       // Grab lowest bit
    if(bcd_bit) {
      digitalWrite(PINS[i], HIGH);
    }
    else {
      digitalWrite(PINS[i], LOW);
    }
    number = number >> 1;     // Shift segments
  }
  
  return 1;
}


ISR(TIMER2_COMPA_vect){//timer1 interrupt 8kHz toggles pin 9
//generates pulse wave of frequency 8kHz/2 = 4kHz (takes two cycles for full wave- toggle high then toggle low)
  cli();
  if (toggle){
    digitalWrite(11, LOW);
    SET_BCD_DRIVER(tens, BCD_PINS);
    digitalWrite(12, HIGH);
    toggle = 0;
  }
  else{
    digitalWrite(12, LOW);
    SET_BCD_DRIVER(ones, BCD_PINS);
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
  temp_int = fahrenheit + 0.5;
  tens = temp_int / 10;
  ones = temp_int % 10;
  digit = ones;
  SET_BCD_DRIVER(digit, BCD_PINS);
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
}

