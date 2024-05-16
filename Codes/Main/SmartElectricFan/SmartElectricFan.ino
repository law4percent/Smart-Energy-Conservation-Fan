/*
  Humidity
  less than 30%  ==> speed1
  30% - 50%      ==> speed2
  more that 50%  ==> speed3

  Temperature
  less than 25°C ==> speed1
  more than 24°C and less than 31°C ==> speed2
  more than 30°C ==> speed3
*/

#include "DHT.h"
#include "BluetoothSerial.h"
BluetoothSerial BT;

#define Serial_Debug

// Pin Used
const byte DHTPIN = 26;
const byte PIR = 25;
const byte Speed1 = 13;
const byte Speed2 = 12;
const byte Speed3 = 14;
const byte Rotate = 27;
const byte LEDindecatorMode = 18;
const byte ReadyLEDIndicator = 19;

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float hum;
float temp;
bool pirStatus;
int countMotion = 0;
int Index = 0;
bool Spd1_status = false;
bool Spd2_status = false;
bool Spd3_status = false;
bool RT_status = false;
bool manualMode = false;
bool Awake = true;
bool pirMode = true;
bool dhtStatus = false;
bool lastRotateData;
bool ShutdownFan = false;

unsigned long previousMillis1 = 0;
const long interval_1min = 60000;  // 1 Min
unsigned long previousMillis = 0;
const long TimeInterval = 30000;

byte last_speed;
const int delay1sec = 2000;

bool startSleep = true;
unsigned long StartToCount_Sleep;
unsigned long EndSleep;

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIR, INPUT);
  pinMode(Speed1, OUTPUT);
  pinMode(Speed2, OUTPUT);
  pinMode(Speed3, OUTPUT);
  pinMode(Rotate, OUTPUT);
  pinMode(LEDindecatorMode, OUTPUT);
  pinMode(ReadyLEDIndicator, OUTPUT);

  digitalWrite(LEDindecatorMode, manualMode);
  digitalWrite(ReadyLEDIndicator, 0);
  BT.begin("Smart Energy Fan");

  UpdateFan();
  delay(500);
  
  Serial.println(F("ready!"));

  while (true) {
    Serial.println("No motion found!");
    if (digitalRead(PIR)) break;

    if (BT.available() > 0) {
      char readBTdata = BT.read();
      UpdateFromBT(readBTdata);
      break;
    } 
    delay(50);
  }
  Serial.println("Motion found!");

  RT_status = true;
  AdjustSpeedFan(3);
  UpdateFan();

  lastRotateData = RT_status;
  StartToCount_Sleep = millis();
}

void loop() {
  countMotion = 0;
  unsigned long currentMillis = millis();
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  pirStatus = digitalRead(PIR);
  delay(2000);

  if (BT.available() > 0) {
    char readBTdata = BT.read();
    Serial.println(readBTdata);
    bool spd = false;

    if (manualMode) {
      switch (readBTdata) {
        case '1':
          AdjustSpeedFan(1);
          spd = true;
          break;

        case '2':
          AdjustSpeedFan(2);
          spd = true;
          break;

        case '3':
          AdjustSpeedFan(3);
          spd = true;
          break;

        case '0':
          AdjustSpeedFan(0);
          RT_status = false;
          spd = true;
          break;
      }
    }

    if (spd) {
      Serial.println("=== Speed Adjusted ===\n");
    }

    UpdateFromBT(readBTdata);
    UpdateFan();
  }

  // Count every motion detected
  if (pirStatus) {
    countMotion++;
  }

  // Sleep after 30 secs when no motion was found
  if (startSleep) {
    if (currentMillis - StartToCount_Sleep >= TimeInterval) {
      StartToCount_Sleep = 0;
      startSleep = false;
      previousMillis = currentMillis;

      if (countMotion < 1 and pirMode) {
        AdjustRotate(false);
        SleepMode(false);
        AdjustSpeedFan(0);
        UpdateFan();
      }
    }
  } else {
    if (currentMillis - previousMillis >= TimeInterval) {
      if (countMotion < 1 and pirMode) {
        AdjustRotate(false);
        SleepMode(false);
        AdjustSpeedFan(0);
        UpdateFan();
      }

      previousMillis = currentMillis;
    }
  }

  // It will update the speed in every one minute if both manual mode and sleep are false
  if (currentMillis - previousMillis1 >= interval_1min) {
    if (!manualMode and Awake) {
      CheckTempHumStatus();
      UpdateFan();
    }
    TempHum_BTupdate();
    
    previousMillis1 = currentMillis;
  }

  // Once the fan is asleep, the only task will be seeking motions
  if (countMotion > 0 and !Awake) {
    if (!manualMode)
      CheckTempHumStatus();
    
    AdjustRotate(lastRotateData);
    SleepMode(false);
    UpdateFan();
    countMotion = 0;
  }

#ifdef Serial_Debug
  printAllComponentStatus();
#endif
}

