#include <Arduino.h>
#include <string>
#include <bitset>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;

void sendFrame(string);
void createSin(bool, int, int, int);
void createSnc(bool, int, int, int);
void createSaw(bool, int);
void createTri(bool, int, int, int);
void createSqu(bool, int, int, int);
void setAll(int, int, int, int, bool);

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

void setMemory(bool channel, int address, int value) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 1 : 0);
  sendBits(16, address);
  sendBits(16, value);
  digitalWrite(cs_n, HIGH);
  delayMicroseconds(20);
}

void setReset(bool channel, int reset) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 3 : 2);
  sendBits(16, reset);
  sendBits(16, 0);
  digitalWrite(cs_n, HIGH);
  delayMicroseconds(20);
}

void setTuningWord(bool channel, int tuningWord) {
  digitalWrite(cs_n, LOW);
  sendBits(3, channel ? 5 : 4);
  sendBits(32, tuningWord);
  digitalWrite(cs_n, HIGH);
  delayMicroseconds(20);
}

// sends number to FPGA via SPI
// - num is a number in decimal
// - len is number of bits (3, 16, or 32)
void sendBits(int len, int num) {
  for (int i = len; i >= 0; i--) {
    if (num > 2^i) {
      num -= 2^i;
      digitalWrite(mosi, HIGH);
    } else {
      digitalWrite(mosi, LOW);
    }
    digitalWrite(sclk, HIGH);
    delayMicroseconds(10);
    digitalWrite(sclk, LOW);
    delayMicroseconds(10);
  }
}

// creates sin wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSin(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) {
    return;
  }

  double inc = 2*M_PI/width;
  char chanHex = channel ? '1' : '0';
  for (int i = 0; i < width; i++) {
    int dec = (sin(inc*i)*amplitude+offset*2+10000)*65535/20000;
    if (dec > 65535) dec = 65535;
    else if (dec < 0) dec = 0;

    stringstream ss;
    ss << chanHex << uppercase << setfill('0') << setw(4) << hex << i << setw(4) << hex << dec;
  
    sendFrame(ss.str());
  }
}


// creates sinc wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSnc(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) {
    return;
  }

  double inc = 14.06619*2/width;
  char chanHex = channel ? '1' : '0';
  for (int i = 0; i < width; i++) {
    int dec = (sin(inc*i-14.06619)*amplitude/(inc*i-14.06619)+offset*2+10000)*65535/20000;
    if (dec > 65535) dec = 65535;
    else if (dec < 0) dec = 0;

    stringstream ss;
    ss << chanHex << uppercase << setfill('0') << setw(4) << hex << i << setw(4) << hex << dec;
  
    sendFrame(ss.str());
  }
}

// creates saw tooth wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSaw(bool channel, int width) {
  if (width > 65536) {
    return;
  }
  
  char chanHex = channel ? '1' : '0';
  for (int i = 0; i < width; i++) {
    int dec = i*65535/(width-1);
    
    stringstream ss;
    ss << chanHex << uppercase << setfill('0') << setw(4) << hex << i << setw(4) << hex << dec;
  
    sendFrame(ss.str());
  }
}

// creates triangle wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createTri(bool channel, int width, int amplitude, int offset) {
  if (width > 65536) {
    return;
  }
  
  if (channel) sendFrame("100000000"); else sendFrame("000000000");
    
  char chanHex = channel ? '1' : '0';
  for (int i = 0; i < width/2; i++) {
    int dec = 2*(i+1)*65535/width;

    stringstream sf;
    stringstream sb;
    sf << chanHex << uppercase << setfill('0') << setw(4) << hex << (i+1) << setw(4) << hex << dec;
    sb << chanHex << uppercase << setfill('0') << setw(4) << hex << (width-1-i) << setw(4) << hex << dec;

    sendFrame(sf.str());
    sendFrame(sb.str());
  }
}

