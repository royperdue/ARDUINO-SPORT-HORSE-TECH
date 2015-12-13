#include <Time.h>
#include <TimeLib.h>
#include <Statistics.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include <PinChangeInt.h>

#define MAX_BEAN_SLEEP 0xFFFFFFFF
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

const int activateBank = 1;
const int gaitRegisterBank = 2;
const int pressureBank = 3;
const int accelerationBank = 4;
const int commandBank = 5;

static int d0 = 0;
static int activatePin = 5;
static int gaitRegisterPin = 4;
static int timeSetPin = 3;
static int processPin = 2;

int address = 0;

void setup()
{
  // Initialize serial communication.
  Serial.begin(57600);

  pinMode(d0, OUTPUT);
  pinMode(activatePin, INPUT_PULLUP);
  pinMode(gaitRegisterPin, INPUT_PULLUP);
  pinMode(timeSetPin, INPUT_PULLUP);
  pinMode(processPin, INPUT_PULLUP);

  Bean.enableConfigSave(false);
  Bean.enableWakeOnConnect(true);
  Bean.enableAdvertising(true);

  uint8_t buffer[1] = {' '};

  // Initialize scratch banks with blank space.
  Bean.setScratchData(1, buffer, 1);
  Bean.setScratchData(2, buffer, 1);
  Bean.setScratchData(3, buffer, 1);
  Bean.setScratchData(4, buffer, 1);
  Bean.setScratchData(5, buffer, 1);

  Bean.setBeanName("lf-hard-coded-id");

  digitalWrite(d0, LOW);
  digitalWrite(activatePin, LOW);
  digitalWrite(gaitRegisterPin, LOW);
  digitalWrite(timeSetPin, LOW);
  digitalWrite(processPin, LOW);
}

void loop()
{
  if (Bean.getConnectionState() && digitalRead(activatePin) == 0 && digitalRead(gaitRegisterPin) == 0)
  {
    evaluateCommand(getCommand());

    if (digitalRead(timeSetPin) == 1)
    {
      digitalClockDisplay();
    }
  }
  else if (Bean.getConnectionState() && digitalRead(activatePin) == 1 && digitalRead(gaitRegisterPin) == 0)
  {
    evaluateCommand(getCommand());

    if (digitalRead(timeSetPin) == 1)
    {
      digitalClockDisplay();
    }
  }
  else if (Bean.getConnectionState() && digitalRead(activatePin) == 1 && digitalRead(gaitRegisterPin) == 1)
  {
    evaluateCommand(getCommand());
    setGait();
  }
  else if (!Bean.getConnectionState() && digitalRead(activatePin) == 1 && digitalRead(processPin) == 1)
  {
    evaluateCommand("PROCESS");
  }
  else if (!Bean.getConnectionState() && digitalRead(activatePin) == 0 && digitalRead(gaitRegisterPin) == 0)
  {
    evaluateCommand("SLEEP");
  }
}

void evaluateCommand(String command)
{
  if (((command.length()) > 0) && (command != " "))
  {
    if (command == "ACTIVATE")
    {
      activate();
    }
    else if (command == "READ_TIME")
    {
      setClockTime();
    }
    else if (command == "DEACTIVATE")
    {

    }
    else if (command == "SET_GAIT")
    {
      digitalWrite(gaitRegisterPin, HIGH);
    }
    else if (command == "STOP_SET_GAIT")
    {
      digitalWrite(gaitRegisterPin, LOW);
      digitalWrite(processPin, HIGH);
    }
    else if (command == "PROCESS")
    {
      processGaitData();
    }
    else if (command == "SLEEP")
    {
      Serial.println("SLEEPING--");
      Bean.sleep(MAX_BEAN_SLEEP);
    }
  }
  delay(1000);
}

void activate()
{
  writeScratchString(activateBank, "lf-hard-coded-id");
  Serial.println("-NAME WRITTEN-");
}

void setClockTime()
{
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");

  if (Serial.available()) {
    processSyncMessage();
  }
  if (timeStatus() != timeNotSet) {
    digitalClockDisplay();
  }

  digitalWrite(activatePin, HIGH);
  Serial.println("-TIME SET-");
  digitalWrite(timeSetPin, HIGH);
}

void setGait()
{
  Serial.println("-GAIT-" + readScratchString(gaitRegisterBank));
  sendAccelerationData();
  sendPressureData();
}

void sendPressureData()
{
  Serial.println("P-" + String(analogRead(A1)));
}

void sendAccelerationData()
{
  AccelerationReading acceleration = {0, 0, 0};
  acceleration = Bean.getAcceleration();

  uint16_t accelerationY = acceleration.yAxis;
  uint16_t accelerationZ = acceleration.zAxis;

  Serial.println("AY-" + String(accelerationY));
  Serial.println("AZ-" + String(accelerationZ));
}

void processGaitData()
{
  Serial.println("--PROCESSING--");
}

String getCommand()
{
  ScratchData scratchCommand = Bean.readScratchData(commandBank);
  String command = "";

  for (int i = 0; i < scratchCommand.length; i++)
  {
    command += (String) (char) scratchCommand.data[i];
  }
  command.toUpperCase();

  // Clear the command so we don't process twice
  uint8_t buffer[1] = { ' ' };
  Bean.setScratchData(commandBank, buffer, 1);
  Serial.println("-COMMAND-" + command);

  return command;
}

void writeScratchString(int nBank, String strScratch)
{
  // Convert the string to a uint8_t array
  uint8_t bufTemp[20];

  for (int i = 0; i < strScratch.length(); i++)
  {
    bufTemp[i] = strScratch.charAt(i);
  }

  // Write string to scratch bank
  Bean.setScratchData((uint8_t) nBank, bufTemp, strScratch.length());

  return;
}

String readScratchString(int nBank)
{
  // Read the scratch bank
  ScratchData scratchRead = Bean.readScratchData((uint8_t) nBank);
  clearScratchString(nBank);

  // Convert to a String object
  String strScratch = "";

  for (int i = 0; i < scratchRead.length; i++)
  {
    strScratch += (String) (char) scratchRead.data[i];
  }

  return strScratch;
}

void clearScratchString(int nBank)
{
  uint8_t buffer[1] = { ' ' };
  Bean.setScratchData((uint8_t) nBank, buffer, 1);

  return;
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
      setTime(pctime); // Sync Arduino clock to the time received on the serial port
    }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);
  return 0; // the time will be sent later in response to serial mesg
}
