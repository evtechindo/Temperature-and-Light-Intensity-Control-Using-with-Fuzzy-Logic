/*--------------------------Library----------------------------------------------------------------------*/
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include "DHT.h"
#include "RTClib.h"
#include <EEPROM.h>
#include <dimmable_light.h>
#define syncPin 2
/*--------------------------Deklarasi Pin DHT11----------------------------------------------------------*/
#define pinDHT A0 //deklarasi pin DHT
#define tipeDHT DHT11 //definisi tipe DHT
/*--------------------------Deklarasi Pin Aktuator-------------------------------------------------------*/
#define pinFan 3
#define pinHeater 5
#define pinCooler 6
DimmableLight fan(pinFan);
DimmableLight heater(pinHeater);
DimmableLight cooler(pinCooler);
/*--------------------------Deklarasi Pin Tombol---------------------------------------------------------*/
#define pinMenu 4
#define pinUp 7
#define pinDown 8
#define pinOk 12
/*--------------------------Deklarasi Pembacaan Button---------------------------------------------------*/
#define Menu digitalRead(pinMenu)
#define Up digitalRead(pinUp)
#define Down digitalRead(pinDown)
#define Ok digitalRead(pinOk)
/*--------------------------Deklarasi Objek--------------------------------------------------------------*/
LiquidCrystal_I2C lcd(0x27, 16, 2); //objek lcd
DHT dht(pinDHT, tipeDHT); //objek dht

