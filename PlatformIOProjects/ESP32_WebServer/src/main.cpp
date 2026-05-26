#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_now.h>

#define MAX_MACS 50

WebServer server(80);
Preferences prefs;

// =====================
// เก็บ MAC แบบ String
// =====================
String macList[MAX_MACS];

// =====================
// เก็บ MAC แบบ Byte Array
// =====================
uint8_t macArray[MAX_MACS][6];

int macCount = 0;

// =====================================================
// แปลง MAC String -> Byte Array
// =====================================================
bool macStringToBytes(String mac, uint8_t *bytes)
{
    int values[6];

    if (sscanf(mac.c_str(),
               "%x:%x:%x:%x:%x:%x",
               &values[0],
               &values[1],
               &values[2],
               &values[3],
               &values[4],
               &values[5]) != 6)
    {
        return false;
    }

    for (int i = 0; i < 6; i++)
    {
        bytes[i] = (uint8_t)values[i];
    }

    return true;
}

// =====================================================
// ตรวจสอบ MAC Format
// =====================================================
bool isValidMac(String mac)
{
    mac.toUpperCase();

    if (mac.length() != 17)
        return false;

    for (int i = 0; i < 17; i++)
    {
        if (i == 2 || i == 5 || i == 8 ||
            i == 11 || i == 14)
        {
            if (mac[i] != ':')
                return false;
        }
        else
        {
            if (!isxdigit(mac[i]))
                return false;
        }
    }

    return true;
}

// =====================================================
// เช็ค MAC ซ้ำ
// =====================================================
bool isDuplicate(String mac)
{
    for (int i = 0; i < macCount; i++)
    {
        if (macList[i].equalsIgnoreCase(mac))
            return true;
    }

    return false;
}

// =====================================================
// Save ลง NVS
// =====================================================
void saveMacs()
{
    prefs.clear();

    prefs.putInt("count", macCount);

    for (int i = 0; i < macCount; i++)
    {
        String key = "mac" + String(i);

        prefs.putString(
            key.c_str(),
            macList[i]
        );
    }
}

// =====================================================
// Load จาก NVS
// =====================================================
void loadMacs()
{
    macCount = prefs.getInt("count", 0);

    if (macCount > MAX_MACS)
        macCount = MAX_MACS;

    Serial.println();
    Serial.println("===== STORED MACS =====");

    for (int i = 0; i < macCount; i++)
    {
        String key = "mac" + String(i);

        macList[i] =
            prefs.getString(key.c_str(), "");

        // แปลงกลับเป็น byte array
        macStringToBytes(
            macList[i],
            macArray[i]
        );

        Serial.print(i + 1);
        Serial.print(" : ");
        Serial.println(macList[i]);
    }

    if (macCount == 0)
    {
        Serial.println("No MAC Stored");
    }

    Serial.println("=======================");
}

// =====================================================
// สร้างหน้าเว็บ
// =====================================================
String createPage(String msg = "")
{
    String html = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>ESP32 MAC Config</title>

<style>

body{
font-family:Arial;
background:#f4f4f4;
text-align:center;
padding-top:50px;
}

.card{
width:400px;
margin:auto;
background:white;
padding:25px;
border-radius:10px;
box-shadow:0 0 10px rgba(0,0,0,0.2);
}

input{
width:280px;
padding:10px;
font-size:16px;
}

button{
padding:10px 20px;
font-size:16px;
}

.msg{
color:green;
font-weight:bold;
}

.list{
text-align:left;
margin-top:20px;
font-size:14px;
}

</style>

</head>

<body>

<div class="card">

<h2>ESP32 MAC Config</h2>

)rawliteral";

    if (msg.length())
    {
        html += "<p class='msg'>";
        html += msg;
        html += "</p>";
    }

    html += R"rawliteral(

<form action="/save" method="POST">

<input
type="text"
name="mac"
placeholder="AA:BB:CC:DD:EE:FF">

<br><br>

<button type="submit">
Save
</button>

</form>

)rawliteral";

    html += "<p>Total MAC : ";
    html += String(macCount);
    html += "</p>";

    html += "<div class='list'>";

    for (int i = 0; i < macCount; i++)
    {
        html += String(i + 1);
        html += ". ";
        html += macList[i];
        html += "<br>";
    }

    html += "</div>";
    html += "</div></body></html>";

    return html;
}

// =====================================================
// Root Page
// =====================================================
void handleRoot()
{
    server.send(
        200,
        "text/html",
        createPage()
    );
}

// =====================================================
// Save MAC
// =====================================================
void handleSave()
{
    if (!server.hasArg("mac"))
    {
        server.send(
            400,
            "text/plain",
            "No MAC"
        );
        return;
    }

    String mac = server.arg("mac");

    mac.trim();
    mac.toUpperCase();

    // ตรวจสอบ format
    if (!isValidMac(mac))
    {
        server.send(
            200,
            "text/html",
            createPage("Invalid MAC Format")
        );
        return;
    }

    // ตรวจสอบซ้ำ
    if (isDuplicate(mac))
    {
        server.send(
            200,
            "text/html",
            createPage("MAC Already Exists")
        );
        return;
    }

    // ตรวจสอบเต็ม
    if (macCount >= MAX_MACS)
    {
        server.send(
            200,
            "text/html",
            createPage("Memory Full")
        );
        return;
    }

    // เก็บ String
    macList[macCount] = mac;

    // แปลงเก็บเป็น byte array
    macStringToBytes(
        mac,
        macArray[macCount]
    );

    macCount++;

    saveMacs();

    Serial.println();
    Serial.println("===== NEW MAC SAVED =====");
    Serial.println(mac);

    // แสดงข้อมูล byte array
    Serial.println("===== BYTE ARRAY =====");

    for (int i = 0; i < macCount; i++)
    {
        Serial.print("MAC ");
        Serial.print(i + 1);
        Serial.print(" : ");

        for (int j = 0; j < 6; j++)
        {
            if (macArray[i][j] < 16)
                Serial.print("0");

            Serial.print(
                macArray[i][j],
                HEX
            );

            if (j < 5)
                Serial.print(":");
        }

        Serial.println();
    }

    Serial.println("=========================");

    server.send(
        200,
        "text/html",
        createPage("Save Success")
    );
}

// =====================================================
// Setup
// =====================================================
void setup()
{
    Serial.begin(115200);

    prefs.begin("macdb", false);

    loadMacs();

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        "ESP32_MAC_CONFIG",
        "12345678"
    );

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 Access Point Started");
    Serial.println("SSID : ESP32_MAC_CONFIG");
    Serial.println("Password : 12345678");
    Serial.print("IP : ");
    Serial.println(WiFi.softAPIP());
    Serial.println("================================");

    // Web Routes
    server.on("/", HTTP_GET, handleRoot);

    server.on(
        "/save",
        HTTP_POST,
        handleSave
    );

    server.begin();

    Serial.println("Web Server Started");
}

// =====================================================
// Loop
// =====================================================
void loop()
{
    server.handleClient();

    // ตัวอย่างดึง MAC ไปใช้
    // เช่น ส่ง ESP-NOW

    /*
    if(macCount > 0)
    {
        esp_now_send(
            macArray[0],
            (uint8_t*)"HELLO",
            5
        );
    }
    */
}