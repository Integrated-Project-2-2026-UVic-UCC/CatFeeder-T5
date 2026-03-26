#include "modulUltra.h"
#include "LedCAT328.h"
#include <Wire.h>

LedCAT328 servop(0,4); //psos servo a portB i pin 4

union Floattbyte{
  float valorf;
  byte vector[4];
};

Floattbyte dist[180];

void setup() {
  // put your setup code here, to run once:
  servop.attach();
  setupUltraso();
  Serial.begin(9600);
  Wire.begin(8);
  //Event Handlers
  Wire.onReceive(DataReceive);
  Wire.onRequest(DataRequest);

}

void loop() {
  for (int i=0;i<180;i++){
      servop.analogF(i);
      dist[i].valorf=lecturaDistancia();
  }
}

void DataRequest()
{
  for (int i=0; i<180; i++){
    Wire.write(dist[i].vector,4);
  }
}


void DataReceive()
{
  if (Wire.available()){
  }
}