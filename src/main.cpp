#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>


// *************************************************
// Definitions
// *************************************************
#define WMODEOFF 0  // OFF-Betriebsart, Pumpe läuft nie, außer bei Boost
#define WMODEON 1   // ON-Betriebsart, Pumpe läuft dauerhaft
#define WMODEAUTO 2 // AUTO-Betriebsart, Pumpe läuft mit Temperatursteuerung
#define WMODEONCE 3 // ONCE-Betriebsart, einmalige Zieltemperaturerreichung, dann vorheriger Modus

#define SMODEBASE 0          // Grundmodus
#define SMODETIMERACTIVE 1   // Warten auf Pumpenaktivierung
#define SMODEPUMPEAKTIV 2    // Pumpe aktiviert, Minimalpumpdauer aktiv
#define SMODEPUMPENACHLAUF 3 // Pumpe aktiviert, warten auf Temperaturerreichung
#define SMODEPUMPENSCHUTZ 4  // Pumpe deaktiviert, Schutzzeit bis nächste Aktivierung

#define CALCMODETARGET 0     // Abschaltung durch Zieltemperatur Rücklauf
#define CALCMODEDIFFERENCE 1 // Abschaltung durch Temperaturunterschied Vorlauf Rücklauf


// *************************************************
// Globals
// *************************************************
float temp_vorlauf, temp_ruecklauf, temp_raum;
Ticker ticker_display;
bool display_refresh = false;
Ticker ticker_temperatures;
bool temperatures_refresh = true;
bool switch1_state;

int working_mode = WMODEAUTO;      // Standardbetriebsmodus == AUTO
int steering_mode = SMODEBASE;     // Steuerungsmodus == SMODEBASE
int last_working_mode = WMODEAUTO; // Merker für ONCE Betriebsmodus

static unsigned long tsPumpenaktivierung = 0;
static unsigned long tsPumpendeaktivierung = 0;


// *************************************************
// Init: Temperature sensors
// *************************************************
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D4
#define TEMPERATURE_PRECISION 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress sensor_vorlauf, sensor_ruecklauf, sensor_raum;


// *************************************************
// Init: OLED display (SSD1306)
// *************************************************
#include <SSD1306.h>
SSD1306 display(0x3c, D2, D1);


// *************************************************
// Init: RC-Switch (433 MHz sender)
// *************************************************
#include <RCSwitch.h>
RCSwitch rc_switch = RCSwitch();


// *************************************************
// Init: WiFiManager
// *************************************************
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>


// *************************************************
// Init: ESPWebServer
// *************************************************
ESP8266WebServer server(80);


// *************************************************
// Init: ESP_EEPROM
// *************************************************
#include <ESP_EEPROM.h>
struct EEPROMSettingsStruct
{
    int calc_mode;
    float RUECKTARGETTEMP;
    float TARGETDIFFERENCE;
    unsigned long MINIMALPUMPDAUER;
    unsigned long MAXIMALPUMPDAUER;
    unsigned long PUMPENSCHUTZZEIT;
} settingsvar;


// *************************************************
// Helper functions
// *************************************************
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // zero pad the address if necessary
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}


// *************************************************
// Temperature functions
// *************************************************
void tickerUpdateTemperatures()
{
    temperatures_refresh = true;
}

// Get temperatures, store them in global variables
void getTemperatures()
{
    sensors.requestTemperatures();
    float tempC;

    // Vorlauf
    tempC = sensors.getTempC(sensor_vorlauf);
    if (temp_vorlauf != tempC)
    {
        temp_vorlauf = tempC;
    }

    // Rücklauf
    tempC = sensors.getTempC(sensor_ruecklauf);
    if (temp_ruecklauf != tempC)
    {
        temp_ruecklauf = tempC;
    }

    // Raum
    tempC = sensors.getTempC(sensor_raum);
    if (temp_raum != tempC)
    {
        temp_raum = tempC;
    }

    temperatures_refresh = false;
}


// *************************************************
// Display functions
// *************************************************
void tickerUpdateDisplay()
{
    display_refresh = true;
}

void updateDisplay()
{
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    // Print temperatures
    display.drawString(0, 0, "V:" + String(temp_vorlauf));
    display.drawString(0, 20, "R:" + String(temp_ruecklauf));
    display.drawString(0, 40, "A:" + String(temp_raum));

    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    // Print working mode
    String wmode;
    if (working_mode == WMODEOFF)
    {
        wmode = "Off";
    }
    if (working_mode == WMODEON)
    {
        wmode = "On";
    }
    if (working_mode == WMODEAUTO)
    {
        wmode = "Auto";
    }
    if (working_mode == WMODEONCE)
    {
        wmode = "Once";
    }
    display.drawString(128, 0, wmode);

    // Print relais mode
    if (switch1_state)
    {
        display.drawString(128, 20, "<*>");
    }
    else
    {
        display.drawString(128, 20, "< >");
    }

    // Print steering mode
    display.drawString(128, 40, String(steering_mode));

    display.display();
    display_refresh = false;

    // TODO: Show WiFi.RSSI()
}

