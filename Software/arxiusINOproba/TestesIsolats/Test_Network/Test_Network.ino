// ==========================================================================
// Test_Network.ino — Programa de Prova de WiFi i Client REST de Supabase
// Projecte Integrat II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC
// ==========================================================================
// FASE 6 (Connexió i Xarxa):
//   - Connecta a la xarxa local amb les credencials de config.h.
//   - Realitza un GET a Supabase per rebre la configuració i horaris de dispositiu.
//   - Realitza un PATCH a Supabase per actualitzar el batec de cor (heartbeat).
// ==========================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Incloem el fitxer de configuració global per heretar les credencials de xarxa i API
#include <config.h>

WiFiClientSecure netClient;

// Capçaleres de Supabase
static const char* HDR_APIKEY = "apikey";
static const char* HDR_AUTH   = "Authorization";

void setup() {
  Serial.begin(115200);
  while (!Serial); // Espera pel Monitor Sèrie
  delay(500);

  Serial.println("\n========================================================");
  Serial.println("   TEST AÏLLAT: CONNEXIÓ WIFI I API REST SUPABASE     ");
  Serial.println("========================================================");
  Serial.printf(" SSID WiFi:      %s\n", WIFI_SSID);
  Serial.printf(" Supabase URL:   %s\n", SUPABASE_URL);
  Serial.printf(" Device UUID:    %s\n", DEVICE_ID);
  Serial.println("========================================================\n");

  // Configura el client TLS per ometre la validació del certificat en el test aïllat
  netClient.setInsecure();

  // 1. Connexió a la xarxa WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("[WIFI] Connectant a %s ", WIFI_SSID);
  uint32_t startMs = millis();
  bool connectat = true;
  
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startMs > WIFI_CONNECT_TIMEOUT_MS) {
      connectat = false;
      break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (connectat) {
    Serial.printf("[WIFI] [OK] Connectat correctament! Adreça IP local: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WIFI] Potència de senyal (RSSI): %d dBm\n\n", WiFi.RSSI());
    
    // Executem les proves de connexió a l'API
    executarProvesSupabase();
  } else {
    Serial.println("[WIFI] [ERROR] No s'ha pogut connectar a la xarxa WiFi.");
    Serial.println("-> Revisa que el nom de xarxa (SSID) i la contrasenya a 'config.h' siguin correctes.");
    Serial.println("-> Comprova que hi hagi bona cobertura i que la xarxa sigui de 2.4GHz (l'ESP32 no suporta 5GHz).");
  }

  Serial.println("\n[TEST] Prova finalitzada. Pots prémer el botó RESET de l'ESP32 per repetir-la.");
}

void loop() {
  // El loop es manté buit ja que tot el procés es fa un cop al setup()
  delay(1000);
}

/**
 * Aplica les capçaleres de seguretat requerides per Supabase
 */
