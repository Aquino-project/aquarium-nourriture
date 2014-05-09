#include <WebServer.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include "RTClib.h"

RTC_Millis RTC;

const int OPTO=6;
const int LEDIR=7;
const int PRESENCE=1;

const int LEDV=3;
const int LEDJ=4;

const int MOTOR=5;

int Turn=0;
int Ordre=0;
int EtatOPTO=0;

char TableauHeure[8][3];
int tableauSize = 0;

int lastHour = 0;

IPAddress ipLocal(192, 168, 0, 200);
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x4F, 0xF9 };

WebServer webserver("", 8080);

void feedCmd (WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {

  Ordre = 1;

  digitalWrite(MOTOR,HIGH);
  EtatOPTO=digitalRead(OPTO);
  delay(1000);

  if(EtatOPTO==1){
    Serial.print("OPTO: "); 
    Serial.println(EtatOPTO);
    Turn = 1;  
  }


  Serial.print("Turn: "); 
  Serial.println(Turn);

  digitalWrite(LEDIR, LOW);
  digitalWrite(LEDV, HIGH);
  digitalWrite(LEDJ, LOW);

  delay(150);

  server.httpSuccess("text/plain");
  server.println("Nourriture donnée !");
}

/**
 *
 * Supprime une heure de nourrissage
 *
 */
void feedDelete (WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  // On recupere l'heure a supprimer
  String rawHeure = getPostDatas(server, "heure");

  cleanPostDatas();

  // On la transforme en int
  int heure = rawHeure.toInt();

  // On parcour les heures sauvegardees
  for(int i = 0 ; i < tableauSize; i++) {
    // On transforme l'heure courante en int
    int heureCourante = atoi( TableauHeure[i] );

    // Si l'heure courante correspond a l'heure a supprimer
    if(heureCourante == heure){
      // On supprime l'heure du tableau de sauvegarde
      memset(TableauHeure[i], 0, 8);
      // On decremente le poids du tableau
      tableauSize = tableauSize - 1;
    }
  }

  server.httpSuccess("text/plain");
  server.println("Heure supprimee");

}

/**
 *
 * Ajoute une heure
 *
 */
void feedAdd (WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  server.httpSuccess("text/plain");

  // On recupere l'heure a ajouter
  String heure = getPostDatas(server, "heure");

  // On transforme l'heure en caractere
  char charHeure[3];
  heure.toCharArray(charHeure, 3);

  cleanPostDatas();
  
  // On stock l'heure dans le tableau
  strcpy(TableauHeure[tableauSize], charHeure);
  // On incremente le poids du tableau de sauvegarde
  tableauSize = tableauSize + 1;

  Serial.print("Heure : ");
  Serial.println(charHeure);
}



void setup() {
  Serial.begin(115200);

  Wire.begin();
  RTC.begin(DateTime(__DATE__,__TIME__));

  pinMode(OPTO, INPUT);
  pinMode(LEDIR, OUTPUT);
  pinMode(LEDJ, OUTPUT);
  pinMode(LEDV, OUTPUT);
  pinMode(MOTOR, OUTPUT);

  digitalWrite(LEDV,LOW);
  digitalWrite(LEDJ, HIGH);


  Ethernet.begin(mac, ipLocal);
  webserver.addCommand("feed/now", &feedCmd);
  webserver.addCommand("feed/add", &feedAdd);
  webserver.addCommand("feed/delete", &feedDelete);
}

void loop() {
  webserver.processConnection();
  DateTime now = RTC.now();
   
  int State = digitalRead(OPTO);

  if(State == 0) {
    digitalWrite(MOTOR, LOW);
    if (Ordre == 1) {
      
      Serial.println(Turn);
      if (Turn == 1) {
        digitalWrite(LEDJ,HIGH);
        Turn=0;
      }
    }
    
    digitalWrite(LEDV, LOW);
    digitalWrite(LEDJ, HIGH);
    Ordre=0;
  }



  uint8_t uHour = now.hour();
  int hour = int(uHour);
  
  if (lastHour != hour) {
    for(int i = 0 ; i < tableauSize; i++) {
      
      // On transforme l'heure courante en int
      int heure = atoi( TableauHeure[i] );

      Serial.println("Verification heure");

      // Si l'heure courante correspond a l'heure a supprimer
      if(hour == heure){
        lastHour = hour;

        Ordre = 1;
        digitalWrite(MOTOR,HIGH);
        EtatOPTO = digitalRead(OPTO);
        delay(1000);

        if(EtatOPTO==1){
          Serial.print("OPTO: "); 
          Serial.println(EtatOPTO);
          Turn = 1;  
        }

        Serial.print("Turn: "); 
        Serial.println(Turn);

        digitalWrite(LEDIR, LOW);
        digitalWrite(LEDV, HIGH);
        digitalWrite(LEDJ, LOW);

        delay(150);

        }
      }
    }
  }

/**********************/
/** HTTP POST SYSTEM **/
/**********************/

// Variable pour stocker les noms des donnees
char postNameDatas[16][16];
// Variable pour stocker les valeurs des donnees
char postValueDatas[16][16];
// Variable de la taille des donnees
int postDatasSize = 0;

/**
 *
 * Recupere les donnees de la requete courrante POST
 * et les stock dans un tableau et retourne celle demandee
 * @param WebServer server Instance la classe Webserver de la requete courante
 * @param String wanted Le nom de la donnee que l'on veut recuperer
 *
 * @return String
 *
 */
String getPostDatas(WebServer &server, String wanted)
{
  bool repeat = true;
  char name[16], value[16];

  // Si aucune donnee n'est stockee dans le tableau on parcour celle de la requete
  if (postDatasSize <= 0) {
    // Tant que tout n'est pas lu
    while(repeat) {
      repeat = server.readPOSTparam(name, 16, value, 16);
      String strName(name);

      if (strName != "") {
        // On stock le nom de la donnee
        strcpy(postNameDatas[postDatasSize], name);
        // On stock la valeur de la donnee
        strcpy(postValueDatas[postDatasSize], value);
        
        postDatasSize++;
      }
    }
  }

  // On parcoure le tableau de stockage des donnees
  for(int i = 0; i < postDatasSize; i++) {
    char * dataName = postNameDatas[i];
    char * dataValue = postValueDatas[i];

    String strDataName(dataName);
    String strDataValue(dataValue);

    // Si la donnee courante correspond a la donnee demande
    // on retourne sa valeur a l'utilisateur
    if (strDataName == wanted) {
      return dataValue;
    }
  }
  
  return "";
  
}

/**
 *
 * Nettoie le tableau de donnees POST
 * A executer en chaque fin de requete HTTP
 *
 */
void cleanPostDatas()
{
  char empty[1];

  for (int i = 0; i < postDatasSize; i++) {
    strcpy(postNameDatas[i], empty);
    strcpy(postValueDatas[i], empty);
  }

  postDatasSize = 0;
}