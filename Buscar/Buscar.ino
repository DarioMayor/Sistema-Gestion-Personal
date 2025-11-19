#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Configuración para el ESP32 (UART 2)
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Configuracion de la IP Local de la PC que ejecuta el script de Python
const char* IP_SERVIDOR = "192.168.0.136";

//Configuracion del WiFi
const char* ssid = "TP-Link_61F8";
const char* password = "81623655";


void setupWiFi(){

  delay(10);
  Serial.println();
  Serial.print("Conectando a la red: ");
  //Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("¡Conexión Wi-Fi exitosa!");
  Serial.print("La dirección IP del ESP32 es: ");
  Serial.println(WiFi.localIP());

}

void setup() {
  Serial.begin(115200); // Comunicación con el PC

  setupWiFi();  //Inicia el modulo WiFi y se conecta a la red

  int8_t rxPin = 26;
  int8_t txPin = 27;
  mySerial.begin(57600, SERIAL_8N1, rxPin, txPin);  // Comunicación con el sensor

  //mySerial.begin(57600);  // Comunicación con el sensor

  if (finger.verifyPassword()) {
    Serial.println("¡Sensor de huellas listo para buscar!");
  } else {
    Serial.println("No se encontró el sensor :(");
    while (1) { delay(1); }
  } 
}

// --- Función para enviar Fichaje Exitoso ---
void enviarFichajeExitoso(int fingerID) {
    String url_servidor = String("http://") + IP_SERVIDOR + ":5000/fichar";
    
    StaticJsonDocument<100> jsonDoc;
    jsonDoc["id_huella"] = fingerID;
    char jsonBuffer[100];
    serializeJson(jsonDoc, jsonBuffer);

    HTTPClient http;
    http.begin(url_servidor);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonBuffer);
    
    if (httpResponseCode > 0) {
        Serial.print("Datos enviados! Código de respuesta: ");
        Serial.println(httpResponseCode);
    } else {
        Serial.print("Error al enviar datos: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}

// ---  Para enviar Fichaje Fallido ---
void enviarFichajeFallido() {
    String url_servidor = String("http://") + IP_SERVIDOR + ":5000/fichar_error";
    
    HTTPClient http;
    http.begin(url_servidor);
    http.addHeader("Content-Type", "application/json");

    // Enviamos un POST vacío, solo para activar la ruta
    int httpResponseCode = http.POST("{}"); 
    
    if (httpResponseCode > 0) {
        Serial.println("Notificación de error enviada.");
    } else {
        Serial.print("Error al enviar notificación de error: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}

// --- LOOP PRINCIPAL (MODIFICADO) ---
void loop() {
    // 1. Espera a que un dedo esté presente
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        delay(1000); // Si no hay dedo, espera 50ms y vuelve a empezar
        return;
    }

    // --- Si llegamos aquí, un dedo fue detectado ---
    Serial.println("Huella detectada, procesando...");

    // 2. Convierte la imagen
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        Serial.println("Error al procesar imagen");
        return; // Vuelve a empezar
    }

    // 3. Busca la huella en la memoria del sensor
    p = finger.fingerSearch();

    // 4. Analiza el resultado de la búsqueda
    if (p == FINGERPRINT_OK) {
        // --- ¡ÉXITO! ---
        int fingerID = finger.fingerID;
        Serial.print("¡Huella encontrada! Corresponde al ID #");
        Serial.println(fingerID);
        
        // Llama a la función de envío exitoso
        enviarFichajeExitoso(fingerID);
        
    } else if (p == FINGERPRINT_NOTFOUND) {
        // --- ¡FALLO (NO RECONOCIDA)! ---
        Serial.println("¡Huella no encontrada en la base de datos!");
        
        // Llama a la función de envío fallido
        enviarFichajeFallido();
        
    } else {
        // Otro tipo de error
        Serial.println("Error desconocido en la búsqueda");
    }

    // Ya sea éxito o fallo, espera 2 segundos antes de permitir
    // escanear de nuevo, para evitar fichajes duplicados.
    Serial.println("Esperando 2 segundos...");
    delay(2000);
}


// Función que busca la huella
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1; // No hay dedo

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1; // Error de imagen

  // Busca la huella en la base de datos interna del sensor
  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) return -1; // No la encontró

  // ¡La encontró!
  // Devuelve el ID y el "nivel de confianza" de la coincidencia
  Serial.print("Confianza: ");
  Serial.println(finger.confidence);
  
  return finger.fingerID; // Devuelve el ID numérico (ej: 1, 2, 3...)
}