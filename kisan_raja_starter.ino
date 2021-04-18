#include<SoftwareSerial.h>
#include <EEPROM.h>

//#define MANU A2

#define REL1 9
#define REL2 10

//#define FACT A1

//#define motoron A4
//#define motorof A3
//#define motortm A5

String deviceID = "12345";
int registered = 0;
int owners = 0;
long interval = 0;
  
int preModeStatus  = 0;    int modeStatus  = 0;       String switchMessage = "auto";
int prePhaseStatus = 0;   int phaseStatus = 0;      String phaseMessage  = "absent";
int preMotorStatus = 0;   int motorStatus = 0;      String motorMessage  = "off";
int preScheduleOn  = 0;    int scheduleOn  = 0;       String scheduleMessage = "off";

int isPhasePresent = 0;

int totalOwnerNumbers = 0;

int ownerAddressEEPROM = 50;

boolean callNotEnded = true;
unsigned long timerNewTime = 0;
unsigned long callDefaultDuration = 30000;
int interruptFromClock = 0;
unsigned long timerOn = 0;
String Corpora = "";
String p[6] = "";
int updatedValues = 0;

int o1=0;

SoftwareSerial sim(5, 6);

void setup() {
//  pinMode(motoron, OUTPUT);
//  pinMode(motorof, OUTPUT);
//  pinMode(motortm, OUTPUT);
//
//  digitalWrite(motorof, HIGH);
//  digitalWrite(motoron, LOW);
//  digitalWrite(motortm, LOW);
  
  initAllMachine();
  Serial.print("Total ownernumbers : ");
  Serial.println(totalOwnerNumbers);
  findIfOwnerExist();
  Serial.println("started kisan motor starter");
  //captureStatus();
  //Inform();

  readAllContacts();
}
void loop()
{
  
  manageSchedule();
  waitForOtherOwner();
  
//  int h = digitalRead(MANU);
//  int o = digitalRead(FACT);


    scheduleOn = 0;
    isPhasePresent = 0;
  ///////////////////////////////////////////////////////////////////////

//  if (o == 0) {
//    factoryResetIt();
//  }
//  if (h == 1) {
    manageAutoOperation();
    Serial.print('>');
//  }
//  else {
//    manageManualMode();
//    Serial.print('<');
//  }
}

void initAllMachine() {
  sim.begin(9600);
  Serial.begin(9600);

//  pinMode(MANU, INPUT);
//  pinMode(FACT, INPUT);

  pinMode(REL1, OUTPUT);
  pinMode(REL2, OUTPUT);  
  digitalWrite(REL1, HIGH);
  
  delay(5000);
  
  sendAndWait("AT+CSMP=17,71,0,0\r\n");
  sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n");
  sendAndWait("AT+CLIP=1\r\n");
  sendAndWait("AT+CMGF=1\r\n");
  sendAndWait("AT+CSMP=17,167,0,16\r\n");
  
  totalOwnerNumbers = EEPROM.read(ownerAddressEEPROM);

  for ( int ji = 0; ji < 5; ji++) {
    
  }
}
void findIfOwnerExist() {

  String N = "";
  if (!readDirectOwnerAt(1).equals("AXI")) {
    N = readDirectPhoneNumberAt(1);
    Serial.println('[' + N + ']');
    if (N.equals("0000000000000")) {
      owners = 0;
      Serial.println("No Owner");
    } else {
      owners = 1;
      Serial.println("Owner");

    }
    Serial.println("all owner=" + String(owners));
    if (owners == 0) {
      while (registered == 0)
      {
        waitForOwner();
        N = readDirectPhoneNumberAt(1);
        if (N.equals("") || strlen(N.c_str()) < 10) {
          N = readDirectPhoneNumberAt(1);
        }
        if (N.equals("") || strlen(N.c_str()) < 10) {
          N = readDirectPhoneNumberAt(1);
        }
        if (N.equals("") || strlen(N.c_str()) < 10) {
          N = readDirectPhoneNumberAt(1);
        }
      }
    }
  }
  p[0] = N;
}


String sendAndWait(String cmd) {
  const char* com = cmd.c_str();
  Serial.println("[IN]:" + cmd);
  while (sim.available() > 0) {
    sim.read();
  }
  sim.write(com);
  sim.readStringUntil('\n');
  String res = sim.readStringUntil('\n');

  while (sim.available() > 0) {
    sim.read();
  }
  delay(150);
  Serial.println("[OU]" + res);
  delay(500);
  return res;
}

