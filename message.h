#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Message structure equivalent to Java's Message class
typedef struct {
    char *role;
    char *content;
} Message;

// Function to create a new Message object
Message* message_create(const char *role, const char *content);

// Function to free a Message object's memory
void message_free(Message *msg);

// Function to print a Message object (for debugging)
void message_print(const Message *msg);

#endif // MESSAGE_H