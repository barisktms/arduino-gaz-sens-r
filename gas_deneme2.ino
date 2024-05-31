#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ESP32_MailClient.h"
 
const char* ssid = "ZAZA";        
const char* password = "123456789";   
 
#define emailSenderAccount    "arslan190512@gmail.com"    
#define emailSenderPassword   "nhzbkywtmqwxqfig"            
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "UYARI! Gaz Kaçağı Tespit Edildi"   

int buzzerPin = 19; 


String inputMessage = "ciphatay858@gmail.com";   
String enableEmailChecked = "checked";
String inputMessage2 = "true";
 

String inputMessage3 = "80.0";                    
String lastgaslevel;
 

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Gaz Seviyesi ile E-posta Bildirimi</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body{
      background-color: #90CAF9;
    }
    h2{
      background-color: yellow;
      color: black;
      text-align: center;
      font-weight:bold
      font-size: 40px;
    }
    h3{
      padding: 15px;
      text-align: center;
      font-size: 30px;
      font-weight: bold;
      border: solid 2px black;
    }
    p{
      text-align: center;
      font-weight: bold;
      font-size: 20px;
    }
    form{
      border: solid 2px black;
      text-align: center;
    }
    form #mail{
      font-weight: bold;
      margin: 8px;
      font-size: 15px;
    }
    form #bildirim{
      font-weight: bold;
      margin: 8px; 
      font-size: 15px;
    }
    form #esik{
      font-weight: bold;
      margin: 8px; 
      font-size: 15px;
    }
    #buton{
      cursor: pointer;
      background-color: #4CAF50;
      color: black;
      font-size: 16px;
      font-weight: bold;
      padding: 5px 7px;
      margin: 6px;
      text-decoration: none;
      border-radius: 6px;
    }
    #buton:hover{
      background-color: #EEFF41;
      font-weight: bold;
    }
  </style>
  </head><body>
  <h2>Gaz Seviyesi Tespiti</h2> 
  <p>Gaz Seviyesi</p>
  <h3>%GASVALUE%</h3>
  <br>
  <hr>
  <br>
  <h2>ESP E-posta Uyarisi</h2>
  <form action="/get">
    Mail Adresi: <input type="email" id="mail" name="email_input" value="%EMAIL_INPUT%" required><br>
    E-posta Bildirimini Etkinlestir <input type="checkbox" id="bildirim" name="enable_email_input" value="true" %ENABLE_EMAIL%><br>
    Gaz Seviyesi Esigi: <input type="number" id="esik" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br>
    <input type="submit" value="Gonder" id="buton">
  </form>
</body></html>)rawliteral";
 
void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}
AsyncWebServer server(80);
 
String processor(const String& var)
{
  if(var == "GASVALUE")
  {
    return lastgaslevel;
  }
  else if(var == "EMAIL_INPUT")
  {
    return inputMessage;
  }
  else if(var == "ENABLE_EMAIL")
  {
    return enableEmailChecked;
  }
  else if(var == "THRESHOLD")
  {
    return inputMessage3;
  }
  return String();
}
 
 
bool emailSent = false;
const char* PARAM_INPUT_1 = "email_input";
const char* PARAM_INPUT_2 = "enable_email_input";
const char* PARAM_INPUT_3 = "threshold_input";
 
unsigned long previousMillis = 0;     
const long interval = 5000;    
 
SMTPData smtpData;
 
 
void setup() 
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      if (request->hasParam(PARAM_INPUT_2)) {
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
        enableEmailChecked = "checked";
      }
      else 
      {
        inputMessage2 = "false";
        enableEmailChecked = "";
      }
      if (request->hasParam(PARAM_INPUT_3)) {
        inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
      }
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    Serial.println(inputMessage2);
    Serial.println(inputMessage3);
    request->send(200, "text/html", "HTTP GET istegi ESP'nize gonderildi.<br><a href=\"/\">Ana Sayfaya Don</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}
 
 
 
void loop() 
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
 
    float gas_analog_value = analogRead(35);
    float gas_value = ((gas_analog_value/1023)*100);
    Serial.print(gas_analog_value);
    Serial.print(", ");
    Serial.println(gas_value);
    
    lastgaslevel = String(gas_value);
    
    if(gas_value > inputMessage3.toFloat() && inputMessage2 == "true" && !emailSent){
      String emailMessage = String("Gaz Seviyesi eşiğin üzerinde. Mevcut Gaz Seviyesi: " + String(gas_value));
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }    
    }
    else if((gas_value < inputMessage3.toFloat()) && inputMessage2 == "true" && emailSent) 
    {
      String emailMessage = String("Gaz Seviyesi eşiğin üzerinde. Mevcut Gaz Seviyesi: ") + 
                            String(gas_value) + String(" C");
      if(sendEmailNotification(emailMessage)) 
      {
        Serial.println(emailMessage);
        emailSent = false;
      }
      else {
        Serial.println("Email failed to send");
      }
    }

    if(gas_value > inputMessage3.toFloat()) {      // Sensörden okunan değer eşik değerinden büyükse çalışır.
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
    }
    else {                       // Sensörden okunan değer eşik değerinin altındaysa çalışır.
    digitalWrite(buzzerPin, LOW);
    }
  }

}
 
 
bool sendEmailNotification(String emailMessage)
{
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
  
  smtpData.setSender("ESP32_Gas_Alert_Mail", emailSenderAccount);
  
  smtpData.setPriority("High");
  
  smtpData.setSubject(emailSubject);
  
  smtpData.setMessage(emailMessage, true);
  
  smtpData.addRecipient(inputMessage);
  smtpData.setSendCallback(sendCallback);
  
  if (!MailClient.sendMail(smtpData)) 
{
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
 
  smtpData.empty();
  return true;
}
 
 
void sendCallback(SendStatus msg) 
{
  Serial.println(msg.info());
  
  if (msg.success()) 
{
    Serial.println("----------------");
  }
}
