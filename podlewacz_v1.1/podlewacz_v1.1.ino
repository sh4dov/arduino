const byte masterOn = 3;
const byte multiplexerA = 0;
const byte multiplexerB = 1;
const byte multiplexerC = 2;
const byte led = 4;
const byte sensors[] = { 3, 0, 1, 2 };

void setup() {
    pinMode(masterOn, OUTPUT);
    pinMode(multiplexerA, OUTPUT);
    pinMode(multiplexerB, OUTPUT);
    pinMode(multiplexerC, OUTPUT);
    pinMode(led, OUTPUT);
    
    digitalWrite(masterOn, LOW);
    digitalWrite(multiplexerA, LOW);
    digitalWrite(multiplexerB, LOW);
    digitalWrite(multiplexerC, LOW);
    digitalWrite(led, LOW);

    welcome();
}

void loop() {
    for (byte i = 0; i < sizeof(sensors); i++){
        byte sensor = sensors[i];
        byte a = (sensor & 1) ? HIGH : LOW;
        byte b = (sensor & 2) ? HIGH : LOW;
        byte c = (sensor & 4) ? HIGH : LOW;

        flash(i + 1);
                
        digitalWrite(multiplexerA, a);
        digitalWrite(multiplexerB, b);
        digitalWrite(multiplexerC, c);

        digitalWrite(masterOn, HIGH);
        
        delay(5 * 1000);

       digitalWrite(masterOn, LOW);
       delay(1000);
    }

    goodbye();
    digitalWrite(masterOn, LOW);

    delay(1 * 60 * 1000);
}

void goodbye(){
    for (int i = 0; i < 10; i++){
        digitalWrite(led, HIGH);
        delay(50);
        digitalWrite(led, LOW);
        delay(50);
    }
}

void welcome(){
    for (int i = 0; i < 3; i++){
        digitalWrite(led, HIGH);
        delay(100);
        digitalWrite(led, LOW);
        delay(100);
    }
    delay(2000);
}

void flash(byte number){
    for (byte i = 0; i < number; i++){
        digitalWrite(led, HIGH);
        delay(500);
        digitalWrite(led, LOW);
        delay(500);
    }
}
