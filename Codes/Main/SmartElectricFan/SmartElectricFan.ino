/*
  Humidity
  less than 20%  ==> speed1
  less than 30%  ==> speed1
  30% - 50%      ==> speed2
  more that 50%  ==> speed3

  Temperature
  less than 15°C ==> speed1
  less than 20°C ==> speed1
  20°C - 24°C    ==> speed2
  more than 24°C ==> speed3
*/

#include "DHT.h"
#include "BluetoothSerial.h"
BluetoothSerial BT;

// #define Serial_Debug

// Pin Used
const byte DHTPIN = 26;
const byte PIR = 25;
const byte Speed1 = 13;
const byte Speed2 = 12;
const byte Speed3 = 14;
const byte Rotate = 27;
const byte LEDindecatorMode = 18;
const byte ReadyLEDIndicator = 19;
// const byte LEDindecatorDHT11 = 23;
// const byte LEDindecatorPIR = 21;

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float hum;
float temp;
bool pirStatus;
int countMotion = 0;
int Index = 0;
bool Spd1_status = true;
bool Spd2_status = false;
bool Spd3_status = false;
bool RT_status = true;
bool manualMode = false;
bool Awake = true;
bool pirMode = true;
bool dhtStatus = false;
bool lastRotateData;
bool ShutdownFan = false;

unsigned long previousMillis1 = 0;
const long interval1 = 60000;  // 1 Min
unsigned long previousMillis = 0;
const long TimeInterval = 60000;

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(5000);

  lastRotateData = RT_status;

  pinMode(PIR, INPUT);
  pinMode(Speed1, OUTPUT);
  pinMode(Speed2, OUTPUT);
  pinMode(Speed3, OUTPUT);
  pinMode(Rotate, OUTPUT);
  pinMode(LEDindecatorMode, OUTPUT);
  pinMode(ReadyLEDIndicator, OUTPUT);
  // pinMode(LEDindecatorDHT11, OUTPUT);
  // pinMode(LEDindecatorPIR, OUTPUT);

  digitalWrite(LEDindecatorMode, manualMode);
  digitalWrite(ReadyLEDIndicator, 0);
  // digitalWrite(LEDindecatorPIR, pirMode);
  // digitalWrite(LEDindecatorDHT11, dhtStatus);

  Serial.print(F("Preparing"));
  for (int cnt = 0; cnt < 10; cnt++) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(F("ready!"));
  delay(500);

  BT.begin("Smart Energy Fan");
  digitalWrite(ReadyLEDIndicator, 1);
  delay(500);

  digitalWrite(Speed1, Spd1_status);
  digitalWrite(Speed2, Spd2_status);
  digitalWrite(Speed3, Spd3_status);
  digitalWrite(Rotate, RT_status);
}

void loop() {

  delay(2000);

  countMotion = 0;
  unsigned long currentMillis = millis();
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  pirStatus = digitalRead(PIR);


  String BTdata = "";
  bool rData = false;

  while (BT.available() > 0) {
    char readBTdata = BT.read();
    if (readBTdata != '\n' or readBTdata != '\r')
      BTdata += readBTdata;
    else
      break;
    rData = true;
  }

  if (rData) {
    BTdata.trim();
  }

  if (BTdata.length() >= 2) {
    Serial.println(BTdata);
    bool spd = false;
    if (manualMode) {
      if (BTdata.equals("s1")) {
        AdjustSpeedFan(1);
        spd = true;
      } else if (BTdata.equals("s2")) {
        AdjustSpeedFan(2);
        spd = true;
      } else if (BTdata.equals("s3")) {
        AdjustSpeedFan(3);
        spd = true;
      } else if (BTdata.equals("s0")) {
        AdjustSpeedFan(0);
        RT_status = false;
        spd = true;
      }
    }

    if (spd) {
      Serial.println("=== Speed Adjusted ===");
      Serial.println();
    }

    if (BTdata.equals("r1")) {
      RT_status = true;
    } else if (BTdata.equals("r0")) {
      RT_status = false;
    } else if (BTdata.equals("m1")) {
      manualMode = true;
    } else if (BTdata.equals("m0")) {
      manualMode = false;
    } else if (BTdata.equals("p1")) {
      pirMode = true;
    } else if (BTdata.equals("p0")) {
      pirMode = false;
      Awake = true;
    } else if (BTdata.equals("sht1")) {
      ShutdownFan = true;
      AdjustSpeedFan(0);
    } else if (BTdata.equals("sht0")) {
      ShutdownFan = false;
    } else if (BTdata.equals("st")) {
      TempHum_BTupdate();
      delay(3000);
      SendFanStatus();
    }
    UpdateFan();
  }

  // It will update the speed in every one minute if both manual mode and sleep are false
  if (currentMillis - previousMillis1 >= interval1) {
    if (!manualMode and Awake and !ShutdownFan) {
      CheckTempHumStatus();
      UpdateFan();
    }
    
    if (!ShutdownFan and pirMode) {
      Awake = false;
    } else {
      Awake = true;
    }

    if (!ShutdownFan) {
      TempHum_BTupdate();
      delay(1000);
    }
    previousMillis1 = currentMillis;
  }

  // Count every motion detected
  if (pirStatus) {
    countMotion++;
  }

  // Check updates kun naa pabay naglihok 1 min
  if (currentMillis - previousMillis >= TimeInterval) {
    if (countMotion == 0 and pirMode and !ShutdownFan) {
      lastRotateData = RT_status;
      Awake = false;
      AdjustSpeedFan(0);
      UpdateFan();
      BT.println('l');
      delay(3000);
    }
    previousMillis = currentMillis;
  }

  // Once the fan is asleep, the only task will be seeking motions
  if (countMotion > 0 and !ShutdownFan and !Awake) {
    Serial.println("Wake up!");
    if (!manualMode)
      CheckTempHumStatus();
    RT_status = lastRotateData;
    Awake = true;
    UpdateFan();
    countMotion = 0;
  }
#ifdef Serial_Debug
  printAllComponentStatus();
#endif
}

