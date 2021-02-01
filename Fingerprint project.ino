#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal.h>
#include <Servo.h>


#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)

SoftwareSerial mySerial(2, 3);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal lcd(13, 7, 4, A0, 11, 12);

uint8_t id;
Servo myservo;
int addFingerPrintButton = A2;
int deleteFingerPrintButton = 9;
int buzzer = 10;
int freeID = 0;
int retroEclairage = 5;
int insideButton = A3;
bool ledState = LOW;
bool serrureState; //HIGH = ouvert       LOW = Fermé 
unsigned long retroEclairageMillis = 0; 
unsigned long serrureMillis = 0; 
const long interval = 5000; 
const long intervalSerrure = 10000; 
unsigned long currentMillis;
int lastFingerPrintIDdetected;
int iFoundID = 0;
boolean foundID = false;
String noms[3] = { "", "Lucas", "Paul", "Mams", "Florian" };

uint8_t eAigu[8] = {130,132,142,145,159,144,142,128};
uint8_t eGrave[8] = {136,132,142,145,159,144,142,128};

void setup() {
  lcd.createChar(0, eAigu);
  lcd.createChar(1, eGrave);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Capteur digital");
  lcd.setCursor(0, 1);
  lcd.print("En attente ...");
  
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  finger.begin(57600);
  pinMode(addFingerPrintButton, INPUT);
  pinMode(deleteFingerPrintButton, INPUT);
  pinMode(retroEclairage, OUTPUT);
  pinMode(insideButton, INPUT);
  

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  Serial.println("Bienvenue !");
  myservo.attach(8);
  myservo.write(80);
  serrureState = LOW;
  delay(500);  
  myservo.detach();
}


//uint8_t readnumber(void) {
//  uint8_t num = 0;
//
//  while (num == 0) {
//    while (! Serial.available());
//    num = Serial.parseInt();
//  }
//  return num;
//}


void loop() {

  // led en mode violet respiration
  //finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_PURPLE);
  // delay(3000);

  currentMillis = millis();
  if (currentMillis - retroEclairageMillis >= interval) {
    if (ledState == HIGH) {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(retroEclairage, ledState);
  }

  if (currentMillis - serrureMillis >= intervalSerrure) {
    if (serrureState == HIGH) {
      myservo.attach(8);
      myservo.write(80);
      serrureState = LOW;
      delay(500);  
      myservo.detach();
    }
  }
  
  if(digitalRead(addFingerPrintButton) == HIGH){ // Je vais ajouter une empreinte
    digitalWrite(retroEclairage, HIGH);
    retroEclairageMillis = currentMillis;
    Serial.print("Je lance l'ajout d'une empreinte");
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Posez votre ");
    lcd.setCursor(0, 1);
    lcd.print("doigt ...");
    delay(50);
    foundID = false;
    iFoundID = 0;
    freeID = foundFreeID();
    Serial.print("Enrolling ID #");
    Serial.println(freeID);
    while (!  ajouterEmpreinte() );
  }

  if(digitalRead(deleteFingerPrintButton) == HIGH){ // Je vais retirer une empreinte
    digitalWrite(retroEclairage, HIGH);
    retroEclairageMillis = currentMillis;
    Serial.print("Je lance la suppression d'une empreinte");
    Serial.print("Deleting ID #");
    Serial.println(lastFingerPrintIDdetected);
    deleteFingerprint(lastFingerPrintIDdetected);
  }

  if(digitalRead(insideButton) == HIGH){
    myservo.attach(8);
    myservo.write(0);
    delay(500); 
    myservo.detach();
    serrureState = HIGH;
    serrureMillis = currentMillis;
  }

   // On regarde si une empreinte correspond
  getFingerprintID();
  delay(50); 

}



uint8_t ajouterEmpreinte() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(freeID);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enlevez votre ");
  lcd.setCursor(0, 1);
  lcd.print("doigt ...");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(freeID);
  p = -1;
  Serial.println("Place same finger again");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reposez le ");
  lcd.setCursor(0, 1);
  lcd.print("meme doigt ...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(freeID);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(freeID);
  p = finger.storeModel(freeID);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lastFingerPrintIDdetected = id;
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);
    delay(500);
    finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_BLUE);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}


uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      digitalWrite(retroEclairage, HIGH);
      retroEclairageMillis = currentMillis;
      finger.getTemplateCount();
      Serial.print("Nombre d'empreintes : "); 
      Serial.println(finger.templateCount);
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);
    tone(buzzer, 2500, 300);
    //delay(700);
    myservo.attach(8);
    myservo.write(0);
    delay(500); 
    myservo.detach();
    serrureState = HIGH;
    serrureMillis = currentMillis;
    finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_BLUE);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Accès refusé");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
    tone(buzzer, 800, 125); delay(80);
    noTone(buzzer); delay(80);
    tone(buzzer, 800, 125); delay(80);
    noTone(buzzer); delay(80);
    lcd.clear();
    lcd.print("Acc"); lcd.write(byte(1)); lcd.print("s refus"); lcd.write(byte(0));
    delay(700);
    finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
    delay(1000);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  if(finger.fingerID < (sizeof(noms) / sizeof(noms[0]))) {
    Serial.println("Bienvenue "); 
    Serial.println(noms[finger.fingerID]);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bienvenue");
    lcd.setCursor(0, 1);
    lcd.print(noms[finger.fingerID]);
  } else {
    Serial.print("Entree autorisee"); 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Entr"); lcd.write(byte(0)); lcd.print("e autoris"); lcd.write(byte(0)); lcd.print("e");
  }
  lastFingerPrintIDdetected = finger.fingerID;
  return finger.fingerID;
}


uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
    delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}


uint8_t foundFreeID() {
  while(foundID != true){
    iFoundID++;
    delay(15);
    downloadFingerprintTemplate(iFoundID);
  }
  return iFoundID;
}

uint8_t downloadFingerprintTemplate(uint16_t id)
{
  uint8_t p = finger.loadModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("l'empreinte ");
      Serial.print(id);
      Serial.println(" existe");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.print("l'empreinte ");
      Serial.print(id);
      Serial.println(" n'existe pas");
      foundID = true;
      break;
    default:
      Serial.print("l'empreinte ");
      Serial.print(id);
      Serial.println(" n'existe pas");
      foundID = true;
      break;
  }
}
