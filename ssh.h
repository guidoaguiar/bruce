//SSH borrowed from https://github.com/m5stack/M5Cardputer :)

//TODO: Display is kinda glitchy :P figure out some way to show better outputs also

#include "libssh_esp32.h"
#include <libssh/libssh.h>

String ssid     = "";      // Replace with your WiFi SSID

//TODO: mudar para char 8bits ao inves de String
String password = "";  // Replace with your WiFi password

// SSH server configuration (initialize as empty strings)
String ssh_host     = "";
String ssh_user     = "";
String ssh_password = "";

// M5Cardputer setup
//M5Canvas canvas(&DISP);
String commandBuffer              = "> ";
int cursorY                       = 0;
const int lineHeight              = 32;
unsigned long lastKeyPressMillis  = 0;
const unsigned long debounceDelay = 200;  // Adjust debounce delay as needed

//ssh_bind sshbind = (ssh_bind)state->input;

//ssh_init sshbind;
ssh_session my_ssh_session;
ssh_channel channel;

void waitForInput(String& input);

bool filterAnsiSequences =
    true;  // Set to false to disable ANSI sequence filtering


void waitForInput(String& input) {
    unsigned long startTime           = millis();
    unsigned long lastKeyPressMillis  = 0;
    const unsigned long debounceDelay = 200;  // Adjust debounce delay as needed
    String currentInput               = input;

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            if (status.del && currentInput.length() > 0) {
                // Handle backspace key
                currentInput.remove(currentInput.length() - 1);
                
                DISP.setCursor(
                    DISP.getCursorX() - 6,
                    DISP.getCursorY());
                DISP.print(" ");
                DISP.setCursor(
                    DISP.getCursorX() - 6,
                    DISP.getCursorY());
                cursorY            = DISP.getCursorY();
                lastKeyPressMillis = millis();
            }

            for (auto i : status.word) {
                if (millis() - lastKeyPressMillis >= debounceDelay) {
                    currentInput += i;
                    DISP.print(i);
                    cursorY            = DISP.getCursorY();
                    lastKeyPressMillis = millis();
                }
            }

            if (status.enter) {
                DISP.println();  // Move to the next line
                input = currentInput;
                break;
            }
        }

        if (millis() - startTime > 180000) {  // Timeout after 3 minutes
            DISP.println("\nInput timeout. Rebooting...");
            delay(1000);    // Delay for 1 second to allow the message to be
                            // displayed
            ESP.restart();  // Reboot the ESP32
        }
    }
}

void ssh_loop() {
    M5Cardputer.update();
    DISP.setTextColor(WHITE, BGCOLOR);

    // Handle keyboard input with debounce
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastKeyPressMillis >= debounceDelay) {
            lastKeyPressMillis               = currentMillis;
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            for (auto i : status.word) {
                commandBuffer += i;
                DISP.print(i);
                cursorY = DISP.getCursorY();
            }

            if (status.del && commandBuffer.length() > 2) {
                commandBuffer.remove(commandBuffer.length() - 1);
                DISP.setCursor(
                    DISP.getCursorX() - 6,
                    DISP.getCursorY());
                DISP.print(" ");
                DISP.setCursor(
                    DISP.getCursorX() - 6,
                    DISP.getCursorY());
                cursorY = DISP.getCursorY();
            }

            if (status.enter) {
                commandBuffer.trim();  // Trim the command buffer to remove
                                       // accidental whitespaces/newlines
                String message = commandBuffer.substring(
                    2);  // Get the command part, exclude the "> "
                ssh_channel_write(channel, message.c_str(),
                                  message.length());  // Send the command
                ssh_channel_write(channel, "\r",
                                  1);  // Send exactly one carriage return (try
                                       // "\n" or "\r\n" if needed)

                commandBuffer = "> ";  // Reset command buffer
                DISP.print(
                    '\n');  // Move to the next line on display
                cursorY =
                    DISP.getCursorY();  // Update cursor position
            }
        }
    }

    // Check if the cursor has reached the bottom of the display
    if (cursorY > DISP.height() - lineHeight) {
        DISP.scroll(0, -lineHeight);
        cursorY -= lineHeight;
        DISP.setCursor(DISP.getCursorX(),
                                      cursorY);
    }

    // Read data from SSH server and display it, handling ANSI sequences
    char buffer[128];  // Reduced buffer size for less memory usage
    int nbytes =
        ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);
    bool isAnsiSequence =
        false;  // To track when we are inside an ANSI sequence

    if (nbytes > 0) {
        for (int i = 0; i < nbytes; ++i) {
            char c = buffer[i];
            if (filterAnsiSequences) {
                if (c == '\033') {
                    isAnsiSequence = true;  // Enter ANSI sequence mode
                } else if (isAnsiSequence) {
                    if (isalpha(c)) {
                        isAnsiSequence = false;  // Exit ANSI sequence mode at
                                                 // the end character
                    }
                } else {
                    if (c == '\r') continue;  // Ignore carriage return
                    DISP.write(c);
                    cursorY = DISP.getCursorY();
                }
            } else {
                if (c == '\r') continue;  // Ignore carriage return
                DISP.write(c);
                cursorY = DISP.getCursorY();
            }
        }
    }

    // Handle channel closure and other conditions
    if (nbytes < 0 || ssh_channel_is_closed(channel)) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        DISP.println("\nSSH session closed.");
        DISP.setTextColor(FGCOLOR, BGCOLOR);
        return;  // Exit the loop upon session closure
    }
}

