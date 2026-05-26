#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define MAX_MACS 50

WebServer server(80);
Preferences prefs;

String macList[MAX_MACS];
int macCount = 0;

// =====================
// ตรวจสอบ MAC Format
// =====================
bool isValidMac(String mac)
{
    mac.toUpperCase();

    if(mac.length() != 17)
        return false;

    for(int i=0;i<17;i++)
    {
        if(i==2 || i==5 || i==8 || i==11 || i==14)
        {
            if(mac[i] != ':')
                return false;
        }
        else
        {
            if(!isxdigit(mac[i]))
                return false;
        }
    }

    return true;
}

// =====================
// เช็ค MAC ซ้ำ
// =====================
bool isDuplicate(String mac)
{
    for(int i=0;i<macCount;i++)
    {
        if(macList[i].equalsIgnoreCase(mac))
            return true;
    }

    return false;
}

// =====================
// Save ลง NVS
// =====================
void saveMacs()
{
    prefs.clear();

    prefs.putInt("count", macCount);

    for(int i=0;i<macCount;i++)
    {
        String key = "mac" + String(i);
        prefs.putString(key.c_str(), macList[i]);
    }
}

// =====================
// Load จาก NVS
// =====================
void loadMacs()
{
    macCount = prefs.getInt("count", 0);

    if(macCount > MAX_MACS)
        macCount = MAX_MACS;

    Serial.println();
    Serial.println("===== STORED MACS =====");

    for(int i=0;i<macCount;i++)
    {
        String key = "mac" + String(i);

        macList[i] =
            prefs.getString(key.c_str(), "");

        Serial.print(i + 1);
        Serial.print(" : ");
        Serial.println(macList[i]);
    }

    if(macCount == 0)
    {
        Serial.println("No MAC Stored");
    }

    Serial.println("=======================");
}

// =====================
// สร้างหน้าเว็บ
// =====================
String createPage(String msg = "")
{
    String html = R"rawliteral(

<!DOCTYPE html>
<html>
<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

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
}

button{
padding:10px 20px;
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

    if(msg.length())
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

    for(int i=0;i<macCount;i++)
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

// =====================
// Root Page
// =====================
void handleRoot()
{
    server.send(
        200,
        "text/html",
        createPage()
    );
}

// =====================
// Save MAC
// =====================
void handleSave()
{
    if(!server.hasArg("mac"))
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

    if(!isValidMac(mac))
    {
        server.send(
            200,
            "text/html",
            createPage("Invalid MAC Format")
        );
        return;
    }

    if(isDuplicate(mac))
    {
        server.send(
            200,
            "text/html",
            createPage("MAC Already Exists")
        );
        return;
    }

    if(macCount >= MAX_MACS)
    {
        server.send(
            200,
            "text/html",
            createPage("Memory Full")
        );
        return;
    }

    macList[macCount] = mac;
    macCount++;

    saveMacs();

    Serial.println();
    Serial.println("===== NEW MAC SAVED =====");
    Serial.println(mac);
    Serial.println("=========================");

    server.send(
        200,
        "text/html",
        createPage("Save Success")
    );
}

// =====================
// Setup
// =====================
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

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);

    server.begin();

    Serial.println("Web Server Started");
}

// =====================
// Loop
// =====================
void loop()
{
    server.handleClient();
}