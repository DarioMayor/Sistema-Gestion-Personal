#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// --- DECLARACIONES ADELANTADAS ---
void handleRoot();
void handleStatus();
void handleSetMode();
void handleDelete();
void handleConfigIP();
void rutinaFichar();
void rutinaRegistrar();
int getFirstFreeID();
void enviarFichajeExitoso(int id);
void enviarFichajeFallido();

// --- CONFIGURACIÓN ---
#define RX_PIN 26
#define TX_PIN 27
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const char* ssid = "TP-Link_61F8";       
const char* password = "81623655";

WebServer server(80);
Preferences preferences; 
String ip_servidor = "192.168.0.198"; 

enum SystemMode { MODE_IDLE, MODE_FICHAR, MODE_ENROLL, MODE_DELETE };
SystemMode currentMode = MODE_FICHAR;

int enrollStep = 0;
int enrollID = 0;
unsigned long lastActionTime = 0;
String globalMessage = "Sistema Listo. Modo FICHAR activo.";

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  preferences.begin("config", false); 
  ip_servidor = preferences.getString("serverIP", "192.168.0.198"); 

  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);
  if (finger.verifyPassword()) {
    Serial.println("Sensor encontrado.");
  } else {
    Serial.println("Error: Sensor no encontrado.");
    globalMessage = "⚠️ ERROR: Sensor desconectado.";
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi OK: " + WiFi.localIP().toString());

  server.on("/", handleRoot);              
  server.on("/status", handleStatus);      
  server.on("/setMode", handleSetMode);    
  server.on("/delete", handleDelete);
  server.on("/configIP", handleConfigIP); 
  
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

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    int id = finger.fingerID;
    String msg = "✅ Fichada Exitosa ID #" + String(id);
    Serial.println(msg);
    globalMessage = msg;
    enviarFichajeExitoso(id);
    delay(2000); 
    globalMessage = "Esperando huella...";
  } else if (p == FINGERPRINT_NOTFOUND) {
    globalMessage = "⛔ Huella NO reconocida.";
    enviarFichajeFallido();
    delay(2000);
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
        globalMessage = "⚠️ Error: Memoria llena.";
        currentMode = MODE_IDLE;
      } else {
        globalMessage = "☝ Pon el dedo para ID #" + String(enrollID);
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
      if (millis() - lastActionTime > 2000) { 
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) {
           globalMessage = "☝ Vuelve a poner el MISMO dedo.";
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
           globalMessage = "✨ ÉXITO: Huella guardada ID #" + String(enrollID);
         } else {
           globalMessage = "❌ Error al guardar.";
         }
      } else {
         globalMessage = "❌ Error: No coinciden.";
      }
      delay(2000);
      currentMode = MODE_FICHAR; 
      enrollStep = 0;
      break;
  }
}

int getFirstFreeID() {
  for (int i = 1; i <= 127; i++) {
    if (finger.loadModel(i) != FINGERPRINT_OK) return i;
  }
  return -1;
}

// ==========================================================
// INTERFAZ WEB MEJORADA
// ==========================================================

