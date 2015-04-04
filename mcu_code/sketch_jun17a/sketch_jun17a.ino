#include <OneWire.h>


const char TEMP_REQ[10] = "conv-temp";
const int TEMP_REQ_SIZE = 9;
int count = 0;


// DS18S20 Temperature chip i/o
OneWire ds(12);  // pass pin it's on

void setup(void) {
	// initialize inputs/outputs
	// start serial port
	Serial.begin(9600);
}

void loop(void) {

	char serial_in[100];
	int HighByte, LowByte, TReading, SignBit;
	float Temp;
	byte i;
	byte present = 0;
	byte data[12];
	byte addr[8];


	delay(100);	// Ensure all data is received
	int pos = 0;

	while(Serial.peek() != -1)
	{
		serial_in[pos] = Serial.read();
		++pos;
	}
	serial_in[pos] = '\0';
	Serial.flush();
	++count;


	ds.reset();
	ds.skip();
	ds.write(0x44);		// start conversion, with parasite power on at the end

	delay(900);		// maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.skip(); 
	ds.write(0xBE);		// Read Scratchpad

	for ( i = 0; i < 9; i++) {	// we need 9 bytes
		data[i] = ds.read();
	}
 
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;	// test most sig bit

        //for(i = 0; i < 9; i = i + 1) {
        Serial.print(TReading, BIN);
        Serial.println(" ");
	if (SignBit)			// negative
	{
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
  
	Temp = TReading;
	Temp = Temp / 16;
	Temp = Temp * 9 / 5 + 32;
       	//Serial.println(Temp);
        delay(1000);
}
