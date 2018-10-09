/**
 * created by       : Sudip Ghimire
 * email            : ghimiresdp@gmail.com
 * created on       : 2018/02/06
 * */

#include <Arduino.h>
#include <SoftwareSerial.h>
#define PIN_TX 10
#define PIN_RX 11
int ign = 8;
int pwrPin = 9;
uint32_t uid = 8;
SoftwareSerial simSerial(PIN_TX, PIN_RX);
//_____________________________________________________________________
char latitude[11];
char logitude[12];
char speedOTG[7];
//_____________________________________________________________________

void setup()
{
    simSerial.begin(9600);
    Serial.begin(9600);
    delay(1000);
    pinMode(pwrPin, OUTPUT);
    pinMode(ign, INPUT);
    checkSim();
}

//_____________________________________________________________________
//**************** Main Module*********************
//_____________________________________________________________________
void loop()
{
    if (initSim())
    {
        get_GPS();
        checkSMS();
        if (digitalRead(ign))
            uploadData();
        delay(1000);
    }
    else
        checkSim();
}
//_____________________________________________________________________
//**************** Power Device*********************
//_____________________________________________________________________
void powerDevice()
{
    digitalWrite(pwrPin, HIGH);
    delay(3000);
    digitalWrite(pwrPin, LOW);
    delay(2000);
}
//_____________________________________________________________________
//**************** Send AT Command*********************
//_____________________________________________________________________
String simTerm(String atcmd, int del)
{
    simSerial.println(atcmd);
    delay(del);
    if (simSerial.available())
        return simSerial.readString();
    else
        return "";
}
//_____________________________________________________________________
//**************** Initialize Module*********************
//_____________________________________________________________________
bool initSim()
{
    String rx = simTerm("AT", 100);
    if (rx.indexOf("OK") > 0)
        return true;
    else
        return false;
}
//_____________________________________________________________________
//****************| Check the status|*********************
//_____________________________________________________________________
void checkSim()
{
    if (!initSim())
    {
        delay(100);
        int count = 0;
        while (!initSim())
        {
            //Serial.println("Powering Device");
            powerDevice();
            if (initSim())
                break;
            count++;
            if (count == 10)
            {
                count = 0;
                for (int i = 0; i < 500; i++)
                {
                    Serial.println("GPS Device is Damaged !!");
                    delay(5000);
                }
            }
        }
    }
    String vars[] = {
        "AT+CGATT = 1",
        "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",
        "AT+SAPBR=3,1,\"APN\",\"Web\"",
        "AT+SAPBR=1,1", //3
        "AT+SAPBR=2,1",
        "AT+HTTPINIT",
        "AT+HTTPPARA=\"CID\",1",
        "AT+CGNSPWR=1"};
    //Serial.println("Device initialized successfully !!");
    for (int i = 0; i < 8; i++)
    {
        bool flag = true;
        int errcount = 0;
        while (flag)
        {
            String rx = simTerm(vars[i], 100);
            //Serial.println(rx); //only for debugging
            if (rx.indexOf("ERROR") > 0)
            {
                flag = true;
                Serial.println("error code: 0X0" + String(i));
                delay(1000);
                errcount++;
                if (errcount == 10)
                {
                    //Serial.println("Rebooting Device");
                    powerDevice();
                    break;
                }
            }
            else
                flag = false;
        }
        if (errcount >= 10)
        {
            break;
        }
    }
}
//_____________________________________________________________________
//**************** get GPS Data*********************
//_____________________________________________________________________
void get_GPS()
{
    char frame[150];
    char GNSSrunstatus[2];
    char Fixstatus[2];
    //char UTCdatetime[19];
    //char altitude[9];
    simSerial.println("AT+CGNSINF");
    int8_t counter, answer;
    long previous;
    //_____________________________________________________________________
    counter = 0;
    answer = 0;
    memset(frame, '\0', sizeof(frame));
    previous = millis();
    do
    {

        if (simSerial.available() != 0)
        {
            frame[counter] = simSerial.read();
            counter++;
            // check if the desired answer is in the response of the module
            if (strstr(frame, "OK") != NULL)
            {
                answer = 1;
            }
        }
        // Waits for the asnwer with time out
    } while ((answer == 0) && ((millis() - previous) < 2000));
    frame[counter - 3] = '\0';
    //Serial.println(String(frame));
    strtok_single(frame, " ");
    strcpy(GNSSrunstatus, strtok_single(NULL, ",")); // Gets GNSSrunstatus
    strcpy(Fixstatus, strtok_single(NULL, ","));     // Gets Fix status

    //strcpy(UTCdatetime, strtok_single(NULL, ","));   // Gets UTC date and time
    strtok_single(NULL, ","); // Gets UTC date and time

    strcpy(latitude, strtok_single(NULL, ",")); // Gets latitude
    strcpy(logitude, strtok_single(NULL, ",")); // Gets longitude

    //strcpy(altitude, strtok_single(NULL, ","));      // Gets MSL altitude
    strtok_single(NULL, ",");

    strcpy(speedOTG, strtok_single(NULL, ",")); // Gets speed over ground
}
//_____________________________________________________________________
//**************** Check the String Token *********************
//_____________________________________________________________________
static char *strtok_single(char *str, char const *delims)
{
    static char *src = NULL;
    char *p, *ret = 0;

    if (str != NULL)
        src = str;

    if (src == NULL || *src == '\0') // Fix 1
        return NULL;

    ret = src; // Fix 2
    if ((p = strpbrk(src, delims)) != NULL)
    {
        *p = 0;
        src = ++p;
    }
    else
        src += strlen(src);

    return ret;
}
//_____________________________________________________________________
//**************** Upload the data*********************
//_____________________________________________________________________
void uploadData()
{
    String baseURL = "http://sajiloschoolmanager.com/ios/api/vecloc";
    String uploadURL = String("AT+HTTPPARA=\"URL\",\"") + baseURL +
                       String("?") + String("uid=") + String(uid) +
                       String("&") + String("lat=") + String(latitude) +
                       String("&") + String("lng=") + String(logitude) +
                       String("&") + String("mod=") + String(digitalRead(ign)) +
                       String("&") + String("aspd=") + String(speedOTG) +
                       String("\"");
    Serial.println(uploadURL);
    String rx = simTerm(uploadURL, 100);
    if (rx.indexOf("OK") > 0)
    {
        String rx = simTerm("AT+HTTPACTION=0", 1000);
        Serial.println(rx);
    }
}
//_____________________________________________________________________
//**************** check if SMS arrived*********************
//_____________________________________________________________________
bool checkSMS()
{
    String rx = simTerm("AT+CMGF=1", 100);
    if (rx.indexOf("OK") > 0)
    {
        String rx = simTerm("AT+CMGR=1", 100);
        if (rx.indexOf("OK") > 12)
        {
            rx = rx.substring(12, rx.indexOf("OK") - 4);
            rx.toUpperCase();
            //Serial.println(rx);
            checkMessage(rx);
        }
    }
}

