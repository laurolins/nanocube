#include <iostream>
#include <fstream>

#include "nanocube_language.hh"

namespace nanocube {
    
    namespace lang {
        
        // Node
        
        Node::Node(Type type):
        type(type)
        {}
        
        // Program
        
        Program::Program(std::string name):
        Node(PROGRAM),
        name(name)
        {}
        
        Program& Program::setFirstCall(Call *call) {
            this->first_call = call;
            return *this;
        }
        
        Call* Program::findCallByName(std::string call) {
            auto v = this->first_call;
            while (v != nullptr) {
                if (v->name.compare(call) == 0)
                    return v;
                v = v->next_call;
            }
            return nullptr;
        }
        
        // String
        
        String::String(std::string st):
        Node(STRING),
        st(st)
        {}
        
        // Number
        
        Number::Number(uint64_t number):
        Node(NUMBER),
        number(number)
        {}
        
        // Call
        
        Call::Call(std::string name):
        Node(CALL),
        name(name)
        {}
        
        Call& Call::setNextCall(Call *call) {
            this->next_call = call;
            return *this;
        }
        
        Call& Call::addParameter(Node *node) {
            this->params.push_back(node);
            return *this;
        }
        
        // List
        
        List::List():
        Node(LIST)
        {}
        
        List& List::addItem(Node *node) {
            this->items.push_back(node);
            return *this;
        }
        
        // IO
        
        std::ostream &operator<<(std::ostream& os, Node& node) {
            switch (node.type) {
                case CALL:
                    os << reinterpret_cast<Call&>(node);
                    break;
                case LIST:
                    os << reinterpret_cast<List&>(node);
                    break;
                case NUMBER:
                    os << reinterpret_cast<Number&>(node);
                    break;
                case STRING:
                    os << reinterpret_cast<String&>(node);
                    break;
                case PROGRAM:
                    os << reinterpret_cast<Program&>(node);
                    break;
                default:
                    throw std::runtime_error("ooops");
            }
            return os;
        }
        
        std::ostream &operator<<(std::ostream& os, Program& program) {
            os << program.name;
            if (program.first_call)
                os << *program.first_call;
            return os;
        }
        
        std::ostream &operator<<(std::ostream& os, String& string) {
            os << "\"" << string.st << "\"";
            return os;
        }
        
        std::ostream &operator<<(std::ostream& os, Number& number) {
            os << number.number;
            return os;
        }
        
        std::ostream &operator<<(std::ostream& os, Call& call) {
            os << "." << call.name << "(";
            bool first = true;
            for (auto node_p: call.params) {
                if (!first) {
                    os << ",";
                }
                os << *node_p;
                first = false;
            }
            os << ")";
            if (call.next_call)
                os << *call.next_call;
            return os;
        }
        
        std::ostream &operator<<(std::ostream& os, List& list) {
            os << "[";
            bool first = true;
            for (auto node_p: list.items) {
                if (!first) {
                    os << ",";
                }
                os << *node_p;
                first = false;
            }
            os << "]";
            return os;
        }
        
    } // nanocube::lang
    
} // nanocube

