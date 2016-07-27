
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(13, OUTPUT);
}

void loop() {
    int a0 = analogRead(A0);
    int a1 = analogRead(A1);
    Serial.print("A0: ");
    Serial.println(a0);
    Serial.print("A1: ");
    Serial.println(a1);
    delay(200);
}

