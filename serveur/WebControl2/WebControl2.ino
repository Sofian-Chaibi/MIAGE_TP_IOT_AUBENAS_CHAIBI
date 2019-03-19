/*********
 created by cs
*********/
#include "OneWire.h"
#include "DallasTemperature.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>

/*===== Variable const ========*/
const int LEDPIN =19; // green light
const int ERRORLEDPIN =18; // red light
const float tempMax = 25;  // temperature trop eleve

/*===== Variable switch process  ========*/
const unsigned char LIGHTOFF ='0';
const unsigned char LIGHTON ='1';
const unsigned char TEMP ='2';
const unsigned char BRIGHTNESS ='3';
/*===== Variable Wifi  ========*/
const char* ssid ="Livebox-CBB4";
const char* pass="19445155EADA3345D4D33267DC"; // ne pirater pas ma box svp :p
/*===== Variable de reception  ========*/
unsigned char receivedChar; //reception du serveur
float termoSensorValue; //reception de temp
int brightness; //reception du lumiere
/*===== Variable de diverse ========*/
int port= 23; // port de connexion
int status;

String ipServeur ="192.168.1.11"; // adress a changer ici
unsigned long previousMillis = 0;

const long interval = 5000;    

OneWire onWire(port); //definit le port pour oneWire
DallasTemperature tempSensor(&onWire);// entité qui uilise le capteur

IPAddress server(192,168,1,11); // address du serveur


WiFiClient client;//client wifi
WebSocketsClient webSocket; // websocket object


/*===== Action a effectuer en fonction de l'ordre recus ========*/
void process(char inChar) {
  String s;
  switch (inChar) {
    case LIGHTOFF : // eteindre la lumiere
      digitalWrite(LEDPIN, LOW);
      delay(1000);
      Serial.println(""); // casse le problème de desynchronisation avec l'ouverture du port et reception de l'info
      Serial.print("OFF");
      webSocket.sendTXT("OFF");
      delay(1000);
      break;
    case LIGHTON : // Allumer la lumiere
      digitalWrite(LEDPIN, HIGH);
      delay(1000);
      Serial.println("");
      Serial.print("ON");
      webSocket.sendTXT("ON");
      delay(1000);
      break;
    case TEMP : // envoyer temp
      termoSensorValue = getOneWireTemperature();
      delay(1000);
      Serial.println(""); 
      Serial.println(termoSensorValue,DEC);
      if(termoSensorValue>=tempMax) // temperature trop elevé message d'erreur
      {
        s="salle 1 Etemp";
      }
      else
      {
        s=String(termoSensorValue);
      }
      webSocket.sendTXT("Temp : "+s);
      delay(1000);
      break;
    case BRIGHTNESS :// envoyer temp
      brightness = getBrightness();
      delay(1000);
      Serial.println(""); 
      Serial.println(brightness,DEC);
      s=String(brightness);
      webSocket.sendTXT("Light : "+s);
      delay(1000);
      break;
    default:
      Serial.println(""); 
  }
}

/*===== methode d'affichage Wifi ========*/
void print_ip_status(){
  Serial.print("WiFi connected \n");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print("\n");
  Serial.print("MAC address: ");
  Serial.print(WiFi.macAddress());
  Serial.print("\n"); 
}

/*===== Reception lumiere ========*/
int getBrightness()
{
  return analogRead(A0); 
}
/*===== Reception lumiere ========*/
float getOneWireTemperature()
{
  float tempe;
  tempSensor.requestTemperaturesByIndex(0); // le capteur realise une acquisition
  tempe= tempSensor.getTempCByIndex(0);
  return tempe;
}
/*===== Event lié au socket  ========*/
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  int lengthd=0;
  char * c = new char[1];
  switch(type) {
    case WStype_DISCONNECTED: // si le server est fermé
      Serial.printf("[WSc] Disconnected!\n");
      digitalWrite(ERRORLEDPIN, HIGH);
      break;
    case WStype_CONNECTED: // lors de la connexion
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      digitalWrite(ERRORLEDPIN, LOW);
      // send message to server when Connected
      webSocket.sendTXT("Arduino");
      break;
    case WStype_TEXT: // si reception de text
      Serial.println("reçus case");
      Serial.printf("[WSc] get text: %s\n", payload);
      c = (char *)payload;
      process(c[0]);
      break;

    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // Begin port to reach
  pinMode(LEDPIN,OUTPUT);
  pinMode(ERRORLEDPIN,OUTPUT);
  digitalWrite(ERRORLEDPIN, HIGH);
  int status = WiFi.begin(ssid, pass);
  Serial.print("\nConnecting Wifi...\n");
  while(WiFi.status() != WL_CONNECTED){
   Serial.println("Attempting to connect ..\n");
   delay(1000);
 }
 digitalWrite(ERRORLEDPIN, LOW);
  print_ip_status();

   webSocket.begin(ipServeur, 8080, "/");
   webSocket.onEvent(webSocketEvent);
   delay(1000);
   Serial.println("Arduino");
   

}  

void loop() {
  webSocket.loop();
  /*===== permet d'effectuer un delay car le webSocket.onEvent est une methode asynch
   et la methode delay  est dites bloquant et donc gene l'envois /reception========*/
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
  process('2');;
  process('3');
  }

}
