#include <OneWire.h>

// Pin defines
#define DISP_PWM_PIN 10
#define MENU_PIN 2
#define UP_PIN 9
#define DOWN_PIN 8
#define RELAY_PIN 17
#define SENSOR_PIN 3

// Constant defines
#define START_TARGET_TEMP 65       // initial goal temperature
#define MENU_BUTTON_HOLD_TIME 3    // in seconds
#define MAX_TEMP_SETTING 75
#define MIN_TEMP_SETTING 20
#define TEMP_OVERSHOOT 3           // How high past target we will let temp rise
#define TEMP_UNDERSHOOT 1          // How far to cool under target temp
#define MIN_COMPRESSOR_DOWNTIME 300  // Have to wait at least 5 minutes (300 seconds) before turning compressor on again
#define PRINT_INTERVAL 30          // time in seconds

// Relay states
#define RELAY_ON HIGH
#define RELAY_OFF LOW

OneWire ds(SENSOR_PIN);  // One wire data pin

// Housekeeping variables
int digit, toggle, tens, ones, intensity;

// Important variables
int targetTemperature;

// Currently pins 4-10
// pin wise, top right to top left, bottom right to bottom left
int BCD_PINS[] =
		{	4,	// 0
			5,	// 1
			6,	// 2
			7};	// 3

// Function prototypes
int SET_BCD_DRIVER( int, int []);
void setDisplayIntensity(int);
void setDisplay(int);
int targetTempMenu(void);
float getTemperature(void);



void setup(void) {
  
  // Housekeeping variables
  digit = 0;
  toggle = 0;
  intensity = 50;  // 7-segment PWM intesity, out of 255
  
  setDisplay(88);
  setDisplayIntensity(intensity);
  
  // Important variables
  targetTemperature = START_TARGET_TEMP;
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);    // Make sure relay is OFF
  
  pinMode(DISP_PWM_PIN, OUTPUT);
  pinMode(MENU_PIN, INPUT);
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  
  Serial.begin(9600);
  
  for(int i = 0; i < 4; i++) {
    pinMode(BCD_PINS[i], OUTPUT);
    digitalWrite(BCD_PINS[i], HIGH);
  }
  
  // *************************
  //  SET UP INTERRUPTS
  // *************************
  //
  cli();//stop interrupts
  
  // Set up mode pin interrupt on INT1
  // must check SREG I-flag for INT1
  // set interrupt mask as well
  
  // Set ISC01, ISC00 to 0 for low level INT0 interrupt. Don't affect ISC10, 11
  EICRA &= 0xFF ^ (1 << ISC00);
  EICRA &= 0xFF ^ (1 << ISC01);
  
  // Set external interrupt mask to enable INT0, disable INT1
  EIMSK &= 0;
  EIMSK |= (1 << INT0);
  
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



void loop(void) {

  int temp = 0;
  float tempFloat = 0.0;  // Set it low so we won't turn on until we get a real temp
  int freezerState = 0;
  int printInterval = PRINT_INTERVAL;          // print every XX seconds
  unsigned long lastPrint = 0;
  unsigned long timeSinceLastShutoff = 0;      // Won't allow compressor to turn on for the first 300 seconds (5 minutes) of startup, and every 5 after freezer shuts off.  Timed in seconds.
  unsigned long lastCompressorShutoff = 0;    // Time in millis() that the freezer last shut off
  
  digitalWrite(RELAY_PIN, RELAY_OFF);
  
  while(1) {
    tempFloat = getTemperature();
    temp = (int) (tempFloat + 0.5);
    setDisplay(temp);
  
    if ((freezerState == 0) && (temp >= (targetTemperature + TEMP_OVERSHOOT)) && (timeSinceLastShutoff > MIN_COMPRESSOR_DOWNTIME)) {
      freezerState = 1;
      digitalWrite(RELAY_PIN, RELAY_ON);  // Turn it on
      //Serial.println("FREEZER ON");
    }
    else if ( (freezerState == 1) && (temp <= (targetTemperature - TEMP_UNDERSHOOT)) ) {
      freezerState = 0;
      digitalWrite(RELAY_PIN, RELAY_OFF); // Turn it off
      //Serial.println("FREEZER OFF");
      
      // Update compressor shutoff time
      lastCompressorShutoff = millis();
    }
  
    delay (25);
    timeSinceLastShutoff =  ( millis() - lastCompressorShutoff ) / 1000;    // Time in seconds
    delay (100);
    
    if( ((millis() - lastPrint) / 1000) >= printInterval ) {
      Serial.print(targetTemperature);
      Serial.print(",");
      Serial.print(tempFloat);
      Serial.print(",");
      //Serial.print((millis() - lastPrint) / 1000);
      //Serial.print(",");
      if(freezerState) {
        Serial.println("On");
      } else {
        Serial.println("Off");
      }
      lastPrint = millis();
    }
    
    // In case millis overflows
    if(millis() < lastCompressorShutoff) {
      // We've overflowed
      lastCompressorShutoff = 0;
      lastPrint = 0;
    }
  }

}



