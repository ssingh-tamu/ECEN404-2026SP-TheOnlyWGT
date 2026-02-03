#include <Arduino.h>
#include <string>
#include <bitset>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;

void sendFrame(string); // final remove

// FPGA SPI communication functions

void sendBits(int, int);
void setMemory(bool, int, int);
void setReset(bool, int);
void setTuningWord(bool, int);
void run();

// Waveform creation functions

void createSin(bool, int, int, int);
void createSnc(bool, int, int, int);
void createSaw(bool, int); // final remove
void createTri(bool, int, int, int);
void createSqu(bool, int, int, int);

// Main function to set all parameters and create waveform

void setAll(int, int, int, int, bool);

// MCU pin definitions

#define mosi 10
#define rstn 11
#define sclk 12
#define cs_n 13

// sends 35 bit frame to FPGA with SPI
// - frame must be 9 hex characters
// - first hex character must be [0, 7]
void sendFrame(string frame) {

  //decode hex
  ostringstream binaryStream;

  unsigned int firstValue = stoul(frame.substr(0, 1), nullptr, 16);
  bitset<4> firstBits(firstValue);
  binaryStream << firstBits.to_string().substr(1);
  
  for (size_t i = 1; i < frame.size(); ++i) {
    unsigned int value = stoul(frame.substr(i, 1), nullptr, 16);
    bitset<4> bits(value);
    binaryStream << bits.to_string();
  }

  string bframe = binaryStream.str();

  //set pins
  digitalWrite(cs_n, LOW);

  for (char b : bframe) {

    if (b == '0') 
      digitalWrite(mosi, LOW);
    else 
      digitalWrite(mosi, HIGH);

    digitalWrite(sclk, HIGH);
    delayMicroseconds(10);
    digitalWrite(sclk, LOW);
    delayMicroseconds(10);

  }

  digitalWrite(cs_n, HIGH);
  delayMicroseconds(20);

}

// sends number to FPGA via SPI
// - num is a number in decimal
// - len is number of bits (3, 16, or 32)
// - should only be called by set memory, reset, and tuning word functions
void sendBits(int len, int num) {
  for (int i = len-1; i >= 0; i--) {
    if (num >= (1 << i)) {
      num -= (1 << i);
      digitalWrite(mosi, HIGH);
    } else {
      digitalWrite(mosi, LOW);
    }
    digitalWrite(sclk, HIGH);
    delayMicroseconds(1);
    digitalWrite(sclk, LOW);
    delayMicroseconds(1);
  }
}

void setMemory(bool channel, int address, int value) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 1 : 0);
  sendBits(16, address);
  sendBits(16, value);
  digitalWrite(cs_n, HIGH);
  // delayMicroseconds(5);
}

void setReset(bool channel, int reset) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 3 : 2);
  sendBits(16, reset);
  sendBits(16, 0);
  digitalWrite(cs_n, HIGH);
  // delayMicroseconds(5);
}

void setTuningWord(bool channel, int tuningWord) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 5 : 4);
  sendBits(16, tuningWord >> 16);
  sendBits(16, tuningWord % (1 << 16));
  digitalWrite(cs_n, HIGH);
  // delayMicroseconds(5);
}

void run() {
  digitalWrite(cs_n, LOW);
  sendBits(3, 7);
  sendBits(16, 0);
  sendBits(16, 0);
  digitalWrite(cs_n, HIGH);
  // delayMicroseconds(10);
}

// creates sin wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSin(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) return;

  double inc = 2*M_PI/width;
  for (int i = 0; i < width; i++) {
    // int dec = (sin(inc*i)*amplitude+offset*2+10000)*65535/20000;
    int dec = (sin(inc*i)*0.5+0.5)*65535;
    if (dec > 65535) dec = 65535;
    else if (dec < 0) dec = 0;

    setMemory(channel, i, dec);
  }
}

// creates sinc wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSnc(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) return;

  double inc = 14.06619*2/width;
  for (int i = 0; i < width; i++) {
    int dec = (sin(inc*i-14.06619)*amplitude/(inc*i-14.06619)+offset*2+10000)*65535/20000;
    if (dec > 65535) dec = 65535;
    else if (dec < 0) dec = 0;

    setMemory(channel, i, dec);
  }
}

// creates saw tooth wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSaw(bool channel, int width) {
  if (width > 65536) return;
  
  for (int i = 0; i < width; i++) {
    int dec = i*65535/(width-1);
    setMemory(channel, i, dec);
  }
}

// creates triangle wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createTri(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) return;
  
  setMemory(channel, 0, 32678-3.2678*amplitude+6.5535*offset);
  for (int i = 0; i < width/2; i++) {
    int dec = 2*(i+1)*6.5535*amplitude/width + 32678 - 3.2678*amplitude + 6.5535*offset;
    if (dec > 65535) dec = 65535;
    else if (dec < 0) dec = 0;

    setMemory(channel, i+1, dec);
    setMemory(channel, width-1-i, dec);
  }
}

