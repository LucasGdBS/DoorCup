#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <secrets.h>
#include <OneWire.h>
#include <DallasTemperature.h>
/*
Create secretes.h with the following structure:

#define WIFI_SSID <WIFI_SSID>
#define WIFI_PASSWORD <WIFI_PASSWORD>
#define DATABASE_URL <DATABASE_URL>
#define DATABASE_SECRET <DATABASE_SECRET>

*/

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* FireBase Config vars */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Temperature PIN
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 



void TaskTemperature(void *pvParameters){
    while (true){               
      sensors.requestTemperatures(); 
      float temperatureC = sensors.getTempCByIndex(0);

      if (Firebase.setFloat(fbdo, "/temperature", temperatureC)){
        Serial.println("Success on sending data temp");
      } else {
        Serial.println("Fail on sending data tempo");
      }

      Serial.print(temperatureC);
      Serial.println("ºC");
      
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


void setup()
{
  Serial.begin(115200);


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);


  /* Assign the database URL and database secret(required) */
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  /* Initialize the library with the Firebase authen and config */
  Firebase.begin(&config, &auth);

  xTaskCreatePinnedToCore(
      TaskTemperature,
      "TaskTemperature",
      8192,
      NULL,
      1,
      NULL,
      1
  );
}

void loop()
{

}
