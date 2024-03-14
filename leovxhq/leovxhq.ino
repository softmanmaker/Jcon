#include <Arduino.h>
#include <EEPROM.h>
#include "iivx_leo.h"
iivxReport_t report; 

#define REPORT_DELAY 1000
// Number of microseconds between HID reports
// 1000 = 1000hz
#define LIGHT_ADDR 0
#define ENC_L_A 0
#define ENC_L_B 1
#define ENC_L_B_ADDR 3
#define ENC_R_A 2
#define ENC_R_B 3
#define ENC_R_B_ADDR 0
#define ENCODER_SENSITIVITY (double) 2.34375
#define ENCODER_PORT PIND
// encoder sensitivity = number of positions per rotation (600/256) / number of positions for HID report (256)
/*
 * connect encoders
 * VOL-L to pins 0 and 1
 * VOL-R to pins 2 and 3
 */

int tmp;
uint8_t buttonCount = 7;
uint8_t lightMode = 2;
uint8_t mode = 0;
uint8_t ledPins[] = {6,7,8,9,10,11,12};
uint8_t buttonPins[] = {13,18,19,20,21,22,23};
uint8_t vccPin = 4;
uint8_t gndPin = 5;
int32_t encL = 0, encR = 0;
/* current pin layout
 *  pins 6 to 12 = LED 1 to 7
 *    connect pin to + termnial of LED
 *    connect ground to resistor and then - terminal of LED
 *  pins 13, A0 to A5 = Button input 1 to 7
 *    connect button pin to ground to trigger button press
 *  pin 4 = VCC
 *  pin 5 = GND
 */
 

void doEncL(){
  if((ENCODER_PORT >> ENC_L_B_ADDR)&1){
    encL++;
  } else {
    encL--;
  }
}

void doEncR(){
  if((ENCODER_PORT >> ENC_R_B_ADDR)&1){
    encR++;
  } else {
    encR--;
  }
}

void lights(uint8_t lightDesc){
  for(int i=0;i<buttonCount;i++){
     if((lightDesc>>i)&1){
         digitalWrite(ledPins[i],HIGH);
     } else {
         digitalWrite(ledPins[i],LOW);
     }
  }
}

void setup() {
  delay(1000);
  // Setup I/O for pins
  for(int i=0;i<buttonCount;i++){
    pinMode(buttonPins[i],INPUT_PULLUP);
    pinMode(ledPins[i],OUTPUT);
  }
  pinMode(vccPin,OUTPUT);
  digitalWrite(vccPin,HIGH);
  pinMode(gndPin,OUTPUT);
  digitalWrite(gndPin,LOW);
  //setup interrupts
  pinMode(ENC_L_A,INPUT_PULLUP);
  pinMode(ENC_L_B,INPUT_PULLUP);
  pinMode(ENC_R_A,INPUT_PULLUP);
  pinMode(ENC_R_B,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), doEncL, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), doEncR, RISING);
  iivx = iivx_();
  lightMode = EEPROM.read(LIGHT_ADDR);
  if(lightMode>3) {
    lightMode = 2;
    EEPROM.write(LIGHT_ADDR,2);
  }
}

void loop() {
  // Read buttons
  report.buttons = 0;
  for(int i=0;i<buttonCount;i++)
    if(digitalRead(buttonPins[i])!=HIGH)
      report.buttons |= (uint16_t)1 << i;
  // Read Encoders
  report.xAxis = (uint8_t)((int32_t)(encL / ENCODER_SENSITIVITY) % 256);
  report.yAxis = (uint8_t)((int32_t)(encR / ENCODER_SENSITIVITY) % 256);
  // Light LEDs
  switch(mode) {
    case 0:
      switch(lightMode) {
        case 0:
          lights(0);
          break;
        case 1:
          lights(127);
          break;
        case 2:
          lights(report.buttons);
          break;
        case 3:
          lights(iivx_led);
          break;
        default:
          break;
      }
      break;
    case 1:
      lights(2<<lightMode|97);
      break;
    case 2:
      lights(109);
      break;
    default:
      break;
  }
  // Detect lighting and state changes
  switch(mode) {
    case 0:
      switch(report.buttons) {
        case 3:
          mode = 1;
          break;
        case 17:
          mode = 2;
          break;
        case 5:
          report.buttons = (uint16_t)1 << (buttonCount);
          break;
        case 9 :
          report.buttons = (uint16_t)1 << (buttonCount + 1);
          break;
        default:
          break;
      }
      break;
    case 1:
      switch(report.buttons) {
        case 96:
          mode = 0;
          break;
        case 2:
          lightMode = 0;
          if(lightMode != EEPROM.read(LIGHT_ADDR)) EEPROM.write(LIGHT_ADDR,lightMode);
          break;
        case 4:
          lightMode = 1;
          if(lightMode != EEPROM.read(LIGHT_ADDR)) EEPROM.write(LIGHT_ADDR,lightMode);
          break;
        case 8:
          lightMode = 2;
          if(lightMode != EEPROM.read(LIGHT_ADDR)) EEPROM.write(LIGHT_ADDR,lightMode);
          break;
        case 16:
          lightMode = 3;
          if(lightMode != EEPROM.read(LIGHT_ADDR)) EEPROM.write(LIGHT_ADDR,lightMode);
          break;
        default:
          break;
      }
      break;
    case 2:
      switch(report.buttons) {
        case 4:
          report.buttons = (uint16_t)1 << (buttonCount);
          break;
        case 8:
          report.buttons = (uint16_t)1 << (buttonCount + 1);
          break;
        case 96:
          mode = 0;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  // Send report and delay
  iivx.setState(&report);
  delayMicroseconds(REPORT_DELAY);
}

