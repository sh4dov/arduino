void setup() {
    Serial.begin(115200);
    pinMode(A0, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);

    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
}

void loop() {
    digitalWrite(2, LOW);
    digitalWrite(5, HIGH);
    delay(1000);
    pinMode(A0, INPUT);
    int result = analogRead(A0);
    digitalWrite(5, LOW);
    Serial.print("Sensor: ");
    Serial.println(result);


    digitalWrite(2, HIGH);
    pinMode(A0, OUTPUT);
    digitalWrite(A0, result > 512 ? HIGH : LOW);

    delay(1000);
}