void showMessage(String line1, String line2, String line3)
{
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);

    if (line3.length() > 0)
    {
        display.drawString(64, 0, String(line1));
        display.drawString(64, 20, String(line2));
        display.drawString(64, 40, String(line3));
    }
    else
    {
        display.drawString(64, 0, String(line1));
        display.drawString(64, 32, String(line2));
    }
    display.display();
}


// *************************************************
// RC Switch functions
// *************************************************
void setRCSwitchState(bool state)
{
    if (switch1_state != state)
    {
        switch1_state = state;
        if (state)
        {
            // Switch ON
            rc_switch.switchOn("10000", "01000");
        }
        else
        {
            // Switch OFF
            rc_switch.switchOff("10000", "01000");
        }
    }
}


// *************************************************
// WiFi functions
// *************************************************
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    showMessage("Config mode", myWiFiManager->getConfigPortalSSID(), "");
}

String getLocalIP()
{
    String ipString = String(WiFi.localIP()[0]);
    for (byte octet = 1; octet < 4; ++octet)
    {
        ipString += '.' + String(WiFi.localIP()[octet]);
    }
    return ipString;
}


// *************************************************
// Steering functions
// *************************************************
void statemachine()
{
    if (working_mode == WMODEOFF || working_mode == WMODEON)
    {
        // Steuerungsmodus in Grundzustand zurücksetzen
        steering_mode = SMODEBASE;

        // Betriebsmodus OFF
        if (working_mode == WMODEOFF)
        {
            setRCSwitchState(false); // Turns switch 1 off
        }

        // Betriebsmodus ON
        if (working_mode == WMODEON)
        {
            setRCSwitchState(true); // Turns switch 1 on
        }
    }

    // Betriebsmodus AUTO oder ONCE
    if (working_mode == WMODEAUTO || working_mode == WMODEONCE)
    {

        // Steuerung im Grundzustand, entscheiden was zu tun ist
        if (steering_mode == SMODEBASE)
        {
            if (temp_ruecklauf < settingsvar.RUECKTARGETTEMP)
            {
                // Rücklauftemperatur nicht ausreichend, Durchlauf starten
                steering_mode = SMODEPUMPEAKTIV;
                tsPumpenaktivierung = millis();
                setRCSwitchState(true);
            }
            else
            {
                // Temperatur ausreichend, warten
                steering_mode = SMODETIMERACTIVE;
                setRCSwitchState(false);
            }
        }

        // Warten auf Pumpenaktivierung
        if (steering_mode == SMODETIMERACTIVE)
        {
            if (temp_ruecklauf < settingsvar.RUECKTARGETTEMP)
            {
                // Rücklauftemperatur nicht ausreichend, Durchlauf starten
                steering_mode = SMODEPUMPEAKTIV;
                tsPumpenaktivierung = millis();
                setRCSwitchState(true);
            }
        }

        // Pumpe aktiv, warten auf Ablauf Minimalpumpdauer
        if (steering_mode == SMODEPUMPEAKTIV)
        {
            if (tsPumpenaktivierung + settingsvar.MINIMALPUMPDAUER <= millis())
            {
                // Minimalpumpdauer abgelaufen
                steering_mode = SMODEPUMPENACHLAUF;
            }
        }

        // Pumpe aktiv, Minimalpumpdauer abgelaufen, warten auf Temperaturerreichung, oder MAXIMALPUMPDAUER
        if (steering_mode == SMODEPUMPENACHLAUF)
        {
            bool tempErreicht = false;

            if (settingsvar.calc_mode == CALCMODETARGET)
            {
                tempErreicht = (temp_ruecklauf >= settingsvar.RUECKTARGETTEMP) ? true : false;
            }
            if (settingsvar.calc_mode == CALCMODEDIFFERENCE)
            {
                float tempDiff = temp_ruecklauf - temp_vorlauf;
                tempErreicht = (abs(tempDiff) <= settingsvar.TARGETDIFFERENCE) ? true : false;
            }

            if (tempErreicht || tsPumpenaktivierung + settingsvar.MAXIMALPUMPDAUER <= millis())
            {
                // Temperatur erreicht, Pumpe abschalten
                setRCSwitchState(false);
                steering_mode = SMODEPUMPENSCHUTZ;
                tsPumpenaktivierung = 0;
                tsPumpendeaktivierung = millis();

                if (working_mode == WMODEONCE)
                {
                    // Alten Betriebsmodus wiederherstellen
                    working_mode = last_working_mode;
                    Serial.println("Wiederherstellung last working mode: ");
                    Serial.println(last_working_mode);
                }
            }
        }

        // Pumpe inaktiv, Schutzzeit bis nächste Aktivierung
        if (steering_mode == SMODEPUMPENSCHUTZ)
        {
            if (tsPumpendeaktivierung + settingsvar.PUMPENSCHUTZZEIT <= millis())
            {
                // Pumpenschutzzeit abgelaufen
                steering_mode = SMODETIMERACTIVE;
            }
        }
    }
}


