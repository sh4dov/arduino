
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(A0, INPUT);
    pinMode(13, OUTPUT);
}

void loop() {
    int r = analogRead(A0);
    digitalWrite(13, r > 500 ? HIGH : LOW);
    Serial.println(r);
    delay(200);
}
