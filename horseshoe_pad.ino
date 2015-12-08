#include <PinChangeInt.h> 

#define MAX_BEAN_SLEEP 0xFFFFFFFF

bool setNewValue = false;

uint16_t previousAccelerationX = 0;
uint16_t accumulatedAccelerationX = 0;
uint16_t averageAccelerationX = 0;

uint16_t previousAccelerationY = 0;
uint16_t accumulatedAccelerationY = 0;
uint16_t averageAccelerationY = 0;

uint16_t previousAccelerationZ = 0;
uint16_t accumulatedAccelerationZ = 0;
uint16_t averageAccelerationZ = 0;

uint8_t previousStepPressure = 0;
uint8_t accumulatedStepPressure = 0;
uint8_t averageStepPressure = 0;
uint8_t numberOfSteps = 0;

const int walkBank = 1;
const int trotBank = 2;
const int canterBank = 3;
const int miscBank = 4;
const int utilityBank = 5;

void setup()
{
  // Initialize serial communication.
  Serial.begin(57600);

  pinMode( 0, INPUT_PULLUP );

  attachPinChangeInterrupt(0, processGaitData, CHANGE);
  Bean.enableWakeOnConnect(true);
  Bean.enableAdvertising(true);

  uint8_t buffer[1] = {' '};

  // Initialize scratch banks with blank space.
  Bean.setScratchData(1, buffer, 1);
  Bean.setScratchData(2, buffer, 1);
  Bean.setScratchData(3, buffer, 1);
  Bean.setScratchData(4, buffer, 1);
  Bean.setScratchData(5, buffer, 1);

  Bean.setBeanName("lf");

  pinMode(0, OUTPUT);     // Set D0 as output.
  digitalWrite(0, LOW);   // Turn it off initially.
}

void loop()
{
  if (Bean.getConnectionState())
  {
    // Bean name is set when pad is initalized by the horse owner.
    if (!String(Bean.getBeanName()).equals("lf") && !String(Bean.getBeanName()).equals("lr")
        && !String(Bean.getBeanName()).equals("rf") && !String(Bean.getBeanName()).equals("rr")) {

      Serial.println("Set leg id");
    }
    else
    {
      // 1. IF IS AWAKE THEN PROCESS GAIR DATA.
      if (setNewValue)
      {
        processGaitData();
      }
    }
  }
  else
  {
    // Sleep unless woken.
    Bean.sleep(MAX_BEAN_SLEEP);
  }
}

void processGaitData()
{
  uint8_t upperThreshold = 150;
  stepCompleted = false;
  uint8_t currentStepPressure = 0;
  uint8_t variationX = 0;
  uint8_t variationY = 0;
  uint8_t variationZ = 0;
  digitalWrite(0, HIGH);
  AccelerationReading acceleration = {0, 0, 0};
  acceleration = Bean.getAcceleration();
  uint16_t accelerationX = acceleration.xAxis;
  uint16_t accelerationY = acceleration.yAxis;
  uint16_t accelerationZ = acceleration.zAxis;
 
  // Check if has horse is lifting foot.
  if (accelerationX > upperThreshold || accelerationY > upperThreshold || accelerationZ > upperThreshold) {
    
      currentStepPressure = analogRead(A1);

      // IF THE CURRENT STEP PRESSURE IS VERY SIMILAR TO THE HORSES STANDING STEP PRESSURE THEN SLEEP.
      // WHILE SLEEPING UPDATE THE NUMBER OF SLEEP CYCLES EVERY 2 SECONDS.
      if(numberOfSteps > 2)
      {
        if(currentStepPressure > (standingPressure - 6) || currentStepPressure < (standingPressure - 6))
        {
          setNewValue = false;

          do
          {
            Bean.sleep(2000);
            incrementSleepCycles();
          } while (!setNewValue);
        }
      }
      
      // check to make sure not recording the same pressure twice.
      if (currentStepPressure != previousStepPressure)
      {
        // CHECK IF READING IS ZERO.
        if (previousAccelerationX == 0)
        {
          previousAccelerationX = accelerationX;
          averageAccelerationX = accelerationX;
        }

        if (previousAccelerationY == 0)
        {
          previousAccelerationY = accelerationY;
          averageAccelerationY = accelerationY;
        }

        if (previousAccelerationZ == 0)
        {
          previousAccelerationZ = accelerationZ;
          averageAccelerationZ = accelerationZ;
        }
        
        // NOT EQUAL TO ZERO.
        if (averageAccelerationX != 0)
        {
          // calculate the variation between the newly read acceleration rate and the average acceleration rate.
          variationX = getVariationAccelerationX(accelerationX, averageAccelerationX);
        }

        if (averageAccelerationY != 0)
        {
          // calculate the variation between the newly read acceleration rate and the average acceleration rate.
          variationY = getVariationAccelerationY(accelerationY, averageAccelerationY);
        }

        if (averageAccelerationZ != 0)
        {
          // calculate the variation between the newly read acceleration rate and the average acceleration rate.
          variationZ = getVariationAccelerationZ(accelerationZ, averageAccelerationZ);
        }
        
        // CHECK IF SIMILAR TO LAST STEP.--->NEED TO ADD X AND Y AXIS ACCELERATION CHECKS.
        if (variationZ <= 25)
        {
          numberOfSteps++;

          accumulatedStepPressure += currentStepPressure;

          averageStepPressure = accumulatedStepPressure / numberOfSteps;

          Serial.println("Current pressure: " + String(currentStepPressure));
          Serial.println("Accumulated pressure: " + String(accumulatedStepPressure));
          Serial.println("Average pressure: " + String(averageStepPressure));

          accumulatedAccelerationZ += accelerationZ;
          averageAccelerationZ = accumulatedAccelerationZ / numberOfSteps;

          Serial.println("Average accelerationZ: " + String(averageAccelerationZ));
        }
        else if (variation > 25)
        {
          Serial.println("variation > 25");

          determineStorageBank();
        }
      }
    }
  }
  else {
    Bean.sleep(1000);
  }
}