//_____________________________________________________________________
//**************** checkMessage*********************
//_____________________________________________________________________
void checkMessage(String data)
{
    if (data.indexOf("ET " > 0))
    {
        String phoneNumber = findPhoneNumber(data);
        data = data.substring(data.indexOf("ET "), data.length());
        String rx = simTerm("AT+CMGD=1", 100);
        //Serial.println("Message Cleared!!"); //comment later
        if (data.indexOf(" REBOOT") > 0)
        {
            Serial.println("Reboot message Received!!");
            sendSMS(phoneNumber, "Device is Rebooting Now");
            delay(3000);
            powerDevice();
        }
        else if (data.indexOf(" STATUS") > 0)
        {
            Serial.println("Status Message Received!!");
            String mode = "Parking";
            if (digitalRead(ign))
            {
                mode = "Active";
            }
            String message = String("Mode: ") + String(mode) +
                             String(", Latitude:") + String(latitude) +
                             String(", Longitude:") + String(logitude);
            sendSMS(phoneNumber, message);
        }
        else if (data.indexOf(" PANIC") > 0)
        {
            Serial.println("Panic Message Received!!");
            sendSMS(phoneNumber, "Panic Mode Enabled");
        }
        else if (data.indexOf(" NOPANIC") > 0)
        {
            Serial.println("No-panic Message Received!!");
            sendSMS(phoneNumber, "Panic Mode Disabled");
        }
    }
}
//_____________________________________________________________________
//**************** Find Phone Number *********************
//_____________________________________________________________________
String findPhoneNumber(String data)
{
    int pos = data.indexOf("READ") + 7;
    String phNo = data.substring(pos, data.length());
    phNo = data.substring(pos, data.indexOf("\",\"\""));
    return phNo;
}
//_____________________________________________________________________
//**************** send SMS*********************
//_____________________________________________________________________
void sendSMS(String phoneNumber, String message)
{
    String rx = simTerm("AT+CMGF=1", 100);
    if (rx.indexOf("OK") > 0)
    {
        rx = simTerm("AT+CMGS=\"" + phoneNumber + "\"", 100);
        simSerial.print(message);
        simSerial.write(0x1A); // sends ctrl+z end of message
        delay(1000);
        simSerial.write(0x0D); // Carriage Return in Hex
        delay(1000);
        simSerial.write(0x0A);
        delay(1000);
    }
    else
    {
        Serial.println("ErrorSending Message.");
    }
}
