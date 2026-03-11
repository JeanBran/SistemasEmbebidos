#include <Arduino.h>
#include <math.h>
#include <DHT.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include "secrets.h"

// ─── Pines ─────────────────────────────
#define DHT_PIN 4
#define BME_SDA 21
#define BME_SCL 22
// ─── Configuración DHT ────────────────
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;


// ─── WiFi / ThingSpeak ────────────────
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;

// ─── Tiempos ──────────────────────────
unsigned long lastDHT = 0;
unsigned long lastThingSpeak = 0;

const unsigned long INTERVALO_DHT = 2000;
const unsigned long INTERVALO_THINGSPEAK = 15000;

// ─── Variables ─────────────────────────
float temperatura = NAN;
float humedad = NAN;
float presion = NAN;
float temp2 = NAN;
float puntoRocio = 0;
float sensacion = 0;


// ─── FUNCIONES ─────────────────────────

float calcularPuntoRocio(float temp, float hum) {
    float a = 17.27;
    float b = 237.7;
    float alpha = ((a * temp) / (b + temp)) + log(hum / 100.0);
    float roc = (b * alpha) / (a - alpha);
    return roc;
}

float calcularSensacion(float temp, float hum) {
    float hi = -8.784695 +
               1.61139411 * temp +
               2.338549 * hum +
               -0.14611605 * temp * hum +
               -0.012308094 * temp * temp +
               -0.016424828 * hum * hum +
               0.002211732 * temp * temp * hum +
               0.00072546 * temp * hum * hum +
               -0.000003582 * temp * temp * hum * hum;
    return hi;
}


// ─── WiFi ─────────────────────────────

void conectarWiFi() {

    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("Conectando a %s\n", ssid);

    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado");
}


// ─── SETUP ────────────────────────────

void setup() {

    Serial.begin(115200);

    dht.begin();

    WiFi.mode(WIFI_STA);

    conectarWiFi();

    ThingSpeak.begin(client);
}


// ─── LOOP ─────────────────────────────

void loop() {

    unsigned long ahora = millis();

    conectarWiFi();

    // ─── Leer DHT ─────────────────

    if (ahora - lastDHT >= INTERVALO_DHT) {

        lastDHT = ahora;

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (isnan(h) || isnan(t)) {

            Serial.println("Error leyendo DHT");

        } else {

            temperatura = t;
            humedad = h;

            puntoRocio = calcularPuntoRocio(temperatura, humedad);
            sensacion = calcularSensacion(temperatura, humedad);

            Serial.printf(
                "Temp: %.1f °C | Hum: %.1f %% | Rocio: %.1f °C | Sens: %.1f °C\n",
                temperatura,
                humedad,
                puntoRocio,
                sensacion
            );
        }
    }


    // ─── Enviar a ThingSpeak ─────────────

    if (ahora - lastThingSpeak >= INTERVALO_THINGSPEAK) {

        lastThingSpeak = ahora;

        if (!isnan(temperatura) && !isnan(humedad)) {

            ThingSpeak.setField(1, temperatura);
            ThingSpeak.setField(2, humedad);
            ThingSpeak.setField(3, temp2);
            ThingSpeak.setField(4, temp2);
            ThingSpeak.setField(5, puntoRocio;)
            ThingSpeak.setField(6, sensacion);

            int httpCode =
                ThingSpeak.writeFields(
                    myChannelNumber,
                    myWriteAPIKey
                );

            if (httpCode == 200) {

                Serial.println("Datos enviados a ThingSpeak");

            } else {

                Serial.printf(
                    "Error HTTP %d\n",
                    httpCode
                );
            }
        }
    }
}