// Interrupt for display muliplexing
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


// Interrupt for menu button press
ISR(INT0_vect){
  // Critical zone
  cli();
  EIMSK &= 0;  // 0b 1111 1110
  sei();
  int holdTime = (MENU_BUTTON_HOLD_TIME * 1000) / 10;
  int delayElapsed = 0;    // in tens of milliseconds, or centiseconds
  int toggleDisplay = 1;
  int displayOld = (tens * 10) + ones;
  
  // Set display to 88 to acknowledge button press
  setDisplay(88);
  
  // Disable this interupt for the time being
  // DelayElapsed will overflow after 5.5 minutes or so
  while(digitalRead(MENU_PIN) == LOW) {
    delay(10);
    // Blink the digits every 250 ms if we've crossed the required hold time
    // but stay in this loop.
    if(delayElapsed > holdTime) {
      if(delayElapsed % 25 == 0) {
        toggleDisplay = (toggleDisplay + 1) % 2;
        if(toggleDisplay) {
          setDisplayIntensity(intensity);  // Turn on
        } else {
          setDisplayIntensity(0);          // Turn off
        }
      }
    }
    ++delayElapsed;
  }
  
  // Make sure display gets turned back on
  setDisplayIntensity(intensity);
  
  // The button has been let go, if time threshold reached go to menu procedure
  if(delayElapsed > holdTime) {
    targetTempMenu();
  }
  
  // Set display back to original output before interrupt
  setDisplay(displayOld);
  
  // Critical zone, renable this input then exit
  cli();
  EIMSK |= (1 << INT0);
  sei();
}


// Actually takes the ones/tens numbers and updates
// the 4511 BCD to 7 segment driver with single BCD digit.
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


// Sets the PWM intensity of the 7 segment display, 0 - 255.
void setDisplayIntensity(int pwm) {
  if(pwm < 0) {
    pwm = 0;
  }
  if(pwm > 255) {
    pwm = 255;
  }
  analogWrite(DISP_PWM_PIN, pwm);
}


// Splits the number into 10s and 1s and pushes output
// to the display
void setDisplay(int num) {
  if(num < 100 && num >= 0) {
    tens = num / 10;
    ones = num % 10;
  }
}


// Menu for setting target temperature.  Button presses
// are asserted LOW.  Would be nice to save the temperature
// somewhere static in case of power outage.
int targetTempMenu(void) {
  int tempTarget =  targetTemperature;
  int pressDelay = 3;             // 3 ms
  setDisplay(tempTarget);
  
  // Must press menu pin again to exit menu
  while(digitalRead(MENU_PIN) == HIGH) {
    if(digitalRead(UP_PIN) == LOW) {
      if(tempTarget < MAX_TEMP_SETTING) {
        setDisplay(++tempTarget);
      }
      while(digitalRead(UP_PIN) == LOW) {
        delay(pressDelay);
      }
    }
    else if(digitalRead(DOWN_PIN) == LOW) {
      if(tempTarget > MIN_TEMP_SETTING) {
        setDisplay(--tempTarget);
      }
      while(digitalRead(DOWN_PIN) == LOW) {
        delay(pressDelay);
      }
    }
    
    targetTemperature = tempTarget;
  }
  
  // Catch exit button press so we don't restart interrupt
  while(digitalRead(MENU_PIN) == LOW) {
    delay(pressDelay);
  }
  
  // Blink the chosen temperature on the way out as affirmation
  for(int i = 0; i < 4; i++) {
    setDisplayIntensity(0);
    delay(250);
    setDisplayIntensity(intensity);
    delay(250);
  }
}

float getTemperature(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  int temp_int;
  
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      //Serial.println("CRC is not valid!");
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44);         // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

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
  //temp_int = fahrenheit + 0.5;
  
  // Set BCD digits, background interrupt will send to display
  setDisplay(temp_int);
  
  return fahrenheit;    // return the rounded integer temperature
}

