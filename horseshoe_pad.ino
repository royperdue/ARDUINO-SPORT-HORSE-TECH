#include <Arduino.h>
#include <StringUtil.h>
#include <Statistics.h>

using namespace StringUtil;

Statistics accelerationStatsX(10);
Statistics accelerationStatsY(10);
Statistics accelerationStatsZ(10);
Statistics forceStats(10);

#define MAX_BEAN_SLEEP 0xFFFFFFFF

const int forceBank = 1;
const int xAccelerationBank = 2;
const int yAccelerationBank = 3;
const int zAccelerationBank = 4;
const int commandBank = 5;

static int d0 = 0;

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
  Bean.setScratchData(forceBank, buffer, 1);
  Bean.setScratchData(xAccelerationBank, buffer, 1);
  Bean.setScratchData(yAccelerationBank, buffer, 1);
  Bean.setScratchData(zAccelerationBank, buffer, 1);
  Bean.setScratchData(commandBank, buffer, 1);

  // DURING PRODUCTION THE PAD ID MUST BE HARD CODED HERE AND AGAIN ON LINES 143, 147, 154, 158, 165, 169, 176, & 180.
  // OR ANOTHER POSSIBILITY IS TO MAKE 3 MORE FILES, ONE FOR LH, LF, RH, & RF.
  if (Bean.getBeanName() != "RH-1")
  {
    Bean.setBeanName("RH-1");
    Serial.println("-BEAN-NAME-CHANGED-");
  }

  digitalWrite(d0, LOW);
}

void loop()
{
  if (Bean.getConnectionState())
  {
    evaluateCommand(getCommand());
    // THIS DELAY MAY NEED ADJUSTING SO IT COORDINATES WITH THE HORSES FOOT STEPS.
    delay(1000);
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
      command = "";
      digitalWrite(d0, HIGH);
    }
    else if (command == "PAUSE_READINGS")
    {
      command = "";
      digitalWrite(d0, LOW);
    }
    else if (command == "BANK_DATA")
    {
      command = "";
      sendData();
    }
  }
  else if (digitalRead(d0) == HIGH)
  {
    takeReadings();
  }
  else if (digitalRead(d0) == LOW)
  {
    Serial.println("-PAUSED-");
    Bean.sleep(1000);
  }
}

void takeReadings()
{
  AccelerationReading  acceleration = Bean.getAcceleration();
  uint16_t accelerationX = abs(acceleration.xAxis);
  uint16_t accelerationY = abs(acceleration.yAxis);
  uint16_t accelerationZ = abs(acceleration.zAxis);

  if (accelerationX > 0)
  {
    accelerationStatsX.addData(accelerationX);
  }

  if (accelerationY > 0)
  {
    accelerationStatsY.addData(accelerationY);
  }

  if (accelerationZ > 0)
  {
    accelerationStatsZ.addData(accelerationZ);
  }

  uint16_t force = analogRead(A0);

  // IF GREATER THAN WHEN HORSE FOOT STILL IN AIR AND NOT TOUCHING GROUND DURING STEP.
  if (force > 30)
  {
    forceStats.addData(force);
    writeScratchString(commandBank, "BANK_DATA");
  }
}

void sendData()
{
  char bufferForce[256];
  char bufferAccelerationX[256];
  char bufferAccelerationY[256];
  char bufferAccelerationZ[256];

  float forceMean = forceStats.mean();

  if (forceMean > 0.0)
  {
    sprintf(bufferForce, F("RH-%f"), forceMean);
  }
  else
  {
    sprintf(bufferForce, F("RH-%f"), 0.0);
  }

  if (writeScratchString(forceBank, bufferForce))
  {
    if (accelerationStatsX.mean() > 0.0)
    {
      sprintf(bufferAccelerationX, F("RH-%f"), accelerationStatsX.mean());
    }
    else
    {
      sprintf(bufferAccelerationX, F("RH-%f"), 0.0);
    }

    if (writeScratchString(xAccelerationBank, bufferAccelerationX))
    {
      if (accelerationStatsY.mean() > 0.0)
      {
        sprintf(bufferAccelerationY, F("RH-%f"), accelerationStatsY.mean());
      }
      else
      {
        sprintf(bufferAccelerationY, F("RH-%f"), 0.0);
      }

      if (writeScratchString(yAccelerationBank, bufferAccelerationY))
      {
        if (accelerationStatsZ.mean() > 0.0)
        {
          sprintf(bufferAccelerationZ, F("RH-%f"), accelerationStatsZ.mean());
        }
        else
        {
          sprintf(bufferAccelerationZ, F("RH-%f"), 0.0);
        }
          writeScratchString(zAccelerationBank, bufferAccelerationZ);
      }
    }
  }
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

bool writeScratchString(int nBank, String strScratch)
{
  // Convert the string to a uint8_t array
  uint8_t bufTemp[20];

  for (int i = 0; i < strScratch.length(); i++)
  {
    bufTemp[i] = strScratch.charAt(i);
  }

  // Write string to scratch bank
  bool success = Bean.setScratchData((uint8_t)nBank, bufTemp, strScratch.length());
  delay(100);

  return success;
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

