#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Ref:");
  lcd.setCursor(0, 1);
  lcd.print("Actual:");
}

void loop() 
{
  lcd.setCursor(8, 0);
  float refVoltage = ReadVoltage(A0);
  lcd.print(refVoltage);
  lcd.setCursor(8, 1);
  float fanVoltage = ReadVoltage(A2);
  lcd.print(fanVoltage);
  if(fanVoltage > refVoltage)
  {
    digitalWrite(13, HIGH);
    lcd.setCursor(13, 1);
    lcd.print("On ");
  }
  else
  {
    digitalWrite(13, LOW);
    lcd.setCursor(13, 1);
    lcd.print("Off");
  }
}

float ReadVoltage(int pin)
{
  int sensorValue = analogRead(pin);
  return sensorValue * (5.0 / 1023.0);
}

