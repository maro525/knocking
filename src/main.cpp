#include <Arduino.h>
#include <pt.h>
#include <Queue.h>

#define PT_WAIT(pt, timestamp, usec) PT_WAIT_UNTIL(pt, millis() - *timestamp > usec);*timestamp = millis();

static struct pt pt1, pt2;
Queue<float> mvQueue = Queue<float>(10);

const int sPin = A0;
int outpin = 8;
int high_thresh = 3;
int low_thresh =1;

float min_interval = 0.2;
float min_knock_interval = 0.4; // minimum knock interval
float max_interval = 4.0; // min:4.5 - max:6.5
float normal_wait = 2.0; // normal wait time.
float normal_piezo_value = 40; // interval time is decided according to this value
float max_piezo_value = 70;

float last_knock_time = 0.0; // last knock time
float normal_rest_time = 0.08; // rest time after knocking
float rest_time;

int noise_value = 1;

///////////////
///// thread1 : read

boolean bPotential = false;
int max_knock = 0;
int knock_count = 0;
int max_knock_count = 20;

void read_sensor(int *v) {
    *v = analogRead(sPin);
}

float interval;
void set_interval(int value){
    if(value > max_piezo_value) {
        interval = max_interval - normal_wait * value / normal_piezo_value;
        Serial.print("knock, higher than ");
        Serial.println(max_piezo_value);
    }
    else if(value <= max_piezo_value) {
        interval = max_interval * value / max_piezo_value;
        Serial.print("knock, lower than ");
        Serial.println(max_piezo_value);
    }

    if(interval < min_interval) {
        interval = min_interval;
    }
}

void write_time(int value, float time) {

    set_interval(value);
    float mvTime = time + interval;
    Serial.print("[");
    Serial.print(time);
    Serial.print("]interval = ");
    Serial.print(interval);
    if(mvTime - last_knock_time > min_knock_interval){
        mvQueue.Push(mvTime);
        Serial.print(" -> WriteTime : ");
        Serial.println(mvTime);
    }
    else {
        Serial.println(" -> knock interval is less than min_knock_interval");
    }
}

float rest_thresh = 15.0;
void judge(){
    int s;
    read_sensor(&s);
    float now = millis()/1000.0f;

    if(knock_count != 0 && now - last_knock_time > rest_thresh){
        knock_count = 0;
        Serial.println("rest count rest to 0");
    }

    // record max knock value
    if(s > max_knock) {
        max_knock = s;
    } else if (s < noise_value && (!bPotential)){ // if value is under noise thresh, return
        return;
    }

    Serial.print("piezo : ");
    Serial.println(s);

    // if enough time is not passed after last knock,
    // no knocking occurs
    if(now - last_knock_time < rest_time){
        Serial.print("[");
        Serial.print(now);
        Serial.println("] got potential knock! but I am still resting...");
        return;
    }

    if (s >= high_thresh && !bPotential){ // if s is higher than high_thresh, begin to monitor.(bPotential flag turns true.)
        bPotential = true;
        Serial.print("Got Potential! v:");
        Serial.println(s);
    }
    else if (s <= low_thresh && bPotential) { // if s is lower than lower thresh and if bPotential is true, it turn
        bPotential = false;
        Serial.print("Got Knock! v:");
        Serial.println(max_knock);

        write_time(max_knock, now);
        max_knock = 0.0;
    }
}

////////////
/////// thread2 : work

void set_rest_time(){
    rest_time = normal_rest_time*knock_count;
    Serial.print("[Set Rest Time] rest time => ");
    Serial.println(rest_time);
}

void knock() {
    Serial.print("===========Knock=====");
    digitalWrite(outpin, HIGH);
    delay(50);
    digitalWrite(outpin, LOW);

    knock_count += 1;
    if(knock_count > max_knock_count){
        knock_count = 1;
    }

    set_rest_time();
}

float check_thresh_time = 0.5;
void work(){
    float lastMVtime;
    if (mvQueue.front(&lastMVtime)){
        float now = millis() / 1000.0f;
        if(lastMVtime>now+check_thresh_time) {
            return;
        }
        else if(lastMVtime < now - check_thresh_time*2){
            mvQueue.Pop();
            Serial.print("mv time: ");
            Serial.print(lastMVtime);
            Serial.println("-> last time overpassed -> discard data");
        }
        else if(lastMVtime < last_knock_time + min_knock_interval){
            mvQueue.Pop();
            Serial.print("mv time: ");
            Serial.print(lastMVtime);
            Serial.println("-> knock_interval lower than min_knock_interval -> discard data");
        }
        else {
            Serial.print("[");
            Serial.print(now);
            Serial.print("]");
            if (lastMVtime < now) {
                Serial.print("mv time: ");
                Serial.print(lastMVtime);
                Serial.print("-> last time passed -> knock!");
            }
            Serial.println();
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
