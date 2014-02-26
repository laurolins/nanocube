#include "json.hh"

namespace json {

//-----------------------------------------------------------------------------
// JsonWriter
//-----------------------------------------------------------------------------

JsonWriter::JsonWriter(std::ostream &os):
    os(os)
{
    context_stack.push(Context(this, PLAIN));
}

JsonWriter& JsonWriter::plain() {
    *this << ""; // make sure current context separators are triggered
    context_stack.push(Context(this, PLAIN));
    return *this;
}

JsonWriter& JsonWriter::dict(std::string name) {
    *this << ""; // make sure current context separators are triggered
    if (name.size() > 0)
        os << "\"" << name << "\"" << ":";
    context_stack.push(Context(this, DICT));
    return *this;
}

JsonWriter& JsonWriter::dict_entry(std::string name, std::string value) {
    *this << "\"" + name + "\":" + "\"" + value + "\"";
    return *this;
}

JsonWriter& JsonWriter::dict_entry(std::string name, uint64_t value) {
    *this << "\"" + name + "\":" + std::to_string(value) ;
    return *this;
}

JsonWriter& JsonWriter::list(std::string name) {
    *this << ""; // make sure current context separators are triggered
    if (name.size() > 0)
        os << "\"" << name << "\"" << ":";
    context_stack.push(Context(this, LIST));
    return *this;
}

JsonWriter& JsonWriter::pop() {
    context_stack.pop();
    return *this;
}

JsonWriter::~JsonWriter() {
    while (!context_stack.empty())
        context_stack.pop();
}

auto JsonWriter::operator<<(const std::string &st) -> JsonWriter& {
    Context &context = context_stack.top();
    context << st;
    return *this;
}

//-----------------------------------------------------------------------------
// Context Implementation
//-----------------------------------------------------------------------------

Context::Context(Context&& other) {
    *this = std::move(other);
}

Context& Context::operator=(Context&& other) {
    std::swap(writer,   other.writer);
    std::swap(start_st, other.start_st);
    std::swap(end_st,   other.end_st);
    std::swap(sep_st,   other.sep_st);
    std::swap(use_sep,  other.use_sep);
    return *this;
}

Context::Context(JsonWriter *writer, ContextType type):
    writer(writer)
{
    if (type == LIST) {
        start_st = "[ ";
        end_st   = " ]";
        sep_st   = ", ";
    }
    else if (type == DICT) {
        start_st = "{ ";
        end_st   = " }";
        sep_st   = ", ";
    }
    writer->os << start_st;
}


Context::~Context() {
    if (writer)
        writer->os << end_st;
}

auto Context::operator<<(const std::string &st) -> Context& {
    if (writer) {
        if (use_sep)
            writer->os << sep_st;
        else
            use_sep = true;
        writer->os << st;
    }
    return *this;
}


//-----------------------------------------------------------------------------
// Context Guard
//-----------------------------------------------------------------------------

ContextGuard::ContextGuard(JsonWriter *writer):
    writer(writer)
{}

ContextGuard::ContextGuard(JsonWriter &writer):
    writer(&writer)
{}

ContextGuard::ContextGuard(ContextGuard&& other) {
    *this = std::move(other);
}

ContextGuard& ContextGuard::operator=(ContextGuard&& other) {
    std::swap(writer,other.writer);
    return *this;
}

ContextGuard::~ContextGuard()
{
    if (writer)
        writer->context_stack.pop();
}

}