void SendFanStatus() {
  Serial.print(F("Sending..."));
  if (Spd1_status) {
    BT.println('a');
  } else if (Spd2_status) {
    BT.println('b');
  } else if (Spd3_status) {
    BT.println('c');
  } else {
    BT.println('d');
  }
  delay(3000);

  if (RT_status) {
    BT.println('e');
  } else {
    BT.println('f');
  }
  delay(3000);

  if (manualMode) {
    BT.println('g');
  } else {
    BT.println('h');
  }
  delay(3000);

  if (pirMode) {
    BT.println('i');
  } else {
    BT.println('j');
  }
  delay(3000);

  if (Awake) {
    BT.println('k');
  } else {
    BT.println('l');
  }
  delay(3000);

  if (!dhtStatus) {
    BT.println('m');
  }
  delay(3000);

  if (ShutdownFan) {
    BT.println('n');
  } else {
    BT.println('o');
  }
  delay(3000);

  BT.println('A');
  delay(2000);
  Serial.println(F("done!"));
}

void printAllComponentStatus() {
#ifdef Serial_Debug
  Serial.print(F("Speed: "));
  Serial.print(Spd1_status);
  Serial.print(Spd2_status);
  Serial.println(Spd3_status);
  Serial.print(F("Rotation: "));
  Serial.println(RT_status);
  Serial.print(F("Awake: "));
  Serial.println(Awake);
  Serial.print(F("Counted Motions: "));
  Serial.println(countMotion);
  Serial.print(F("Manual: "));
  Serial.println(manualMode);

  Serial.print(F("PIR: "));
  Serial.println(pirMode);
  Serial.print(F("Temp: "));
  Serial.print(int(temp));
  Serial.print(F(" Humd: "));
  Serial.println(int(hum));
  Serial.println();
#endif
}

void TempHum_BTupdate() {
  if (isnan(hum) || isnan(temp)) {
    dhtStatus = false;
    // digitalWrite(LEDindecatorDHT11, dhtStatus);
    Serial.println(F("DHT failed, need to fix!"));
    BT.println("m");
  } else {
    String sendHumTemp = "";
    dhtStatus = true;
    // digitalWrite(LEDindecatorDHT11, dhtStatus);
    sendHumTemp += String(int(hum));
    sendHumTemp += "-";
    sendHumTemp += String(int(temp));
    BT.println(sendHumTemp);
    Serial.println(sendHumTemp);
  }
}

void AdjustSpeedFan(const byte speed) {
  switch (speed) {
    case 0:
      Spd1_status = 0;
      Spd2_status = 0;
      Spd3_status = 0;
      RT_status = false;
      break;
    case 1:
      Spd1_status = 1;
      Spd2_status = 0;
      Spd3_status = 0;
      break;
    case 2:
      Spd1_status = 0;
      Spd2_status = 1;
      Spd3_status = 0;
      break;
    case 3:
      Spd1_status = 0;
      Spd2_status = 0;
      Spd3_status = 1;
      break;
  }
}

void CheckTempHumStatus() {
  
  const int vtemp = int(temp);
  const int vhum = int(hum);
  byte speed;

  if (vhum < 30) {
    RT_status = lastRotateData;
    speed = 1;
  } else if (vhum > 50) {
    RT_status = lastRotateData;
    speed = 3;
  } else {
    RT_status = lastRotateData;
    speed = 2;
  }

  if (vtemp < 20) {
    RT_status = lastRotateData;
    speed = 1;
  } else if (vtemp > 24) {
    RT_status = lastRotateData;
    speed = 3;
  } else {
    RT_status = lastRotateData;
    speed = 2;
  }

  AdjustSpeedFan(speed);
  switch (speed) {
    case 1:
      BT.println("a");
      break;
    case 2:
      BT.println("b");
      break;
    case 3:
      BT.println("c");
      break;
  }
}

void UpdateFan() {
  digitalWrite(Speed1, Spd1_status);
  digitalWrite(Speed2, Spd2_status);
  digitalWrite(Speed3, Spd3_status);
  digitalWrite(Rotate, RT_status);
  digitalWrite(LEDindecatorMode, manualMode);
  // digitalWrite(LEDindecatorPIR, pirMode);
}