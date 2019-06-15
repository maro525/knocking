#include <Arduino.h>
#include <pt.h>
#include <Queue.h>

#define PT_WAIT(pt, timestamp, usec) PT_WAIT_UNTIL(pt, millis() - *timestamp > usec);*timestamp = millis();

static struct pt pt1, pt2;
Queue<int> mvQueue = Queue<int>(10);

const int sPin = A0;
int outpin = 8;
int thresh = 10;

///////////////
///// thread1

int read_sensor() {
    int sValue = analogRead(sPin);
    Serial.println(sValue);
    return sValue;
}

void write_time(int s) {
    int now = millis();
    Serial.print("time : ");
    Serial.print(now);
    Serial.println(" : thread 1");
    int mvTime = now + 1000;
    mvQueue.Push(mvTime);
}

void judge(){
    int s = read_sensor();
    if (s > thresh){
        write_time(s);
    }
}

////////////
/////// thread2

void knock() {
    digitalWrite(outpin, HIGH);
    delay(50);
    digitalWrite(outpin, LOW);
}

void work(){
    int now = millis();
    Serial.print("time : ");
    Serial.print(now);
    Serial.println(" : thread 1");
    int lastMVtime;
    if (mvQueue.Pop(&lastMVtime)){
        if(now - lastMVtime < 10) {
            knock();
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
}
