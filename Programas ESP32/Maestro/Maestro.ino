#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h> 

// --- DECLARACIONES ADELANTADAS ---
void handleRoot();
void handleStatus();
void handleSetMode();
void handleDelete();
void handleConfigIP();
void handleBackup();
void handleRestore();
void handleDownload();
void handleFileUpload(); 
void rutinaFichar();
void rutinaRegistrar();
int getFirstFreeID();
void enviarFichajeExitoso(int id);
void enviarFichajeFallido();

// Funciones auxiliares sensor
void writeFingerprintToSensor(uint16_t id, uint8_t* temp);
void sendDataPacket(uint8_t type, const uint8_t* data, uint16_t len);
uint8_t readConfirmation();

// ==========================================
// 🔒 SEGURIDAD
// ==========================================
const char* login_user = "admin";
const char* login_pass = "preguntaleaedu"; 

// --- CONFIGURACIÓN ---
#define RX_PIN 26
#define TX_PIN 27
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// --- CONTRASEÑA WIFI ---
const char* ssid = "Red-INTI";       
const char* password = "reDES1957"; // 

WebServer server(80);
Preferences preferences; 
String ip_servidor = "192.168.0.198"; 

enum SystemMode { MODE_IDLE, MODE_FICHAR, MODE_ENROLL, MODE_DELETE };
SystemMode currentMode = MODE_FICHAR;

int enrollStep = 0;
int enrollID = 0;
unsigned long lastActionTime = 0;
String globalMessage = "Sistema Listo. Modo FICHAR activo.";
File uploadFile; 

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin(true)){
    Serial.println("Error SPIFFS");
    globalMessage = "Error memoria interna.";
  }

  preferences.begin("config", false); 
  ip_servidor = preferences.getString("serverIP", "192.168.0.198"); 

  mySerial.setRxBufferSize(1024);
  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);
  if (finger.verifyPassword()) {
    Serial.println("Sensor OK.");
    finger.getParameters();
    Serial.print("Capacidad Sensor: "); Serial.println(finger.capacity);
  } else {
    globalMessage = "⚠️ ERROR: Sensor desconectado.";
  }

  // Conexión WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  unsigned long inicioIntentos = millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (millis() - inicioIntentos > 10000) {
        Serial.println("\nReintentando conexión...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        inicioIntentos = millis();
      }
  }

  Serial.println("\nIP Asignada: " + WiFi.localIP().toString());
  // RUTAS WEB
  server.on("/", handleRoot);              
  server.on("/status", handleStatus);      
  server.on("/setMode", handleSetMode);    
  server.on("/delete", handleDelete);
  server.on("/configIP", handleConfigIP);
  server.on("/backup", handleBackup);
  server.on("/restore", handleRestore);
  server.on("/download", handleDownload);
  
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Carga Completada");
  }, handleFileUpload);
  
  server.begin();
}

// --- LOOP ---
void loop() {
  server.handleClient(); 
  switch (currentMode) {
    case MODE_FICHAR: rutinaFichar(); break;
    case MODE_ENROLL: rutinaRegistrar(); break;
    default: break;
  }
}

// --- LÓGICA FICHAR ---
void rutinaFichar() {
  if (millis() - lastActionTime < 1500) return; 
  lastActionTime = millis();

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return; 

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    int id = finger.fingerID;
    String msg = "✅ Fichada ID #" + String(id);
    Serial.println(msg);
    globalMessage = msg;
    enviarFichajeExitoso(id);
    delay(2000); 
    globalMessage = "Esperando huella...";
  } else if (p == FINGERPRINT_NOTFOUND) {
    globalMessage = "⛔ Huella NO reconocida.";
    enviarFichajeFallido();
    //delay(2000);
    globalMessage = "Esperando huella...";
  }
}