void aplicarCapcaleresSupabase(HTTPClient& http, bool patch = false) {
  http.addHeader(HDR_APIKEY, SUPABASE_ANON_KEY);
  http.addHeader(HDR_AUTH, String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  if (patch) {
    http.addHeader("Prefer", "return=minimal");
  }
}

/**
 * Executa les peticions GET i PATCH de prova a l'API de Supabase
 */
void executarProvesSupabase() {
  Serial.println("========================================================");
  Serial.println("  INICIANT LECTURA / ESCRIPTURA AMB SUPABASE ");
  Serial.println("========================================================");

  // ------------------------------------------------------------------
  // PROVA 1: GET - Obtenir configuració del dispositiu (device_config)
  // ------------------------------------------------------------------
  Serial.println("\n[PROVA 1] Realitzant petició GET per obtenir configuració i horaris...");
  HTTPClient httpGet;
  String urlGet = String(SUPABASE_URL) + "/rest/v1/device_config?device_id=eq." + DEVICE_ID + "&select=schedules_json,calibration_factor&limit=1";
  
  httpGet.begin(netClient, urlGet);
  aplicarCapcaleresSupabase(httpGet);
  
  int codiHttpGet = httpGet.GET();
  Serial.printf("[GET] Resposta HTTP Codi: %d\n", codiHttpGet);

  if (codiHttpGet == 200) {
    String resposta = httpGet.getString();
    Serial.println("[GET] [OK] Dades de configuració rebudes correctament:");
    Serial.println(resposta);

    // Masteguem el JSON per provar la llibreria ArduinoJson
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, resposta);
    
    if (!error) {
      if (doc.is<JsonArray>() && doc.size() > 0) {
        JsonObject config = doc[0];
        Serial.println("-> JSON decodificat correctament.");
        
        // Calibració
        if (config.containsKey("calibration_factor")) {
          float calibration = config["calibration_factor"] | 0.0f;
          Serial.printf("-> Factor de calibració remot: %.2f\n", calibration);
        }
        
        // Horaris
        if (config.containsKey("schedules_json")) {
          JsonArray schedules = config["schedules_json"].as<JsonArray>();
          Serial.printf("-> S'han detectat %u horaris programats al núvol.\n", schedules.size());
          
          for (int i = 0; i < schedules.size(); i++) {
            JsonObject s = schedules[i];
            bool enabled = s["enabled"] | false;
            int hour = s["hour"] | 0;
            int minute = s["minute"] | 0;
            float portion = s["portion_grams"] | 0.0f;
            Serial.printf("   [%d] Hora: %02d:%02d | Porció: %.1f g | Estat: %s\n", 
                          i + 1, hour, minute, portion, enabled ? "ACTIU" : "INACTIU");
          }
        }
      } else {
        Serial.println("[GET] [AVÍS] No s'ha trobat cap configuració a la taula per a aquest UUID.");
        Serial.println("-> Assegura't de tenir creat un registre a 'device_config' amb el teu DEVICE_ID.");
      }
    } else {
      Serial.printf("[GET] [ERROR] Error al decodificar el JSON: %s\n", error.c_str());
    }
  } else {
    Serial.println("[GET] [ERROR] Petició GET fallida.");
    String errorMsg = httpGet.getString();
    Serial.printf("[GET] Detall de l'error del servidor: %s\n", errorMsg.c_str());
    Serial.println("-> Comprova que la clau anon de Supabase i la URL del projecte siguin les correctes.");
    Serial.println("-> Assegura't que les polítiques RLS (Row Level Security) a Supabase permeten la lectura.");
  }
  httpGet.end();

  delay(1000);

  // ------------------------------------------------------------------
  // PROVA 2: PATCH - Enviar un batec de cor (heartbeat)
  // ------------------------------------------------------------------
  Serial.println("\n[PROVA 2] Realitzant petició PATCH per registrar Batec de Cor (Heartbeat)...");
  HTTPClient httpPatch;
  String urlPatch = String(SUPABASE_URL) + "/rest/v1/devices?id=eq." + DEVICE_ID;
  
  StaticJsonDocument<256> docPatch;
  docPatch["last_seen"] = "now";
  docPatch["status"] = "test_wifi";
  docPatch["firmware_version"] = "0.1.0-test-network";

  String bodyPatch;
  serializeJson(docPatch, bodyPatch);

  httpPatch.begin(netClient, urlPatch);
  aplicarCapcaleresSupabase(httpPatch, true);
  
  int codiHttpPatch = httpPatch.PATCH(bodyPatch);
  Serial.printf("[PATCH] Resposta HTTP Codi: %d\n", codiHttpPatch);

  if (codiHttpPatch >= 200 && codiHttpPatch < 300) {
    Serial.println("[PATCH] [OK] Batec de cor transmès correctament! L'ESP32 es mostra connectat al dashboard del núvol.");
  } else {
    Serial.println("[PATCH] [ERROR] Petició PATCH fallida.");
    String errorMsg = httpPatch.getString();
    Serial.printf("[PATCH] Detall de l'error del servidor: %s\n", errorMsg.c_str());
    Serial.println("-> Comprova que el teu DEVICE_ID existeix prèviament a la taula 'devices' de la teva base de dades.");
    Serial.println("-> Revisa les polítiques RLS d'escriptura/actualització de Supabase.");
  }
  httpPatch.end();
  
  Serial.println("\n========================================================");
  Serial.println("               FI DE LES PROVES DE NETWORK             ");
  Serial.println("========================================================");
}
