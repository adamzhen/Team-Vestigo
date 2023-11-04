#include "ESPNOWOperation.h"
#include "SharedVariables.h"

void setup()
{
  Serial.begin(115200);

  setupESPNOW();
}

void loop() 
{
  // System Blackout Reset
  if (millis() - lastReceptionTime > 60000)
  {
    lastReceptionTime = millis();
    Serial.println("Total System Failure, Restarting Network");
    deviceResetFlag.reset = true;

    for (int i = 0; i < 7; i++)
    {
      sendToPeer(slaveMacs[i], &deviceResetFlag, sizeof(deviceResetFlag));
    }

    sendToPeer(masterMac, &deviceResetFlag, sizeof(deviceResetFlag));
  }
}