void enviarFichajeExitoso(int id) {
  if(WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = "http://" + ip_servidor + ":5000/fichar";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<100> doc;
  doc["id_huella"] = id;
  String json;
  serializeJson(doc, json);
  http.POST(json);
  http.end();
}

void enviarFichajeFallido() {
  if(WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = "http://" + ip_servidor + ":5000/fichar_error";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.POST("{}");
  http.end();
}

// --- LÓGICA REGISTRO ---
void rutinaRegistrar() {
  uint8_t p;
  switch(enrollStep) {
    case 0: 
      enrollID = getFirstFreeID();
      if (enrollID == -1) {
        globalMessage = "⚠️ Memoria llena.";
        currentMode = MODE_IDLE;
      } else {
        globalMessage = "☝ Pon dedo para ID #" + String(enrollID);
        enrollStep = 1;
      }
      break;
    case 1: 
      p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        if (finger.image2Tz(1) == FINGERPRINT_OK) {
           globalMessage = "👌 Quita el dedo.";
           enrollStep = 2;
           lastActionTime = millis(); 
        }
      }
      break;
    case 2: 
      if (millis() - lastActionTime > 3000) { 
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) {
           globalMessage = "☝ Pon el MISMO dedo.";
           enrollStep = 3;
        }
      }
      break;
    case 3: 
      p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        if (finger.image2Tz(2) == FINGERPRINT_OK) {
           enrollStep = 4;
        }
      }
      break;
    case 4: 
      p = finger.createModel();
      if (p == FINGERPRINT_OK) {
         p = finger.storeModel(enrollID);
         if (p == FINGERPRINT_OK) {
           globalMessage = "✨ ÉXITO: ID #" + String(enrollID);
         } else {
           globalMessage = "❌ Error Flash.";
         }
      } else {
         globalMessage = "❌ No coinciden.";
      }
      delay(3000);
      currentMode = MODE_FICHAR; 
      enrollStep = 0;
      break;
  }
}

int getFirstFreeID() {
  int cap = (finger.capacity > 0) ? finger.capacity : 256;
  for (int i = 1; i <= cap; i++) {
    if (finger.loadModel(i) != FINGERPRINT_OK) return i;
  }
  return -1;
}

// ==========================================================
// 🛠️ BACKUP / RESTORE / UPLOAD HANDLERS
// ==========================================================

void handleBackup() {
  if (!server.authenticate(login_user, login_pass)) return server.requestAuthentication();
  
  // Limpiamos backup anterior
  if (SPIFFS.exists("/backup.dat")) SPIFFS.remove("/backup.dat");
  
  globalMessage = "⏳ GENERANDO BACKUP... ESPERE...";
  File f = SPIFFS.open("/backup.dat", FILE_WRITE);
  int count = 0;
  
  // Timeout generoso para asegurar recepción
  mySerial.setTimeout(2000);

  // Ajusta al número máximo de huellas detectado
  int cap = (finger.capacity > 0) ? finger.capacity : 256;
  for (int id = 1; id <= cap; id++) {
    
    // Limpieza de buffer
    while(mySerial.available()) mySerial.read();
    
    // Cargar modelo del Flash del sensor a su Buffer 1
    if (finger.loadModel(id) == FINGERPRINT_OK) {
       
       // Pedir al sensor que suba el modelo (Upload)
       if (finger.getModel() == FINGERPRINT_OK) {
          
          // EL SENSOR ENVÍA: 4 paquetes de 139 bytes (128 datos + 11 overhead)
          // Total esperado: 556 bytes (No 534)
          uint8_t bytesReceived[556]; 
          memset(bytesReceived, 0xff, 556);

          size_t leidos = mySerial.readBytes(bytesReceived, 556);
          
          // Verificamos si llegaron los 4 paquetes completos
          if (leidos == 556) { 
             uint8_t fingerTemplate[512]; 
             
             // --- AQUÍ ESTABA EL ERROR ANTERIOR ---
             // Debemos extraer 128 bytes de cada uno de los 4 paquetes
             // Saltando los 9 bytes de encabezado de cada paquete
             
             // Paquete 1
             memcpy(fingerTemplate,       bytesReceived + 9,             128);
             // Paquete 2 (Inicio en 139)
             memcpy(fingerTemplate + 128, bytesReceived + 139 + 9,       128);
             // Paquete 3 (Inicio en 278)
             memcpy(fingerTemplate + 256, bytesReceived + 139 * 2 + 9,   128);
             // Paquete 4 (Inicio en 417)
             memcpy(fingerTemplate + 384, bytesReceived + 139 * 3 + 9,   128);
   
             // Guardamos en archivo: ID + 512 bytes limpios
             uint8_t id_byte = (uint8_t)id;
             f.write(&id_byte, 1);
             f.write(fingerTemplate, 512);
             count++;
          } else {
             Serial.printf("Error ID %d: Tamaño incorrecto (%d bytes)\n", id, leidos);
          }
       }
    }
    delay(50); 
    yield();
  }
  
  f.close();
  globalMessage = "✅ Backup OK: " + String(count) + " huellas.";
  server.send(200, "text/plain", "OK"); 
}  

