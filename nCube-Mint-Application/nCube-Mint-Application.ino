/**
 * Copyright (c) 2017, OCEAN
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Created by Il-Yeup Ahn in KETI on 2017-07-11.
 */

#include <WiFi101.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "OneM2MClient.h"

#include "TasCO2.h"

#define LEDPIN 13

#define LED_RED_PIN 10
#define LED_GREEN_PIN 9
#define LED_BLUE_PIN 6
#define LED_GND_PIN 5

#define RELAY_PIN 12

const String AE_ID = "mint";                   // guide: this is same with AE name
const String MQTT_BROKER_IP = "203.253.128.161";  // guide: set IP address of MQTT broker for Mobius as IoT Platform
const uint16_t MQTT_BROKER_PORT = 1883;           // guide: set MQTT port used

OneM2MClient nCube(MQTT_BROKER_IP, MQTT_BROKER_PORT, AE_ID); // AE-ID

TasCO2 TasCO2Sensor;

unsigned long req_previousMillis = 0;
const long req_interval = 2000;

// guide: set sensing period, modify or add for your sensors
unsigned long sensing_previousMillis = 0;
const long sensing_interval = (1000 * 5);
/////////////////////////////////////////////////////

short action_flag = 0;
short sensing_flag = 0;
short control_flag = 0;

String noti_con = "";

char body_buff[400];  //for inputting data to publish
char req_id[10];       //for generating random number for request packet id

String resp_rqi = "";

String state = "init";

String curValue = "";

/*************************** Sketch Code ************************************/

void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

void upload_callback(String con) {
    if (state == "create_cin") {
        curValue = con;
        sensing_flag = 1;
    }
}

void resp_callback(String topic, JsonObject &root) {
    int response_code = root["rsc"];
    String request_id = String(req_id);
    String response_id = root["rqi"];

    if (request_id == response_id) {
        if (action_flag == 0) {
            if (response_code == 2000 || response_code == 2001 || response_code == 2002 || response_code == 4105 ||
                response_code == 4004) {
                action_flag = 1;
            }
        }

        Serial.print(topic);
        Serial.println(F(" - RESP_TOPIC receive a message."));
    }
}

void noti_callback(String topic, JsonObject &root) {
    if (state == "create_cin") {
        if (root["pc"]["sgn"]["sur"] == (nCube.resource[4].to + "/" + nCube.resource[4].rn)) { // guide: uri of subscription resource for notification
            String con = root["pc"]["sgn"]["nev"]["rep"]["m2m:cin"]["con"];
            noti_con = con;

            const char *rqi = root["rqi"];
            resp_rqi = String(rqi);
            control_flag = 1;
        }
        else if (root["pc"]["sgn"]["sur"] == (nCube.resource[5].to + "/" + nCube.resource[5].rn)) { // guide: uri of subscription resource for notification
            String con = root["pc"]["sgn"]["nev"]["rep"]["m2m:cin"]["con"];
            noti_con = con;

            const char *rqi = root["rqi"];
            resp_rqi = String(rqi);
            control_flag = 2;
        }
    }
}

void buildResource() {
    // temperally build resource structure into Mobius as oneM2M IoT Platform

    // AE resource
    uint8_t index = 0;
    nCube.resource[index].ty = "2";
    nCube.resource[index].to = "/Mobius";
    nCube.resource[index].rn = AE_ID;
    nCube.resource[index++].status = 0;

    // Container resource
    nCube.resource[index].ty = "3";
    nCube.resource[index].to = "/Mobius/" + nCube.resource[0].rn;
    nCube.resource[index].rn = "co2";
    nCube.resource[index++].status = 0;

    nCube.resource[index].ty = "3";
    nCube.resource[index].to = "/Mobius/" + nCube.resource[0].rn;
    nCube.resource[index].rn = "led-ctrl";
    nCube.resource[index++].status = 0;

    nCube.resource[index].ty = "3";
    nCube.resource[index].to = "/Mobius/" + nCube.resource[0].rn;
    nCube.resource[index].rn = "relay-ctrl";
    nCube.resource[index++].status = 0;
    
    // Subscription resource
    nCube.resource[index].ty = "23";
    nCube.resource[index].to = "/Mobius/" + nCube.resource[0].rn + '/' + nCube.resource[2].rn;
    nCube.resource[index].rn = "led-sub";
    nCube.resource[index++].status = 0;

    nCube.resource[index].ty = "23";
    nCube.resource[index].to = "/Mobius/" + nCube.resource[0].rn + '/' + nCube.resource[3].rn;
    nCube.resource[index].rn = "relay-sub";
    nCube.resource[index++].status = 0;

    nCube.resource_count = index;
}

