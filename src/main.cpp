#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <secrets.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <freertos/semphr.h>
/*
Create secretes.h with the following structure:

#define WIFI_SSID <WIFI_SSID>
#define WIFI_PASSWORD <WIFI_PASSWORD>
#define DATABASE_URL <DATABASE_URL>
#define DATABASE_SECRET <DATABASE_SECRET>

*/

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// LED RGB PINS
#define LED_RED_PIN    21
#define LED_GREEN_PIN  22
#define LED_BLUE_PIN   23

/* FireBase Config vars */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Temperature PIN
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

float lastTemperatureRead = 0.0;

// Semaphore
SemaphoreHandle_t firebaseMutex;

void setColorRGB(bool r, bool g, bool b) {
  digitalWrite(LED_RED_PIN, r);
  digitalWrite(LED_GREEN_PIN, g);
  digitalWrite(LED_BLUE_PIN, b);
}

void TaskTemperature(void *pvParameters){
    while (true){           
      if (xSemaphoreTake(firebaseMutex, portMAX_DELAY)) {
        sensors.requestTemperatures(); 
        float temperatureC = sensors.getTempCByIndex(0);
        lastTemperatureRead = temperatureC;
        
        if (Firebase.setFloat(fbdo, "/temperature", temperatureC)){
          Serial.println("Success on sending data temp");
        } else {
          Serial.println("Fail on sending data temp");
        }
        
        Serial.print(temperatureC);
        Serial.println("ºC");
        
        xSemaphoreGive(firebaseMutex);
      }    
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void TaskReceiveTemperature(void *pvParameters){
  while (true){
    if (xSemaphoreTake(firebaseMutex, portMAX_DELAY)) {

      if (Firebase.getFloat(fbdo, "/setTemperature")) {
        float setTemp = fbdo.to<float>();
        Serial.print("Valor lido de /setTemperature: ");
        Serial.println(setTemp);

        if (abs(lastTemperatureRead - setTemp) < 1) {
          Serial.println("Temperatura igual! Piscando Led verde...");
          for (int i=0; i<0; i++){
            setColorRGB(LOW, HIGH, LOW); // Green
            vTaskDelay(pdMS_TO_TICKS(100));
            setColorRGB(LOW, LOW, LOW);  // OFF
            vTaskDelay(pdMS_TO_TICKS(100));
          }
        }

        // RGB LED color control
        if (setTemp >= 0 && setTemp < 12.5) {
          // Blue
          setColorRGB(LOW, LOW, HIGH);
        } else if (setTemp >= 12.5 && setTemp < 37.5) {
          // Pink (Red + Blue)
          setColorRGB(HIGH, LOW, HIGH);
        } else {
          // Red
          setColorRGB(HIGH, LOW, LOW);
        }
      } else {
        Serial.print("Erro ao ler /setTemperature: ");
        Serial.println(fbdo.errorReason());
      }

      xSemaphoreGive(firebaseMutex);
      
    }
    vTaskDelay(500 / portTICK_PERIOD_MS); // aguarda 500ms antes de ler de novo
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

  /* Initialize Temperature sensor */
  sensors.begin();

  /* Initialize Samaphore */
  firebaseMutex = xSemaphoreCreateMutex();

  /* Initialialize RGB Led*/
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  setColorRGB(LOW, HIGH, LOW); // Set green light on initialize


  xTaskCreatePinnedToCore(
      TaskTemperature,
      "TaskTemperature",
      8192,
      NULL,
      1,
      NULL,
      1
  );

  xTaskCreatePinnedToCore(
      TaskReceiveTemperature,
      "TaskReceiveTemperature",
      8192,
      NULL,
      1,
      NULL,
      1
  );
}

void loop() {}
