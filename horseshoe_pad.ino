#include <Arduino.h>
#include <StringUtil.h>
#include <Statistics.h>

using namespace StringUtil;


Statistics accelerationStatsX(20);
Statistics accelerationStatsY(20);
Statistics accelerationStatsZ(20);
Statistics forceStats(20);

#define MAX_BEAN_SLEEP 0xFFFFFFFF

const int bankForce = 1;
const int bankAccelerationX = 2;
const int bankAccelerationY = 3;
const int bankAccelerationZ = 4;
const int commandBank = 5;

static int d0 = 0;
static bool activateFlag = false;
static bool gaitRegisterFlag = false;
static bool timeSetFlag = false;
static int processFlag = false;

int string_width;

void setup()
{
  // Initialize serial communication.
  Serial.begin(57600);

  pinMode(d0, OUTPUT);

  Bean.enableConfigSave(false);
  Bean.enableWakeOnConnect(true);
  Bean.enableAdvertising(true);

  uint8_t buffer[1] = {' '};

  // Initialize scratch banks with blank space.
  Bean.setScratchData(bankForce, buffer, 1);
  Bean.setScratchData(bankAccelerationX, buffer, 1);
  Bean.setScratchData(bankAccelerationY, buffer, 1);
  Bean.setScratchData(bankAccelerationZ, buffer, 1);
  Bean.setScratchData(commandBank, buffer, 1);

  if (Bean.getBeanName() != "pad-1")
  {
    Bean.setBeanName("pad-1");
    Serial.println("-BEAN-NAME-CHANGED-");
  }
  
  digitalWrite(d0, LOW);
}

void loop()
{
  if (Bean.getConnectionState()) 
  {
    evaluateCommand(getCommand());
    delay(500);
  } 
  else 
  {
    digitalWrite(d0, LOW);
    Bean.sleep(0xFFFFFFFF);
  }
}

void evaluateCommand(String command)
{
  if (((command.length()) > 0) && (command != " "))
  {
    if (command == "TAKE_READINGS")
    {
      digitalWrite(d0, HIGH);
    }
    else if (command == "PAUSE_READINGS")
    {
      digitalWrite(d0, LOW);
    }
    else if (command == "BANK_DATA")
    {
      
    }
    else if (command == "CHECK_BATTERY")
    {
      
    }
  }
  else if (digitalRead(d0) == HIGH)
  {
    takeReadings();
  }
  else
  {
    Serial.println("-PAUSED-");
  }
}

void takeReadings()
{
  AccelerationReading acceleration = {0, 0, 0};
  acceleration = Bean.getAcceleration();
  uint16_t accelerationX = acceleration.xAxis;
  uint16_t accelerationY = acceleration.yAxis;
  uint16_t accelerationZ = acceleration.zAxis;

  accelerationStatsX.addData(accelerationX);
  accelerationStatsY.addData(accelerationY);
  accelerationStatsZ.addData(accelerationZ);

  int force = analogRead(A0);

  // IF GREATER THAN WHEN HORSE FOOT STILL IN AIR AND NOT TOUCHING GROUND DURING STEP.
  if (force > 30)
  {
    char buf[256];
    forceStats.addData(force);

    Serial.print("AccelerationX mean: ");
    Serial.print(accelerationStatsX.mean());
    Serial.print("AccelerationY mean: ");
    Serial.print(accelerationStatsY.mean());
    Serial.print("AccelerationZ mean: ");
    Serial.print(accelerationStatsZ.mean());
    Serial.print("Force mean: ");
    Serial.print(forceStats.mean());

    float xAccelerationMean = accelerationStatsX.mean();

    fprintf(Serial, "float=%f ", 12.123456);
  }
}

void sendData()
{
  //Bean.setScratchData(commandBank, buffer, 1);
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