void waitForOwner() {
  Serial.println("WAIT");
  while (!sim.find("SM")) {
    Serial.print('.');
    // digitalWrite(MANL, HIGH); delay(5); digitalWrite(MANL, LOW); delay(5);
  }
  int messNumber = sim.parseInt();
  String SMD = sendAndWait("AT+CMGR=" + String(messNumber) + "\r\n");
  String message = sim.readStringUntil('\n');
  String senderM = SMD.substring(24, 34);
  Serial.println("{" + senderM);
  Serial.println("{" + message);
  String One = message.substring(1, 6);
  String Two = message.substring(7, 12);
  String Three = message.substring(13, 23);
  Serial.println(One + " " + Two + " " + Three);
  if (One.equals("kisan") ) {
    if (deviceID.equals(Two)) {
      Serial.println("deviceId matched");
      if (strlen(Three.c_str()) >= 10) {
        Serial.println(sendAndWait("AT+CPBW=" + String(totalOwnerNumbers + 1) + ",\"" + String(Three) + "\",129,\"AXI\"\r\n"));
        sendSMSAnyNumber("Registration OK", Three);
        registered = 1;
        delay(500);
        EEPROM.write(ownerAddressEEPROM, (totalOwnerNumbers + 1));
        Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
        delay(500);
      }
      else {
        sendSMSAnyNumber("Please refer the manuals", senderM);
        registered = 0;
        Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
      }
    } else {
      Serial.println("deviceId wrong");
      sendSMSAnyNumber("Registration failed", senderM);
      registered = 0;
      Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
    }
  } else {
    sendSMSAnyNumber("Please refer the manuals", senderM);
    registered = 0;
    Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
  }
}


/////////////////AUTO MODE

void manageAutoOperation() {
  Serial.print('-');
  Serial.print('+');
  ///////////////
  if(o1=0)
  {
  if (sim.find("+CLIP:")) {
    sendAndWait("ATA\r\n");
    o1=1;
    voiceMethod();
  }}

}

void voiceMethod() {
  sendAndWait("ATA\r\n");
  delay(1000);
  sendAndWait("AT+CREC=5\r\n");
  delay(1000);
  sendAndWait("AT+CREC=4,1,0,80\r\n");
  delay(1000);
  sendAndWait("AT+DDET=1,0,0\r\n");
  delay(1000);

   while (sim.available()) {
      if (sim.find("+DTMF: ")) {
        int choice = sim.parseInt();
        //sendAndWait("ATH\r\n"); delay(1000);
        Serial.println("PRESSED" + String(choice));

      
          switch (choice) {
            o1=0;
            case (1): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(2);
                delay(1000);
                motorTurnON();
                break;
              }
            case (2): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(3);
                delay(1000);
                motorTurnOFF();
                break;
              }
            case (3): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(4);
                setSchedule(1);
                delay(1000);
                break;
              }
            case (4): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(5);
                setSchedule(2);
                break;
              }
            case (5): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(6);
                setSchedule(3);
                break;
              }
            case (6): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(7);
                setSchedule(4);
                break;
              }
            case (7): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(8);
                setSchedule(5);
                break;
              }
            case (8): {
              sendAndWait("AT+CREC=5\r\n");
              delay(1000);
                playRecord(9);
                break;
              }
          }

        }
    }
  }

////////////////MANUAL MODE

void manageManualMode() {
  Serial.print('+');
  ///////////////
  if (sim.find("+CLIP:")) {
    String callingStatement = sim.readStringUntil('\"');
    Serial.println("[caller1 :" + callingStatement);
    callingStatement = sim.readStringUntil('\"');
    Serial.println("[caller2 :" + callingStatement);
    if (callingStatement.equals(p[1]) || callingStatement.equals("+91" + p[2]) || callingStatement.equals(p[3]) || callingStatement.equals(p[4]) || callingStatement.equals(p[5]) ) {
      Serial.println("ownerCalling");
      voiceMethodForManual(callingStatement);
    } else {
      Serial.println(sendAndWait("ATH\r\n"));
      sendSMSAnyNumber("You are not registered to this kisan Device", callingStatement);
    }
  }
  ////////////////
}
void voiceMethodForManual(String ownerNumber) {
  boolean callNotEnded = true;
  sendAndWait("ATA\r\n"); delay(1000);
  playRecord(9);
  delay(4000);
  sendAndWait("ATH\r\n"); delay(4000);
}
//////////// SMS methods
void sendSMSAnyNumber(String message, String phoneNumber) {
  sim.print("AT+CMGS=\"" + phoneNumber + "\"\r\n");
  Serial.println(sim.read());
  delay(2000);
  sendAndWait(message);
  delay(2000);
  delay(200);
  sim.write('\x1A');
  delay(100);
}
///////// MOTOR Operations
void motorTurnON() {
  Serial.println("Motor trying to on");
//  digitalWrite(motoron, HIGH);
//  digitalWrite(motorof, LOW);
  digitalWrite(REL1, HIGH);
  digitalWrite(REL2, HIGH);
  delay(3000);
  digitalWrite(REL2, LOW);
}
void motorTurnOFF() {

  Serial.println("Motor trying to off");
//  digitalWrite(motorof, HIGH);
//  digitalWrite(motoron, LOW);
  digitalWrite(REL1, LOW);
//  digitalWrite(motortm, LOW);
  delay(6000);
  digitalWrite(REL1, HIGH);
  //playRecord();
  //sendAndWait("ATH/r/n");
}
//////////////Schedule methods
void manageSchedule() {
  if (scheduleOn == 1) {
    timerNewTime = millis();
    if (timerNewTime - timerOn > interval) {
      interruptFromClock = 1;
      motorTurnOFF();
//      digitalWrite(motortm, LOW);
      scheduleOn = 0;
    }
  }
}
void setSchedule(unsigned long hours) {
  motorTurnON();
//  digitalWrite(motortm, HIGH);
  delay(4000);
  timerOn = millis();
  scheduleOn = 1;
  interval = hours * 60 *  60000;
  Serial.println("sch interval=" + String(interval));
}
///////////phonebook methods
String readDirectPhoneNumberAt(int location) {
  String cmd = "AT+CPBR=" + String(location) + "\r\n";
  Serial.println("C :" + cmd);
  String u = sendAndWait(cmd);
  Serial.println("[CON] : " + u);
  Serial.println(u.substring(10, 21));
  return (u.substring(10, 21));
}
String readDirectOwnerAt(int location) {
  String cmd = "AT+CPBR=" + String(location) + "\r\n";
  Serial.println("[COM] :" + cmd);
  String u = sendAndWait(cmd);
  sim.readStringUntil('\"');
  sim.readStringUntil('\"');
  sim.readStringUntil('\"');
  String ownerName = sim.readStringUntil('\"');
  Serial.println("OWNER :" + ownerName);
  return (ownerName);
}

