// ESP32
//Libraries
#include <Arduino.h>
#include <Wire.h>//https://www.arduino.cc/en/reference/wire
#include <DS3231.h>//https://github.com/NorthernWidget/DS3231
#include <LiquidCrystal_I2C.h>  // pour écran version finale
//#include <rgb_lcd.h>            // pour écran groove
#include <Rotary.h>             // Pour encodeur rotatif
#include <EEPROM.h>             // Gestion EEPROM (1024 octets)
//#include <Variables.h>

// Constantes:
const int colorR = 0;       // valeur rouge LCD RVB
int colorV = 10;            // valeur verte LCD RVB
const int colorB = 0;       // valeur bleu LCD RVB
const int maxHeure = 23;    // borne maxi pour réglage heure
const int maxMinute = 59;   // borne maxi pour réglage minutes
const int maxMenu = 7;      // Nb maxi de pages du menu principal
const int maxMenuE = 2;     // Nb maxi de pages du case 4 "CDE MANUELLE"
const int maxMenu5 = 2;     // Nb maxi de pages du case 5 "RESET PORTE"
const int veille = 2000;    // Durée en ms du passage en veille si inactivité utilisateur
const int flashLed = 5000;  // Durée entre flash led
const int dureeFlashLed = 50; // Durée flash led 
const int adresseDureeMontee = 0x00;    // On choisi d'écrire la durée Montée  à l'adresse 0x00 de l'EEPROM
const int adresseDureeDescente = 0x04;  // On choisi d'écrire le durée  descente à l'adresse 0x01 de l'EEPROM
const int adresseSetupOk = 0x08;        // On choisi d'écrire la valeur du setup à l'adresse 0x02 de l'EEPROM
const int adressePositionPorte = 0x09;  // On choisi d'écrire la valeur position porte à l'adresse 0x02 de l'EEPROM
// Variables :
byte Year ;
byte Month ;
byte Date ;
byte DoW ;
byte Hour ;
byte Minute ;
byte Second ;

byte A1Day = 0;           // Variables nécessaires pour DS3231 Al (ouverture) 
byte A1Hour;
byte A1Minute;
byte A1Second = 0;
byte A1AlarmBits = 0;
bool A1Dy = false;
bool A1h12 = false;
bool A1PM = false;

byte A2Day = 0;           // Variables nécessaires pour DS3231 A2 (fermeture) 
byte A2Hour;
byte A2Minute;
byte A2Second = 0;
byte A2AlarmBits = 0;
bool A2Dy = false;
bool A2h12 = false;
bool A2PM = false;

bool Century  = false;
bool h12 = false;
bool PM ;
byte maxCounter = maxMenu;      // Pour l'initialisation maxCounter doit être égal à maxMenu
byte maxCounterE = maxMenuE;    // idem  
byte tempHeure = 0;
byte tempMinute = 0;
byte col = 0;                   // colonne afficheur
byte ligne = 0;                 // ligne afficheur
byte valeur = 0;                // Valeur à afficher
bool wakeup = true;
unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long currentTimeFlashLed = 0;
unsigned long previousTimeFlashLed = 0;
unsigned long tempoMonte = 0;
unsigned long tempoDescente = 0;
float temperature = 0;
int tempInt = 0;

byte counter = 0;               // +1 ou -1 à chaque pas de rotation encodeur
byte counterMenuSave = 0;
int bp = 4;                     // Bp sur E4
int montee_porte = 7;           // Cde de montée porte sur sortie 7
int descente_porte = 6;         // Cde de descente porte sur sortie 6
int led_visu = 5;               // Led de visu mode de fonctionnement sur sortie 5                   
bool newBp = 1;
bool oldBp = 1;
bool click = false;
bool positionPorte = 0;         // 0 porte fermée, 1 porte ouverte
bool setupOk = false;           // setup Porte fait ou non
byte minCounterA2Hour = 0;      // pour que A2Hour>= A1Hour
byte minCounterA2Minute = 0;    // pour que A2Minute > A1Minute si A2Hour = A1Hour (pour ne pas avoir fermeture avant ouverture)
byte minCounter = 0;            // 0 si on regle horloge ou ouverture, A1Hour ou A1min
int totalMinutesOuverture = 0;    // Convertion heure minutes ouverture en minutes
int totalMinutesFermeture = 0;    // Convertion heure minutes fermeture en minutes
int totalMinutesHorloge = 0;
int compteurInterruptT2Overflow = 0;  // compteur d'interruptions overflow T2
// Pour que le compilateur ne génère pas de défaut car fonctions déclarées après appel

