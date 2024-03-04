#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
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
    Serial.println("Gagal membaca file konfigurasi. Mohon pastikan file 8266.json ada");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Sedang mengkoneksikan ke wifi");
  }
  Serial.println("Tersambung ke wifi");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP8266 MAC Address Monitoring</title>";
    html += "<style>body{font-family:Arial, sans-serif;background-color:#f4f4f4;margin:0;padding:0;display:flex;align-items:center;justify-content:center;height:100vh}";
    html += "h2,p{text-align:center;color:#333}p{font-size:1.2em;margin-top:10px} @media (max-width: 600px) { h2, p { font-size: 1em; } }</style>";
    html += "</head><body><h2>Update MAC Address ESP8266 Terbaru</h2>";
    html += "<p>MAC Address ESP8266: " + WiFi.macAddress() + "</p>";

    if (client.connected()) {
        html += "<p>Status Koneksi MQTT: Terhubung</p>";
    } else {
        html += "<p>Return Kode: " + String(client.state()) + "</p>";
        html += "<p>Status Koneksi MQTT: Tidak Terhubung</p>";
    }

    html += "<p>Hostname ESP8266: " + WiFi.hostname() + "</p>";
    html += "</body></html>";
    
    request->send(200, "text/html", html);
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
    File fileConf = SPIFFS.open("/8266.json", "r");
    
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
      client.subscribe("esp8266/status");
    } else {
      Serial.println(" mencoba ulang 5 detik lagi...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String receivedTopic = String(topic);
  if (receivedTopic == "esp32/control") {
    payload[length] = '\0';
    String command = String((char*)payload);
    if (command == "gantimac") {
      changeMac();
    }
  }
}

void changeMac() {
  char newMac[18];
  sprintf(newMac, "%02X:%02X:%02X:%02X:%02X:%02X", random(256), random(256), random(256), random(256), random(256), random(256));
  WiFi.macAddress(newMac);
  Serial.print("Mac Address baru: ");
  Serial.println(WiFi.macAddress());
}
