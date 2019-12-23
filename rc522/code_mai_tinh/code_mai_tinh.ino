#include <WiFi.h>
#include <PubSubClient.h>
const char* ssid = "PARTH_Labs";
const char* password = "0905738734";
const char* mqtt_server = "209.97.173.222";

#include <SPI.h>      //include the SPI bus library
#include <MFRC522.h>  //include the RFID reader library
#define SS_PIN 21  //slave select pin
#define RST_PIN 22  //reset pin
byte tracecoderecive[36];
byte tracename[16] = "trace.elefos.io";
MFRC522 mfrc522(SS_PIN, RST_PIN);  // instatiate a MFRC522 reader object.
MFRC522::MIFARE_Key key, key2;          //create a MIFARE_Key struct named 'key', which will hold the card information


WiFiClient espClient;
PubSubClient client(espClient);

//this is the block number we will write into and then read.
int block=4;  
int block2=5;
int block3 =6;

byte blockcontent[16]; 
byte blockcontent2[16];
//This array is used for reading out a block.
byte readbackname[18];
byte readbackblock[18];
byte readbackblock2[18];
byte tracecode[36];

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    tracecoderecive[i] = payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
//  if (topic == "get_name"){
//  for (int i = 0; i < length; i++) {
//    tracename[i] = payload[i];
//    Serial.print((char)payload[i]);
//  }
//  Serial.println();
//  }

}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("get_tracecode");
      client.subscribe("elefos001");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setup() 
{
    Serial.begin(115200);        // Initialize serial communications with the PC
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    SPI.begin();               // Init SPI bus
    mfrc522.PCD_Init();        // Init MFRC522 card (in case you wonder what PCD means: proximity coupling device)
    Serial.println("Scan a MIFARE Classic card");
  
  // Prepare the security key for the read and write functions.
//  for (byte i = 0; i < 6; i++) {
//    key.keyByte[i] = 0xFF;  //keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
//  }
     key.keyByte[0] = 's';
     key.keyByte[1] = 'o';
     key.keyByte[2] = 'f';
     key.keyByte[3] = 'e';
     key.keyByte[4] = 'l';
     key.keyByte[5] = 'e';

     key2.keyByte[0] = 'e';
     key2.keyByte[1] = 'l';
     key2.keyByte[2] = 'e';
     key2.keyByte[3] = 'f';
     key2.keyByte[4] = 'o';
     key2.keyByte[5] = 's';

}
void slipt_code (){
  for (int j=0; j<16; j++){
     blockcontent[j] = tracecoderecive[j];
     blockcontent2[j] = tracecoderecive[j+16];
  } 
}
void loop()
{  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
    Serial.println("card selected");
         
   //the blockcontent array is written into the card block
   slipt_code();
   writeBlock(block, tracename);
   writeBlock(block2, blockcontent);
   writeBlock(block3, blockcontent2);
   //read the block back
   readBlock(block, readbackname);
   readBlock(block2, readbackblock);
   readBlock(block3, readbackblock2);
   
   //uncomment below line if you want to see the entire 1k memory with the block written into it.
   mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  // mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, 1);
   // Serial.println();
   for (int j=0; j <16; j++){
    tracecode[j] = readbackblock[j];
    tracecode[16+j] = readbackblock2[j];
   }
   //print the block contents
   Serial.print("read name: ");
   for (int j=0 ; j<16 ; j++)
   {
     Serial.write (readbackname[j]);
   }
   Serial.println("");
   Serial.print("read block: ");
   for (int j=0 ; j<36 ; j++)
   {
     Serial.write (tracecode[j]);
   }
   Serial.println("");
   client.publish("abc", "ok");
}



//Write specific block    
int writeBlock(int blockNumber, byte arrayAddress[]) 
{
  //this makes sure that we only write into data blocks. Every 4th block is a trailer block for the access/security info.
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector
  if (blockNumber > 2 && (blockNumber+1)%4 == 0){Serial.print(blockNumber);Serial.println(" is a trailer block:");return 2;}
  Serial.print(blockNumber);
  Serial.println(" is a data block:");
  
  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed: ");
//         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
  
  //writing the block 
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  //status = mfrc522.MIFARE_Write(9, value1Block, 16);
  if (status != MFRC522::STATUS_OK) {
           Serial.print("MIFARE_Write() failed: ");
//           Serial.println(mfrc522.GetStatusCodeName(status));
           return 4;//return "4" as error message
  }
  Serial.println("block was written");
}



//Read specific block
int readBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
//         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }

//reading a block
byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
         // Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }
  Serial.println("block was read");
}
