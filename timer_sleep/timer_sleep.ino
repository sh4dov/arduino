/*
 * Sketch for testing sleep mode with wake up on timer.
 * Donal Morrissey - 2011.
 *
 */
#include <avr/sleep.h>
#include <avr/power.h>

#define LED_PIN (13)

volatile int f_timer=0;



/***************************************************
 *  Name:        ISR(TIMER1_OVF_vect)
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Timer1 Overflow interrupt.
 *
 ***************************************************/
ISR(TIMER1_OVF_vect)
{
  /* set the flag. */
   if(f_timer == 0)
   {
     f_timer = 1;
   }
}


/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
{
    /* Clear the timer counter register.
    * You can pre-load this register with a value in order to
    * reduce the timeout period, say if you wanted to wake up
    * ever 4.0 seconds exactly.
    */
    TCNT1 = 0x0000;

    /* Configure the prescaler for 1:1024, giving us a
    * timeout of 4.09 seconds.
    */
    TCCR1B = 0x05;  

  set_sleep_mode(SLEEP_MODE_IDLE);
  
  sleep_enable();


  /* Disable all of the unused peripherals. This will reduce power
   * consumption further and, more importantly, some of these
   * peripherals may generate interrupts that will wake our Arduino from
   * sleep!
   */
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer2_disable();
  power_twi_disable();  

  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the timer timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();

  TCCR1B = 0x1;
}



/***************************************************
 *  Name:        setup
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Setup for the serial comms and the
 *                timer. 
 *
 ***************************************************/
void setup()
{
  Serial.begin(9600);
  
  /* Don't forget to configure the pin! */
  pinMode(LED_PIN, OUTPUT);

  /*** Configure the timer.***/
  
  /* Normal timer operation.*/
  TCCR1A = 0x00; 

  /* Enable the timer overlow interrupt. */
  TIMSK1 = 0x01;
}



/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Main application loop.
 *
 ***************************************************/
void loop()
{
    for (int i = 0; i < 2; i++){
        digitalWrite(LED_PIN, HIGH);
        enterSleep();
        digitalWrite(LED_PIN, LOW);
        enterSleep();
    }

    for (int i = 0; i < 4; i++){
        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);
        delay(1000);
    }

    for (int i = 0; i < 3; i++){
        digitalWrite(LED_PIN, HIGH);
        enterSleep();
        digitalWrite(LED_PIN, LOW);
        enterSleep();
    }

    for (int i = 0; i < 10; i++){
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }

    return;
  if(f_timer==1)
  {
    f_timer = 0;
    /* Toggle the LED */
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    
    /* Re-enter sleep mode. */
    enterSleep();

    int r = digitalRead(LED_PIN);
    if (r == LOW){
        for (int i = 0; i < 10; i++){
            digitalWrite(LED_PIN, HIGH);
            delay(1000);
            digitalWrite(LED_PIN, LOW);
            delay(1000);
        }
    }
  }
}