void ssh_setup(){
    DISP.clear();
    DISP.setCursor(0, 0);
    Serial.begin(115200);  // Initialize serial communication for debugging
    Serial.println("Starting Setup");

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    DISP.setRotation(1);
    DISP.setTextSize(SMALL_TEXT);  // Set text size
    DISP.clear();
    DISP.print("WIFI SSID:\n");
    waitForInput(ssid);
    DISP.print("WIFI Password: \n");
    waitForInput(password);

    // Connect to WiFi
    WiFi.begin(ssid, password.c_str());
    delay(2000);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        DISP.setTextColor(RED, BGCOLOR);
        DISP.print("FAILED TO CONNECT!");
        DISP.setTextColor(FGCOLOR, BGCOLOR);
        return;
    }
    DISP.setTextColor(GREEN, BGCOLOR);
    DISP.println("WiFi Connected");
    delay(2000);
    DISP.setTextColor(FGCOLOR, BGCOLOR);
    // Initialize the cursor Y position
    
    DISP.clear();
    cursorY = DISP.getCursorY();

    DISP.setCursor(0, 0);
    // Prompt for SSH host, username, and password
    DISP.print("SSH Host: \n");
    waitForInput(ssh_host);
    DISP.print("\nSSH Username: ");
    waitForInput(ssh_user);
    DISP.print("\nSSH Password: ");
    waitForInput(ssh_password);
    

    my_ssh_session = ssh_new();



    if (my_ssh_session == NULL) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH Session creation failed.");
        return;
    }
    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, ssh_host.c_str());
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, ssh_user.c_str());

    if (ssh_connect(my_ssh_session) != SSH_OK) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH Connect error.");
        ssh_free(my_ssh_session);
        return;
    }

    if (ssh_userauth_password(my_ssh_session, NULL, ssh_password.c_str()) !=
        SSH_AUTH_SUCCESS) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH Authentication error.");
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return;
    }

    channel = ssh_channel_new(my_ssh_session);
    if (channel == NULL || ssh_channel_open_session(channel) != SSH_OK) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH Channel open error.");
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return;
    }

    if (ssh_channel_request_pty(channel) != SSH_OK) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH PTY request error.");
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return;
    }

    if (ssh_channel_request_shell(channel) != SSH_OK) {
        DISP.setTextColor(RED, BGCOLOR);
        DISP.println("SSH Shell request error.");
        Serial.println("SSH Shell request error.");
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return;
    }

    Serial.println("SSH setup completed.");
    DISP.setTextColor(GREEN, BGCOLOR);
    DISP.println("SSH Conected!");
    delay(2000);
    DISP.clear();
    DISP.setTextColor(WHITE, BGCOLOR);
    DISP.setTextSize(TINY_TEXT);  // Set text size
}





/*
#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>

#define TAG "TELNET_CLIENT"

String telnet_server_string = ""; // Replace with your TELNET server's IP address
char telnet_server_ip[12];
static const int telnet_server_port = 23; // TELNET protocol default port

static int sock;

char* stringTochar(String s)
{
    if (s.length() == 0) {
        return nullptr; // or handle the case where the string is empty
    }

    static char arr[12]; // Make sure it's large enough to hold the IP address
    s.toCharArray(arr, sizeof(arr));
    return arr;
}

static void telnet_setup() {
   DISP.clear();
    DISP.setCursor(0, 0);
    Serial.begin(115200);  // Initialize serial communication for debugging
    Serial.println("Starting Setup");

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    DISP.setRotation(1);
    DISP.setTextSize(SMALL_TEXT);  // Set text size
    DISP.clear();
    DISP.print("WIFI SSID:\n");
    waitForInput(ssid);
    DISP.print("WIFI Password: \n");
    waitForInput(password);

    // Connect to WiFi
    WiFi.begin(ssid, password.c_str());
    delay(2000);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        DISP.setTextColor(RED, BGCOLOR);
        DISP.print("FAILED TO CONNECT!");
        DISP.setTextColor(FGCOLOR, BGCOLOR);
        return;
    }
    DISP.setTextColor(GREEN, BGCOLOR);
    DISP.println("WiFi Connected");
    delay(2000);
    DISP.setTextColor(FGCOLOR, BGCOLOR);
    // Initialize the cursor Y position
    
    DISP.clear();
    cursorY = DISP.getCursorY();

    DISP.setCursor(0, 0);
    DISP.print("TELNET Host: \n");
    waitForInput(telnet_server_string);
    telnet_server_ip = stringTochar(telnet_server_string);
    
}

static void telnet_loop() {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(telnet_server_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(telnet_server_port);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        return;
    }

    if (connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) != 0) {
        ESP_LOGE(TAG, "Socket connection failed");
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "Connected to TELNET server");

    while (1) {
        // Your TELNET client logic goes here

        // For example, sending a command to the server
        const char *command = "Hello TELNET server!\r\n";
        send(sock, command, strlen(command), 0);

        // You can also receive data from the server
        char buffer[128];
        int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received from server: %s", buffer);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

*/