//////////// Playback methods
void playRecord(int number) {
  Serial.println("-----------PLAYING------------");
  Serial.println("DEBUG : " + String(number) + "");
  delay(1000);
  sendAndWait("AT+CREC=4," + String(number) + ",0,80\r\n");
  delay(4000);
  sendAndWait("AT+CREC=5\r\n");
}


///////////////call ststus
String getStatusForCall() {
  int pI = 0;  int mI = 0;  int sI = 0;

  if (phaseStatus == 0) {
    pI = 6 ;
  } else {
    pI = 5;
  }
  if (modeStatus == 0)  {
    sI = 9;
  } else {
    sI = 10;
  }

  playRecord(pI);  delay(4000);
  playRecord(sI);  delay(4000);

  // sendAndWait("ATH\r\n");

}
///////////// facoryt
void factoryResetIt() {

  /*#define PHSC 9
    #define MOTO 13
    #define PHSF 11
    #define MANL 12
  */
  for (int flushOwners = 1; flushOwners <= 5; flushOwners++) {
    sendAndWait("AT+CPBW=" + String(flushOwners) + ",\"0000000000000\",129,\"A\"\r\n");
  }
  EEPROM.write(ownerAddressEEPROM, 0);


  for (int u = 0; u <= 5; u++) {

  }
}

void waitForOtherOwner() {
  Serial.println("^");
  if (sim.find("SM") ){
  int messNumber = sim.parseInt();
    String SMD = sendAndWait("AT+CMGR=" + String(messNumber) + "\r\n");
    String message = sim.readStringUntil('\n');
    String senderM = SMD.substring(24, 34);
    Serial.println("{" + senderM);
    Serial.println("{" + message);
    String One = message.substring(1, 6);
    String Two = message.substring(7, 12);
    String Three = message.substring(13, 23);
    Serial.println(One + " " + Two + " " + Three);
    if (One.equals("kisan") ) {
      if (deviceID.equals(Two)) {
        Serial.println("deviceId matched");
        if (strlen(Three.c_str()) >= 10) {
          Serial.println(sendAndWait("AT+CPBW=" + String(totalOwnerNumbers + 1) + ",\"" + String(Three) + "\",129,\"AXI\"\r\n"));
          sendSMSAnyNumber("Registration OK", Three);
          registered = 1;
          EEPROM.write(ownerAddressEEPROM, (totalOwnerNumbers+1));
          totalOwnerNumbers=EEPROM.read(ownerAddressEEPROM);
          readAllContacts();
          Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
        }
        else {
          sendSMSAnyNumber("Please refer the manuals", senderM);
          registered = 0;
          Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));  
        }
      } else {
        Serial.println("deviceId wrong");
        sendSMSAnyNumber("Registration failed", senderM);
        registered = 0;
        Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
      }
    } else {
      sendSMSAnyNumber("Please refer the manuals", senderM);
      registered = 0;
      Serial.println(sendAndWait("AT+CMGDA=\"DEL ALL\"\r\n"));
    }
  }
}

void readAllContacts() {
  for (int loopCount = 1; loopCount <= totalOwnerNumbers; loopCount++)
  {
    if (!readDirectOwnerAt(loopCount).equals("AXI")) {
      p[loopCount] = readDirectPhoneNumberAt(loopCount);
      Serial.println('[' + p[loopCount] + ']');
    }
  }
}
