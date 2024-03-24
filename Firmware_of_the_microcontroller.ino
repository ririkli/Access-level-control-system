#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>                           // Подключаем библиотеку Wire                           
#include <TimeLib.h>                        // Подключаем библиотеку TimeLib
#include <DS1307RTC.h>                      // Подключаем библиотеку DS1307RTC
#include <SD.h>

File myFile;

#define SS_PIN 10
#define RST_PIN 9
#define RED_LED_PIN 7
#define GREEN_LED_PIN 8
#define YELLOW_LED_PIN 3
#define RELE_PIN 4
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

tmElements_t tm; 

void setup() { 
  Serial.begin(115200);
 // SetConsoleOutputCP(CP_UTF8);
  SPI.begin(); // Init SPI bus
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  
  pinMode(RELE_PIN, OUTPUT);

  pinMode(5, OUTPUT);
  pinMode(RST_PIN, OUTPUT);

  digitalWrite(RELE_PIN, HIGH);

  Serial.print("Initializing SD card...");
  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    digitalWrite(YELLOW_LED_PIN, HIGH);
    while (1);
  }else{
    blink(GREEN_LED_PIN);
  }

  digitalWrite(RELE_PIN, LOW);
  digitalWrite(5, HIGH); 
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(5, LOW);
  
  rfid.PCD_Init(); // Init MFRC522 

  rfid.PCD_SetAntennaGain(rfid.RxGain_max);  // Установка усиления антенны
  rfid.PCD_AntennaOff();           // Перезагружаем антенну
  rfid.PCD_AntennaOn();            // Включаем антенну

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.println(F("Using the following key"));
  blink(GREEN_LED_PIN);
   
}
 
void loop() {

  static uint32_t rebootTimer = millis(); // Важный костыль против зависания модуля!
  if (millis() - rebootTimer >= 1000) {   // Таймер с периодом 1000 мс
    rebootTimer = millis();               // Обновляем таймер
    digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
    delayMicroseconds(2);                 // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
    rfid.PCD_Init();   
  }


  if (Serial.available() > 0) {
    String data = Serial.readString();
    if(data[0] == 'c'){
      bool parse=false;
      bool config=false;
      String cdate = "", ctime = "";

      for(int i = 2; i < 10; i++){
        ctime += data[i];
      }
      for(int i = 11; i < 21; i++){
        cdate += data[i];
      }
 
      // get the date and time the compiler was run
      if (getDate(cdate.c_str()) && getTime(ctime.c_str())) {
        parse = true;
        // and configure the RTC with this info
        if (RTC.write(tm)) {
          config = true;
        }
      }

      if (parse && config) {
        Serial.print("DS1307 configured Time=");
        Serial.print(ctime);
        Serial.print(", Date=");
        Serial.println(cdate);
      } else if (parse) {
        Serial.println("DS1307 Communication Error :-{");
        Serial.println("Please check your circuitry");
      } else {
        Serial.print("Could not parse info from the compiler, Time=\"");
        Serial.print(ctime);
        Serial.print("\", Date=\"");
        Serial.print(cdate);
        Serial.println("\"");
      }
    
    }
  }


  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;


  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    blink(YELLOW_LED_PIN);
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    String cardUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      cardUID += String(rfid.uid.uidByte[i], HEX);
    }
    Serial.print(cardUID);
    Serial.println();

    uint32_t pos = 0;
    bool access = false;

    digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
    digitalWrite(5, HIGH); 
    delayMicroseconds(2);                 // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
    digitalWrite(5, LOW);
    rfid.PCD_Init(); 
    
    digitalWrite(RELE_PIN, HIGH);
    
    Serial.print("Initializing SD card...");
    if (!SD.begin(5)) {
      Serial.println("initialization failed!");
      digitalWrite(YELLOW_LED_PIN, HIGH);
      while (1);
    }
    Serial.println("initialization done.");
    RTC.read(tm);
    if(tm.Hour < 11){
      myFile = SD.open("breakfast.txt");
    }else{
      myFile = SD.open("dinner.txt"); 
    }
    if (myFile) {
      String date = "";
      int st = 0;
      date += String(tmYearToCalendar(tm.Year)) + '-';
      if(tm.Month > 9){
        date += String(tm.Month);
      }else{
        date += "0" + String(tm.Month);
      }
      date += '-';
      if(tm.Day > 9){
        date += String(tm.Day);
      }else{
        date += "0" + String(tm.Day);
      }
      Serial.println(date);
      Serial.println(tm.Hour);
      Serial.println("hour");
      char buff = myFile.read();
      while (buff != 0x0d){
        int i = 0;
        while(buff == date[i]){
          buff = myFile.read(); 
          i++;
        }
        if(i != 10){
          buff = myFile.read(); 
          while(buff != '&'){
            buff = myFile.read(); 
          }
          st++;
          buff = myFile.read(); 
        }else{
          buff = myFile.read(); 
          while(buff != 0x0d){
            buff = myFile.read(); 
          }
        }
      }
      myFile.read(); 

      Serial.write("Stolbez: ");
      Serial.println(st);
      buff = myFile.read();
      while (myFile.available()){
        int i = 0; 
        while(buff == cardUID[i]){
          buff = myFile.read(); 
          i++;
        }
        //buff = myFile.read(); 
        if(buff != '&'){
          buff = myFile.read(); 
          while(buff != 0x0d && myFile.available()){
            buff = myFile.read(); 
          }
          buff = myFile.read(); 
        }else{
          buff = myFile.read(); 
          while (int(buff) != int('&') && myFile.available()) {
            Serial.write(buff);
            buff = myFile.read(); 
          }
          Serial.println(myFile.position());
          for(int q = 0; q <= (st-2)*2; q++){
            buff = myFile.read(); 
          }
          pos = myFile.position();
          Serial.write("Access: ");
          Serial.println(buff);
          if(buff == '1'){
            blink(GREEN_LED_PIN);
            access = true;
          }else{
            blink(RED_LED_PIN);
          }
          break;
        }
        if(myFile.available()){
          buff = myFile.read();
        }else{
          blink(RED_LED_PIN);
        }
      }
    }else{
      Serial.write("File is not avalible");
    }

    myFile.close();
    if(access){
      if(tm.Hour < 11){
        myFile = SD.open("breakfast.txt", O_RDWR);
      }else{
        myFile = SD.open("dinner.txt", O_RDWR); 
      }

      Serial.println(pos);
      if (myFile) {
        myFile.seek(pos-1);
        myFile.write('-'); 
      }
  
      myFile.close();
      Serial.println("All done");
    }

    digitalWrite(RELE_PIN, LOW);
    
    
    
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

bool getTime(const char *str)
{
  int Hour, Min, Sec;
 
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}
 
bool getDate(const char *str)
{
  int Day, Year, Month;
 
  if (sscanf(str, "%d.%d.%d", &Day, &Month, &Year) != 3) return false;
  tm.Day = Day;
  tm.Month = Month;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void blink(int pin){
  digitalWrite(pin, HIGH);
  delay(1000);
  digitalWrite(pin, LOW);
}
