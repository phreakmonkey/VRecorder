#include <Arduino.h>
#include <EEPROM.h>
#include "LowPower.h"

struct eConfig {
  unsigned int millivolts;
  unsigned int delays;
};

struct vStore {
  char id;
  int voltage1;
  int voltage2;
};

eConfig config = {
  49,
  450
};

unsigned long t = 0;
bool interactive = false;
unsigned int eeAddress = sizeof(eConfig);

void write_config() {
  EEPROM.put(0, config);
}

void get_config() {
  eConfig tempconfig;
  EEPROM.get(0, tempconfig);
  if (tempconfig.millivolts == 0) write_config();
  else {
    config.millivolts = tempconfig.millivolts;
    config.delays = tempconfig.delays;
  }
  Serial.println("\r\nConfiguration:");
  Serial.print("Millivolts per unit: ");
  Serial.println(config.millivolts);
  Serial.print("8s delays: ");
  Serial.println(config.delays);
}

void find_next_eeAddress() {
  vStore evalue;
  for (eeAddress=sizeof(eConfig); eeAddress < EEPROM.length() - sizeof(vStore); eeAddress += sizeof(vStore)) {
    EEPROM.get(eeAddress, evalue);
    if (evalue.id != 'V') break;
  }
}

void take_reading() {
  if (eeAddress > EEPROM.length() - sizeof(vStore)) return;
  vStore reading;
  delay(10);  // Let ADC stabilize after sleep
  reading.id = 'V';
  reading.voltage1 = analogRead(A0);
  reading.voltage2 = analogRead(A1);
  EEPROM.put(eeAddress, reading);
  eeAddress += sizeof(vStore);
  Serial.println(eeAddress);
}

void dump_values() {
  vStore evalue;
  char line[50];
  Serial.println("-----");
  for (unsigned int i=sizeof(eConfig); i < EEPROM.length() - sizeof(vStore); i += sizeof(vStore)) {
    EEPROM.get(i, evalue);
    if (evalue.id == 'V') {
      sprintf(line, "%d,%d", evalue.voltage1 * config.millivolts,
          evalue.voltage2 * config.millivolts);
      Serial.println(line);
    } else break;
  }
  Serial.println("-----");
}

void clear_values() {
  Serial.print("\r\nClearing EEPROM");
  for (unsigned int i = sizeof(eConfig); i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
    Serial.print(".");
  }
  find_next_eeAddress();
  Serial.println("\r\nDone.");
}

void console_prompt() {
  Serial.println("\r\nCommands:");
  Serial.println("d - dump eeprom");
  Serial.println("e - erase eeprom");
  Serial.println("v - show current status");
  Serial.println("c - update configuration");
  Serial.print("> ");
 }

int read_int() {
  char input[20];
  int i = 0;
  while (i < 19) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c > 47 && c < 58) {
        input[i] = c;
        i++;
        Serial.print(c);
      } 
      else if (c == 8 || c == 127) {
        if (i > 0) {
          i--;
          Serial.print("\b \b");
        }
      }
      else if (c == 10 || c == 13) {
        input[i] = 0;
        Serial.println("");
        break;
      }
    }
  }
  return atoi(input);
}

void update_config() {
  get_config();
  Serial.print("new millivolts per unit: ");
  config.millivolts = read_int();
  Serial.print("new 8s delays per reading: ");
  config.delays = read_int();
  write_config();
}

void show_status() {
  char line[80];
  sprintf(line, "EEPROM: %d/%d", eeAddress, EEPROM.length());
  Serial.println(line);
  get_config();
  sprintf(line, "A0: %d\r\nA1: %d", analogRead(A0) * config.millivolts,
          analogRead(A1) * config.millivolts);
  Serial.println(line);
}

void console_loop() {
  console_prompt();
  while (interactive) {
    if (Serial.available() > 0) {
      switch (Serial.read()) {
        case 'd':
          dump_values();
          break;
        case 'e':
          clear_values();
          break;
        case 'v':
          show_status();
          break;
        case 'c':
          update_config();
        default:
          console_prompt();
      }
    }
  }
}

void setup() {
  int spaces = 0;
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  find_next_eeAddress();
  delay(8000);
  Serial.begin(115200);
  Serial.flush();
  Serial.println("Voltage Recorder 0.2");
  get_config();
  Serial.println("Press <SPACE> 3 times in the next 10 seconds for interactive mode.");
  t = millis();
  while (millis() - t < 10000) {
    if (Serial.available() > 0) {
      char b = Serial.read();
      if (b == 32) spaces++;
      else spaces = 0;
      if (spaces == 3) {
        interactive = true;
        Serial.println("Interactive Mode Activated");
        break;
      }
    }
  }
}

void loop() {
  if (interactive) console_loop();
  take_reading();
  for (unsigned int i=0; i < config.delays; i++)
     LowPower.idle(SLEEP_8S, ADC_OFF, TIMER4_OFF, TIMER3_OFF, TIMER1_OFF, 
              		 TIMER0_OFF, SPI_OFF, USART1_OFF, TWI_OFF, USB_OFF); 
     
}