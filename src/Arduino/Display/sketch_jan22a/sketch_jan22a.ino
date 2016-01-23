#include <LiquidCrystal.h>

float prevfanVoltage = 5.0;

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
    float refVoltage = ReadVoltage(A0);
    float fanVoltage = ReadVoltage(A2);
    if(prevfanVoltage > refVoltage && fanVoltage < refVoltage)
    {
      for(int i = 0; i< 500; i++)
      {
        PORTB |= _BV(PB5);
        PORTB &= ~_BV(PB5);
      }
    }
    else
    {
      digitalWrite(13, HIGH);
    }
}

float ReadVoltage(int pin)
{
  int sensorValue = analogRead(pin);
  return sensorValue * (5.0 / 1023.0);
}

