
int LED1 = 12;
void setup()
{
  pinMode(LED1, OUTPUT);
}

void loop()
{
  digitalWrite(LED1, HIGH);
  delay(1000); // Wait for 1000 millisecond(s)
  digitalWrite(LED1, LOW);
  delay(1000); // Wait for 1000 millisecond(s)
}