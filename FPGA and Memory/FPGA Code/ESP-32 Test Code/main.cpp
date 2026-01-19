#include <Arduino.h>
#include <string>
#include <bitset>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;

void sendFrame(string);
void createSin(bool, int);
void createSaw(bool, int);
void createTri(bool, int);

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

// creates sin wave by writing amplitude values to memory values [0, width)
// - SRAM0 - channel = false, SRAM1 - channel = true
// - width must be between [1, 65536]
void createSin(bool channel, int width) {
  if (width > 65536) {
    return;
  }

  double inc = 2*M_PI/width;
  char chanHex = channel ? '1' : '0';
  for (int i = 0; i < width; i++) {
    int dec = (sin(inc*i)+1)/2*65535;
    
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
void createTri(bool channel, int width) {
  if (width > 65536) {
    return;
  }
  
  if (channel) 
    sendFrame("100000000");
  else 
    sendFrame("000000000");
    
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

  //=// square wave //=//
  sendFrame("30001FFFF");
  // sendFrame("43FFFFFFF");
  sendFrame("50FFFFFFF");
  sendFrame("100000000");
  sendFrame("100010001");
  sendFrame("7FFFFFFFF");

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

}
