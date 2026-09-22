# 🌬️ Monitor de Calidad del Aire (CO2, Temp, Humedad) con ESP32 y Telegram

Este proyecto es un monitor inteligente de calidad del aire diseñado para interiores (como un dormitorio). Utiliza un microcontrolador ESP32 junto con un sensor Sensirion SCD4x y una pantalla OLED. Además de mostrar los datos en tiempo real de forma local, cuenta con integración a Telegram para enviar alertas de CO2 alto y permitir consultas remotas de las mediciones.

## ✨ Características Principales

*   **Lectura de Alta Precisión:** Mide niveles de CO2 (ppm), temperatura (°C) y humedad relativa (%) utilizando el sensor fotoacústico Sensirion SCD40/SCD41.
*   **Visualización Local (OLED):** Muestra los datos en una pantalla SSD1306 (128x64) con iconos gráficos que evalúan la calidad de cada métrica (Óptimo 🟢, Regular ⚠️, Malo 🛑).
*   **Bot de Telegram Integrado:**
    *   **Alertas Automáticas:** Envía un mensaje de advertencia si el CO2 supera los límites establecidos (ej. > 1000 ppm) con una frecuencia controlada para no saturar.
    *   **Comandos a Demanda:** Responde al comando `/medidas` enviando un reporte actualizado al instante.
*   **Seguridad:** Filtra los mensajes de Telegram para responder únicamente al usuario autorizado (mediante Chat ID).
*   **Código No Bloqueante:** Gestión eficiente de las lecturas del sensor y el polling de Telegram.

## 🛠️ Requisitos de Hardware

1.  Placa de desarrollo **ESP32** (cualquier variante estándar con pines I2C).
2.  Sensor de CO2 **Sensirion SCD40 o SCD41** (Módulo I2C).
3.  Pantalla **OLED 0.96" SSD1306** (128x64, I2C).
4.  Cables Dupont y Protoboard.

**Conexiones I2C por defecto (ESP32):**
*   **SDA:** Pin 21
*   **SCL:** Pin 22
*(Tanto la pantalla como el sensor van conectados a los mismos pines I2C).*

## 📚 Librerías Necesarias (Dependencias)

Asegúrate de instalar las siguientes librerías desde el Gestor de Librerías de Arduino IDE o PlatformIO:

*   `Sensirion I2C SCD4x` (Para el sensor de CO2)
*   `UniversalTelegramBot` (Para la comunicación con la API de Telegram)
*   `ArduinoJson` (Requerida por el bot de Telegram)
*   `Adafruit GFX Library` (Gráficos base)
*   `Adafruit SSD1306` (Controlador de la pantalla OLED)

## ⚙️ Configuración y Uso

Antes de compilar y subir el código a tu ESP32, debes editar las siguientes variables en el archivo principal:

1. **Credenciales WiFi:**
   ```cpp
   const char* ssid = "TU_RED_WIFI";
   const char* password = "TU_CONTRASENA";
   ```

2. **Datos de Telegram:**
   Necesitarás crear un bot en Telegram usando *BotFather* y obtener tu *Chat ID* personal (puedes usar bots como *IDBot* para averiguar el tuyo).
   ```cpp
   String botToken = "TU_TOKEN_DEL_BOT"; 
   String chatID = "TU_CHAT_ID"; 
   ```

3. **Umbrales (Opcional):**
   Puedes ajustar los límites para considerar qué es una medida óptima, aceptable o mala modificando las constantes `CO2_OPTIMO`, `TEMP_MIN_OPTIMA`, etc., en la sección "UMBRALES" del código.

## 🚀 Cómo funciona

1. Al encenderse, el ESP32 inicializará la pantalla, se conectará al WiFi y configurará el cliente seguro para Telegram.
2. Te enviará un mensaje a Telegram indicando: `✅ Monitor Dormitorio Iniciado`.
3. En la pantalla OLED verás las lecturas actualizándose continuamente junto con un icono de estado (Círculo = Bien, Triángulo = Alerta, Cuadrado = Mal).
4. Abre el chat con tu Bot en Telegram y escribe `/medidas`. El bot te contestará con un reporte instantáneo de CO2, Temperatura y Humedad.
