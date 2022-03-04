#include <Arduino.h>
#include <U8g2lib.h>
#include <iostream>
#include <STM32FreeRTOS.h> 

//Constants
  const uint32_t interval = 100; //Display update interval

//Pin definitions
  //Row select and enable
  const int RA0_PIN = D3;
  const int RA1_PIN = D6;
  const int RA2_PIN = D12;
  const int REN_PIN = A5;

  //Matrix input and output
  const int C0_PIN = A2;
  const int C1_PIN = D9;
  const int C2_PIN = A6;
  const int C3_PIN = D1;
  const int OUT_PIN = D11;

  //Audio analogue out
  const int OUTL_PIN = A4;
  const int OUTR_PIN = A3;

  //Joystick analogue in
  const int JOYY_PIN = A0;
  const int JOYX_PIN = A1;

  //Output multiplexer bits
  const int DEN_BIT = 3;
  const int DRST_BIT = 4;
  const int HKOW_BIT = 5;
  const int HKOE_BIT = 6;

  volatile uint8_t keyArray[7];
  volatile uint8_t key_pressed_step_size = 12;
  const int32_t stepSizes [] = {51076056, 54113197, 57330935, 60740010, 64351798, 68178356, 72232452, 76527617, 81078186, 85899345, 91007186, 96418755};
  const char* notes[13] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", " "};
  volatile int32_t currentStepSize;

//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
      digitalWrite(REN_PIN,LOW);
      digitalWrite(RA0_PIN, bitIdx & 0x01);
      digitalWrite(RA1_PIN, bitIdx & 0x02);
      digitalWrite(RA2_PIN, bitIdx & 0x04);
      digitalWrite(OUT_PIN,value);
      digitalWrite(REN_PIN,HIGH);
      delayMicroseconds(2);
      digitalWrite(REN_PIN,LOW);
}

void sampleISR(){
  static uint32_t phaseAcc = 0; 
  phaseAcc += currentStepSize;
  uint32_t Vout = phaseAcc >> 24; 
  analogWrite(OUTR_PIN, Vout + 128);
}

uint8_t readCols(){


  uint8_t x = digitalRead(C0_PIN);
  uint8_t C1 = digitalRead(C1_PIN);
  uint8_t C2 = digitalRead(C2_PIN);
  uint8_t C3 = digitalRead(C3_PIN);

  x = (C1 << 1) | x;
  x = (C2 << 2) | x;
  x = (C3 << 3) | x;

  return x;
}

void setRow(uint8_t rowIdx){

  digitalWrite(REN_PIN, LOW);

  if ((rowIdx) == 2) {
    digitalWrite(RA2_PIN, LOW);
    digitalWrite(RA1_PIN, HIGH);
    digitalWrite(RA0_PIN, LOW);
  }
  if ((rowIdx) == 1) {
    digitalWrite(RA2_PIN, LOW);
    digitalWrite(RA1_PIN, LOW);
    digitalWrite(RA0_PIN, HIGH);
  }
  if ((rowIdx) == 0) {
    digitalWrite(RA2_PIN, LOW);
    digitalWrite(RA1_PIN, LOW);
    digitalWrite(RA0_PIN, LOW);
  }

  digitalWrite(REN_PIN, HIGH);

}


void scanKeysTask(void * pvParameters) {

  const TickType_t xFrequency = 50/portTICK_PERIOD_MS; 
  TickType_t xLastWakeTime = xTaskGetTickCount(); 

  while(1){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    uint8_t key_Pressed = 0;

    for(uint8_t i = 0; i < 3; i++){
      setRow(i);
      delayMicroseconds(3);
      keyArray[i] = readCols();
    }

    switch(keyArray[0]){
      case 0xE:
        key_pressed_step_size= 0;
        key_Pressed = 1;
        break;
      case 0xD:
        key_pressed_step_size= 1;
        key_Pressed = 1;
        break;
      case 0xB:
        key_pressed_step_size= 2;
        key_Pressed = 1;
        break;     
      case 0x7:
        key_pressed_step_size = 3;
        key_Pressed = 1;
        break;
      default:
        break;    
    }

    switch(keyArray[1]){
      case 0xE:
        key_pressed_step_size = 4;
        key_Pressed = 1;
        break;
      case 0xD:
        key_pressed_step_size = 5;
        key_Pressed = 1;
        break;
      case 0xB:
        key_pressed_step_size = 6;
        key_Pressed = 1;
        break;     
      case 0x7:
        key_pressed_step_size = 7;
        key_Pressed = 1;
        break;
      default:
        break;    
    }

    switch(keyArray[2]){
      case 0xE:
        key_pressed_step_size = 8;
        key_Pressed = 1;
        break;
      case 0xD:
        key_pressed_step_size = 9;
        key_Pressed = 1;
        break;
      case 0xB:
        key_pressed_step_size = 10;
        key_Pressed = 1;
        break;     
      case 0x7:
        key_pressed_step_size = 11;
        key_Pressed = 1;
        break;
      default:
        break;    
    }
    
    if(key_Pressed == 0){
      key_pressed_step_size = 12;
      __atomic_store_n(&currentStepSize, 0, __ATOMIC_RELAXED);
    }
    else{
      __atomic_store_n(&currentStepSize, stepSizes[key_pressed_step_size], __ATOMIC_RELAXED);
    }
  }
}

void displayUpdateTask(void * pvParameters){

  const TickType_t xFrequency = 100/portTICK_PERIOD_MS; 
  TickType_t xLastWakeTime = xTaskGetTickCount(); 

  while(1){
    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font u8g2_font_ncenB08_tr
    u8g2.drawStr(2,10,"hello world");  // write something to the internal memory
    u8g2.setCursor(2,20);
    u8g2.print(keyArray[0],HEX);
    u8g2.print(keyArray[1],HEX);
    u8g2.print(keyArray[2],HEX);
    u8g2.setCursor(2,30);
    u8g2.print(notes[key_pressed_step_size]);
    u8g2.sendBuffer();
  }
}

void setup() {
  // put your setup code here, to run once:

  //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  TIM_TypeDef *Instance = TIM1; 
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);

  sampleTimer->setOverflow(22000, HERTZ_FORMAT); 
  sampleTimer->attachInterrupt(sampleISR); 
  sampleTimer->resume();

  TaskHandle_t scanKeysHandle = NULL; 
  TaskHandle_t displayUpdateHandle = NULL; 
  xTaskCreate( 
    scanKeysTask,  /* Function that implements the task */ 
    "scanKeys",   /* Text name for the task */ 
    64,        /* Stack size in words, not bytes */ 
    NULL,    /* Parameter passed into the task */ 
    1,   /* Task priority */ 
    &scanKeysHandle );  /* Pointer to store the task handle */
  
  xTaskCreate( 
    displayUpdateTask,  /* Function that implements the task */ 
    "displayKeys",   /* Text name for the task */ 
    256,        /* Stack size in words, not bytes */ 
    NULL,    /* Parameter passed into the task */ 
    1,   /* Task priority */ 
    &displayUpdateHandle );  /* Pointer to store the task handle */


  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  vTaskStartScheduler();
}






void loop() {
  // put your main code here, to run repeatedly:

    
  
}


