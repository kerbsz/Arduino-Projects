const int trigPin = 7;
const int echoPin = 8;

const int yellowLed = 5;
const int blueLed = 4;
const int orangeLed = 3;
const int redLed = 2;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  pinMode(yellowLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(orangeLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  
  Serial.begin(9600);
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  digitalWrite(yellowLed, (distance > 30 && distance <= 40) ? HIGH : LOW);
  digitalWrite(blueLed,   (distance > 20 && distance <= 30) ? HIGH : LOW);
  digitalWrite(orangeLed, (distance > 10 && distance <= 20) ? HIGH : LOW);
  digitalWrite(redLed,    (distance > 0  && distance <= 10) ? HIGH : LOW);



  delay(100);
}
