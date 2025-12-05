/*
  PROGRAMA 2: RESTAURAR HUELLAS (ESP32)
  Lee el archivo "huellas_backup.h" y carga los datos al sensor.
*/

#include <Adafruit_Fingerprint.h>
#include "huellas_backup.h" // <--- ACA SE IMPORTA EL ARCHIVO DE BACKUP

#define RX_PIN 26
#define TX_PIN 27

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);

  Serial.println("\n\n--- INICIANDO RESTAURACION ---");
  
  if (finger.verifyPassword()) {
    Serial.println("Sensor detectado.");
  } else {
    Serial.println("Sensor no encontrado.");
    while (1);
  }

  Serial.print("Se encontraron "); Serial.print(cantidadBackups); Serial.println(" huellas en el archivo de backup.");

  // Recorremos la lista del archivo y subimos una por una
  for (int i = 0; i < cantidadBackups; i++) {
    uint16_t id = listaHuellas[i].id;
    Serial.print("Restaurando ID #"); Serial.print(id); Serial.print("... ");
    
    writeFingerprintToSensor(id, listaHuellas[i].data);
  }
  
  Serial.println("\n--- PROCESO TERMINADO ---");
}

void loop() {}

// Función que toma los datos del array y los envía al sensor
void writeFingerprintToSensor(uint16_t id, const uint8_t* temp) {
  // 1. Comando DownChar (0x09) para bajar al Buffer 1
  uint8_t packet[] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x09, 0x01, 0x00, 0x0F };
  mySerial.write(packet, sizeof(packet));

  if (readConfirmation() != 0x00) {
    Serial.println("Error de comunicacion (DownChar).");
    return;
  }

  // 2. Enviar los 512 bytes en paquetes de 128
  for (int idx = 0; idx < 512; idx += 128) {
     uint8_t packetType = (idx + 128 >= 512) ? 0x08 : 0x02;
     sendDataPacket(packetType, temp + idx, 128);
  }

  // 3. Guardar el Buffer 1 en Flash (StoreModel)
  uint8_t p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("OK.");
  } else {
    Serial.print("Error guardando: 0x"); Serial.println(p, HEX);
  }
}

void sendDataPacket(uint8_t type, const uint8_t* data, uint16_t len) {
  uint16_t sum = type + len + 2;
  mySerial.write((uint8_t)0xEF); mySerial.write((uint8_t)0x01);
  mySerial.write((uint8_t)0xFF); mySerial.write((uint8_t)0xFF); 
  mySerial.write((uint8_t)0xFF); mySerial.write((uint8_t)0xFF);
  mySerial.write(type);
  mySerial.write((uint8_t)((len + 2) >> 8));
  mySerial.write((uint8_t)((len + 2) & 0xFF));
  for (int i = 0; i < len; i++) {
    mySerial.write(data[i]);
    sum += data[i];
  }
  mySerial.write((uint8_t)(sum >> 8));
  mySerial.write((uint8_t)(sum & 0xFF));
}

uint8_t readConfirmation() {
  uint8_t reply[12];
  int idx = 0;
  uint32_t timer = millis();
  while (millis() - timer < 1000 && idx < 12) {
    if (mySerial.available()) reply[idx++] = mySerial.read();
  }
  if (idx >= 10) return reply[9];
  return 0xFF;
}