void UpdateFromBT(char readBTdata) {
  switch (readBTdata) {
    case 'q':
      RT_status = true;
      break;

    case 'w':
      RT_status = false;
      break;

    case 'e':
      manualMode = false; 
      break;

    case 'r':
      manualMode = true;
      break;

    case 't':
        pirMode = true;
      break;

    case 'y':
      pirMode = false;
      Awake = true;
      break;

    case 'u':
      ShutdownFan = true;
      AdjustSpeedFan(0);
      break;

    case 'i':
      ShutdownFan = false;
      break;
  
    case 'o':
      TempHum_BTupdate();
      delay(3000);
      SendFanStatus();
      break;
  }
}

void SendFanStatus() {
  Serial.print(F("Sending..."));
  const int delay2sec = 2000;
  if (Spd1_status) {
    BT.println('a');
  } else if (Spd2_status) {
    BT.println('b');
  } else if (Spd3_status) {
    BT.println('c');
  } else {
    BT.println('d');
  }
  delay(delay2sec);

  if (RT_status) {
    BT.println('e');
  } else {
    BT.println('f');
  }
  delay(delay2sec);

  if (manualMode) {
    BT.println('g');
  } else {
    BT.println('h');
  }
  delay(delay2sec);

  if (pirMode) {
    BT.println('i');
  } else {
    BT.println('j');
  }
  delay(delay2sec);

  if (Awake) {
    BT.println('k');
  } else {
    BT.println('l');
  }
  delay(delay2sec);

  if (!dhtStatus) {
    BT.println('m');
  }
  delay(delay2sec);

  if (ShutdownFan) {
    BT.println('n');
  } else {
    BT.println('o');
  }
  delay(delay2sec);

  BT.println('A');
  delay(delay2sec);
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
    Serial.println(F("DHT failed, need to fix!"));
    BT.println("m");
  } else {
    String sendHumTemp = "";
    dhtStatus = true;
    sendHumTemp += String(int(hum));
    sendHumTemp += "-";
    sendHumTemp += String(int(temp));
    BT.println(sendHumTemp);
    Serial.println(sendHumTemp);
  }
  delay(delay1sec);
}

void AdjustRotate(bool rotate) {
  lastRotateData = rotate;
  if (rotate) {
    RT_status = true;
  } else {
    RT_status = false;
  }
  delay(delay1sec);
}

void AdjustSpeedFan(const byte speed) {
  last_speed = speed;
  switch (speed) {
    case 0:
      Spd1_status = 0;
      Spd2_status = 0;
      Spd3_status = 0;
      RT_status = false;

      BT.println('d');
      break;

    case 1:
      Spd1_status = 1;
      Spd2_status = 0;
      Spd3_status = 0;

      BT.println('a');
      break;

    case 2:
      Spd1_status = 0;
      Spd2_status = 1;
      Spd3_status = 0;
      
      BT.println('b');
      break;

    case 3:
      Spd1_status = 0;
      Spd2_status = 0;
      Spd3_status = 1;
      
      BT.println('c');
      break;
  }
  delay(delay1sec);
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

  if (vtemp < 25) {
    RT_status = lastRotateData;
    speed = 1;
  } else if (vtemp > 30) {
    RT_status = lastRotateData;
    speed = 3;
  } else {
    RT_status = lastRotateData;
    speed = 2;
  }

  AdjustSpeedFan(speed);
}

void UpdateFan() {
  digitalWrite(Speed1, Spd1_status);
  digitalWrite(Speed2, Spd2_status);
  digitalWrite(Speed3, Spd3_status);
  digitalWrite(Rotate, RT_status);
  digitalWrite(LEDindecatorMode, manualMode);
}

void SleepMode(bool sleep) {
  if (sleep) {
    Awake = true;
    BT.println("k");
  } else {
    Awake = false;
    BT.println("l");
  }
  delay(delay1sec);
}