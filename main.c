#include <stdio.h>
#include <stdlib.h>
#include "message.h"
#include "ollama_llm.h"

// 示例的 ToolManager 和 AvailableToolSet 逻辑也需要翻译成 C 语言
// 为简单起见，我们将手动构建工具模式的 JSON 以进行演示。
// 在实际应用中，您也会使用 json-c 以编程方式构建它。

int main() {
    LocalOllamaLLM *llm = ollama_llm_create("qwen3:8b", "http://localhost:11434/api/chat");
    if (llm == NULL) {
        return 1;
    }

    printf("--- 基本聊天示例 ---\n");
    Message *chat_messages[] = {
        message_create("user", "天空为什么是蓝色的？") // 用户消息：天空为什么是蓝色的？
    };
    int num_chat_messages = sizeof(chat_messages) / sizeof(chat_messages[0]);

    json_object *chat_response = ollama_llm_stream(llm, chat_messages, num_chat_messages, NULL);
    if (chat_response) {
        printf("聊天响应:\n%s\n", json_object_to_json_string_ext(chat_response, JSON_C_TO_STRING_PRETTY));
        json_object_put(chat_response); // 减少响应的引用计数
    } else {
        fprintf(stderr, "获取聊天响应失败。\n"); // 打印错误信息：获取聊天响应失败。
    }

    // 释放聊天消息
    for (int i = 0; i < num_chat_messages; i++) {
        message_free(chat_messages[i]);
    }

    printf("\n--- 带有工具的聊天示例 (get_current_weather) ---\n"); // 带有工具的聊天示例 (获取当前天气)
    // 手动创建工具模式的 JSON 以进行演示
    json_object *tools_array = json_object_new_array();
    json_object *weather_tool_schema = json_object_new_object();
    json_object_object_add(weather_tool_schema, "type", json_object_new_string("function"));
    json_object *weather_function = json_object_new_object();
    json_object_object_add(weather_function, "name", json_object_new_string("get_current_weather"));
    json_object_object_add(weather_function, "description", json_object_new_string("获取某个地点的当前天气")); // 工具描述：获取某个地点的当前天气
    json_object *weather_params = json_object_new_object();
    json_object_object_add(weather_params, "type", json_object_new_string("object"));
    json_object *weather_properties = json_object_new_object();
    json_object *loc_prop = json_object_new_object();
    json_object_object_add(loc_prop, "type", json_object_new_string("string"));
    json_object_object_add(loc_prop, "description", json_object_new_string("要获取天气的地点，例如：旧金山, CA")); // 参数描述：要获取天气的地点，例如：旧金山, CA
    json_object_object_add(weather_properties, "location", loc_prop);
    json_object *format_prop = json_object_new_object();
    json_object_object_add(format_prop, "type", json_object_new_string("string"));
    json_object_object_add(format_prop, "description", json_object_new_string("返回天气的格式，例如 '摄氏度' 或 '华氏度'")); // 参数描述：返回天气的格式，例如 '摄氏度' 或 '华氏度'
    json_object *enum_array = json_object_new_array();
    json_object_array_add(enum_array, json_object_new_string("celsius"));
    json_object_array_add(enum_array, json_object_new_string("fahrenheit"));
    json_object_object_add(format_prop, "enum", enum_array);
    json_object_object_add(weather_properties, "format", format_prop);
    json_object_object_add(weather_params, "properties", weather_properties);
    json_object *required_array = json_object_new_array();
    json_object_array_add(required_array, json_object_new_string("location"));
    json_object_array_add(required_array, json_object_new_string("format"));
    json_object_object_add(weather_params, "required", required_array);
    json_object_object_add(weather_function, "parameters", weather_params);
    json_object_object_add(weather_tool_schema, "function", weather_function);
    json_object_array_add(tools_array, weather_tool_schema); // tools_array 现在包含天气工具模式

    Message *tool_messages[] = {
        message_create("user", "巴黎今天天气怎么样？") // 用户消息：巴黎今天天气怎么样？
    };
    int num_tool_messages = sizeof(tool_messages) / sizeof(tool_messages[0]);

    json_object *tool_context = json_object_new_object();
    json_object_object_add(tool_context, "tools", tools_array); // tool_context 现在包含 tools_array

    json_object *tool_response = ollama_llm_stream(llm, tool_messages, num_tool_messages, tool_context);
    if (tool_response) {
        printf("工具响应:\n%s\n", json_object_to_json_string_ext(tool_response, JSON_C_TO_STRING_PRETTY));
        json_object_put(tool_response);
    } else {
        fprintf(stderr, "获取工具响应失败。\n"); // 打印错误信息：获取工具响应失败。
    }

    // 释放工具消息
    for (int i = 0; i < num_tool_messages; i++) {
        message_free(tool_messages[i]);
    }
    json_object_put(tool_context); // 释放上下文对象，这也会减少 tools_array 的引用计数


    printf("\n--- 带有结构化输出的聊天示例 ---\n"); // 带有结构化输出的聊天示例
    json_object *format_schema = json_object_new_object();
    json_object_object_add(format_schema, "type", json_object_new_string("object"));
    json_object *props = json_object_new_object();
    json_object *age_prop = json_object_new_object();
    json_object_object_add(age_prop, "type", json_object_new_string("integer"));
    json_object_object_add(props, "age", age_prop);
    json_object *available_prop = json_object_new_object();
    json_object_object_add(available_prop, "type", json_object_new_string("boolean"));
    json_object_object_add(props, "available", available_prop);
    json_object_object_add(format_schema, "properties", props);
    json_object *required_props = json_object_new_array();
    json_object_array_add(required_props, json_object_new_string("age"));
    json_object_array_add(required_props, json_object_new_string("available"));
    json_object_object_add(format_schema, "required", required_props);

    Message *structure_messages[] = {
        message_create("user", "Ollama 22岁，正忙着拯救世界。返回一个包含年龄和可用性的 JSON 对象。") // 用户消息：Ollama 22岁，正忙着拯救世界。返回一个包含年龄和可用性的 JSON 对象。
    };
    int num_structure_messages = sizeof(structure_messages) / sizeof(structure_messages[0]);

    json_object *structure_context = json_object_new_object();
    json_object_object_add(structure_context, "format", format_schema); // format_schema 的引用计数增加

    json_object *structure_response = ollama_llm_stream(llm, structure_messages, num_structure_messages, structure_context);
    if (structure_response) {
        printf("结构化响应:\n%s\n", json_object_to_json_string_ext(structure_response, JSON_C_TO_STRING_PRETTY));
        json_object_put(structure_response);
    } else {
        fprintf(stderr, "获取结构化响应失败。\n"); // 打印错误信息：获取结构化响应失败。
    }

    // 释放结构化消息
    for (int i = 0; i < num_structure_messages; i++) {
        message_free(structure_messages[i]);
    }
    json_object_put(structure_context); // 释放上下文对象，这也会减少 format_schema 的引用计数


    ollama_llm_free(llm);
    curl_global_cleanup(); // 清理 libcurl 资源
    return 0;
}