void handleRestore() {
  if (!server.authenticate(login_user, login_pass)) return server.requestAuthentication();
  
  if (!SPIFFS.exists("/backup.dat")) {
    server.send(404, "text/plain", "No File");
    return;
  }

  globalMessage = "⏳ RESTAURANDO... ESPERE...";
  File f = SPIFFS.open("/backup.dat", FILE_READ);
  int count = 0;
  
  while (f.available() >= 513) { 
     uint8_t id;
     f.read(&id, 1);
     uint8_t temp[512];
     f.read(temp, 512);
     writeFingerprintToSensor(id, temp);
     count++;
     delay(100); // Pausa necesaria para que el sensor grabe en su Flash interna
  }
  f.close();
  globalMessage = "✅ Restaurado: " + String(count) + " huellas.";
  server.send(200, "text/plain", "OK"); 
}

void handleDownload() {
  if (!server.authenticate(login_user, login_pass)) return server.requestAuthentication();
  if (SPIFFS.exists("/backup.dat")) {
    File f = SPIFFS.open("/backup.dat", "r");
    server.streamFile(f, "application/octet-stream");
    f.close();
  } else {
    server.send(404, "text/plain", "No hay backup.");
  }
}

void handleFileUpload() {
  if (!server.authenticate(login_user, login_pass)) return; 

  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/backup.dat"; 
    uploadFile = SPIFFS.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      globalMessage = "✅ Archivo subido correctamente.";
    }
  }
}

// Funciones de bajo nivel Sensor
void writeFingerprintToSensor(uint16_t id, uint8_t* temp) {
  uint8_t packet[] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x09, 0x01, 0x00, 0x0F };
  mySerial.write(packet, sizeof(packet));
  delay(20); 
  if (readConfirmation() != 0x00) return;
  for (int idx = 0; idx < 512; idx += 128) {
     uint8_t packetType = (idx + 128 >= 512) ? 0x08 : 0x02;
     sendDataPacket(packetType, temp + idx, 128);
  }
  finger.storeModel(id);
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

// ==========================================================
// INTERFAZ WEB SPA (AJAX)
// ==========================================================

