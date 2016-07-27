#define TrigPin 3
#define EchoPin 2

int distance;
long duration = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);
}

void loop() {
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);

  duration = pulseIn(EchoPin, HIGH);

  distance = duration/58;

  Serial.println(distance);
  if(distance < 2 || distance > 200)
  {
    Serial.println("---- out of range");  
  }
  else
  {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  delay(500);
}
