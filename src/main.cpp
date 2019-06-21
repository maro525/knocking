#include <Arduino.h>
#include <pt.h>
#include <Queue.h>

#define PT_WAIT(pt, timestamp, usec) PT_WAIT_UNTIL(pt, millis() - *timestamp > usec);*timestamp = millis();

static struct pt pt1, pt2;
Queue<float> mvQueue = Queue<float>(10);

const int sPin = A0;
int outpin = 8;
int high_thresh = 30;
int low_thresh = 5;
float last_knock_time = 0.0;

///////////////
///// thread1 : read

boolean bPotential = false;
int max_knock = 0;

int read_sensor() {
    int sValue = analogRead(sPin);
    // Serial.print(sValue);
    // Serial.print(" ");
    return sValue;
}

void write_time(int value, float time) {
    float mvTime = time + 2.0;
    mvQueue.Push(mvTime);
    Serial.print("WriteTime : ");
    Serial.println(mvTime);
}

void judge(){
    int s = read_sensor();
    float now = millis()/1000.0f;
    if(s > max_knock) {
        max_knock = s;
    }
    if(now - last_knock_time < 0.5){
        return;
    }
    else if (s > high_thresh && !bPotential){
        bPotential = true;
        // Serial.print("Got Potential : value->");
        // Serial.println(s);
    }
    else if (s < low_thresh && bPotential) {
        bPotential = false;
        Serial.print("Got Knock! ");
        if(max_knock < 100) {
            write_time(s, now);
        } else {
            write_time(s, now);
            write_time(s, now+0.8);
        }
    }
}

////////////
/////// thread2 : work

void knock() {
    Serial.println("[Knock]");
    digitalWrite(outpin, HIGH);
    delay(50);
    digitalWrite(outpin, LOW);
}

float Thread2TimeStamp = 0.0;

void work(){
    float lastMVtime;
    if (mvQueue.front(&lastMVtime)){
        float now = millis() / 1000.0f;
        if(lastMVtime>now) {
            return;
        }
        else if (lastMVtime < Thread2TimeStamp) {
            Serial.print("lastMVTime : ");
            Serial.print(lastMVtime);
            Serial.print(" lastThread TIme : ");
            Serial.print(Thread2TimeStamp);
            Serial.println(" last time passed");
            mvQueue.Pop();
        }
        else if(lastMVtime > Thread2TimeStamp) {
            knock();
            last_knock_time = now;
            mvQueue.Pop();
        }
    }
}

///////////////////////
//////////////////////

static int thread1(struct pt *pt) {
    static unsigned long timestamp = 0;
    PT_BEGIN(pt);

    while(true) {
        PT_WAIT(pt, &timestamp, 10);
        judge();
    }

    PT_END(pt);
}

static int thread2(struct pt *pt) {
    static unsigned long timestamp = 0;
    PT_BEGIN(pt);

    while(true) {
        PT_WAIT(pt,&timestamp, 10);
        work();
        Thread2TimeStamp = timestamp/1000.0f;
    }
    PT_END(pt);
}

//////////
//////////

void setup() {
    Serial.begin(9600);
    pinMode(outpin, OUTPUT);

    PT_INIT(&pt1);
    PT_INIT(&pt2);
}

void loop() {
    thread1(&pt1);
    thread2(&pt2);
    millis();
}
