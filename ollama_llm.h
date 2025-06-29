#ifndef OLLAMA_LLM_H
#define OLLAMA_LLM_H

#include <curl/curl.h>
#include <json-c/json.h>
#include "message.h" // Include the Message struct

// Define a structure to hold the response from curl
typedef struct {
    char *memory;
    size_t size;
} MemoryStruct;

// LocalOllamaLLM structure
typedef struct {
    char *model_name;
    char *ollama_api_url; // Base URL for Ollama API, e.g., "http://localhost:11434/api/chat"
} LocalOllamaLLM;

// Callback function for curl to write response data
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

// Function to create a new LocalOllamaLLM instance
LocalOllamaLLM* ollama_llm_create(const char *model_name, const char *api_url);

// Function to free LocalOllamaLLM instance memory
void ollama_llm_free(LocalOllamaLLM *llm);

// Core function to send chat requests to Ollama
// Returns a json_object* which needs to be freed by the caller
json_object* ollama_llm_chat(LocalOllamaLLM *llm, Message **messages, int num_messages);

// Function to send chat requests with tools
json_object* ollama_llm_chat_with_tool(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *tools_json_array);

// Function to send chat requests with structured output format
json_object* ollama_llm_chat_with_structure(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *format_json_object);

// Stream function that dispatches based on context
// context can be NULL, or a json_object containing "tools" or "outputFormat"
json_object* ollama_llm_stream(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *context);

#endif // OLLAMA_LLM_H