void affichage(byte col, byte ligne, byte valeur);
void rotate();
void menu();
void boutton();
void regHeure();
void regMinutes();
void setupPorte();

//****************************************************************
// CONSTRUCTION OBJETS
//****************************************************************
DS3231 Clock;
LiquidCrystal_I2C lcd(0x27,16,2); // Constructeur écran LCD version finale
//rgb_lcd lcd;                    // Constructeur eccran LCD
Rotary rotary = Rotary(2, 3);     // Constructeur encodeur branché sur PIN 2 et 3.

//*********************FIN CONSTRUCTION OBJETS********************

// ******************************************************
// INITIALISATION
// ******************************************************
void setup(){
Serial.begin(115200);
Serial.println(F("Initialize System"));
Wire.setClock(400000);
Wire.begin();

lcd.init();                      // initialize the lcd 
lcd.backlight();

//lcd.begin(16,2);
//lcd.setRGB(colorR,colorV,colorB);
pinMode (bp,INPUT);
pinMode (montee_porte,OUTPUT);    // sortie 7
pinMode (descente_porte,OUTPUT);  // sortie 6
pinMode (led_visu,OUTPUT);        // sortie 5
digitalWrite (montee_porte,LOW);
digitalWrite (descente_porte, LOW);
digitalWrite (led_visu, LOW);
// Initialisation timer 2
TCCR2A = 0b00000000; //Compteur type normal default 
TCCR2B = 0b00000110; // Réglage du préscaler : clk/256 est incrémenté toutes les 16uS  
TIMSK2 = 0b00000001; // Bit 0 – TOIE2: Timer/Counter2 Autorisation d'interruption sur débordement Timer 2
sei();               // autorise les interruptions (positionne le Bit 7 : I à 1 du registre SREG)

EEPROM.get(adresseDureeMontee, tempoMonte);       // On charge tempoMontée avec la val en eeprom 0x00
Serial.println(tempoMonte);
EEPROM.get(adresseDureeDescente, tempoDescente);  // On charge tempoDescente avel la val en eeprom 0x00
Serial.println(tempoDescente);
setupOk = EEPROM.read(adresseSetupOk);            // On charge le setupOk avec la VAl en EEPROM 0x08
Serial.println(setupOk);

// Autorisation d'interruptions sur un changement d'état sur les PIN 2 et 3.
attachInterrupt(0, rotate, CHANGE);
attachInterrupt(1, rotate, CHANGE);
// On va lire les alarmes pour savoir si elles sont déja paramétrées
Clock.getA1Time(A1Day, A1Hour, A1Minute, A1Second, A1AlarmBits, A1Dy, A1h12, A1PM);
Clock.getA2Time(A2Day, A2Hour, A2Minute, A2AlarmBits, A2Dy, A2h12, A2PM);
if (A1Hour == 0 && A2Hour == 0 && A1Minute == 0 && A2Minute == 0) // si non, on met les valeurs par défaut 6h00 et 19h00
{
 A1Hour = 6;
 A1Minute = 0;
 A2Hour = 19;
 A2Minute = 0;
 Clock.setA1Time(A1Day, A1Hour, A1Minute, A1Second, A1AlarmBits, A1Dy, A1h12, A1PM);
 Clock.setA2Time(A2Day, A2Hour, A2Minute, A2AlarmBits, A2Dy, A2h12, A2PM);
}
 totalMinutesOuverture = (A1Hour * 60) + A1Minute;
 totalMinutesFermeture = (A2Hour * 60) + A2Minute;
}
// ************** FIN INITIALISATION ****************

