/*
  PROGRAMA 1: BACKUP DE HUELLAS (ESP32)
  Genera el contenido para un archivo "huellas_backup.h"
*/

#include <Adafruit_Fingerprint.h>

// Configuración para ESP32 con pines personalizados
#define RX_PIN 26 // Conectar al cable Amarillo (TX) del sensor
#define TX_PIN 27 // Conectar al cable Azul (RX) del sensor

HardwareSerial mySerial(2); // Usamos el UART 2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200); // Velocidad consola ESP32
  
  // Inicializamos el Serial del sensor MANUALMENTE con los pines 26 y 27
  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);

  Serial.println("\n\n--- INICIANDO GENERADOR DE BACKUP ---");

  if (finger.verifyPassword()) {
    Serial.println("// Sensor encontrado.");
  } else {
    Serial.println("No se encuentra el sensor. Revisa conexiones.");
    while (1);
  }

  finger.getTemplateCount();
  int totalHuellas = finger.templateCount;

  if (totalHuellas == 0) {
    Serial.println("El sensor esta vacio.");
    return;
  }

  // --- INICIO DE LA GENERACIÓN DEL ARCHIVO ---
  Serial.println("\n// =================================================");
  Serial.println("// COPIA DESDE AQUI HASTA EL FINAL Y GUARDALO COMO: ");
  Serial.println("// huellas_backup.h");
  Serial.println("// =================================================\n");

  Serial.println("#ifndef HUELLAS_BACKUP_H");
  Serial.println("#define HUELLAS_BACKUP_H");
  Serial.println("#include <Arduino.h>");
  Serial.println("");
  Serial.println("// Estructura de datos");
  Serial.println("struct HuellaBackup {");
  Serial.println("  uint16_t id;");
  Serial.println("  const uint8_t data[512];");
  Serial.println("};");
  Serial.println("");
  Serial.println("// Lista de huellas");
  Serial.println("const HuellaBackup listaHuellas[] = {");

  // Recorrer y descargar cada huella
  for (int id = 1; id <= 127; id++) {
    downloadFingerprintAsCode(id);
  }

  Serial.println("};");
  Serial.println("");
  Serial.print("const int cantidadBackups = "); Serial.print(totalHuellas); Serial.println(";");
  Serial.println("");
  Serial.println("#endif");
  Serial.println("\n// ================= FIN DEL ARCHIVO ==================");
}

void loop() {}

// Función auxiliar para imprimir el formato de código
void downloadFingerprintAsCode(uint16_t id) {
  uint8_t p = finger.loadModel(id);
  if (p != FINGERPRINT_OK) return; 

  p = finger.getModel();
  if (p != FINGERPRINT_OK) return;

  uint8_t bytesReceived[534]; 
  memset(bytesReceived, 0xff, 534);
  uint32_t starttime = millis();
  int i = 0;
  while (i < 534 && (millis() - starttime) < 20000) {
    if (mySerial.available()) bytesReceived[i++] = mySerial.read();
  }

  // Limpieza de paquetes
  uint8_t fingerTemplate[512]; 
  int uindx = 9, index = 0;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);
  uindx += 256 + 2 + 9;
  index += 256;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);

  // Imprimir bloque de código
  Serial.print("  { "); Serial.print(id); Serial.println(", {");
  Serial.print("    ");
  for (int j = 0; j < 512; ++j) {
    Serial.print("0x");
    printHex(fingerTemplate[j], 2);
    if (j < 511) Serial.print(", ");
    if ((j + 1) % 16 == 0) Serial.print("\n    ");
  }
  Serial.println("} },");
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[128];
  sprintf(format, "%%.%dX", precision);
  sprintf(tmp, format, num);
  Serial.print(tmp);
}