#include "message.h"

Message* message_create(const char *role, const char *content) {
    if (role == NULL || strlen(role) == 0) {
        fprintf(stderr, "Error: Message role cannot be null or empty.\n");
        return NULL;
    }
    // Content can be empty, but not NULL, based on your Java comment
    if (content == NULL) {
        content = ""; // Treat NULL content as empty string for safety
    }

    Message *msg = (Message *)malloc(sizeof(Message));
    if (msg == NULL) {
        perror("Failed to allocate memory for Message");
        return NULL;
    }

    msg->role = strdup(role);
    if (msg->role == NULL) {
        perror("Failed to allocate memory for Message role");
        free(msg);
        return NULL;
    }

    msg->content = strdup(content);
    if (msg->content == NULL) {
        perror("Failed to allocate memory for Message content");
        free(msg->role);
        free(msg);
        return NULL;
    }

    return msg;
}

void message_free(Message *msg) {
    if (msg != NULL) {
        free(msg->role);
        free(msg->content);
        free(msg);
    }
}

void message_print(const Message *msg) {
    if (msg != NULL) {
        printf("Message{role='%s', content='%s'}\n", msg->role, msg->content);
    }
}