// creates a square wave
// - SRAM0 - channel = false, SRAM1 - channel = true
void createSqu(bool channel, int frequency, int amplitude, int offset) {
  setReset(channel, 1);

  // int stw = frequency*2*(1 << 31)/100000000000;
  int stw = frequency*0.042949673;
  setTuningWord(channel, stw);
  
  int top = (10000*32768+amplitude*32768+offset*65536)/10000;
  if (top > 65535) top = 65535;
  else if (top < 0) top = 0;  
  int bot = (10000*32768-amplitude*32768+offset*65536)/10000;
  if (bot > 65535) bot = 65535;
  else if (bot < 0) bot = 0;

  setMemory(channel, 0, top);
  setMemory(channel, 1, bot);
}

// takes in data received from app
// - shape:   0 = sine,   1 = square,   2 = triangle,   3 = sinc,   4 = arbitrary
// - frequency (0 to 100000000 mHz),   amplitude (0 to 10000 mV),   offset (-5000 to 5000 mV)
// - channel:   false = Channel0,   true = Channel1
void setAll(int shape, int frequency, int amplitude, int offset, bool channel) {

  // determine high or low frequency
  int width = 0;
  if (frequency <= 15258) { // 15.258 Hz
    // low frequency, variable tuning word

    width = 65535; // Use max width of 2^16
    setReset(channel, 65535);
    // int tw = frequency*(1 << 48)/100000000000;
    int tw = frequency*1407.37488;
    setTuningWord(channel, tw);

  } else {
    // high frequency, variable reset

    width = 1000000000/frequency;
    setReset(channel, width);
    setTuningWord(channel, 21474836); // set tuning word to 1 MHz, 0.01(2^31) = 21474836
  }

  // call function to generate shape
  switch(shape) {
    case 0:
      createSin(channel, width, amplitude, offset);
      break;
    case 1:
      createSqu(channel, frequency, amplitude, offset);
      break;
    case 2:
      createTri(channel, width, amplitude, offset);
      break;
    case 3:
      createSnc(channel, width, amplitude, offset);
      break;
    case 4:
      // arbitrary waveform
      break;
  }

  // start waveform generation
  run();
}

void setup() {

  Serial.begin(115200);
  pinMode(rstn, OUTPUT);
  pinMode(sclk, OUTPUT);
  pinMode(mosi, OUTPUT);
  pinMode(cs_n, OUTPUT);

  digitalWrite(rstn, LOW);
  digitalWrite(sclk, LOW);
  digitalWrite(mosi, LOW);
  digitalWrite(cs_n, LOW);
  delayMicroseconds(20);
  digitalWrite(rstn, HIGH);

  setAll(0, 230*1000, 5*1000, 0*1000, false);
  // delay(5000);
  // setReset(true, 4347);
  // run();
  // delay(5000);
  // setReset(true, 4347/2);
  // run();

  // // 15 -> 240 repeating memory??????
  // // 244 = 4096 = 2^12????
  // // memory transfer at 65k too slow

  // setAll(1, 1000*1000, 10*1000, 0*1000, true);
  // // sqaure works
  // setAll(1, 0.024*1000, 10*1000, 0*1000, true);
  // // works at very low frequency, about 1 cycle every 42 seconds
  // setAll(1, 100000*1000, 0.1*1000, 5*1000, true);
  // // works at very high frequency

  // setAll(2, 1000*1000, 5*1000, 0*1000, true);
  // tri doesn't work at all

  // setAll(3, 1000*1000, 10*1000, -2*1000, true);
  // // sinc works, but equation needs adjustment
  // setAll(3, 10000*1000, 10*1000, -2*1000, true);
  // // works, test higher later
  // setAll(3, 100*1000, 10*1000, -2*1000, true);

  

  // //=// square wave //=//
  // setReset(true, 1);
  // setTuningWord(true, 4095);
  // setMemory(true, 0, 65535);
  // setMemory(true, 1, 0);
  // run();

  // sendFrame("20001FFFF");
  // sendFrame("4000FFFFF");
  // sendFrame("000000000");
  // sendFrame("00001FFFF");

  // sendFrame("30001FFFF");
  // sendFrame("5000FFFFF");
  // sendFrame("100000000");
  // sendFrame("10001FFFF");

  // sendFrame("7FFFFFFFF");

  //=// sin and saw waves //=//
  // sendFrame("500FFFFFF"); // set phase0 to h00FFFFFF
  // sendFrame("303FFFFFF"); // set reset0 to 1023
  // createSin(1, 1024);     // load sin wave into SRAM0
  // // sendFrame("20003FFFF"); // set reset0 to 3
  // // createSaw(0, 4);
  // sendFrame("7FFFFFFFF");

}

void loop() {

  // setAll(0, 1000000, 4000, 1000, 1);
  // delay(2000);
  // setAll(0, 1000000, 8000, 1000, 1);
  // delay(2000);
  // setTuningWord(1, 10737418);
  // run();
  // delay(2000);
  // setTuningWord(1, 42949672);
  // run();
  // delay(2000);

  // setAll(1, 230*1000, 5*1000, 0*1000, true);
  // delay(2000);


}