void determineStorageBank()
{
  uint16_t averageAccelerationBankZ = 0;
  int averageStepPressureBank = 0;
  int numberOfStepsBank = 0;

  updateTotalTime();
}

int getVariationAccelerationX(uint16_t accelerationX, uint16_t averageAccelerationX)
{
  return (abs(accelerationX) - abs(averageAccelerationX));
}

int getVariationAccelerationY(uint16_t accelerationY, uint16_t averageAccelerationY)
{
  return (abs(accelerationY) - abs(averageAccelerationY));
}

int getVariationAccelerationZ(uint16_t accelerationZ, uint16_t averageAccelerationZ)
{
  return (abs(accelerationZ) - abs(averageAccelerationZ));
}

void incrementSleepCycles()
{
  // Updates the number of sleep times.
  writeScratchString(utilityBank, String(readScratchString(utilityBank).toInt() + 1));
  Serial.println("NUMBER OF SLEEP CYCLES " + readScratchString(utilityBank));
  return;
}

void updateTotalTime()
{
  int sleepCycles = String(readScratchString(utilityBank)).toInt();
  long missedTime = sleepCycles * 1000;
  Bean.setScratchNumber(utilityBank, Bean.readScratchNumber(utilityBank) + missedTime);

  Serial.println("TOTAL TIME: " + String(Bean.readScratchNumber(utilityBank)));
}

void writeScratchString(int nBank, String strScratch)
{
  // Convert the string to a uint8_t array
  uint8_t bufTemp[20];

  for ( int i = 0; i < strScratch.length(); i++ )
  {
    bufTemp[i] = strScratch.charAt(i);
  }

  // Write string to scratch bank
  Bean.setScratchData( (uint8_t) nBank, bufTemp, strScratch.length() );

  return;
}

String readScratchString( int nBank )
{
  // Read the scratch bank
  ScratchData scratchRead = Bean.readScratchData( (uint8_t) nBank );
  //clearScratchString( nBank );

  // Convert to a String object
  String strScratch = "";

  for (int i = 0; i < scratchRead.length; i++)
  {
    strScratch += (String) (char) scratchRead.data[i];
  }

  return strScratch;
}

void startStopAdvertising()
{
   setNewValue = true;
}

// This function calculates the difference between two acceleration readings
int getAccelDifference(AccelerationReading readingOne, AccelerationReading readingTwo) {
  int deltaX = abs(readingTwo.xAxis - readingOne.xAxis);
  int deltaY = abs(readingTwo.yAxis - readingOne.yAxis);
  int deltaZ = abs(readingTwo.zAxis - readingOne.zAxis);
  // Return the magnitude
  return deltaX + deltaY + deltaZ;
}
