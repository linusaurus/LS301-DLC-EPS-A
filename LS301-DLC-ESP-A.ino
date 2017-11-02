/*
MQTT client LIFT_SLIDE Motor Controller
r.young 
8.29.2017
LS301-DLC-ESP-A
*/
#include <Encoder.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define DEBUG 1
#define ENCODER_OUTPUT 1
// Use this ON 157 7199 Only- Door 1
Encoder encoder(5,4);// Need to be changed 5,4 to this for the 01 door??

const char* ssid = "MonkeyRanch";
const char* password = "12345678";
const char* mqtt_server = "192.168.10.92";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int PWMpin = 15;
int DIRpin = 12;

int Switch_HI = 2;
int Switch_LO = 13;

byte MotorState= 0;  // 0=STOP; 1=RAISE ; 2=LOWER
long HI_LIMIT = 30000;
long LO_LIMIT = 0;

long newPosition;
long oldPosition;
byte newDoorPosition;
byte oldDoorPosition;

byte ButtonState_HI = HIGH;

const char* MQTT_CLIENT_ID = "LS301DLC-01";
const char* outTopic = "STATUS";
const char* inTopic = "DLIN";

//int relay_pin = 12;
int button_pin = 0;
bool relayState = LOW;

// Instantiate a Bounce object :
Bounce debouncer = Bounce(); 

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {   
    for(int i = 0; i<500; i++){     
      delay(1);
    }
    #ifdef DEBUG
    Serial.print(".");
    #endif
  }
  #ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  #endif
}

void callback(char* topic, byte* payload, unsigned int length) {
  #ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] "); 
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  #endif
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '0') {
    MotorState = 0;  // Stop Motor
    #ifdef DEBUG
    Serial.println("Motor State -> 0");
    #endif
    
  } else if ((char)payload[0] == '1') {    
    MotorState = 1;  // Raise Doors
    #ifdef DEBUG
    Serial.println("Motor State -> 1");
    #endif
   
  } else if ((char)payload[0] == '2') {
    MotorState = 2;  //Lower Doors
    #ifdef DEBUG
    Serial.print("Motor State -> 2 ");
    #endif
       
  }
}

void reconnect() {
  //Tag the MCU with the ESO8266 chip id
  // Loop until we're reconnected
  char CHIP_ID[8];
  String(ESP.getChipId()).toCharArray(CHIP_ID,8);
  
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
    #endif
    // Attempt to connect
    if (client.connect(CHIP_ID)) {
      #ifdef DEBUG
      Serial.println("connected");
      #endif
      // Once connected, publish an announcement...
      client.publish(outTopic, CHIP_ID );
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      #endif
      // Wait 5 seconds before retrying
      for(int i = 0; i<5000; i++){
        
        delay(1);
      }
    }
  }
}



void OnPositionChanged(){
  // check for value change
   if(newDoorPosition != oldDoorPosition){
      if(newDoorPosition==1){
      client.publish(outTopic, "1");
      EEPROM.write(0,newDoorPosition);
      EEPROM.commit();
      #ifdef DEBUG
      Serial.println("-> Send 1");
      #endif
        }
      if(newDoorPosition==2){
        client.publish(outTopic, "2");
        EEPROM.write(0,newDoorPosition);
        EEPROM.commit();
        #ifdef DEBUG
        Serial.println("-> Send 2");
        #endif
        }
              
      oldDoorPosition = newDoorPosition; 
   }
}

void setup() {
  
  EEPROM.begin(512);              // Begin eeprom to store on/off state
  pinMode(button_pin, INPUT);     // Initialize the relay pin as an output
  pinMode(13,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(PWMpin,OUTPUT);
  pinMode(DIRpin,OUTPUT);
  pinMode(Switch_HI, INPUT_PULLUP);
  pinMode(Switch_LO,INPUT_PULLUP);
  digitalWrite(PWMpin,LOW);
  newDoorPosition = EEPROM.read(0);
 
  delay(500);
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  setup_wifi();                   // Connect to wifi 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   
  newPosition = encoder.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    #ifdef ENCODER_OUTPUT
    Serial.println(newPosition);
    #endif
  }
  
/// ---------------------Switches----------------------------------------------------------
   if(digitalRead(Switch_HI)==LOW){MotorState=1; }
   if(digitalRead(Switch_LO)==LOW){MotorState=2; }  
/// ---------------------------------------------------------------------------------------
  
/// ---------------------State Table-----------------------------------------
  if(newPosition < HI_LIMIT && MotorState==1){Raise();}
  if(newPosition >= HI_LIMIT && MotorState==1){
    newDoorPosition=1;//Door is Raised
    MotorState=0;

    }
  
  if(newPosition > LO_LIMIT && MotorState==2){Lower();}
  if(newPosition <= LO_LIMIT && MotorState==2){
    newDoorPosition=2;//Door us Lowered
    MotorState=0;
    
    }
    
  if(MotorState==0){Stop();} 

  
///----------------------END-LOOP------------------------

   OnPositionChanged();
}

//------------------------------------------------
////////////////// Motor Functions /////////////////////
//------------------------------------------------

void Stop(){
  digitalWrite(DIRpin,LOW);
  digitalWrite(PWMpin, LOW);
}
void Raise(){
  digitalWrite(DIRpin,HIGH);
  digitalWrite(PWMpin, HIGH);
}
void Lower(){
  digitalWrite(DIRpin,LOW);
  digitalWrite(PWMpin, HIGH);
}

  