// *************************************************
// WebServer functions
// *************************************************
void commitSettings()
{
    EEPROM.put(0, settingsvar);

    // write the data to EEPROM
    boolean ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
}

void sendStatusJSON()
{
    String response = "{";
    response += "temp_vorlauf:" + String(temp_vorlauf) + ",";
    response += "temp_ruecklauf:" + String(temp_ruecklauf) + ",";
    response += "temp_raum:" + String(temp_raum) + ",";
    response += "working_mode:" + String(working_mode) + ",";
    response += "last_working_mode:" + String(last_working_mode) + ",";
    response += "steering_mode:" + String(steering_mode) + ",";
    response += "calc_mode:" + String(settingsvar.calc_mode) + ",";
    response += "switch1_state:" + String(switch1_state) + ",";
    response += "knock_state:0,";
    response += "RUECKTARGETTEMP:" + String(settingsvar.RUECKTARGETTEMP) + ",";
    response += "TARGETDIFFERENCE:" + String(settingsvar.TARGETDIFFERENCE) + ",";
    response += "MINIMALPUMPDAUER:" + String(settingsvar.MINIMALPUMPDAUER) + ",";
    response += "MAXIMALPUMPDAUER:" + String(settingsvar.MAXIMALPUMPDAUER) + ",";
    response += "PUMPENSCHUTZZEIT:" + String(settingsvar.PUMPENSCHUTZZEIT) + "";
    response += "}";
    server.send(200, "application/json", response);
}

String getValueFromQuery()
{
    if (server.args() > 0)
    {
        return String(server.arg(0));
    }
}