// **************************************************
//              BOUCLE PRINCIPALE
// **************************************************
void loop(){

// ****************************************
//          SETUP INITIALISATION
// ****************************************

if (setupOk == false){
  lcd.print("  SETUP PORTE   ");
  setupPorte();

}

//******************************************    
//          GESTION DE LA VEILLE
//******************************************    
  if (wakeup == false){                                     // Si pas d'activité humaine 
    lcd.clear();                                            // On éteint l'affichage
    lcd.noDisplay();                                        // On éteint le backlight
  //  lcd.setRGB(0,0,0);
    counter = 0;                                            // On revient au menu principal pour le reveil
    currentTimeFlashLed = millis();  
      if ((currentTimeFlashLed - previousTimeFlashLed) > flashLed){
        for (int i = 0; i < 3; i++){                        // Signal de vie en mode veille
          digitalWrite(5,HIGH);
          delay (10);
          digitalWrite(5, LOW);
          delay (50);
          previousTimeFlashLed = millis();
        }
      }  
    }
    else{
    previousTimeFlashLed = millis();
    lcd.display();
    //lcd.setRGB(0,10,0);
    }
//******************************************* 
//            GESTION DES MENUS
//*******************************************
// Menu PouPoule : 
    boutton();
    switch(counter){
      case 0:
          click = false;        
          lcd.setCursor(6,1);
          lcd.print(":");    
          lcd.setCursor(9,1);
          lcd.print(":");          
          lcd.setCursor(0,0);
          lcd.print("  PouPoule.com >");
          col = 4;
          ligne = 1;
          valeur = Clock.getHour(h12, PM);
          affichage(col, ligne, valeur);
          col = 7;
          valeur = Clock.getMinute();
          affichage(col, ligne, valeur);
          col = 10;
          valeur = Clock.getSecond();
          affichage(col, ligne, valeur);
      break;
   
// Menu réglage de l'heure courante :
      case 1:
          lcd.setCursor(0,0);
          lcd.print("<*INIT HORLOGE >");
          lcd.setCursor(6,1);
          lcd.print(":");    
          lcd.setCursor(9,1);
          lcd.print(":");
          col = 4;
          ligne = 1;
          valeur = Clock.getHour(h12, PM);        
          affichage(col, ligne, valeur);
          col = 7;
          valeur = Clock.getMinute();
          affichage(col, ligne, valeur);
          col = 10;
          valeur = 0;
          affichage(col, ligne, valeur);
         
          if (click == true){ // si click
              click = false;  // on aquitte le click et on va régler l'heure
              regHeure();}
          else
              lcd.noCursor();
              lcd.noBlink();
      break;        

// Menu réglage de l'horaire d'ouverture :
      case 2:
          Clock.getA1Time(A1Day, A1Hour, A1Minute, A1Second, A1AlarmBits, A1Dy, A1h12, A1PM);
          lcd.setCursor(0,0);
          lcd.print("<*  OUVERTURE  >");
          lcd.setCursor(6,1);
          lcd.print(":");    
          lcd.setCursor(9,1);
          lcd.print(":");
          col = 4;
          ligne = 1;
          valeur = A1Hour;//heureOuverture;
          affichage(col, ligne, valeur);
          col = 7;
          valeur =  A1Minute;//minuteOuverture;
          affichage(col, ligne, valeur);
          col = 10;
          valeur = 0;
          affichage (col,ligne,valeur);
          boutton();
          if (click == true){ // si click
              click = false;  // on aquitte le click et on va régler l'heure
              regHeure();}
          else
              lcd.noCursor();
              lcd.noBlink();
      break;

// Menu réglage de l'horaire de fermeture :
      case 3:
          Clock.getA2Time(A2Day, A2Hour, A2Minute, A2AlarmBits, A2Dy, A2h12, A2PM);
          lcd.setCursor(0,0);
          lcd.print("<*  FERMETURE  >");
          lcd.setCursor(6,1);
          lcd.print(":");    
          lcd.setCursor(9,1);
          lcd.print(":");
          col = 4;
          ligne = 1;
          valeur = A2Hour;//heureFermeture;
          affichage(col, ligne, valeur);
          col = 7;
          valeur = A2Minute;//minuteFermeture;
          affichage(col, ligne, valeur);
          col = 10;
          valeur = 0;
          affichage (col,ligne,valeur);
          boutton();
          if (click == true){ // si click
              click = false;  // on aquitte le click et on va régler l'heure
              regHeure();}
          else
              lcd.noCursor();
              lcd.noBlink();
      break;

// Menu commande porte manuelle:
      case 4:
          lcd.setCursor(0,0);
          lcd.print("<*CDE MANUELLE >");
          lcd.setCursor(0,1);
          lcd.print("                ");
      if (click == true){                             // si click
          click = false;                              // on aquitte le click 
          //delay(500); 
          counterMenuSave = counter;                  // On sauvegarde le compteur menu prinncipal
          counter = 0;
          maxCounter = maxCounterE;                   // On charge le maxi compteur du menu cde manuelle
          do {
            switch(counter){
            case 0:
            lcd.setCursor(0,1);
            lcd.print(" *   Ouvrir    >");            
            boutton();
            if (click == true && positionPorte == 0){ // On ouvre la porte si click et porte basse
              click = false;
              oldBp = 1;
              digitalWrite(montee_porte,HIGH);
              delay(tempoMonte);
              digitalWrite(montee_porte,LOW);
              positionPorte = 1;
              EEPROM.write(adressePositionPorte, positionPorte);
            }else{
              click = false;                          // Si appui avec porte déja ouverte on ne fait rien et on aquite le click
              oldBp = 1;}
            break;                           
            
            case 1:                                  
            lcd.setCursor(0,1);
            lcd.print("<*   Fermer    >");
            boutton();
            if (click == true && positionPorte == 1){  // On ferme la porte si click et porte haute
                click = false;
                digitalWrite(descente_porte,HIGH);
                delay(tempoDescente);
                digitalWrite(descente_porte,LOW);
                positionPorte = 0;
                EEPROM.write(adressePositionPorte, positionPorte);
            }else{
              click = false;}                          // Si appui avec porte déja fermée on ne fait rien et on aquite le click
            break;
            
            case 2:
            lcd.setCursor(0,1);
            lcd.print("<*   Sortie     ");
            boutton();
            break;
            } // fin switch
      } // fin do
      while (click == false);
      click = false;     
      counter = 0;
      maxCounter = maxMenu;                       // on revient au menu principal donc on charge le max du menu principal
//      delay(500);
      lcd.clear();          
    }else{            // fin boucle if. Si setup non fait on ne rentre pas dans menu mais on aquite click
      click = false;

    } 
      break;
      
      case 5 :
          lcd.setCursor(0,0);
          lcd.print("<* RESET PORTE >");
          lcd.setCursor(0,1);
          lcd.print("                ");
          if (click == true){
            click = false;
            counterMenuSave = counter;
            maxCounter = maxMenu5;
            counter = 0;
            do{
            switch(counter){
            case 0:
              lcd.setCursor(0,1);
              lcd.print(EEPROM.get(adresseDureeMontee, tempoMonte));
              lcd.setCursor(4,1);
              lcd.print("ms ");
              lcd.setCursor(7,1);
              lcd.print(EEPROM.get(adresseDureeDescente, tempoDescente));
              lcd.setCursor(11,1);
              lcd.print("ms  ");
              lcd.setCursor(15,1);
              lcd.print(EEPROM.read(adressePositionPorte));
            break;
            case 1:
              //lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("<* RESET PORTE >");
              lcd.setCursor(0,1);
              lcd.print("click pour RESET");
              boutton();
                if (click == true){ // On ouvre la porte si click et porte basse
                    click = false;
                    setupOk = false;
                    if (positionPorte == 1){
                    digitalWrite(descente_porte, HIGH);
                    delay(tempoDescente);     // on ferme
                    digitalWrite(descente_porte, LOW);
                    positionPorte = 0;
                    EEPROM.write(adressePositionPorte, positionPorte);
                    }
                    EEPROM.write(adresseSetupOk, setupOk);
                    tempoMonte = 0;
                    tempoDescente = 0;
                    EEPROM.put(adresseDureeMontee, tempoMonte);
                    EEPROM.put(adresseDureeDescente, tempoDescente);
                    lcd.setCursor(0,1);
                    lcd.print("    RESET OK    ");
                    delay(500);
                    counter = 2;
                    click = true;
                  }
            break;
            case 2:
               lcd.setCursor(0,1);
              lcd.print("<*   Sortie     ");
              boutton();
            break;
            }// fin switch
          } // fin do
            while (click == false);
      click = false;     
      counter = 0;
      maxCounter = maxMenu;                       // on revient au menu principal donc on charge le max du menu principal
//      delay(500);
      lcd.clear();
      }// fin if
      break;
// Menu température:
      case 6:
          lcd.setCursor(0,0);
          lcd.print("< TEMPERATURE  >");
          lcd.setCursor(5,1);
          temperature = Clock.getTemperature();     // convertion float en int pour ne garder que la partie entière
          tempInt = static_cast<int>( temperature ); 
          lcd.print(tempInt); 
          lcd.setCursor(7,1);
          lcd.print("Deg");  
      break;

// Menu version:
      case 7:
          lcd.setCursor(0,0);
          lcd.print("<   FIRMWARE    ");
          lcd.setCursor(0,1);
          lcd.print("      V1.0      ");
      break;
    } //fin switch


//**********************************************
// Test plage horaire
//**********************************************
if(setupOk == true){
totalMinutesHorloge = (Clock.getHour(h12,PM) * 60) + Clock.getMinute(); // on convertit horloge en total minutes
  switch (positionPorte) {
  case 0:                     // si porte basse et on est dans intervalle jour
    if (totalMinutesOuverture <= totalMinutesHorloge && totalMinutesHorloge < totalMinutesFermeture){
    positionPorte = 1;
    EEPROM.write(adressePositionPorte, positionPorte);
    digitalWrite(montee_porte, HIGH);
    delay(tempoMonte);        // on ouvre
    digitalWrite(montee_porte, LOW);
    }
    break;    
  case 1:                     // si porte haute et pas dans intervelle jour
    if ((totalMinutesOuverture <= totalMinutesHorloge && totalMinutesHorloge < totalMinutesFermeture) == false){
    positionPorte = 0;
    EEPROM.write(adressePositionPorte, positionPorte);
    digitalWrite(descente_porte, HIGH);
    delay(tempoDescente);     // on ferme
    digitalWrite(descente_porte, LOW);
    }
    break;  
  }
//Serial.print(positionPorte);
//Serial.print (" ");
//Serial.print(totalMinutesOuverture);
//Serial.print (" ");
//Serial.print(totalMinutesHorloge);
//Serial.print (" ");
//Serial.print(totalMinutesFermeture);
//Serial.println (" ");
} //fin if

} // fin boucle loop