RTC_DS1307 rtc;
char namaHari[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
/*--------------------------Variabel Button--------------------------------------------------------------*/
int lockMenu = 0;
int lockUp = 0;
int lockDown = 0;
int lockOk = 0;
int dis;
int mode = 0;
int mode1 = 0;
int mode2 = 0;
/*--------------------------Variabel DHT-----------------------------------------------------------------*/
float temp = 0.00;
float hum = 0.00;
String kondisiSuhu;
String kondisiKelembapan;
String kondisiUmur;
/*--------------------------Variabel Status Aktuator-----------------------------------------------------*/
String sFan;
String sHeater;
String sCooler;
/*--------------------------Variabel RTC-----------------------------------------------------------------*/
int inTahun, inBulan, inTanggal, inJam, inMenit, inDetik;
int saveTahun, saveBulan, saveTanggal, saveJam, saveMenit, saveDetik;
int batasTanggal;
/*--------------------------Variabel Umur-----------------------------------------------------------------*/
int inTahun1, inBulan1, inTanggal1, inJam1, inMenit1, inDetik1;
int saveTahun1, saveBulan1, saveTanggal1, saveJam1, saveMenit1, saveDetik1;
int batasTanggal1;
/*--------------------------Variabel Umur-----------------------------------------------------------------*/
unsigned long timeAwal, timeAkhir;
unsigned int umurSekarang = 0;
unsigned int readumurSekarang = 0;
unsigned long selisih;
int tanda = 0;
int lockUmur = 0;
int fasa;
String range;
unsigned long sekarang, sebelum;
unsigned long tampilSekarang, tampilSebelum;
int address = 0;

int pwmFan;
int pwmHeater;
int pwmCooler;
int rule;

#include <UnixTime.h>

UnixTime stamp(7);  // указать GMT (3 для Москвы)
uint32_t unix;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  dht.begin();
  /*--------------------------Konfidurasi Pin IO--------------------------------------------------------------*/
  pinMode(pinMenu,  INPUT_PULLUP);
  pinMode(pinUp,    INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  pinMode(pinOk,    INPUT_PULLUP);
  pinMode(pinFan, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  pinMode(pinCooler, OUTPUT);

  Serial.print("Initializing the dimmable light class... ");
  DimmableLight::setSyncPin(syncPin);
  DimmableLight::begin();
  Serial.println("Done!");

  lcd.setCursor(1, 0);
  lcd.print("Smart  Farming");
  lcd.setCursor(3, 1);
  lcd.print("C Enggar W");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ELEKTRO  UNTIDAR");
  lcd.setCursor(3, 1);
  lcd.print("V1.0  2022");

  if (! rtc.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC GAGAL");
    while (1);
  }
  if (! rtc.isrunning()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC NOT RUNNING");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  umurSekarang = 0;
  delay(3000);
  lcd.clear();
}

void loop() {
  fungsiMillis();
  tampil();
  bacaSuhu();
  bacaRTC();
  bacaSetup();
  logicFuzzy();
}
void fungsiMillis() {
  sekarang = millis();
  if ((sekarang - sebelum > 30000) && dis == 1) {
    lcd.clear();
    tanda = 0;
    dis = 0;
    sebelum = millis();
  }

  tampilSekarang = millis();
  if ((tampilSekarang - tampilSebelum) > 5000 && dis == 0) {
    tampilSebelum = millis();
    if (tanda == 0) {
      tanda = 2;
      lcd.clear();
    } else {
      tanda = 0;
      lcd.clear();
    }
  }
}
void tampil() {
  /*--------------------------Fungsi Tampil Awal--------------------------------------------------------------*/
  if (tanda == 0) {
    dis = 0;
    lcd.setCursor(0, 0);
    lcd.print("Umur: " + String(umurSekarang) + " hari");
    lcd.setCursor(0, 1);
    lcd.print(String(temp, 2) + " C");
    lcd.setCursor(9, 1);
    lcd.print(String(hum, 2) + " %");
  }
  if (tanda == 2) {
    dis = 0;
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print(String(now.day(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.year(), DEC) + " " + String(now.hour(), DEC) + ":" + String(now.minute(), DEC));
    lcd.setCursor(0, 1);
    lcd.print(pwmFan);
    lcd.setCursor(6, 1);
    lcd.print(pwmHeater);
    lcd.setCursor(12, 1);
    lcd.print(pwmCooler);
  }
}
/*--------------------------Fungsi Pembacaan Suhu--------------------------------------------------------------*/
void bacaSuhu() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  if (temp <= 20.5) {
    kondisiSuhu = "DINGIN";
  } else if (temp >= 20.5 && temp <= 24.5) {
    kondisiSuhu = "SEJUK";
  } else if (temp >= 24.5 && temp <= 28.5) {
    kondisiSuhu = "NORMAL";
  } else if (temp >= 28.5 && temp <= 32.5) {
    kondisiSuhu = "HANGAT";
  } else if (temp >= 32.5) {
    kondisiSuhu = "PANAS";
  }

  if (hum <= 59.5) {
    kondisiKelembapan = "KERING";
  } else if (hum >= 59.5 && hum <= 70.5) {
    kondisiKelembapan = "LEMBAP";
  } else if (hum >= 70.5 && hum <= 100.0) {
    kondisiKelembapan = "BASAH";
  }

  fasaTernak();
}

/*--------------------------Fungsi Pembacaan RTC---------------------------------------------------------------*/
void bacaRTC() {
  DateTime now = rtc.now();
}

void fasaTernak() {
  if (umurSekarang <= 7) {
    kondisiUmur = "KECIL";
  } else if (umurSekarang >= 7 && umurSekarang <= 12) {
    kondisiUmur = "REMAJA";
  } else if (umurSekarang >= 12) {
    kondisiUmur = "DEWASA";
  }
}

void logicFuzzy() {
  if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "KERING" && kondisiUmur == "KECIL") {
    rule = 1;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "KERING" && kondisiUmur == "REMAJA") {
    rule = 2;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "KERING" && kondisiUmur == "DEWASA") {
    rule = 3;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "KECIL") {
    rule = 4;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "REMAJA") {
    rule = 5;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "DEWASA") {
    rule = 6;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "BASAH" && kondisiUmur == "KECIL") {
    rule = 7;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "BASAH" && kondisiUmur == "REMAJA") {
    rule = 8;
  } else if (kondisiSuhu == "DINGIN" && kondisiKelembapan == "BASAH" && kondisiUmur == "DEWASA") {
    rule == 9;
  }

  else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "KERING" && kondisiUmur == "KECIL") {
    rule = 10;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "KERING" && kondisiUmur == "REMAJA") {
    rule = 11;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "KERING" && kondisiUmur == "DEWASA") {
    rule = 12;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "KECIL") {
    rule = 13;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "REMAJA") {
    rule = 14;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "DEWASA") {
    rule = 15;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "BASAH" && kondisiUmur == "KECIL") {
    rule = 16;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "BASAH" && kondisiUmur == "REMAJA") {
    rule = 17;
  } else if (kondisiSuhu == "SEJUK" && kondisiKelembapan == "BASAH" && kondisiUmur == "DEWASA") {
    rule == 18;
  }

  else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "KERING" && kondisiUmur == "KECIL") {
    rule = 19;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "KERING" && kondisiUmur == "REMAJA") {
    rule = 20;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "KERING" && kondisiUmur == "DEWASA") {
    rule = 21;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "KECIL") {
    rule = 22;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "REMAJA") {
    rule = 23;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "DEWASA") {
    rule = 24;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "BASAH" && kondisiUmur == "KECIL") {
    rule = 25;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "BASAH" && kondisiUmur == "REMAJA") {
    rule = 26;
  } else if (kondisiSuhu == "NORMAL" && kondisiKelembapan == "BASAH" && kondisiUmur == "DEWASA") {
    rule == 27;
  }

  else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "KERING" && kondisiUmur == "KECIL") {
    rule = 28;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "KERING" && kondisiUmur == "REMAJA") {
    rule = 29;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "KERING" && kondisiUmur == "DEWASA") {
    rule = 30;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "KECIL") {
    rule = 31;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "REMAJA") {
    rule = 32;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "DEWASA") {
    rule = 33;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "BASAH" && kondisiUmur == "KECIL") {
    rule = 34;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "BASAH" && kondisiUmur == "REMAJA") {
    rule = 35;
  } else if (kondisiSuhu == "HANGAT" && kondisiKelembapan == "BASAH" && kondisiUmur == "DEWASA") {
    rule == 36;
  }

  else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "KERING" && kondisiUmur == "KECIL") {
    rule = 37;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "KERING" && kondisiUmur == "REMAJA") {
    rule = 38;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "KERING" && kondisiUmur == "DEWASA") {
    rule = 39;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "KECIL") {
    rule = 40;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "REMAJA") {
    rule = 41;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "LEMBAP" && kondisiUmur == "DEWASA") {
    rule = 42;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "BASAH" && kondisiUmur == "KECIL") {
    rule = 43;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "BASAH" && kondisiUmur == "REMAJA") {
    rule = 44;
  } else if (kondisiSuhu == "PANAS" && kondisiKelembapan == "BASAH" && kondisiUmur == "DEWASA") {
    rule == 45;
  }
  logicRule();
}