// creates a square wave
// - SRAM0 - channel = false, SRAM1 - channel = true
void createSqu(bool channel, int frequency, int amplitude, int offset) {
  if (channel) sendFrame("30001FFFF"); else sendFrame("20001FFFF");

  int stw = frequency*2*(2^32)/100000000000;
  char chanHex = channel ? '5' : '4';
  stringstream sss;
  sss << chanHex << uppercase << setfill('0') << setw(8) << hex << stw;
  sendFrame(sss.str());
  
  chanHex = channel ? '1' : '0';
  int top = (10000*2^15+amplitude*2^15+offset*2^16)/10000;
  if (top > 65535) top = 65535;
  else if (top < 0) top = 0;  
  int bot = (10000*2^15-amplitude*2^15+offset*2^16)/10000;
  if (bot > 65535) bot = 65535;
  else if (bot < 0) bot = 0;

  stringstream st;
  stringstream sb;
  st << chanHex << "0000" << uppercase << setfill('0') << setw(4) << hex << top;
  sb << chanHex << "0001" << uppercase << setfill('0') << setw(4) << hex << bot;
  sendFrame(st.str());
  sendFrame(sb.str());
}

// takes in data received from app
// - shape: 0 = sine, 1 = square, 2 = triangle, 3 = sinc
// - frequency (0 to 100000000 mHz), amplitude (0 to 10000 mV), offset (-5000 to 5000 mV)
// - channel: false = Channel0, true = Channel1
void setAll(int shape, int frequency, int amplitude, int offset, bool channel) {

  // determine high or low frequency
  int width = 0;
  if (frequency <= 15258) { // 15.258 Hz
    // low frequency, variable tuning word

    width = 65535; // Use max width of 2^16
    if (channel) sendFrame("3FFFFFFFF"); else sendFrame("2FFFFFFFF"); // set reset to 2^16
    int tw = frequency*(2^48)/100000000000;

    // set tuning word
    char chanHex = channel ? '5' : '4';
    stringstream ss;
    ss << chanHex << uppercase << setfill('0') << setw(8) << hex << tw;
    sendFrame(ss.str());

  } else {
    // high frequency, variable reset

    if (channel) sendFrame("5028F5C29"); else sendFrame("4028F5C29"); // set sample rate to 1 MHz
    width = 1000000000/frequency;
    
    // set reset
    char chanHex = channel ? '3' : '2';
    stringstream ss;
    ss << chanHex << uppercase << setfill('0') << setw(4) << hex << width << "FFFF";
    sendFrame(ss.str());

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
  }

  // start waveform generation
  sendFrame("7FFFFFFFF");
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

  setAll(0, 1000000, 5000, 1000, 0);
  // setAll(0, 1000000, 5000, 1000, 1);

  //=// square wave //=//
  // sendFrame("30001FFFF");
  // // // sendFrame("43FFFFFFF");
  // sendFrame("50FFFFFFF");
  // sendFrame("100000000");
  // sendFrame("10001FFFF");
  // sendFrame("7FFFFFFFF");

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

  // digitalWrite(rstn, LOW);
  // delay(100);
  // digitalWrite(rstn, HIGH);
  // sendFrame("20004FFFF"); // set reset0 to 4
  // sendFrame("30003FFFF"); // set reset1 to 3
  // sendFrame("400FFFFFF"); // set phase0
  // sendFrame("5000FFFFF"); // set phase1
  // sendFrame("000009999"); // set SRAM0 values
  // sendFrame("00001AAAA");
  // sendFrame("00002BBBB");
  // sendFrame("00003CCCC");
  // sendFrame("00004DDDD");
  // sendFrame("100000000"); // set SRAM1 values
  // sendFrame("100011111"); 
  // sendFrame("100022222"); 
  // sendFrame("100033333"); 
  // sendFrame("7FFFFFFFF"); // run
  // delay(5000);
  // sendFrame("20001FFFF"); // set reset0 to 1
  // sendFrame("30002FFFF"); // set reset1 to 2
  // sendFrame("000001111");
  // sendFrame("100009999"); 
  // sendFrame("100018888"); 
  // sendFrame("100027777"); 
  // sendFrame("4007FFFFF"); // slow down clock0 by 1/2
  // sendFrame("7FFFFFFFF"); // run
  // delay(5000);

  setAll(0, 1000000, 4000, 1000, 1);
  delay(2000);
  setAll(0, 1000000, 8000, 1000, 1);
  delay(2000);

}
