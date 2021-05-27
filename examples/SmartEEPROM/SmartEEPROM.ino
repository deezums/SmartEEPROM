#include <SmartEEPROM.h>
SmartEEPROM EEPROM;

#define EEPROM_EMULATION_SIZE 4096
// Valid sizes are 0, 512, 1024, 2048, 4096, 8192, 16384, 32768, and 65536

void setup() {

Serial.begin(115200);

// Start EEPROM emulation, sets fuses if necessary, will reset CPU after changing fuses
uint8_t test = EEPROM.init();	
if (test != 1){
  Serial.println("EEPROM Locked ERROR");
} else {
  Serial.println("EEPROM Config OK!");
}

Serial.println("First read test");
Serial.println("EEPROM Data");
for (int i = 0; i < 255; i++){
  Serial.print(i);Serial.print(" = ");
  Serial.println(EEPROM.read(i));
}
Serial.println("First Read done");
Serial.println("Write Test");
for (int i = 0; i < 255; i++){
  EEPROM.update(i, 50);
}
Serial.println("Write test done");
Serial.println("Second read test");
Serial.println("EEPROM Data");
for (int i = 0; i < 255; i++){
  Serial.print(i);Serial.print(" = ");
  Serial.println(EEPROM.read(i));
}
Serial.println("Second read test done");
}

void loop() {
}
