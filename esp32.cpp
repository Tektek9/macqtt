#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FS.h>

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (bukaConf()) {
    Serial.println("File konfigurasi berhasil dibaca");
  } else {
    Serial.println("Gagal membaca file konfigurasi. Mohon pastikan file 32.json ada");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Sedang mengkoneksikan ke wifi");
  }
  Serial.println("Tersambung ke wifi");

  client.setServer(mqtt_server, mqtt_port);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Akang ESP32</title><style>body{font-family:Arial,sans-serif;background-color:#f4f4f4;margin:0;padding:0;display:flex;align-items:center;justify-content:center;height:100vh}form{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1)}label{display:block;margin-bottom:10px;font-weight:bold}button{background-color:#4caf50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer}</style></head><body><form action='/submit' method='post'><label for='mac'>Ganti MAC ESP 8266:</label><button id='mac' name='mac' type='submit' value='submit'>Ganti Mac ESP 8266</button></form></body></html>");
  });

  server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request){
    String newMac = request->arg("mac");
    client.publish(mqtt_topic, newMac.c_str());
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

bool bukaConf() {
  int maxRetry = 3;
  int retryCount = 0;

  while (retryCount < maxRetry) {
    File fileConf = SPIFFS.open("/32.json", "r");
    
    if (!fileConf) {
      Serial.println("Gagal membuka file konfigurasi");
      delay(1000);
      retryCount++;
      continue;
    }

    size_t size = fileConf.size();
    std::unique_ptr<char[]> buf(new char[size]);
    fileConf.readBytes(buf.get(), size);
    fileConf.close();

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error) {
      Serial.println("Gagal melakukan parse JSON");
      delay(1000);
      retryCount++;
      continue;
    }

    const char *ssid = doc["a"];
    const char *password = doc["b"];
    const char *mqtt_server = doc["c"];
    const char *mqtt_topic = doc["d"];
    int mqtt_port = doc["e"];

    return true;
  }
  return false; 
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Sedang mengkoneksikan ke MQQT...");
    if (client.connect("esp32Client")) {
      Serial.println("Sukses konek MQTT broker");
      client.subscribe("esp8266/status");
    } else {
      Serial.print("Gagal, return kode ");
      Serial.print(client.state());
      Serial.println(" mencoba ulang 5 detik lagi...");
      delay(5000);
    }
  }
}