//******************************************************************
// FONCTIONS
//******************************************************************

// **********************************************************************
// Routine d'interruption  appelé sur un changement d'état des PIN 2 et 3
// **********************************************************************
void rotate() {
  unsigned char result = rotary.process();
  compteurInterruptT2Overflow = 0;                        // Tant qu'il y a de l'activité humaine on resete le compteur d'interruption pour le passage en veille
  wakeup = true;                                          // On reste reveillé
  if (result == DIR_CW) {
    if (counter<maxCounter){  
        counter++;}
   // Serial.println(counter);
   }
  else if (result == DIR_CCW) {
    if (counter > minCounter){                         // minCounter et minCounterA2Hour et minCounterA2Minute
        counter--;}
   // Serial.println(counter);
   } 
}

// ***************************************************
// Routine d'interruption Timer 2
// ***************************************************
    ISR (TIMER2_OVF_vect) 
    {  
      // 256-6 --> 250X16uS = 4mS  
      // Recharge le timer pour que la prochaine interruption se déclenche dans 4mS
      TCNT2 = 6;
     
      if (compteurInterruptT2Overflow++ == veille) {
        //1000*4mS = 4S - On passe en veille
        compteurInterruptT2Overflow = 0;  
        wakeup = false;
      }  
    } 

//*************************************
//Click enregistré sur frond descendant
//*************************************
/*
void boutton(){
newBp = digitalRead(bp);
if (newBp != oldBp && newBp == 0){
  click  = true;
  oldBp = newBp;
  compteurInterruptT2Overflow = 0;                 // Tant qu'il y a de l'activité (clic ou tourne), on remet à zero le compteur veille 
  wakeup = true;                                   // Et on reste réveillé
  }
}
*/

