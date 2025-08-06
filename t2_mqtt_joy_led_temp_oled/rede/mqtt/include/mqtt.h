#ifndef MQTT_H
#define MQTT_H
#include <stdbool.h> 

// Function prototypes for MQTT operations
void mqtt_setup(const char *client_id, const char *broker_addr, const char *user, const char *pass, char *text_buffe);
// Function to publish data to a specific topic
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);
// Function to check if the MQTT connection is established
bool mqtt_is_connected();
#endif