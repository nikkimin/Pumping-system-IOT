#ifndef MQTT_DEBUG_H
#define MQTT_DEBUG_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "hivemq_config.h"

// ========== DEBUG LEVELS ==========
#define DEBUG_LEVEL_NONE 0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARN 2
#define DEBUG_LEVEL_INFO 3
#define DEBUG_LEVEL_VERBOSE 4

// Set debug level (change this to control verbosity)
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_VERBOSE

// ========== MQTT DEBUG HELPER ==========
class MQTTDebugger {
public:
    // Debug và test từng bước kết nối MQTT
    static void debugMQTTConnection(WiFiClientSecure& client, PubSubClient& mqtt) {
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║  🔍 MQTT CONNECTION DEBUG TOOL        ║");
        Serial.println("╚════════════════════════════════════════╝\n");
        
        // STEP 1: Check WiFi connection
        checkWiFiConnection();
        
        // STEP 2: Check time sync (critical for TLS)
        checkTimeSync();
        
        // STEP 3: Test DNS resolution
        testDNSResolution(HIVEMQ_HOST);
        
        // STEP 4: Test TCP connection
        testTCPConnection(client, HIVEMQ_HOST, HIVEMQ_PORT);
        
        // STEP 5: Test TLS/SSL handshake
        testTLSHandshake(client, HIVEMQ_HOST, HIVEMQ_PORT);
        
        // STEP 6: Check MQTT credentials format
        checkMQTTCredentials();
        
        // STEP 7: Attempt MQTT connection with detailed logging
        debugMQTTConnect(mqtt);
        
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║  ✅ DEBUG COMPLETE                     ║");
        Serial.println("╚════════════════════════════════════════╝\n");
    }

private:
    // STEP 1: Kiểm tra kết nối WiFi
    static void checkWiFiConnection() {
        Serial.println("📶 STEP 1: Checking WiFi Connection...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("   ✅ WiFi: CONNECTED");
            Serial.printf("   → SSID: %s\n", WiFi.SSID().c_str());
            Serial.printf("   → IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("   → Signal Strength (RSSI): %d dBm\n", WiFi.RSSI());
            Serial.printf("   → Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("   → DNS: %s\n\n", WiFi.dnsIP().toString().c_str());
        } else {
            Serial.println("   ❌ WiFi: NOT CONNECTED");
            Serial.println("   → Cannot proceed with MQTT connection\n");
            return;
        }
    }
    
    // STEP 2: Kiểm tra đồng bộ thời gian (critical cho TLS)
    static void checkTimeSync() {
        Serial.println("🕒 STEP 2: Checking Time Synchronization (NTP)...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        time_t now = time(nullptr);
        
        if (now > 1600000000) { // Valid timestamp > year 2020
            Serial.println("   ✅ Time Sync: OK");
            Serial.printf("   → Current Time: %s", ctime(&now));
            Serial.printf("   → Timestamp: %ld\n", now);
            Serial.println("   → TLS/SSL certificate validation: ENABLED\n");
        } else {
            Serial.println("   ❌ Time Sync: FAILED");
            Serial.printf("   → Current Timestamp: %ld (invalid)\n", now);
            Serial.println("   ⚠️  WARNING: TLS certificate validation will FAIL!");
            Serial.println("   → Solution: Wait for NTP sync or check internet connectivity\n");
        }
    }
    
    // STEP 3: Test DNS resolution
    static void testDNSResolution(const char* host) {
        Serial.println("🌐 STEP 3: Testing DNS Resolution...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.printf("   → Resolving: %s\n", host);
        
        IPAddress serverIP;
        if (WiFi.hostByName(host, serverIP)) {
            Serial.println("   ✅ DNS Resolution: SUCCESS");
            Serial.printf("   → Resolved IP: %s\n\n", serverIP.toString().c_str());
        } else {
            Serial.println("   ❌ DNS Resolution: FAILED");
            Serial.println("   → Cannot resolve HiveMQ hostname");
            Serial.println("   → Check: Internet connection, DNS server\n");
        }
    }
    
    // STEP 4: Test TCP connection
    static void testTCPConnection(WiFiClientSecure& client, const char* host, int port) {
        Serial.println("🔌 STEP 4: Testing TCP Connection...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.printf("   → Connecting to: %s:%d\n", host, port);
        
        // Tạm thời bỏ qua cert validation để test TCP
        client.setInsecure();
        
        unsigned long startTime = millis();
        if (client.connect(host, port)) {
            unsigned long connectTime = millis() - startTime;
            Serial.println("   ✅ TCP Connection: SUCCESS");
            Serial.printf("   → Connection time: %lu ms\n", connectTime);
            Serial.println("   → Socket is open\n");
            client.stop();
        } else {
            Serial.println("   ❌ TCP Connection: FAILED");
            Serial.println("   → Cannot establish socket connection");
            Serial.println("   → Check: Firewall, Port 8883 access, Host reachability\n");
        }
    }
    
    // STEP 5: Test TLS/SSL handshake
    static void testTLSHandshake(WiFiClientSecure& client, const char* host, int port) {
        Serial.println("🔐 STEP 5: Testing TLS/SSL Handshake...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // Test với certificate validation
        extern const char* hivemq_root_ca;
        client.setCACert(hivemq_root_ca);
        
        Serial.printf("   → Attempting TLS connection to: %s:%d\n", host, port);
        Serial.println("   → Certificate validation: ENABLED");
        
        unsigned long startTime = millis();
        if (client.connect(host, port)) {
            unsigned long tlsTime = millis() - startTime;
            Serial.println("   ✅ TLS Handshake: SUCCESS");
            Serial.printf("   → TLS connection time: %lu ms\n", tlsTime);
            Serial.println("   → Certificate validation: PASSED");
            Serial.println("   → Secure channel established\n");
            client.stop();
        } else {
            Serial.println("   ❌ TLS Handshake: FAILED");
            Serial.println("   → Certificate validation failed");
            Serial.println("   → Possible causes:");
            Serial.println("      1. Time not synced (NTP failure)");
            Serial.println("      2. Wrong Root CA certificate");
            Serial.println("      3. Certificate expired");
            Serial.println("      4. Hostname mismatch\n");
            
            // Try insecure connection để xác định vấn đề
            Serial.println("   🔓 Retrying with INSECURE mode (debug only)...");
            client.setInsecure();
            if (client.connect(host, port)) {
                Serial.println("   ⚠️  Insecure connection: SUCCESS");
                Serial.println("   → Problem is with CERTIFICATE VALIDATION");
                Serial.println("   → Fix: Update Root CA cert or sync NTP time\n");
                client.stop();
            } else {
                Serial.println("   ❌ Insecure connection: ALSO FAILED");
                Serial.println("   → Problem is NOT certificate-related");
                Serial.println("   → Check: Network connectivity, Firewall\n");
            }
        }
    }
    
    // STEP 6: Kiểm tra MQTT credentials
    static void checkMQTTCredentials() {
        Serial.println("🔑 STEP 6: Checking MQTT Credentials...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        Serial.printf("   → Client ID: '%s'\n", MQTT_CLIENT_ID);
        Serial.printf("   → Username: '%s'\n", MQTT_USERNAME);
        Serial.printf("   → Password: '%s' (length: %d)\n", MQTT_PASSWORD, strlen(MQTT_PASSWORD));
        
        // Validation checks
        bool valid = true;
        
        if (strlen(MQTT_CLIENT_ID) == 0) {
            Serial.println("   ❌ Client ID is EMPTY!");
            valid = false;
        }
        
        if (strlen(MQTT_USERNAME) == 0) {
            Serial.println("   ❌ Username is EMPTY!");
            valid = false;
        }
        
        if (strlen(MQTT_PASSWORD) == 0) {
            Serial.println("   ❌ Password is EMPTY!");
            valid = false;
        }
        
        // Check for special characters that might cause issues
        if (strchr(MQTT_USERNAME, ' ') || strchr(MQTT_PASSWORD, ' ')) {
            Serial.println("   ⚠️  WARNING: Credentials contain SPACES!");
            Serial.println("   → This may cause authentication issues");
            valid = false;
        }
        
        if (valid) {
            Serial.println("   ✅ Credentials format: OK");
            Serial.println("   → All fields are non-empty");
        }
        
        Serial.println("\n   📝 IMPORTANT: Verify these credentials match HiveMQ Console:");
        Serial.println("      1. Go to HiveMQ Cloud Console");
        Serial.println("      2. Navigate to 'Access Management'");
        Serial.println("      3. Verify username and password match exactly\n");
    }
    
    // STEP 7: MQTT connection với detailed logging
    static void debugMQTTConnect(PubSubClient& mqtt) {
        Serial.println("📡 STEP 7: Attempting MQTT Connection...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        Serial.printf("   → Broker: %s:%d\n", HIVEMQ_HOST, HIVEMQ_PORT);
        Serial.printf("   → Client ID: %s\n", MQTT_CLIENT_ID);
        Serial.printf("   → Username: %s\n", MQTT_USERNAME);
        Serial.println("   → Connecting...\n");
        
        unsigned long startTime = millis();
        bool connected = mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
        unsigned long connectTime = millis() - startTime;
        
        if (connected) {
            Serial.println("   ✅ MQTT Connection: SUCCESS!");
            Serial.printf("   → Connection time: %lu ms\n", connectTime);
            Serial.println("   → Status: AUTHORIZED\n");
            
            Serial.println("   📤 Testing subscription...");
            if (mqtt.subscribe(TOPIC_PUMP_CONTROL)) {
                Serial.printf("   ✅ Subscribed to: %s\n\n", TOPIC_PUMP_CONTROL);
            }
        } else {
            int rc = mqtt.state();
            Serial.println("   ❌ MQTT Connection: FAILED!");
            Serial.printf("   → Connection time: %lu ms\n", connectTime);
            Serial.printf("   → Return code: %d\n\n", rc);
            
            printMQTTError(rc);
        }
    }
    
    // In ra lỗi MQTT chi tiết
    static void printMQTTError(int rc) {
        Serial.println("   🔍 ERROR ANALYSIS:");
        Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        switch(rc) {
            case -4:
                Serial.println("   ❌ MQTT_CONNECTION_TIMEOUT");
                Serial.println("   → Server didn't respond in time");
                Serial.println("   → Causes:");
                Serial.println("      • Broker is down or unreachable");
                Serial.println("      • Network latency too high");
                Serial.println("      • Firewall blocking connection");
                Serial.println("   → Solutions:");
                Serial.println("      1. Check HiveMQ Cloud cluster status");
                Serial.println("      2. Verify broker URL is correct");
                Serial.println("      3. Check network/firewall settings");
                break;
                
            case -3:
                Serial.println("   ❌ MQTT_CONNECTION_LOST");
                Serial.println("   → Network connection lost during handshake");
                Serial.println("   → Causes:");
                Serial.println("      • Unstable WiFi connection");
                Serial.println("      • Network dropped mid-connection");
                Serial.println("   → Solutions:");
                Serial.println("      1. Check WiFi signal strength (RSSI)");
                Serial.println("      2. Move closer to router");
                Serial.println("      3. Restart ESP32");
                break;
                
            case -2:
                Serial.println("   ❌ MQTT_CONNECT_FAILED");
                Serial.println("   → Network connection failed");
                Serial.println("   → Causes:");
                Serial.println("      • Cannot establish TCP connection");
                Serial.println("      • TLS/SSL handshake failed");
                Serial.println("      • Certificate validation failed");
                Serial.println("   → Solutions:");
                Serial.println("      1. Check time sync (must be accurate for TLS)");
                Serial.println("      2. Verify Root CA certificate");
                Serial.println("      3. Check host and port");
                break;
                
            case -1:
                Serial.println("   ❌ MQTT_DISCONNECTED");
                Serial.println("   → Client is disconnected");
                Serial.println("   → Normal state, retry connection");
                break;
                
            case 1:
                Serial.println("   ❌ MQTT_CONNECT_BAD_PROTOCOL");
                Serial.println("   → Server doesn't support MQTT protocol version");
                Serial.println("   → Causes:");
                Serial.println("      • Protocol mismatch");
                Serial.println("   → Solutions:");
                Serial.println("      1. Use PubSubClient library (MQTT 3.1.1)");
                Serial.println("      2. Update library to latest version");
                break;
                
            case 2:
                Serial.println("   ❌ MQTT_CONNECT_BAD_CLIENT_ID");
                Serial.println("   → Client ID rejected by server");
                Serial.println("   → Causes:");
                Serial.println("      • Client ID already in use (duplicate connection)");
                Serial.println("      • Client ID contains invalid characters");
                Serial.println("   → Solutions:");
                Serial.println("      1. Change MQTT_CLIENT_ID to unique value");
                Serial.printf("         Current: %s\n", MQTT_CLIENT_ID);
                Serial.println("      2. Check if another device uses same ID");
                Serial.println("      3. Disconnect other clients from HiveMQ Console");
                break;
                
            case 3:
                Serial.println("   ❌ MQTT_CONNECT_UNAVAILABLE");
                Serial.println("   → MQTT service unavailable");
                Serial.println("   → Causes:");
                Serial.println("      • HiveMQ cluster is down");
                Serial.println("      • Maintenance in progress");
                Serial.println("   → Solutions:");
                Serial.println("      1. Check HiveMQ Cloud status page");
                Serial.println("      2. Wait and retry later");
                break;
                
            case 4:
                Serial.println("   ❌ MQTT_CONNECT_BAD_CREDENTIALS");
                Serial.println("   → Authentication failed - Wrong username/password");
                Serial.println("   → Causes:");
                Serial.println("      • Username or password is incorrect");
                Serial.println("      • Credentials contain typos");
                Serial.println("   → Solutions:");
                Serial.printf("      1. Verify in hivemq_config.h:\n");
                Serial.printf("         Username: '%s'\n", MQTT_USERNAME);
                Serial.printf("         Password: '%s'\n", MQTT_PASSWORD);
                Serial.println("      2. Re-check HiveMQ Console → Access Management");
                Serial.println("      3. Copy-paste credentials to avoid typos");
                Serial.println("      4. Try creating new credentials");
                break;
                
            case 5:
                Serial.println("   ❌ MQTT_CONNECT_UNAUTHORIZED (rc=5) ⚠️");
                Serial.println("   → Client is NOT AUTHORIZED to connect");
                Serial.println("\n   🔍 ROOT CAUSES:");
                Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
                Serial.println("   1. ❌ Client ID is BLOCKED or RESTRICTED");
                Serial.printf("      → Your Client ID: '%s'\n", MQTT_CLIENT_ID);
                Serial.println("      → Check: HiveMQ Console → Access Management → Clients");
                Serial.println();
                
                Serial.println("   2. ❌ DUPLICATE Connection (Same Client ID)");
                Serial.println("      → Another device is using the SAME Client ID");
                Serial.println("      → HiveMQ allows ONLY ONE connection per Client ID");
                Serial.println("      → Check: HiveMQ Console → Clients → Active Connections");
                Serial.println();
                
                Serial.println("   3. ❌ Access Control List (ACL) Restriction");
                Serial.println("      → User doesn't have permission to connect");
                Serial.printf("      → Check permissions for user: '%s'\n", MQTT_USERNAME);
                Serial.println("      → Go to: Access Management → Permissions");
                Serial.println();
                
                Serial.println("   4. ❌ User Account is DISABLED or DELETED");
                Serial.printf("      → Verify user '%s' exists and is ENABLED\n", MQTT_USERNAME);
                Serial.println("      → Check: Access Management → Users");
                Serial.println();
                
                Serial.println("   5. ❌ IP Whitelist / Rate Limiting");
                Serial.println("      → Your IP might be blocked or rate-limited");
                Serial.printf("      → Your IP: %s\n", WiFi.localIP().toString().c_str());
                Serial.println("      → Check: HiveMQ Console → Security Settings");
                Serial.println();
                
                Serial.println("   📋 STEP-BY-STEP SOLUTIONS:");
                Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
                Serial.println("   ✅ SOLUTION 1: Check for Duplicate Connections");
                Serial.println("      1. Go to HiveMQ Console");
                Serial.println("      2. Navigate to 'Clients' or 'Active Connections'");
                Serial.printf("      3. Look for Client ID: '%s'\n", MQTT_CLIENT_ID);
                Serial.println("      4. If found → Disconnect it");
                Serial.println("      5. Then retry ESP32 connection");
                Serial.println();
                
                Serial.println("   ✅ SOLUTION 2: Change Client ID");
                Serial.println("      1. Edit hivemq_config.h");
                Serial.println("      2. Change MQTT_CLIENT_ID to unique value:");
                Serial.println("         Example: ESP32_SmartIrrigation_002");
                Serial.println("      3. Re-upload firmware");
                Serial.println();
                
                Serial.println("   ✅ SOLUTION 3: Re-create User Credentials");
                Serial.println("      1. Go to HiveMQ Console → Access Management");
                Serial.println("      2. DELETE old user");
                Serial.println("      3. CREATE new user with:");
                Serial.println("         - Username: newuser");
                Serial.println("         - Password: NewPassword123");
                Serial.println("         - Permissions: ALL (publish/subscribe to #)");
                Serial.println("      4. Update hivemq_config.h with new credentials");
                Serial.println();
                
                Serial.println("   ✅ SOLUTION 4: Verify Permissions (ACL)");
                Serial.println("      1. Go to Access Management → Permissions");
                Serial.printf("      2. For user '%s', ensure:\n", MQTT_USERNAME);
                Serial.println("         ✓ Can PUBLISH to: #");
                Serial.println("         ✓ Can SUBSCRIBE to: #");
                Serial.println("         ✓ Can CONNECT with any Client ID");
                Serial.println();
                
                Serial.println("   ✅ SOLUTION 5: Use Debug Tool on HiveMQ");
                Serial.println("      1. Go to HiveMQ Console → Tools → MQTT Client");
                Serial.println("      2. Try connecting with SAME credentials");
                Serial.println("      3. If web client works but ESP32 doesn't:");
                Serial.println("         → Problem is with ESP32 config");
                Serial.println("      4. If web client ALSO fails:");
                Serial.println("         → Problem is with HiveMQ account/permissions");
                Serial.println();
                
                Serial.println("   🎯 QUICK FIX (Most Common):");
                Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
                Serial.println("   → Disconnect any active connection with same Client ID");
                Serial.println("   → OR change Client ID to a unique value");
                Serial.printf("   → Current Client ID: %s\n", MQTT_CLIENT_ID);
                Serial.println("   → Try: ESP32_SmartIrrigation_<RANDOM_NUMBER>");
                break;
                
            default:
                Serial.printf("   ❌ UNKNOWN ERROR CODE: %d\n", rc);
                Serial.println("   → Consult MQTT/PubSubClient documentation");
                break;
        }
        
        Serial.println();
    }
};

#endif // MQTT_DEBUG_H
