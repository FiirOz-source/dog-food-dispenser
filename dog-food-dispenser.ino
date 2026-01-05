#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char *ssid = "iPhone de Timothée";
const char *password = "TimAuThe";

ESP8266WebServer server(80);

void setup()
{
    Serial.begin(115200);
    delay(100);

    // Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connexion");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnecté !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Filesystem
    if (!LittleFS.begin())
    {
        Serial.println("Erreur LittleFS !");
        return;
    }

    // Sert index.html à la racine
    server.on("/", HTTP_GET, []()
              {
    File f = LittleFS.open("/index.html", "r");
    if (!f) {
      server.send(500, "text/plain; charset=utf-8", "index.html introuvable dans LittleFS");
      return;
    }
    server.streamFile(f, "text/html; charset=utf-8");
    f.close(); });

    // Sert tous les autres fichiers statiques (CSS/JS/images)
    // Exemple : /style.css -> /style.css dans LittleFS
    server.serveStatic("/", LittleFS, "/");

    // 404
    server.onNotFound([]()
                      { server.send(404, "text/plain; charset=utf-8", "404 - introuvable"); });

    server.begin();
    Serial.println("Serveur OK.");
}

void loop()
{
    server.handleClient();
}