void logicRule() {
  if (rule == 1) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 2) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 3) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 4) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 5) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 6) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 7) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 8) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 9) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 10) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 11) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 12) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 13) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 14) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 15) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 16) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 17) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 18) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 19) {
    sFan = "CEPAT";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 20) {
    sFan = "CEPAT";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 21) {
    sFan = "CEPAT";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 22) {
    sFan = "PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 23) {
    sFan = "PELAN";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 24) {
    sFan = "PELAN";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 25) {
    sFan = "SANGAT PELAN";
    sHeater = "TERANG";
    sCooler = "PANAS";
  } else if (rule == 26) {
    sFan = "SANGAT PELAN";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 27) {
    sFan = "SANGAT PELAN";
    sHeater = "GELAP";
    sCooler = "PANAS";
  } else if (rule == 28) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 29) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 30) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 31) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 32) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 33) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 34) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 35) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 36) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 37) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 38) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 39) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 40) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 41) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 42) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 43) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 44) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  } else if (rule == 45) {
    sFan = "SANGAT CEPAT";
    sHeater = "GELAP";
    sCooler = "DINGIN";
  }
  logicFan();
  logicHeater();
  logicCooler();
}

void logicFan() {
  if (sFan == "SANGAT PELAN") {
    pwmFan = 77;
  } else if (sFan == "PELAN") {
    pwmFan = 103;
  } else if (sFan == "NORMAL") {
    pwmFan = 154;
  } else if (sFan == "CEPAT") {
    pwmFan = 205;
  } else if (sFan == "SANGAT CEPAT") {
    pwmFan = 255;
  }
  fan.setBrightness(pwmFan);
  delay(100);
}