void publisher() {
    int i = 0;
    rand_str(req_id, 8);

    if (state == "create_ae") {
        Serial.println(state);
        if (action_flag == 1) {
            for (i = 0; i < nCube.resource_count; i++) {
                if (nCube.resource[i].ty == "2" && nCube.resource[i].status == 1) {
                    nCube.resource[i].status = 2;
                }
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "2" && nCube.resource[i].status == 0) {
                action_flag = 0;
                nCube.resource[i].status = 1;
                nCube.createAE(req_id, 0, "3.14");
                return;
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "3") {
                state = "create_cnt";
                break;
            }
        }

        if (i == nCube.resource_count) {
            for (i = 0; i < nCube.resource_count; i++) {
                if (nCube.resource[i].ty == "23") {
                    state = "delete_sub";
                    break;
                }
            }
        }

        if (i == nCube.resource_count) {
            state = "create_cin";
        }
        Serial.println(state);
    }

    if (state == "create_cnt") {
        Serial.println(state);
        if (action_flag == 1) {
            for (i = 0; i < nCube.resource_count; i++) {
                if (nCube.resource[i].ty == "3" && nCube.resource[i].status == 1) {
                    nCube.resource[i].status = 2;
                }
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "3" && nCube.resource[i].status != 2) {
                action_flag = 0;
                nCube.resource[i].status = 1;
                nCube.createCnt(req_id, i);
                return;
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "23") {
                state = "delete_sub";
                break;
            }
        }

        if (i == nCube.resource_count) {
            state = "create_cin";
        }
        Serial.println(state);
    }

    if (state == "delete_sub") {
        Serial.println(state);
        if (action_flag == 1) {
            for (i = 0; i < nCube.resource_count; i++) {
                if (nCube.resource[i].ty == "23" && nCube.resource[i].status == 1) {
                    nCube.resource[i].status = 2;
                }
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "23" && nCube.resource[i].status != 2) {
                action_flag = 0;
                nCube.resource[i].status = 1;
                nCube.deleteSub(req_id, i);
                return;
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "23") {
                nCube.resource[i].status = 0;
            }
        }

        state = "create_sub";
        Serial.println(state);
    }

    if (state == "create_sub") {
        Serial.println(state);
        if (action_flag == 1) {
            for (i = 0; i < nCube.resource_count; i++) {
                if (nCube.resource[i].ty == "23" && nCube.resource[i].status == 1) {
                    nCube.resource[i].status = 2;
                }
            }
        }

        for (i = 0; i < nCube.resource_count; i++) {
            if (nCube.resource[i].ty == "23" && nCube.resource[i].status != 2) {
                action_flag = 0;
                nCube.resource[i].status = 1;
                nCube.createSub(req_id, i);
                return;
            }
        }

        action_flag = 0;
        state = "create_cin";
        Serial.println(state);
    }

    if (state == "create_cin") {
    }
}

