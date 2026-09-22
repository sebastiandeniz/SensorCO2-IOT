#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACIÓN DE PANTALLA ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- TUS DATOS (EDITAR AQUÍ) ---
const char* ssid = "";
const char* password = "";

// Datos de Telegram
String botToken = ""; 
String chatID = "";           

// --- UMBRALES (Ajustados para Dormitorio) ---
const int CO2_OPTIMO = 800;
const int CO2_REGULAR = 1000; 

const float TEMP_MIN_OPTIMA = 18.0;
const float TEMP_MAX_OPTIMA = 22.5;
const float TEMP_MIN_ACCEPT = 16.0;
const float TEMP_MAX_ACCEPT = 25.0;

const float HUM_MIN_OPTIMA = 40.0;
const float HUM_MAX_OPTIMA = 60.0;
const float HUM_MIN_ACCEPT = 30.0;
const float HUM_MAX_ACCEPT = 70.0;

const unsigned long FRECUENCIA_AVISO = 1800000; // 30 min

// --- OBJETOS Y VARIABLES GLOBALES ---
SensirionI2cScd4x scd4x;
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

unsigned long ultimoAviso = 0; 
unsigned long lastTimeBotRan = 0;
const int botRequestDelay = 10000; // Frecuencia de sondeo a Telegram (1 seg)

// Almacenamiento de última lectura para respuesta asíncrona
uint16_t g_co2 = 0;
float g_temp = 0.0;
float g_hum = 0.0;

// --- FUNCIÓN PARA DIBUJAR ICONOS DE ESTADO ---
void dibujarIcono(int x, int y, int estado) {
  if (estado == 0) {
    display.drawCircle(x + 6, y + 6, 5, SSD1306_WHITE); 
  } else if (estado == 1) {
    display.drawTriangle(x+6, y, x, y+12, x+12, y+12, SSD1306_WHITE);
    display.drawLine(x+6, y+4, x+6, y+9, SSD1306_WHITE);
  } else {
    display.fillRect(x, y, 12, 12, SSD1306_WHITE);
  }
}

// --- EVALUADORES DE CALIDAD ---
int evaluarCO2(uint16_t val) {
  if (val < CO2_OPTIMO) return 0;
  if (val < CO2_REGULAR) return 1;
  return 2;
}

int evaluarTemp(float val) {
  if (val >= TEMP_MIN_OPTIMA && val <= TEMP_MAX_OPTIMA) return 0;
  if (val >= TEMP_MIN_ACCEPT && val <= TEMP_MAX_ACCEPT) return 1;
  return 2; 
}

int evaluarHum(float val) {
  if (val >= HUM_MIN_OPTIMA && val <= HUM_MAX_OPTIMA) return 0;
  if (val >= HUM_MIN_ACCEPT && val <= HUM_MAX_ACCEPT) return 1;
  return 2; 
}

void actualizarPantalla(uint16_t co2, float temp, float hum) {
  display.clearDisplay();

  int yCO2 = 0;
  int yTemp = 22;
  int yHum = 44;

  // FILA 1: CO2
  display.setTextSize(1); display.setCursor(0, yCO2 + 4); display.print("CO2");
  display.setTextSize(2); display.setCursor(30, yCO2);    display.print(co2);
  dibujarIcono(110, yCO2 + 2, evaluarCO2(co2));

  // FILA 2: TEMP
  display.setTextSize(1); display.setCursor(0, yTemp + 4); display.print("TMP");
  display.setTextSize(2); display.setCursor(30, yTemp);    display.print(temp, 1);
  display.drawCircle(display.getCursorX() + 2, yTemp + 2, 2, SSD1306_WHITE); 
  dibujarIcono(110, yTemp + 2, evaluarTemp(temp));

  // FILA 3: HUM
  display.setTextSize(1); display.setCursor(0, yHum + 4); display.print("HUM");
  display.setTextSize(2); display.setCursor(30, yHum);    display.print(hum, 0);
  display.setTextSize(1); display.print("%");
  dibujarIcono(110, yHum + 2, evaluarHum(hum));

  display.drawLine(0, 19, 100, 19, SSD1306_WHITE);
  display.drawLine(0, 41, 100, 41, SSD1306_WHITE);

  display.display();
}

// --- COMUNICACIÓN TELEGRAM ---
void enviarAlerta(String mensaje) {
  if(WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(chatID, mensaje, ""); // La librería maneja el formato de la URL automáticamente
  }
}

void procesarNuevosMensajes(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_req = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    // Filtro de seguridad: ignorar comandos de usuarios no autorizados
    if (chat_id_req != chatID) continue;

    if (text == "/medidas") {
      if (g_co2 == 0) {
        bot.sendMessage(chat_id_req, "⏳ El sensor está iniciando sus lecturas. Espera unos segundos.", "");
        continue;
      }
      
      String respuesta = "📊 *Estado Actual del Dormitorio*\n\n";
      respuesta += "💨 *CO2:* " + String(g_co2) + " ppm\n";
      respuesta += "🌡️ *Temperatura:* " + String(g_temp, 1) + " °C\n";
      respuesta += "💧 *Humedad:* " + String(g_hum, 1) + " %";
      
      bot.sendMessage(chat_id_req, respuesta, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo OLED"));
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10,20);
  display.println(F("Iniciando Sensor..."));
  display.display();

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // Configuración SSL indispensable para la API de Telegram
  client.setInsecure(); 

  // Sensor
  scd4x.begin(Wire, 0x62);
  scd4x.stopPeriodicMeasurement();
  scd4x.startPeriodicMeasurement();

  enviarAlerta("✅ Monitor Dormitorio Iniciado.\nEscribe /medidas para ver el estado actual.");
}

void loop() {
  // 1. Gestión del sensor SCD41 (No bloqueante)
  bool isDataReady = false;
  uint16_t error = scd4x.getDataReadyStatus(isDataReady);
  
  if (!error && isDataReady) {
    error = scd4x.readMeasurement(g_co2, g_temp, g_hum);

    if (!error && g_co2 != 0) {
      Serial.printf("CO2: %d | T: %.1f | H: %.1f\n", g_co2, g_temp, g_hum);
      actualizarPantalla(g_co2, g_temp, g_hum);

      // Evaluación de alerta
      if (g_co2 > CO2_REGULAR) {
        if (millis() - ultimoAviso > FRECUENCIA_AVISO || ultimoAviso == 0) {
           enviarAlerta("⚠️ CO2 ALTO: " + String(g_co2) + " ppm");
           ultimoAviso = millis(); 
        }
      }
    }
  }

  // 2. Gestión de recepción de comandos de Telegram (Polling periódico)
  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      procesarNuevosMensajes(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  
  delay(10); // Estabilización del scheduler del ESP32
}