void handleRoot() {
  // Construimos el HTML. Usamos String para poder insertar las variables fácilmente.
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Biometría ESP32</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; display: flex; justify-content: center; }";
  html += ".container { background: white; width: 100%; max-width: 400px; padding: 25px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); text-align: center; }";
  html += "h2 { color: #333; margin-bottom: 20px; }";
  
  // Estilo Status
  html += ".status-box { background: #e8f5e9; color: #2e7d32; padding: 15px; border-radius: 10px; border: 1px solid #c8e6c9; margin-bottom: 25px; font-weight: bold; font-size: 1.1em; min-height: 50px; display: flex; align-items: center; justify-content: center; }";
  
  // Estilos Botones
  html += "button { width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; transition: 0.2s; margin-bottom: 12px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += ".btn-fichar { background: #2196F3; color: white; } .btn-fichar:hover { background: #1976D2; }";
  html += ".btn-enroll { background: #4CAF50; color: white; } .btn-enroll:hover { background: #388E3C; }";
  html += ".btn-delete { background: #FF5252; color: white; width: 30% !important; margin-left: 5px; }";
  html += ".btn-danger { background: #d32f2f; color: white; margin-top: 20px; }";
  
  // Estilos Inputs y Formularios
  html += ".input-group { display: flex; justify-content: space-between; align-items: center; margin: 15px 0; background: #f9f9f9; padding: 10px; border-radius: 8px; }";
  html += "input[type='number'], input[type='text'] { padding: 10px; border: 1px solid #ddd; border-radius: 6px; width: 60%; font-size: 16px; outline: none; }";
  html += "hr { border: 0; border-top: 1px solid #eee; margin: 20px 0; }";
  html += ".label { font-size: 0.9em; color: #666; margin-bottom: 5px; display: block; text-align: left; }";
  html += "</style>";

  // JavaScript para los GLOBOS DE DIALOGO (Alerts/Confirms)
  html += "<script>";
  html += "setInterval(function(){fetch('/status').then(r=>r.text()).then(t=>{document.getElementById('msg').innerText=t;});}, 1000);";
  
  html += "function confirmarBorrarUno() {";
  html += "  var id = document.getElementById('id_input').value;";
  html += "  if(!id) { alert('⚠️ Por favor ingresa un número de ID.'); return; }";
  html += "  if(confirm('⚠️ ADVERTENCIA \\n\\n¿Estás seguro que deseas ELIMINAR la huella ID #' + id + '?\\n\\nEsta acción no se puede deshacer.')) {";
  html += "    location.href = '/delete?id=' + id;";
  html += "  }";
  html += "}";

  html += "function confirmarVaciar() {";
  html += "  if(confirm('⛔ PELIGRO CRÍTICO ⛔\\n\\n¿Estás realmente seguro de BORRAR TODAS las huellas?\\n\\n¡Se eliminará toda la base de datos del sensor!')) {";
  html += "    location.href = '/setMode?m=vaciar';";
  html += "  }";
  html += "}";
  html += "</script>";

  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h2>🔐 Control de Acceso</h2>";
  
  // Caja de Estado
  html += "<div class='status-box'><span id='msg'>" + globalMessage + "</span></div>";
  
  // Botones Principales
  html += "<button class='btn-fichar' onclick=\"location.href='/setMode?m=fichar'\">📋 MODO FICHAR (Normal)</button>";
  html += "<button class='btn-enroll' onclick=\"location.href='/setMode?m=registrar'\">➕ REGISTRAR HUELLA</button>";
  
  html += "<hr>";
  
  // Sección Borrar Uno
  html += "<span class='label'>Gestión de Usuarios</span>";
  html += "<div class='input-group'>";
  html += "<input type='number' id='id_input' placeholder='ID a borrar (ej: 5)'>";
  html += "<button class='btn-delete' onclick='confirmarBorrarUno()'>🗑</button>";
  html += "</div>";

  // Sección Config IP
  html += "<span class='label'>Configuración Servidor PC</span>";
  html += "<form action='/configIP' method='get' class='input-group'>";
  html += "<input type='text' name='ip' value='" + ip_servidor + "'>";
  html += "<button type='submit' style='width:30%; background:#607D8B; color:white; margin:0 0 0 5px;'>💾</button>";
  html += "</form>";

  // Zona de Peligro
  html += "<button class='btn-danger' onclick='confirmarVaciar()'>💀 BORRAR TODO</button>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  server.send(200, "text/plain", globalMessage);
}

void handleSetMode() {
  if (!server.hasArg("m")) return;
  String m = server.arg("m");
  
  if (m == "fichar") {
    currentMode = MODE_FICHAR;
    globalMessage = "Modo FICHAR activado.";
  } else if (m == "registrar") {
    currentMode = MODE_ENROLL;
    enrollStep = 0;
    globalMessage = "Iniciando registro...";
  } else if (m == "vaciar") {
    finger.emptyDatabase();
    globalMessage = "⚠️ BASE DE DATOS VACIADA.";
    currentMode = MODE_IDLE;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelete() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id > 0) {
      if (finger.deleteModel(id) == FINGERPRINT_OK) {
        globalMessage = "✅ ID #" + String(id) + " eliminado.";
      } else {
        globalMessage = "❌ Error borrando ID #" + String(id);
      }
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleConfigIP() {
  if (server.hasArg("ip")) {
    String newIP = server.arg("ip");
    if (newIP.length() > 6) { 
      ip_servidor = newIP;
      preferences.putString("serverIP", ip_servidor); 
      globalMessage = "IP guardada: " + ip_servidor;
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}