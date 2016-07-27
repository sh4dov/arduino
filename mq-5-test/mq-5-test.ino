void setup() {
    Serial.begin(115200);
    pinMode(7, INPUT);
    pinMode(A0, INPUT);

}

void loop() {
    int val = analogRead(A0);
    int trig = digitalRead(7);

    Serial.print(val);
    Serial.print("\t");
    Serial.println(trig);

    delay(1000);
}