void boutton(){
newBp = digitalRead(bp);
if (newBp != oldBp && newBp == 0){
    oldBp = newBp;   
    while (oldBp == 0){
        newBp = digitalRead(bp);
        if (newBp != oldBp && newBp == 1){
          oldBp = newBp;
          click = true;
          compteurInterruptT2Overflow = 0;
          wakeup = true;
        } // fin if 2
    } // fin while
  } // fin if 1
} // fin fonction



//*************************************************************
// Réglage heures / minutes pour horloge et ouverture fermeture
//*************************************************************
void regHeure(){
counterMenuSave = counter;
if (counterMenuSave == 1){              // Si menu réglage horloge
  counter = Clock.getHour(h12,PM);      // on charge counter avec l'heure en cours
  tempHeure = counter;                  // Pour éviter mise à zero heure au démarrage et tempo veille
  minCounter = 0;}                      // On charge minCounter avec 0
else if (counterMenuSave == 2){         // Si menu réglage ouverture:
  counter = A1Hour;                     // On charge counter avec l'heure d'ouverture en cours
  tempHeure = counter;
  minCounter = 0;}                      // On charge minCounter = 0
else if (counterMenuSave == 3){         // Si menu reglage fermeture:
  counter = A2Hour;                     // On charge counter avec l'heure de fermeture en cours
  tempHeure = counter;
  minCounter = minCounterA2Hour;}

maxCounter = maxHeure;  // pour ne pas dépasser 23
//delay(500);
do{                                     // Boucle réglage des heures. Tant que pas de click ou tempo veille
//currentTimeSleep = millis();                              // On charge currentTimeSleep avec le compteur millis
//  if ((currentTimeSleep - previousTimeSleep) > veille){   // Si Delta superieur à valeur veille.
//      wakeup = false;}                                    // On s'endort.
col = 4;
ligne = 1;
valeur = counter;
affichage(col,ligne,valeur);              // on affiche heure en cours
boutton();                                // On scrute un click
}
while(click == false && wakeup == true);  // Si click On boucle tans que pas d'appui utilisateur ou fin tempo de veille
click = false;

if(wakeup == true){                       // On ne sauvegarde pas si passage en veille
  tempHeure = counter;}

//**************************************************************************************
//                                  REGLAGE MINUTES
//**************************************************************************************
if (counterMenuSave == 1){                // pareil pour les minutes 
  counter = Clock.getMinute();
  tempMinute = counter;
  minCounter = 0;}
else if (counterMenuSave == 2){
  counter = A1Minute;
  tempMinute = counter;
  minCounter = 0;}
else if (counterMenuSave == 3){
  counter = A2Minute;
  tempMinute = counter;
  minCounter = 0;
}
maxCounter = maxMinute;
//delay(500);
do{
//  currentTimeSleep = millis();                              // On veille à sortir de ce menu et passer en veille si aucun signe de vie utilisateur
//  if ((currentTimeSleep - previousTimeSleep) > veille){   // Si tempo veille atteinte alors on sort
//      wakeup = false;}                                    //
col = 7;
ligne = 1;
valeur = counter;
affichage(col,ligne,valeur);
boutton();
}
while(click == false && wakeup == true);  // && wakeup == true Tant que pas d'appui utilisateur ou tempo veille atteinte
click = false;

if (wakeup == true){                      // On ne sauvegarde pas si on passe en veille
    tempMinute = counter;
}
counter = 0;
maxCounter = maxMenu;
minCounter = 0;

// répartition
if (counterMenuSave == 1){
  Clock.setHour(tempHeure);
  Clock.setMinute(tempMinute);
}
else if (counterMenuSave == 2){
  A1Hour = tempHeure;
  A1Minute = tempMinute;
  totalMinutesOuverture = (A1Hour * 60) + A1Minute;
  if (totalMinutesOuverture < totalMinutesFermeture){
    Clock.setA1Time(A1Day, A1Hour, A1Minute, A1Second, A1AlarmBits, A1Dy, A1h12, A1PM);
    lcd.clear();
    lcd.print("Save...");
    delay(300);
    minCounterA2Hour = A1Hour;
    //minCounterA2Minute = A1Minute;
  }
  else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print ("   Attention !  ");
    lcd.setCursor(0,1);
    lcd.print ("Reglage invalide");
    delay(2000);
    lcd.clear();
    }
}
else if (counterMenuSave == 3){
  A2Hour = tempHeure;
  A2Minute = tempMinute;
  totalMinutesFermeture = (A2Hour * 60) + A2Minute;
  if (totalMinutesOuverture < totalMinutesFermeture){
    Clock.setA2Time(A2Day, A2Hour, A2Minute, A2AlarmBits, A2Dy, A2h12, A2PM);
    lcd.clear();
    lcd.print("Save...");
    delay(300);
  }
  else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print ("   Attention !  ");
    lcd.setCursor(0,1);
    lcd.print ("Reglage invalide");
    delay(2000);
    lcd.clear();
    }  
}
} // fin fonction

