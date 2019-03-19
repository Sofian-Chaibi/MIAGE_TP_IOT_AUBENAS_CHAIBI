/*********
 Based on Rui Santos work :
 https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/
 Modified by gm,cs
*********/
#include "OneWire.h"
#include "DallasTemperature.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

WiFiClient espClient;           // Wifi 
PubSubClient client(espClient); // MQTT client

const float tempMax = 25; // temperature max trop élevé
const int LEDPIN =19; // green pin
const int ERRORLEDPIN =18; //red pin
int port= 23; // port de connexion
int lowLigth= 100;// variable de luminosité basse
int brightness; //variable luminosité
OneWire onWire(port); //definit le port pour oneWire
DallasTemperature tempSensor(&onWire);// entité qui uilise le capteur


/*===== MQTT broker/server and TOPICS ========*/
const char* mqtt_server = "192.168.1.11"; /* ip perso car broker en ligne trop instable */

#define TOPIC_TEMP "miage1/menez/sensors/temp"
#define TOPIC_LED  "miage1/menez/sensors/led"
#define TOPIC_LIGHT  "miage1/menez/sensors/light"
#define TOPIC_ERROR  "miage1/menez/sensors/error"
#define NOM_SALLE_ERROR "salle 2 Etemp"

/*============= deep_sleep ======================*/
#define TIME_TO_SLEEP 30 // Time ESP32 will go to sleep (in seconds)
#define us_TO_S_FACTOR 1000000 // Conversion factor for microseconds to seconds 

/*============= Variable de reception de data  ======================*/
float temperature = 0;
float light = 0;

/*================ WIFI =======================*/
void print_connection_status() {
  Serial.print("WiFi status : \n");
  Serial.print("\tIP address : ");
  Serial.println(WiFi.localIP());
  Serial.print("\tMAC address : ");
  Serial.println(WiFi.macAddress());
}

void connect_wifi() {
  const char* ssid = "Téléphone Mi";
  const char *password= "966ca0b0556b";
  
  Serial.println("Connecting Wifi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect Wifi ..");
    delay(1000);
  }
  digitalWrite(ERRORLEDPIN, LOW);
  Serial.print("Connected to local Wifi\n");
  print_connection_status();
}

/*=============== SETUP =====================*/
void setup() {  
  Serial.begin(9600);
  pinMode(LEDPIN, OUTPUT);
  pinMode(ERRORLEDPIN,OUTPUT);
  digitalWrite(ERRORLEDPIN, HIGH);
  connect_wifi();
  
  client.setServer(mqtt_server, 1883);
  // set callback when publishes arrive for the subscribed topic
  client.setCallback(mqtt_pubcallback); 
}

/*============== CALLBACK ===================*/
void mqtt_pubcallback(char* topic, byte* message, 
                      unsigned int length) {
  // Callback if a message is published on this topic.
  
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  // Byte list to String and print to Serial
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic,
  // you check if the message is either "on" or "off".
  // Changes the output state according to the message with led topic
  if (String(topic) == TOPIC_LED) {
    if(messageTemp=="ON")
    {
       Serial.println("chauffage allumer");
       digitalWrite(LEDPIN,HIGH);
       
    }
    if(messageTemp=="OFF")
    {
       Serial.println("chauffage allumer");
       digitalWrite(LEDPIN,LOW);
       
    }
   
  }
}

/*============= SUBSCRIBE =====================*/
void mqtt_mysubscribe(char *topic) {
  while (!client.connected()) { // Loop until we're reconnected
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32", "try", "try")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(topic);
    } else {
      digitalWrite(ERRORLEDPIN, HIGH);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*============= Reception de temperature =====================*/
float get_Temperature(){
  float tempe=0;
  tempSensor.requestTemperaturesByIndex(0); // le capteur realise une acquisition
  tempe= tempSensor.getTempCByIndex(0);
  return tempe;
}

/*============= Reception de luminosité =====================*/
int getBrightness()
{
  return analogRead(A0); 
}

/*================= LOOP ======================*/
void loop() {
  int32_t period = 5000; // 5 sec
  /*--- subscribe to TOPIC_REQUEST if not yet ! */
  if (!client.connected()) { 
    mqtt_mysubscribe((char *)(TOPIC_LED));
  }

  /*--- Publish Temperature periodically   */
  
  brightness=getBrightness();
  // if lumiere basse = phase de repos / eco d'energie
  if (brightness<=lowLigth)
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * us_TO_S_FACTOR);
    Serial.println("go dodo");
    // Deep sleep mode
    esp_deep_sleep_start() ;
  }
  temperature = get_Temperature();
  // Convert the value to a char array
  char tempString[8];
  char tempInt[8];
  dtostrf(temperature, 1, 2, tempString);
  Serial.print("Published Temperature : "); 
  Serial.println(tempString);
  Serial.println(brightness);
  if(temperature>=tempMax)
  {
    //temperature trop élevé
     client.publish(TOPIC_ERROR, (char *)NOM_SALLE_ERROR);
     
  }
  itoa(brightness,tempInt,10);
  // Serial info
  Serial.print("Published Temperature : "); 
  Serial.println(tempString);
  Serial.println(brightness);
  // MQTT Publish
  client.publish(TOPIC_TEMP, tempString);
  client.publish(TOPIC_LIGHT,tempInt);
  
  delay(period);
  client.loop(); // Process MQTT ... une fois par loop()
}