void logicHeater() {
  if (sHeater == "GELAP") {
    pwmHeater = 128;
  } else if (sHeater == "TERANG") {
    pwmHeater = 255;
  }
  heater.setBrightness(pwmHeater);
  delay(100);
}

void logicCooler() {
  if (sCooler == "PANAS") {
    pwmCooler = 128;
  } else if (sCooler == "DINGIN") {
    pwmCooler = 255;
  }
  cooler.setBrightness(pwmCooler);
  delay(100);
}

/*--------------------------Fungsi Setup Menu---------------------------------------------------------------*/
void bacaSetup() {
  if (Menu == 0 && lockMenu == 0) {
    lockMenu = 1;
  }
  if (Menu != 0 && lockMenu == 1) {
      lockMenu = 0;
      dis = 1;
      tanda = 1;
      lcd.clear();
    }
  /*--------------------------Tampilan Awal Menu-------------------------------------------------------------*/
  if (dis == 1) {
    sekarang = millis();
    lcd.setCursor(1, 0);
    lcd.print("DHT");
    lcd.setCursor(1, 1);
    lcd.print("OUT");
    lcd.setCursor(9, 0);
    lcd.print("RTC");
    lcd.setCursor(9, 1);
    lcd.print("UMUR");

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      mode++;
      if (mode > 3) {
        mode = 0;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      mode--;
      if (mode < 0) {
        mode = 3;
      }
    }

    switch (mode) {
      case 0 : lcd.setCursor(0, 0); lcd.print(">"); break;
      case 1 : lcd.setCursor(0, 1); lcd.print(">"); break;
      case 2 : lcd.setCursor(8, 0); lcd.print(">"); break;
      case 3 : lcd.setCursor(8, 1); lcd.print(">"); break;
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1 && mode == 0) { //masuk ke menu DHT
      lockOk = 0;
      dis = 2;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode == 1) { //masuk ke menu OUTPUT
      lockOk = 0;
      dis = 3;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode == 2) { //masuk ke menu RTC
      lockOk = 0;
      dis = 4;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode == 3) { //masuk ke menu UMUR
      lockOk = 0;
      dis = 11;
      lcd.clear();
    }
  }
  /*--------------------------Menu DHT-------------------------------------------------------------*/
  if (dis == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temp, 2) + " C");
    lcd.setCursor(0, 1);
    lcd.print("Hum : " + String(hum, 2) + " %");

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      dis = 1;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 2) {
      lcd.clear();
      lockMenu = 0;
      dis = 1;
    }
  }
  /*--------------------------Menu OUTPUT-------------------------------------------------------------*/
  if (dis == 3) {
    lcd.setCursor(0, 0);
    lcd.print("T:" + String(temp, 2) + " C");
    lcd.setCursor(10, 0);
    lcd.print("H:" + String(hum, 2) + " %");
    lcd.setCursor(0, 1);
    lcd.print("D:" + String(umurSekarang));
    lcd.setCursor(10, 1);
    lcd.print("R:" + String(rule));

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      dis = 1;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 3) {
      lcd.clear();
      lockMenu = 0;
      dis = 1;
    }
  }
  if (dis == 4) {
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print(String(now.day(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.year(), DEC) + " " + String(now.hour(), DEC) + ":" + String(now.minute(), DEC));
    lcd.setCursor(0, 1);
    lcd.print(">Set Waktu");

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) { //masuk ke setup waktu
      lockOk = 0;
      DateTime now = rtc.now();
      saveJam = now.hour();
      saveMenit = now.minute();
      saveTahun = now.year();
      saveBulan = now.month();
      saveTanggal = now.day();
      dis = 5;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 4) {
      lcd.clear();
      lockMenu = 0;
      dis = 1;
    }
  }
  /*--------------------------Set Menu RTC-------------------------------------------------------------*/
  if (dis == 5) {
    DateTime now = rtc.now();
    lcd.setCursor(1, 0);
    lcd.print("j:" + String(saveJam, DEC));
    lcd.setCursor(12, 0);
    lcd.print("m:" + String(saveMenit, DEC));
    lcd.setCursor(1, 1);
    lcd.print(String(saveTahun, DEC));
    lcd.setCursor(8, 1);
    lcd.print(String(saveBulan, DEC));
    lcd.setCursor(13, 1);
    lcd.print(String(saveTanggal, DEC));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      mode1++;
      if (mode1 > 4) {
        mode1 = 0;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      mode1--;
      if (mode1 < 0) {
        mode1 = 4;
      }
    }

    switch (mode1) {
      case 0 : lcd.setCursor(0, 0); lcd.print(">"); break;
      case 1 : lcd.setCursor(11, 0); lcd.print(">"); break;
      case 2 : lcd.setCursor(0, 1); lcd.print(">"); break;
      case 3 : lcd.setCursor(7, 1); lcd.print(">"); break;
      case 4 : lcd.setCursor(12, 1); lcd.print(">"); break;
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1 && mode1 == 0) { //masuk ke setup jam
      lockOk = 0;
      DateTime now = rtc.now();
      inJam = now.hour();
      dis = 6;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode1 == 1) { //masuk ke setup menit
      lockOk = 0;
      DateTime now = rtc.now();
      inMenit = now.minute();
      dis = 7;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode1 == 2) { //masuk ke setup tahun
      lockOk = 0;
      DateTime now = rtc.now();
      inTahun = now.year();
      dis = 8;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode1 == 3) { //masuk ke setup bulan
      lockOk = 0;
      DateTime now = rtc.now();
      inBulan = now.month();
      dis = 9;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode1 == 4) { //masuk ke setup tanggal
      lockOk = 0;
      DateTime now = rtc.now();
      inTanggal = now.day();
      dis = 10;
      lcd.clear();
    }

    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 5) {
      lcd.clear();
      lockMenu = 0;
      dis = 4;
    }
  }
  if (dis == 6) {
    lcd.setCursor(0, 0);
    lcd.print("Jam: " + String(inJam));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inJam++;
      if (inJam > 60) {
        inJam = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inJam--;
      if (inJam < 1) {
        inJam = 60;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveJam = inJam;
      mode1 = 1;
      dis = 5;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 6) {
      lcd.clear();
      lockMenu = 0;
      dis = 5;
    }
  }
  /*--------------------------Sub Menu Setup Menit-------------------------------------------------------------*/
  if (dis == 7) {
    lcd.setCursor(0, 0);
    lcd.print("Menit: " + String(inMenit));
    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inMenit++;
      if (inMenit > 60) {
        inMenit = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inMenit--;
      if (inMenit < 1) {
        inMenit = 60;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveMenit = inMenit;
      mode1 = 2;
      dis = 5;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 7) {
      lcd.clear();
      lockMenu = 0;
      dis = 5;
    }
  }
  /*--------------------------Sub Menu Setup Tahun-------------------------------------------------------------*/
  if (dis == 8) {
    lcd.setCursor(0, 0);
    lcd.print("Tahun: " + String(inTahun));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inTahun++;
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inTahun--;
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveTahun = inTahun;
      mode1 = 3;
      dis = 5;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 8) {
      lcd.clear();
      lockMenu = 0;
      dis = 5;
    }
  }
  /*--------------------------Sub Menu Setup Bulan-------------------------------------------------------------*/
  if (dis == 9) {
    lcd.setCursor(0, 0);
    lcd.print("Bulan: " + String(inBulan));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inBulan++;
      if (inBulan > 12) {
        inBulan = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inBulan--;
      if (inBulan < 1) {
        inBulan = 12;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveBulan = inBulan;
      mode1 = 4;
      dis = 5;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 9) {
      lcd.clear();
      lockMenu = 0;
      dis = 5;
    }
  }
  /*--------------------------Sub Menu Setup Tanggal-------------------------------------------------------------*/
  if (dis == 10) {
    if (saveBulan == 1 || saveBulan == 3 || saveBulan == 5 || saveBulan == 7 || saveBulan == 8 || saveBulan == 10 || saveBulan == 12) {
      batasTanggal = 31;
    } else if (saveBulan == 4 || saveBulan == 6 || saveBulan == 9 || saveBulan == 11) {
      batasTanggal = 30;
    } else if (saveBulan == 2) {
      batasTanggal = 28;
    }

    lcd.setCursor(0, 0);
    lcd.print("Tanggal: " + String(inTanggal));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inTanggal++;
      if (inTanggal > batasTanggal) {
        inTanggal = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inTanggal--;
      if (inTanggal < 1) {
        inTanggal = batasTanggal;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveTanggal = inTanggal;
      saveDetik = 0;
      rtc.adjust(DateTime(saveTahun, saveBulan, saveTanggal, saveJam, saveMenit, saveDetik));
      mode1 = 0;
      dis = 4;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 10) {
      lcd.clear();
      lockMenu = 0;
      dis = 5;
    }
  }
  /*--------------------------Menu Umur-------------------------------------------------------------*/
  if (dis == 11) {
    if (lockUmur == 0) {
      umurSekarang = 0;
    }
    if (lockUmur == 1) {
      DateTime now = rtc.now();
      timeAkhir = now.unixtime();
      selisih = timeAkhir - timeAwal;
      umurSekarang = selisih / 86400;
    }


    lcd.setCursor(0, 0);
    lcd.print("D:" + String(umurSekarang));
    lcd.setCursor(5, 0);
    lcd.print("Range:" + String(range));
    lcd.setCursor(0, 1);
    lcd.print(">Mulai ternak");

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) { //masuk ke setup start tanam
      lockOk = 0;
      DateTime now = rtc.now();
      saveJam1 = now.hour();
      saveMenit1 = now.minute();
      saveTahun1 = now.year();
      saveBulan1 = now.month();
      saveTanggal1 = now.day();
      dis = 12;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 11) {
      lcd.clear();
      lockMenu = 0;
      dis = 1;
    }
  }
  if (dis == 12) {
    DateTime now = rtc.now();
    lcd.setCursor(1, 0);
    lcd.print("j:" + String(saveJam1, DEC));
    lcd.setCursor(12, 0);
    lcd.print("m:" + String(saveMenit1, DEC));
    lcd.setCursor(1, 1);
    lcd.print(saveTahun1, DEC);
    lcd.setCursor(8, 1);
    lcd.print(saveBulan1, DEC);
    lcd.setCursor(13, 1);
    lcd.print(saveTanggal1, DEC);

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      mode2++;
      if (mode2 > 4) {
        mode2 = 0;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      mode2--;
      if (mode2 < 0) {
        mode2 = 4;
      }
    }

    switch (mode2) {
      case 0 : lcd.setCursor(0, 0); lcd.print(">"); break;
      case 1 : lcd.setCursor(11, 0); lcd.print(">"); break;
      case 2 : lcd.setCursor(0, 1); lcd.print(">"); break;
      case 3 : lcd.setCursor(7, 1); lcd.print(">"); break;
      case 4 : lcd.setCursor(12, 1); lcd.print(">"); break;
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1 && mode2 == 0) { //masuk ke setup jam
      lockOk = 0;
      DateTime now = rtc.now();
      inJam1 = now.hour();
      dis = 13;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode2 == 1) { //masuk ke setup menit
      lockOk = 0;
      DateTime now = rtc.now();
      inMenit1 = now.minute();
      dis = 14;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode2 == 2) { //masuk ke setup tahun
      lockOk = 0;
      DateTime now = rtc.now();
      inTahun1 = now.year();
      dis = 15;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode2 == 3) { //masuk ke setup bulan
      lockOk = 0;
      DateTime now = rtc.now();
      inBulan1 = now.month();
      dis = 16;
      lcd.clear();
    }
    if (Ok != 0 && lockOk == 1 && mode2 == 4) { //masuk ke setup tanggal
      lockOk = 0;
      DateTime now = rtc.now();
      inTanggal1 = now.day();
      dis = 17;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 12) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }
  /*--------------------------Sub Menu Setup Jam-------------------------------------------------------------*/
  if (dis == 13) {
    lcd.setCursor(0, 0);
    lcd.print("Jam: " + String(inJam1));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inJam1++;
      if (inJam1 > 60) {
        inJam1 = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inJam1--;
      if (inJam1 < 1) {
        inJam1 = 60;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveJam1 = inJam1;
      mode2 = 1;
      dis = 12;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 13) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }
  /*--------------------------Sub Menu Setup Menit-------------------------------------------------------------*/
  if (dis == 14) {
    lcd.setCursor(0, 0);
    lcd.print("Menit: " + String(inMenit1));
    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inMenit1++;
      if (inMenit1 > 60) {
        inMenit1 = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inMenit1--;
      if (inMenit1 < 1) {
        inMenit1 = 60;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveMenit1 = inMenit1;
      mode2 = 2;
      dis = 12;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 14) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }

  /*--------------------------Sub Menu Setup Tahun-------------------------------------------------------------*/
  if (dis == 15) {
    lcd.setCursor(0, 0);
    lcd.print("Tahun: " + String(inTahun1));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inTahun1++;
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inTahun1--;
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveTahun1 = inTahun1;
      mode2 = 3;
      dis = 12;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 15) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }
  /*--------------------------Sub Menu Setup Bulan-------------------------------------------------------------*/
  if (dis == 16) {
    lcd.setCursor(0, 0);
    lcd.print("Bulan: " + String(inBulan1));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inBulan1++;
      if (inBulan1 > 12) {
        inBulan1 = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inBulan1--;
      if (inBulan1 < 1) {
        inBulan1 = 12;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveBulan1 = inBulan1;
      mode2 = 4;
      dis = 12;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 16) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }
  /*--------------------------Sub Menu Setup Tanggal-------------------------------------------------------------*/
  if (dis == 17) {
    if (inBulan1 == 1 || inBulan1 == 3 || inBulan1 == 5 || inBulan1 == 7 || inBulan1 == 8 || inBulan1 == 10 || inBulan1 == 12) {
      batasTanggal1 = 31;
    } else if (inBulan1 == 4 || inBulan1 == 6 || inBulan1 == 9 || inBulan1 == 11) {
      batasTanggal1 = 30;
    } else if (saveBulan == 2) {
      batasTanggal1 = 28;
    }

    lcd.setCursor(0, 0);
    lcd.print("Tanggal: " + String(inTanggal1));

    if (Down == 0 && lockDown == 0) {
      lockDown = 1;
    }
    if (Down != 0 && lockDown == 1) {
      lcd.clear();
      lockDown = 0;
      inTanggal1++;
      if (inTanggal1 > batasTanggal1) {
        inTanggal1 = 1;
      }
    }

    if (Up == 0 && lockUp == 0) {
      lockUp = 1;
    }
    if (Up != 0 && lockUp == 1) {
      lcd.clear();
      lockUp = 0;
      inTanggal1--;
      if (inTanggal1 < 1) {
        inTanggal1 = batasTanggal1;
      }
    }

    if (Ok == 0 && lockOk == 0) {
      lockOk = 1;
    }
    if (Ok != 0 && lockOk == 1) {
      lockOk = 0;
      saveTanggal1 = inTanggal1;
      lockUmur = 1;
      inDetik1 = 0;
      stamp.setDateTime(inTahun1, inBulan1, inTanggal1, inJam1, inMenit1, inDetik1);
      timeAwal = stamp.getUnix();
      mode2 = 0;
      dis = 11;
      lcd.clear();
    }
    if (Menu == 0 && lockMenu == 0) {
      lockMenu = 1;
    }
    if (Menu == 0 && dis == 17) {
      lcd.clear();
      lockMenu = 0;
      dis = 11;
    }
  }
}