void handleRoot() {
  if (!server.authenticate(login_user, login_pass)) return server.requestAuthentication();

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Biometría ESP32</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', sans-serif; background-color: #f0f2f5; padding: 20px; display: flex; justify-content: center; }";
  html += ".container { background: white; width: 100%; max-width: 400px; padding: 25px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); text-align: center; }";
  html += ".status-box { background: #e8f5e9; color: #2e7d32; padding: 15px; border-radius: 10px; margin-bottom: 25px; font-weight: bold; }";
  html += "button { width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; margin-bottom: 10px; transition:0.2s; }";
  html += "button:active { transform: scale(0.98); }";
  
  html += ".btn-fichar { background: #2196F3; color: white; }";
  html += ".btn-enroll { background: #4CAF50; color: white; }";
  html += ".btn-delete { background: #FF5252; color: white; width: 30% !important; margin-left:5px; }";
  html += ".btn-backup { background: #673AB7; color: white; }";
  html += ".btn-upload { background: #009688; color: white; }";
  html += ".btn-download { background: #607D8B; color: white; font-size: 14px; padding: 10px; }";
  html += ".btn-danger { background: #d32f2f; color: white; margin-top: 30px; border: 2px solid #b71c1c; }"; // Botón de peligro
  
  html += ".input-group { display: flex; justify-content: space-between; align-items: center; margin: 15px 0; background: #f9f9f9; padding: 10px; border-radius: 8px; }";
  html += "input[type='number'], input[type='text'] { padding: 10px; border: 1px solid #ddd; width: 60%; outline: none; }";
  html += ".label { font-size: 0.9em; color: #666; margin-bottom: 5px; display: block; text-align: left; }";
  html += "</style>";

  html += "<script>";
  html += "setInterval(function(){fetch('/status').then(r=>r.text()).then(t=>{document.getElementById('msg').innerText=t;});}, 1000);";
  
  html += "function callMode(m) { fetch('/setMode?m=' + m); }";
  
  html += "function borrarUno() {";
  html += "  var id = document.getElementById('id_input').value;";
  html += "  if(id && confirm('¿Borrar ID ' + id + '?')) fetch('/delete?id=' + id);";
  html += "}";

  html += "function crearBackup() {";
  html += "  if(!confirm('¿Crear Backup? El sensor se pausará unos segundos.')) return;";
  html += "  document.getElementById('msg').innerText = '⏳ GENERANDO BACKUP...';";
  html += "  fetch('/backup').then(r => { alert('¡Backup Creado!'); });";
  html += "}";

  html += "function restaurarBackup() {";
  html += "  if(!confirm('¿Sobrescribir sensor con el Backup?')) return;";
  html += "  document.getElementById('msg').innerText = '⏳ RESTAURANDO...';";
  html += "  fetch('/restore').then(r => { alert('¡Restauración Completada!'); });";
  html += "}";

  html += "function subirArchivo() {";
  html += "  var input = document.getElementById('fileUpload');";
  html += "  if(input.files.length === 0) return;";
  html += "  var data = new FormData();";
  html += "  data.append('file', input.files[0]);";
  html += "  document.getElementById('msg').innerText = '⬆️ SUBIENDO ARCHIVO...';";
  html += "  fetch('/upload', { method: 'POST', body: data }).then(r => {";
  html += "     alert('Archivo subido con éxito'); input.value = '';";
  html += "  });";
  html += "}";
  
  html += "function guardarIP() {";
  html += "  var ip = document.getElementById('ip_input').value;";
  html += "  fetch('/configIP?ip=' + ip).then(r => alert('IP Guardada'));";
  html += "}";

  html += "</script>";

  html += "</head><body><div class='container'>";
  html += "<h2>🔐 Control Lector de Huellas</h2>";
  html += "<div class='status-box'><span id='msg'>" + globalMessage + "</span></div>";
  
  html += "<button class='btn-fichar' onclick=\"callMode('fichar')\">📋 MODO FICHAR</button>";
  html += "<button class='btn-enroll' onclick=\"callMode('registrar')\">➕ REGISTRAR</button>";
  
  html += "<hr><span class='label'>Backup & Restauración</span>";
  
  html += "<button class='btn-backup' onclick='crearBackup()'>💾 CREAR BACKUP (En ESP32)</button>";
  html += "<button class='btn-backup' style='background:#FF9800' onclick='restaurarBackup()'>♻️ RESTAURAR (Al Sensor)</button>";
  
  html += "<div style='margin-top:10px; border:1px dashed #aaa; padding:10px; border-radius:8px;'>";
  html += "<input type='file' id='fileUpload' style='display:none' onchange='subirArchivo()'>";
  html += "<button class='btn-upload' onclick=\"document.getElementById('fileUpload').click()\">⬆️ CARGAR RESPALDO (Desde PC)</button>";
  html += "<button class='btn-download' onclick=\"location.href='/download'\">⬇️ BAJAR RESPALDO (A PC)</button>";
  html += "</div>";

  html += "<hr><span class='label'>Gestión ID</span>";
  html += "<div class='input-group'><input type='number' id='id_input' placeholder='ID'><button class='btn-delete' onclick='borrarUno()'>🗑</button></div>";

  html += "<span class='label'>IP Servidor</span>";
  html += "<div class='input-group'><input type='text' id='ip_input' value='" + ip_servidor + "'><button style='width:30%;background:#607D8B;color:white' onclick='guardarIP()'>💾</button></div>";
  
  // BOTÓN DE PELIGRO MOVIDO AL FINAL
  html += "<button class='btn-danger' style='width:100% !important' onclick=\"if(confirm('¿Borrar TODO?')) callMode('vaciar')\">💀 BORRAR TODO</button>";

  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  if (!server.authenticate(login_user, login_pass)) return;
  server.send(200, "text/plain", globalMessage);
}

void handleSetMode() {
  if (!server.authenticate(login_user, login_pass)) return;
  if (!server.hasArg("m")) return;
  String m = server.arg("m");
  
  if (m == "fichar") { currentMode = MODE_FICHAR; globalMessage = "Modo FICHAR."; }
  else if (m == "registrar") { currentMode = MODE_ENROLL; enrollStep = 0; globalMessage = "Iniciando registro..."; }
  else if (m == "vaciar") { finger.emptyDatabase(); globalMessage = "⚠️ BD VACIADA."; currentMode = MODE_IDLE; }
  
  server.send(200, "text/plain", "OK");
}

void handleDelete() {
  if (!server.authenticate(login_user, login_pass)) return;
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id > 0 && finger.deleteModel(id) == FINGERPRINT_OK) globalMessage = "✅ ID #" + String(id) + " eliminado.";
    else globalMessage = "❌ Error borrando ID #" + String(id);
  }
  server.send(200, "text/plain", "OK");
}

void handleConfigIP() {
  if (!server.authenticate(login_user, login_pass)) return;
  if (server.hasArg("ip")) {
    ip_servidor = server.arg("ip");
    preferences.putString("serverIP", ip_servidor); 
    globalMessage = "IP guardada.";
  }
  server.send(200, "text/plain", "OK");
}