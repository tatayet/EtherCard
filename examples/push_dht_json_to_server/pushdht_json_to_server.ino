// test envoi Post data 2 server
// code origine 2011-07-08 <jc@wippler.nl>
// 2013-10-22 hneiraf@gmail.com Handle returning code and reset ethernet module if needed
// 2020-06 Envoi à un serveur  de data en Json par methode GET by Dimitri

// License: GPLv2

#include <EtherCard.h>
#define version "0.1"
// change these settings to match your own setup
#define Dest_file "get_json.php"
// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x9A,0x64,0x29,0x70,0x35 };
const char website[] PROGMEM = "192.168.1.103";
byte Ethernet::buffer[350];
uint32_t timer;
Stash stash;
byte session;
//timing variable
int res = 0;
void setup () {
  Serial.begin(9600);
  Serial.println("\n[envoi de data en json]");
  //Initialize Ethernet
  initialize_ethernet();
}
void initialize_ethernet(void){
    for(;;){ // keep trying until you succeed
        //Reinitialize ethernet module
        pinMode(5, OUTPUT);
        Serial.println("Reseting Ethernet...");
        digitalWrite(5, LOW);
        delay(1000);
        digitalWrite(5, HIGH);
        delay(500);
        // Change 'SS' to your Slave Select pin, if you arn't using the default pin
        if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0){
          Serial.println( "Failed to access Ethernet controller");
          continue;
        }
        if (!ether.dhcpSetup()){
          Serial.println("DHCP failed");
          continue;
        }
        ether.printIp("IP:  ", ether.myip);
        ether.printIp("GW:  ", ether.gwip);
        ether.printIp("DNS: ", ether.dnsip);
        if (!ether.dnsLookup(website))
          Serial.println("DNS failed");
        ether.printIp("SRV: ", ether.hisip);
        //reset init value
        res = 0;
        break;
    }
}
void loop () {
  //if correct answer is not received then re-initialize ethernet module
  if (res > 220){
    initialize_ethernet();
  }
  res = res + 1;
  ether.packetLoop(ether.packetReceive());
  //200 res = 10 seconds (50ms each res)
  if (res == 200) {
    //Generate random info
    float tension = random(0,500);
    float courant = random(0,100)/10;

    // generate two fake values as payload - by using a separate stash,

    byte sd = stash.create();
    
    
    stash.print( "{\"tension\":");
    stash.println(tension);
    stash.print( ",\"courant\":");   
    stash.println(courant);
    stash.print( ",\"compilation\":"); 
    stash.println("\"le " __DATE__ " à " __TIME__ "\"");
    stash.println("}");
    stash.save();   

    //Display data to be sent
    Serial.println(tension);
    Serial.println(courant);

// attention bien respecter les espaces et les \r\n  à la moindre erreur Apache génère un code 400 
    Stash::prepare(PSTR("GET /$F HTTP/1.1\r\n"
      "Host: $F\r\n"
      "content-Type: application/json;charset=utf-8\r\n"
      "Content-Length: $D\r\n"
      "\r\n"
      "$H"),
    PSTR(Dest_file), website, stash.size(), sd);    

    session = ether.tcpSend();
  }
   const char* reply = ether.tcpReply(session);
   if (reply != 0) {
     res = 0;
     Serial.println(reply);
   }
   delay(50);
}

