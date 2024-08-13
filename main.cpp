#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

Servo myServo;     // Clase Servo
int servoPin = 18; // Pin al que está conectado el servomotor

//MQTT
const char *ssid = "CLAROMT8X8";
const char *password = "8099308996";
const char *mqttBroker = "broker.emqx.io";
const int mqttPort = 1883;
const char *publishTopic = "halleffect";
const char *subscribeTopic = "servo";

WiFiClient espClient;
PubSubClient client(espClient);

//Mensajes
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (5)
char msg[MSG_BUFFER_SIZE];

String currentCommand = "";

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// Interrupcion que se activa cuando un mensaje es recibido
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    Serial.println(message += (char)payload[i]);
  }

  if (message == "ON") {
    currentCommand = message;
    Serial.println("Turning ON..");
  }
  else if (message == "OFF") {
    currentCommand = message;
    Serial.println("Turning OFF..");
  }
}

void reconnect()
{
  while (!client.connected()) {
    Serial.print("Intentando establecer conexion por MQTT... ");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);  //Se crea un numero de cliente aleatorio

    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado");
      client.subscribe(subscribeTopic);
    }
    else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" Volver a intentar en 5 segundos");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  myServo.attach(servoPin);
  setup_wifi();
  client.setServer(mqttBroker, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (currentCommand == "ON") {
    myServo.write(0);
    delay(500);
    Serial.println("La compuerta se esta abriendo.... ");
    currentCommand = false;
    Serial.println("Compuerta abierta.");
    }
  else if (currentCommand == "OFF") {
    myServo.write(180);
    delay(500);
    Serial.println("La compuerta se esta cerrando.... ");
    currentCommand = false;
    Serial.println("Compuerta cerrada.");
  }
  //Probar la conexion
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    int hallEffectValue = hallRead();
    snprintf(msg, MSG_BUFFER_SIZE, "%d", hallEffectValue);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(publishTopic, msg);
  }
}