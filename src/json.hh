#pragma once

#include <string>
#include <stack>
#include <iostream>

namespace json {

enum ContextType { LIST, DICT, PLAIN };

struct JsonWriter;

//-----------------------------------------------------------------------------
// Context
//-----------------------------------------------------------------------------

struct Context {
public:
    Context() = default;
    Context(JsonWriter *writer, ContextType type);

    Context(const Context& other) = delete;            // copy is
    Context& operator=(const Context& other) = delete; // forbidden

    Context(Context&& other);
    Context& operator=(Context&& other);

    ~Context();

    auto operator<<(const std::string &st) -> Context&;

public:
    JsonWriter  *writer   { nullptr };
    std::string  start_st;
    std::string  end_st;
    std::string  sep_st;
    bool         use_sep  { false }; // flag to use separator from second element on
};

//-----------------------------------------------------------------------------
// JsonWriter
//-----------------------------------------------------------------------------

struct JsonWriter {

    JsonWriter(std::ostream &os);
    JsonWriter(const JsonWriter& other) = delete;
    JsonWriter& operator=(const JsonWriter& other) = delete;

    JsonWriter& plain();
    JsonWriter& dict(std::string name="");
    JsonWriter& dict_entry(std::string name, std::string value);
    JsonWriter& dict_entry(std::string name, uint64_t    value);
    JsonWriter& list(std::string name="");
    JsonWriter& pop();
    ~JsonWriter();

    auto operator<<(const std::string &st) -> JsonWriter&;

    std::ostream &os;
    std::stack<Context> context_stack;


public:
};

//-----------------------------------------------------------------------------
// ContextGuard
//-----------------------------------------------------------------------------

struct ContextGuard {

    ContextGuard() = default;

    ContextGuard(JsonWriter *writer);
    ContextGuard(JsonWriter &writer);
    ContextGuard(ContextGuard&& other);

    ContextGuard& operator=(ContextGuard&& other);

    ~ContextGuard();

    JsonWriter *writer { nullptr };

};

}
