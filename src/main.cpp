#include <Arduino.h>
#include <pt.h>
#include <Queue.h>

#define PT_WAIT(pt, timestamp, usec) PT_WAIT_UNTIL(pt, millis() - *timestamp > usec);*timestamp = millis();

static struct pt pt1, pt2;
Queue<float> mvQueue = Queue<float>(10);

const int sPin = A0;
int outpin = 8;
int high_thresh = 7;
int low_thresh =5;

float min_knock_interval = 0.2; // minimum knock interval
float max_interval = 0.9; // min:4.5 - max:6.5
float min_write_time_interval = 0.1;
float normal_wait = 2.0; // normal wait time.
float normal_piezo_value = 20; // interval time is decided according to this value
float max_piezo_value = 800;

float last_knock_time = 0.0; // last knock time
float rest_time = 0.3; // rest time after knocking

int noise_value = 2;
bool bAfterKnock = false;

///////////////
///// thread1 : read

boolean bPotential = false;
float last_potential_time = 0.0;
int max_knock = 0;
int knock_count = 1;
int max_knock_count = 20;

int read_sensor() {
    int sValue = analogRead(sPin);
    return sValue;
}

float interval;
void write_time(int value, float time) {
    // interval = max_interval - normal_wait * value / normal_piezo_value;
    interval = max_interval * value / normal_piezo_value;
    // interval = max_interval * value / max_piezo_value;
    // interval = 1.2;
    if(interval < min_write_time_interval) { interval = min_write_time_interval; }

    float mvTime = time + interval;
    mvQueue.Push(mvTime);
    Serial.print("[");
    Serial.print(time);
    Serial.print("]WriteTime : ");
    Serial.println(mvTime);

    knock_count += 1;
    if(knock_count > max_knock_count){
        knock_count = 1;
    }
}


void judge(){
    int s = read_sensor();
    float now = millis()/1000.0f;

    // record max knock value
    if(s > max_knock) {
        max_knock = s;
    } else if (s < noise_value && (!bPotential)){ // if value is under noise thresh, return
        return;
    }

    Serial.print("pieze : ");
    Serial.print(s);
    Serial.println(" ");


    // if enough time is not passed after last knock,
    // no knocking occurs
    // if(now - last_knock_time < rest_time*knock_count){
    if(now-last_knock_time<rest_time && bAfterKnock) {
        return;
    } else {
        bAfterKnock = false;
    }

    if (s > high_thresh && !bPotential && now-last_potential_time>min_knock_interval){ // if s is higher than high_thresh, begin to monitor.(bPotential flag turns true.)
        bPotential = true;
        Serial.println();
        Serial.print("Got Potential! v:");
        Serial.println(s);
        last_potential_time = now;
    }
    else if (s < low_thresh && bPotential) { // if s is lower than lower thresh and if bPotential is true, it turn
        bPotential = false;
        Serial.println();
        Serial.print("Got Knock! v:");
        Serial.print(max_knock);
        Serial.print(" ");

        write_time(max_knock, now);
        max_knock = 0.0;
    }
}

////////////
/////// thread2 : work

void knock() {
    Serial.println();
    Serial.println("[Knock]");
    digitalWrite(outpin, HIGH);
    delay(50);
    digitalWrite(outpin, LOW);
    bAfterKnock = true;
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
            Serial.println();
            Serial.print("mv time: ");
            Serial.print(lastMVtime);
            Serial.print(" now: ");
            Serial.print(Thread2TimeStamp);
            Serial.println("-> last time passed");
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
