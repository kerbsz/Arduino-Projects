int Led_pin1 = 11;
int btn_pin = 3;
void setup()
{
  pinMode(Led_pin1, OUTPUT);
  pinMode(btn_pin,INPUT);
}

void loop(){
  if(digitalRead(btn_pin) == HIGH){
    digitalWrite(Led_pin1,HIGH);
  }
  else{
    digitalWrite(Led_pin1,LOW);
  }
}