#include <M5Stack.h>
#include <WiFi.h>
//#define USE_ARDUINO_INTERRUPTS true
#include <MyPulseSensorPlayground.h>
#include <SparkFunMPU9250-DMP.h>
#include <Ambient.h>

#include "myconfig.h"
#include "wifiToGeo.h"

const int PIN_INPUT = 36;
const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle

const int PIN_GSR = 35;
const int MAX_GSR = 2047;

PulseSensorPlayground pulseSensor;
MPU9250_DMP imu;

WiFiClient client;
Ambient ambient;

location_t loc;
int steps = 0;

//位置情報を定期的に更新するタスク
void taskGeo(void * pvParameters) {
    WifiGeo geo;
    loc = geo.getGeoFromWifiAP(); // 初期位置取得
    for(;;) {
        // 10歩以上歩いていたら更新
        if(steps > 10) {
            location_t result = geo.getGeoFromWifiAP();
            if(result.lng != 0.0 && result.lat != 0.0 && result.accuracy != 0.0) {
                loc = geo.getGeoFromWifiAP();
                Serial.printf("lat:%f, lng:%f, accuracy:%f\n", loc.lat, loc.lng, loc.accuracy);
            }
        }
        delay(60000);
    }
}


void setup() {
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
    Serial.begin(115200);

    WiFi.begin(ssid, password); // Wi-Fi APに接続
    while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
        delay(100);
    }

    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());

    ambient.begin(channelId, writeKey, &client); 

    pulseSensor.analogInput(PIN_INPUT);
    //pulseSensor.setSerial(Serial);
    //pulseSensor.setOutputType(OUTPUT_TYPE);
    pulseSensor.setThreshold(THRESHOLD);

    if (!pulseSensor.begin()) {
        Serial.println("PulseSensor.begin: failed");
        for(;;) {
            delay(100);
        }
    }
    if (imu.begin() != INV_SUCCESS) {
        for(;;) {
            Serial.println("Unable to communicate with MPU-9250");
            Serial.println("Check connections, and try again.");
            Serial.println();
            delay(100);
        }
    }
    imu.dmpBegin(DMP_FEATURE_PEDOMETER);
    imu.dmpSetPedometerSteps(0); // バッファを0で初期化
    imu.dmpSetPedometerTime(0);

    // Task 1
    xTaskCreatePinnedToCore(
                    taskGeo,     /* Function to implement the task */
                    "taskGeo",   /* Name of the task */
                    8192,      /* Stack size in words */
                    NULL,      /* Task input parameter */
                    1,         /* Priority of the task */
                    NULL,      /* Task handle. */
                    0);        /* Core where the task should run */
}

const int LCD_WIDTH = 320;
const int LCD_HEIGHT = 240;
const int DOTS_DIV = 30;
#define GREY 0x7BEF

void DrawGrid() {
    for (int x = 0; x <= LCD_WIDTH; x += 2) { // Horizontal Line
        for (int y = 0; y <= LCD_HEIGHT; y += DOTS_DIV) {
            M5.Lcd.drawPixel(x, y, GREY);
        }
        if (LCD_HEIGHT == 240) {
            M5.Lcd.drawPixel(x, LCD_HEIGHT - 1, GREY);
        }
    }
    for (int x = 0; x <= LCD_WIDTH; x += DOTS_DIV) { // Vertical Line
        for (int y = 0; y <= LCD_HEIGHT; y += 2) {
            M5.Lcd.drawPixel(x, y, GREY);
        }
    }
}


#define REDRAW 20 // msec
#define PERIOD 60 // sec
short lastMin = 0, lastMax = 4096;
short minS = 4096, maxS = 0;
int lastY = 0;
int x = 0;

int loopcount = 0;

int ibis[256];
int gsrs[256];
int pointer = 0;


unsigned long pedLastStepCount = 0;

void loop() {
    delay(REDRAW);
    if (pulseSensor.sawStartOfBeat()) {            // Constantly test to see if "a beat happened". 
        int ibi = pulseSensor.getInterBeatIntervalMs();
        int gsr = analogRead(PIN_GSR);

        ibis[pointer] = ibi;
        gsr = min(gsr, MAX_GSR); //センサーの最大値を超えたら最大値に固定
        gsrs[pointer] = gsr;
        pointer++;
    }

    //表示部はじめ---------------------------------------------
    int y = pulseSensor.getLatestSample();
    if (y < minS) minS = y;
    if (maxS < y) maxS = y;
    if (x > 0) {
        y = (int)(LCD_HEIGHT - (float)(y - lastMin) / (lastMax - lastMin) * LCD_HEIGHT);
        M5.Lcd.drawLine(x - 1, lastY, x, y, WHITE);
        lastY = y;
    }
    if (++x > LCD_WIDTH) {
        x = 0;
        M5.Lcd.fillScreen(BLACK);
        DrawGrid();
        lastMin = minS - 20;
        lastMax = maxS + 20;
        minS = 4096;
        maxS = 0;
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(4);
        M5.Lcd.printf("BPM: %d", pulseSensor.getBeatsPerMinute());
    }
    //表示部おわり---------------------------------------------
    
    if (++loopcount > PERIOD * 1000 / REDRAW) {
        loopcount = 0;
        int n = pointer;
        pointer = 0;
        //10拍も取れていないー＞計算をあきらめる
        if(n < 10){
            return;
        }
        
        
        //RMSSD(RR間隔の差の自乗平均平方根)の計算
        int diffCount = 0;
        int sum = 0;
        for(int i = 1;i < n; i++) {
            int a = ibis[i-1];
            int b = ibis[i];

            //差が20%以下のみ使用　https://www.hrv4training.com/blog/issues-in-heart-rate-variability-hrv-analysis-motion-artifacts-ectopic-beats
            if(a * 0.8 < b && a * 1.2 > b) {
                int diff = b - a;
                sum += diff * diff;
                diffCount++;
            }
        }
        double rmssd = sqrt(sum / diffCount);

        //RMSSDをvalenceに変換 (https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5624990/)
        //平均42,最大値75,最小値19を1～9に正規化
        double valence = 0;
        if(valence > 42) {
            valence = (rmssd-42) / (75-42) / 2 + 0.5;
        } else {
            valence = (rmssd-19) / (42-19) / 2;
        }
        valence = valence * 8 + 1;

        //平均心拍数の計算
        sum = 0;
        for(int i = 0; i < n; i++) {
            sum += ibis[i];
        }
        int avg = sum / n;

        double bpm = 60000.0 / avg;

        //平均GSRの計算
        sum = 0;
        for(int i = 0; i < n; i++) {
            sum += gsrs[i];
        }
        double avg_gsr = (double)sum / n;
        double cond = avg_gsr = ((2048-avg_gsr) * 100)/(4096+2*avg_gsr);//校正 uS

        double arousal = cond / 50 * 8 + 1;
        
        unsigned long pedStepCount = imu.dmpGetPedometerSteps();
        steps = (int)(pedStepCount - pedLastStepCount);
        pedLastStepCount = pedStepCount;

        ambient.set(1, bpm);
        ambient.set(2, arousal);
        ambient.set(3, valence);
        ambient.set(4, steps);

        // ambient.set(int, double)だと小数点以下第2位までしかおくれないため(int, char)をつかう
        char latbuf[12], lngbuf[12];
        dtostrf(loc.lat, 11, 7, latbuf);
        dtostrf(loc.lng, 11, 7, lngbuf);

        // 9にlat 10にlngを指定
        ambient.set(9, latbuf);
        ambient.set(10, lngbuf);
        ambient.send();
    }
}
