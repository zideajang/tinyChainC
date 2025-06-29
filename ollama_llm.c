#include "ollama_llm.h"

// Callback function to write received data into a MemoryStruct
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        /* out of memory! */
        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Null-terminate for string operations

    return realsize;
}

LocalOllamaLLM* ollama_llm_create(const char *model_name, const char *api_url) {
    if (model_name == NULL || api_url == NULL) {
        fprintf(stderr, "Error: Model name and API URL cannot be null.\n");
        return NULL;
    }

    LocalOllamaLLM *llm = (LocalOllamaLLM *)malloc(sizeof(LocalOllamaLLM));
    if (llm == NULL) {
        perror("Failed to allocate memory for LocalOllamaLLM");
        return NULL;
    }

    llm->model_name = strdup(model_name);
    if (llm->model_name == NULL) {
        perror("Failed to allocate memory for model_name");
        free(llm);
        return NULL;
    }

    llm->ollama_api_url = strdup(api_url);
    if (llm->ollama_api_url == NULL) {
        perror("Failed to allocate memory for ollama_api_url");
        free(llm->model_name);
        free(llm);
        return NULL;
    }
    return llm;
}

void ollama_llm_free(LocalOllamaLLM *llm) {
    if (llm != NULL) {
        free(llm->model_name);
        free(llm->ollama_api_url);
        free(llm);
    }
}

// Helper function to build the common JSON request body
static json_object* build_base_request_body(LocalOllamaLLM *llm, Message **messages, int num_messages) {
    json_object *request_body = json_object_new_object();
    json_object_object_add(request_body, "model", json_object_new_string(llm->model_name));

    json_object *messages_array = json_object_new_array();
    for (int i = 0; i < num_messages; i++) {
        json_object *msg_obj = json_object_new_object();
        json_object_object_add(msg_obj, "role", json_object_new_string(messages[i]->role));
        json_object_object_add(msg_obj, "content", json_object_new_string(messages[i]->content));
        json_object_array_add(messages_array, msg_obj);
    }
    json_object_object_add(request_body, "messages", messages_array);
    json_object_object_add(request_body, "stream", json_object_new_boolean(FALSE)); // Always false for now

    return request_body;
}

// Helper function to perform the actual HTTP POST request
static json_object* perform_ollama_request(const char *url, json_object *request_body) {
    CURL *curl;
    CURLcode res;
    json_object *response_json = NULL;

    MemoryStruct chunk;
    chunk.memory = malloc(1); // will be grown as needed by realloc
    chunk.size = 0;    // no data at this point

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        const char *request_json_str = json_object_to_json_string_ext(request_body, JSON_C_TO_STRING_PLAIN);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json_str);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse the JSON response
            response_json = json_tokener_parse(chunk.memory);
            if (response_json == NULL) {
                fprintf(stderr, "Failed to parse JSON response from Ollama: %s\n", chunk.memory);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    free(chunk.memory); // Free memory held by the response chunk
    json_object_put(request_body); // Free the request body JSON object

    return response_json;
}


json_object* ollama_llm_chat(LocalOllamaLLM *llm, Message **messages, int num_messages) {
    json_object *request_body = build_base_request_body(llm, messages, num_messages);
    if (request_body == NULL) return NULL; // Error in building base request

    return perform_ollama_request(llm->ollama_api_url, request_body);
}

json_object* ollama_llm_chat_with_tool(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *tools_json_array) {
    json_object *request_body = build_base_request_body(llm, messages, num_messages);
    if (request_body == NULL) return NULL;

    // Add tools array to the request body
    // We create a new json_object of array type and add it to request_body.
    // The tools_json_array passed here should already be a json_object of array type
    // and its refcount will be incremented when added to the request_body.
    json_object_object_add(request_body, "tools", tools_json_array); // tools_json_array's ref_count is incremented

    return perform_ollama_request(llm->ollama_api_url, request_body);
}

json_object* ollama_llm_chat_with_structure(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *format_json_object) {
    json_object *request_body = build_base_request_body(llm, messages, num_messages);
    if (request_body == NULL) return NULL;

    // Add format object to the request body
    json_object *format_wrapper = json_object_new_object();
    json_object_object_add(format_wrapper, "type", json_object_new_string("json")); // Ollama expects format.type: "json" for structured output
    json_object_object_add(format_wrapper, "schema", format_json_object); // format_json_object's ref_count is incremented

    json_object_object_add(request_body, "format", format_wrapper); // format_wrapper's ref_count is incremented

    // Add options with temperature 0 for deterministic output as in your example
    json_object *options = json_object_new_object();
    json_object_object_add(options, "temperature", json_object_new_double(0.0));
    json_object_object_add(request_body, "options", options); // options' ref_count is incremented

    return perform_ollama_request(llm->ollama_api_url, request_body);
}


json_object* ollama_llm_stream(LocalOllamaLLM *llm, Message **messages, int num_messages, json_object *context) {
    if (context != NULL) {
        json_object *tools_node;
        json_object *format_node;

        if (json_object_object_get_ex(context, "tools", &tools_node) && json_object_is_type(tools_node, json_type_array)) {
            // Found tools in context, call chatWithTool
            // Important: json_object_get(tools_node) increments ref_count,
            // which is needed because chat_with_tool will then increment it again
            // when adding to the request_body. The original context will hold
            // its ref, and the request body will hold one.
            return ollama_llm_chat_with_tool(llm, messages, num_messages, json_object_get(tools_node));
        } else if (json_object_object_get_ex(context, "outputFormat", &format_node) && json_object_is_type(format_node, json_type_object)) {
            // Found outputFormat in context, call chatWithStructure
            return ollama_llm_chat_with_structure(llm, messages, num_messages, json_object_get(format_node));
        }
    }
    // No specific context for tools or format, default to regular chat
    return ollama_llm_chat(llm, messages, num_messages);
}