//***************************
// affichage horloge
//***************************
void affichage(byte col,byte ligne,byte valeur){
  if (valeur >= 10){
    lcd.setCursor(col,ligne);
    lcd.print(valeur);}
  else if (valeur < 10){
    lcd.setCursor(col+1,ligne);
    lcd.print(valeur);
    lcd.setCursor(col,ligne);
    lcd.print("0");}
}

//****************************************
// Pilotage porte pour INIT PORTE
//****************************************
void setupPorte(){
  lcd.setCursor(0,1);
  lcd.print(" *   Ouvrir     ");
          
    do{
        previousTime = millis();        // tant que pas click on initialise en boucle previousTime
        boutton();}                     // on teste si click
    while (click == false);             // Si click (appel enregisté)
        click = false;                  // on acquite l'appel
    //while (digitalRead(bp) == LOW){       // tant que le boutton est appuyé
    //    digitalWrite(13,HIGH);}         // sortie active
    //    digitalWrite(13,LOW);           // si boutton relaché, sortie inactive   
    do{
      digitalWrite(montee_porte,HIGH);
    }
    while (digitalRead(bp) == 0);
     digitalWrite(montee_porte,LOW);

        currentTime = millis();         // on sauvegarde millis dans currentTime 
        tempoMonte = (currentTime - previousTime);  // on calcule la durée de l'appui boutton(en ms)
        EEPROM.put(adresseDureeMontee, tempoMonte); // On sauv la val à partir de l'adresse 0x00 (type long : 4 octets)
        positionPorte = 1;
        EEPROM.write(adressePositionPorte, positionPorte);      
 //       Serial.print(tempoMonte/1000);              // on affiche le resultat divisé par 1000 pour avoir des secondes
  //  }
        delay (500);                               // Tempo 1s 

        lcd.setCursor(0,1);             // même procédure pour la fermeture
        lcd.print(" *   Fermer     ");
    do{
        previousTime = millis();
        boutton();}
    while (click == false);
        click = false;
        oldBp = 1;
    while (digitalRead(bp) == 0){
        digitalWrite(descente_porte,HIGH);}
        digitalWrite(descente_porte,LOW);
        
        currentTime = millis();
        tempoDescente = (currentTime - previousTime);
        EEPROM.put(adresseDureeDescente, tempoDescente);    // On sauv la val à partir de l'adresse 0x04 (type long : 4 octets)
        positionPorte = 0;
        EEPROM.write(adressePositionPorte, positionPorte);
   //     Serial.print(tempoDescente/1000);

        counter = 0;                  // on revient au menu principal
        setupOk = true;
        EEPROM.write(adresseSetupOk, setupOk);    
        lcd.clear();
        lcd.print("Setup porte OK !");
        delay(500);
}