// *************************************************
// SETUP
// *************************************************
void setup()
{
    // start serial port
    Serial.begin(9600);
    Serial.println("Pumpduino v2");


    // ========================================
    // Initializing OLED display
    // ========================================
    display.init();
    // display.flipScreenVertically();
    display.setContrast(255);
    showMessage("Pumpduino v2", "Connecting ...", "");
    delay(1000);

    // Init ticker for display update
    ticker_display.attach(1.0, tickerUpdateDisplay);


    // ========================================
    // Initializing RS Switch
    // ========================================
    // Transmitter is connected to pin D3
    rc_switch.enableTransmit(D3);


    // ========================================
    // Initializing OneWire temperature sensors
    // ========================================
    ticker_temperatures.attach(2.0, tickerUpdateTemperatures);

    Serial.println("Init Sensors");
    sensors.begin();

    // locate devices on the bus
    Serial.print("Locating devices...");
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");

    if (!sensors.getAddress(sensor_vorlauf, 0))
    {
        Serial.println("Unable to find address for Device 0 (Vorlauf)");
    }
    Serial.print("Device 0 (Vorlauf) Address: ");
    printAddress(sensor_vorlauf);
    Serial.println();

    if (!sensors.getAddress(sensor_ruecklauf, 1))
    {
        Serial.println("Unable to find address for Device 1 (Rücklauf)");
    }
    Serial.print("Device 1 (Rücklauf) Address: ");
    printAddress(sensor_ruecklauf);
    Serial.println();

    if (!sensors.getAddress(sensor_raum, 2))
    {
        Serial.println("Unable to find address for Device 2 (Raum)");
    }
    Serial.print("Device 2 (Raum) Address: ");
    printAddress(sensor_raum);
    Serial.println();

    // set the resolution to 9 bit
    sensors.setResolution(sensor_vorlauf, TEMPERATURE_PRECISION);
    sensors.setResolution(sensor_ruecklauf, TEMPERATURE_PRECISION);
    sensors.setResolution(sensor_raum, TEMPERATURE_PRECISION);

    Serial.print("Device 0 (Vorlauf) Resolution: ");
    Serial.print(sensors.getResolution(sensor_vorlauf), DEC);
    Serial.println();

    Serial.print("Device 1 (Rücklauf) Resolution: ");
    Serial.print(sensors.getResolution(sensor_ruecklauf), DEC);
    Serial.println();

    Serial.print("Device 2 (Raum) Resolution: ");
    Serial.print(sensors.getResolution(sensor_raum), DEC);
    Serial.println();


    // ========================================
    // Initializing WiFiManager
    // ========================================
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();

    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    if (!wifiManager.autoConnect("PumpduinoAP"))
    {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    showMessage("Connected ...", getLocalIP(), "");
    WiFi.hostname("pumpduino");
    delay(2000);


    // *************************************************
    // Init: ESP_EEPROM
    // *************************************************
    EEPROM.begin(sizeof(EEPROMSettingsStruct));

    if (EEPROM.percentUsed() >= 0)
    {
        EEPROM.get(0, settingsvar);
        Serial.println("EEPROM has data from a previous run.");
        Serial.print(EEPROM.percentUsed());
        Serial.println("% of ESP flash space currently used");
    }
    else
    {
        Serial.println("No EEPROM settings so far, using defaults.");
        settingsvar.calc_mode = CALCMODETARGET;
        settingsvar.RUECKTARGETTEMP = 35.0;
        settingsvar.TARGETDIFFERENCE = 7.0;
        settingsvar.MINIMALPUMPDAUER = 60000;   //  1 Minute
        settingsvar.MAXIMALPUMPDAUER = 1200000; // 20 Minuten
        settingsvar.PUMPENSCHUTZZEIT = 1800000; // 30 Minuten
    }


    // ========================================
    // Initializing WebServer
    // ========================================
    server.onNotFound([]() {
        server.send(404, "text/plain", "404 not found");
    });
    server.on("/", []() {
        sendStatusJSON();
    });
    server.on("/get", []() {
        sendStatusJSON();
    });
    server.on("/setonce", []() {
        working_mode = WMODEONCE;
        sendStatusJSON();
    });
    server.on("/seton", []() {
        working_mode = WMODEON;
        last_working_mode = working_mode;
        sendStatusJSON();
    });
    server.on("/setoff", []() {
        working_mode = WMODEOFF;
        last_working_mode = working_mode;
        sendStatusJSON();
    });
    server.on("/setauto", []() {
        working_mode = WMODEAUTO;
        last_working_mode = working_mode;
        sendStatusJSON();
    });
    server.on("/setcalc", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            int val = value.toInt();
            if (val == CALCMODETARGET || val == CALCMODEDIFFERENCE)
            {
                settingsvar.calc_mode = val;
                Serial.printf("calc_mode: %d\n", settingsvar.calc_mode);
                commitSettings();
            }
        }
        sendStatusJSON();
    });
    server.on("/settt", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            settingsvar.RUECKTARGETTEMP = value.toFloat();
            Serial.printf("RUECKTARGETTEMP: %f\n", settingsvar.RUECKTARGETTEMP);
            commitSettings();
        }
        sendStatusJSON();
    });
    server.on("/setminp", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            settingsvar.MINIMALPUMPDAUER = value.toFloat();
            Serial.printf("MINIMALPUMPDAUER: %f\n", settingsvar.MINIMALPUMPDAUER);
            commitSettings();
        }
        sendStatusJSON();
    });
    server.on("/setmaxp", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            settingsvar.MAXIMALPUMPDAUER = value.toFloat();
            Serial.printf("MAXIMALPUMPDAUER: %f\n", settingsvar.MAXIMALPUMPDAUER);
            commitSettings();
        }
        sendStatusJSON();
    });
    server.on("/setpumps", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            settingsvar.PUMPENSCHUTZZEIT = value.toFloat();
            Serial.printf("PUMPENSCHUTZZEIT: %f\n", settingsvar.PUMPENSCHUTZZEIT);
            commitSettings();
        }
        sendStatusJSON();
    });
    server.on("/settdiff", []() {
        String value = "" + server.arg("value");
        if (value != "")
        {
            settingsvar.TARGETDIFFERENCE = value.toFloat();
            Serial.printf("TARGETDIFFERENCE: %f\n", settingsvar.TARGETDIFFERENCE);
            commitSettings();
        }
        sendStatusJSON();
    });

    server.begin();
    Serial.println("HTTP server started");
}


// *************************************************
// LOOP
// *************************************************
void loop()
{
    if (temperatures_refresh)
    {
        getTemperatures();
    }

    statemachine();

    server.handleClient();

    if (display_refresh)
    {
        updateDisplay();
    }
}