void setup() {
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(LED_GND_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, LOW);
    digitalWrite(LED_GND_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    
    //while (!Serial);
    Serial.begin(115200);
    delay(100);

    nCube.begin();
    nCube.setCallback(resp_callback, noti_callback);
    buildResource();

    state = "create_ae";

    TasCO2Sensor.init();
    TasCO2Sensor.setCallback(upload_callback);
}

void loop() {
    if (nCube.chkConnect()) {
        nCube.chkInitProvision();

        unsigned long currentMillis = millis();

        if (currentMillis - req_previousMillis >= req_interval) {
            req_previousMillis = currentMillis;
            publisher();
        }
        else if (currentMillis - sensing_previousMillis >= sensing_interval) {
            sensing_previousMillis = currentMillis;

            if (state == "create_cin") {
                // guide: in here generate sensing data
                // if get sensing data directly, assign curValue sensing data and set sensing_flag to 1
                // if request sensing data to sensor, set sensing_flag to 0, in other code of receiving sensing data, assign curValue sensing data and set sensing_flag to 1
                TasCO2Sensor.requestData();
                sensing_flag = 0;
            }
        }
        else {
            if (state == "create_cin") {
                if (sensing_flag == 1) {
                    rand_str(req_id, 8);
                    nCube.createCin(req_id, (nCube.resource[1].to + "/" + nCube.resource[1].rn), curValue);
                    sensing_flag = 0;
                }

                if (control_flag == 1) {
                    control_flag = 0;
                    
                    if (noti_con == "0") {
                      digitalWrite(LED_RED_PIN, LOW);
                      digitalWrite(LED_GREEN_PIN, LOW);
                      digitalWrite(LED_BLUE_PIN, LOW);
                    }
                    else if (noti_con == "1") {
                      digitalWrite(LED_RED_PIN, HIGH);
                      digitalWrite(LED_GREEN_PIN, LOW);
                      digitalWrite(LED_BLUE_PIN, LOW);
                    }
                    else if (noti_con == "2") {
                      digitalWrite(LED_RED_PIN, LOW);
                      digitalWrite(LED_GREEN_PIN, HIGH);
                      digitalWrite(LED_BLUE_PIN, LOW);
                    }
                    else if (noti_con == "3") {
                      digitalWrite(LED_RED_PIN, LOW);
                      digitalWrite(LED_GREEN_PIN, LOW);
                      digitalWrite(LED_BLUE_PIN, HIGH);
                    }
                    else if (noti_con == "4") {
                      digitalWrite(LED_RED_PIN, HIGH);
                      digitalWrite(LED_GREEN_PIN, HIGH);
                      digitalWrite(LED_BLUE_PIN, LOW);
                    }
                    else if (noti_con == "5") {
                      digitalWrite(LED_RED_PIN, HIGH);
                      digitalWrite(LED_GREEN_PIN, LOW);
                      digitalWrite(LED_BLUE_PIN, HIGH);
                    }
                    else if (noti_con == "6") {
                      digitalWrite(LED_RED_PIN, LOW);
                      digitalWrite(LED_GREEN_PIN, HIGH);
                      digitalWrite(LED_BLUE_PIN, HIGH);
                    }
                    else if (noti_con == "7") {
                      digitalWrite(LED_RED_PIN, HIGH);
                      digitalWrite(LED_GREEN_PIN, HIGH);
                      digitalWrite(LED_BLUE_PIN, HIGH);
                    }

                    String resp_body = "";
                    resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + resp_rqi + "\"}";
                    resp_body.toCharArray(body_buff, resp_body.length() + 1);
                    nCube.response(body_buff);
                }

                else if (control_flag == 2) {
                  control_flag = 0;

                  if (noti_con == "0") {
                    digitalWrite(RELAY_PIN, LOW);
                  }
                  else if (noti_con == "1") {
                    digitalWrite(RELAY_PIN, HIGH);
                  }

                  String resp_body = "";
                  resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + resp_rqi + "\"}";
                  resp_body.toCharArray(body_buff, resp_body.length() + 1);
                  nCube.response(body_buff);
                }
            }
        }

        TasCO2Sensor.chkCO2Data();
    }
    else {
        nCube.chkInitProvision2();
    }
}
