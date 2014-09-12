#pragma once

#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <ostream>
#include <exception>

#include <boost/config/warning_disable.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/bind.hpp>

namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

namespace nanocube {
    
    namespace lang {

        // Abstract Syntax Tree elements
        
        struct Node;
        struct Program;
        struct String;
        struct Number;
        struct Call;
        struct List;
        
        enum Type { PROGRAM, STRING, CALL, NUMBER, LIST };
        
        struct Node {
        public:
            Node(Type type);
            Type type;
        };
        
        struct Program: public Node {
        public:
            Program(std::string name);
            Program& setFirstCall(Call *call);
            
            
            Call* findCallByName(std::string call);
            
        public:
            std::string name;
            Call* first_call { nullptr };
        };
        
        struct String: public Node {
        public:
            String(std::string st);
        public:
            std::string st;
        };
        
        struct Number: public Node {
        public:
            Number(uint64_t number);
        public:
            uint64_t number;
        };
        
        struct Call: public Node {
        public:
            Call(std::string name);
            Call& setNextCall(Call *call);
            Call& addParameter(Node* param);
        public:
            std::string name;
            std::vector<Node*> params;
            Call* next_call { nullptr };
        };
        
        struct List: public Node {
        public:
            List();
            List& addItem(Node* item);
        public:
            std::vector<Node*> items;
        };
        
        std::ostream &operator<<(std::ostream& os, Node& node);
        std::ostream &operator<<(std::ostream& os, Program& program);
        std::ostream &operator<<(std::ostream& os, String& string);
        std::ostream &operator<<(std::ostream& os, Number& number);
        std::ostream &operator<<(std::ostream& os, Call& call);
        std::ostream &operator<<(std::ostream& os, List& list);


        // Parser
        
        template <typename Iterator>
        struct Parser: qi::grammar< Iterator > // start rule skipper
        {
            
        public: // subtypes
            
            typedef Iterator iterator_type;
            
        public: // constructor
            
            Parser();
            
        public: //
            
            void parse(iterator_type begin, iterator_type end);
            
            template <typename T>
            T* registerNodeAST(T* node);
            
            void reset();
            
        public: // data members
            
            std::vector<std::unique_ptr<Node>> ast_nodes;
            
            std::vector<Node*> stack;
            
            Program *program { nullptr }; // last parsed program
            
            qi::rule< iterator_type, double() >      number;
            
            qi::rule< iterator_type, std::string() > name;
            
            qi::rule< iterator_type >  list; // start rule of grammar
            
            qi::rule< iterator_type >  string; // start rule of grammar
            
            qi::rule< iterator_type >  expression; // start rule of grammar
            
            qi::rule< iterator_type >  call; // start rule of grammar
            
            qi::rule< iterator_type >  start; // start rule of grammar

            // skipper: ascii::space_type
            // qi::rule<iterator_type, std::string(), ascii::space_type> quoted_string;
            
        };
        
        // Implementation
        
        template <typename Iterator>
        void Parser<Iterator>::reset() {
            ast_nodes.clear();
            stack.clear();
            program = nullptr;
        }
        
        template <typename Iterator>
        template <typename T>
        T* Parser<Iterator>::registerNodeAST(T* node) {
            ast_nodes.push_back(std::unique_ptr<T>(node));
            return node;
        }
        
        template <typename Iterator>
        Parser<Iterator>::Parser():
        Parser::base_type(start, "nanocube_query_grammar")
        {
            
            auto &grammar = *this;
            
            auto push_program_begin = [&grammar](std::string name) {
                grammar.reset();
                grammar.program = grammar.registerNodeAST(new Program(name));
                grammar.stack.clear();
            };
            
            auto push_program_end = [&grammar]() {
                auto &stack = grammar.stack;
                Call *previous_call = nullptr;
                for (auto it=stack.begin();it!=stack.end();++it) {
                    auto node = *it;
                    if (node == nullptr || node->type != CALL)
                        throw std::runtime_error("Not expecting non-call params for main program");
                    Call *call = reinterpret_cast<Call*>(*it);
                    if (previous_call) {
                        previous_call->setNextCall(call);
                    }
                    else {
                        grammar.program->setFirstCall(call);
                    }
                    previous_call = call;
                }
                stack.clear();
            };
            
            auto push_call_begin = [&grammar](std::string name) {
                auto &stack = grammar.stack;
                stack.push_back(grammar.registerNodeAST(new Call(name)));
                stack.push_back(nullptr); // mark the beginning of a parenthesis
            };
            
            auto push_call_end = [&grammar]() {
                auto &stack = grammar.stack;
                auto back_it  = stack.rbegin();
                while (back_it != stack.rend() && *back_it != nullptr) {
                    ++back_it;
                }
                if (back_it == stack.rend() || back_it == (stack.rend()-1))
                    throw std::runtime_error("didn't find list beginning");
                
                if (*(back_it+1) == nullptr || (*(back_it+1))->type != CALL)
                    throw std::runtime_error("didn't find list beginning");
                
                auto call = reinterpret_cast<Call*>(*(back_it+1));
                
                // &*ri == &*(ri.base() - 1)
                auto forward_it = back_it.base()-1;
                auto params_it = forward_it + 1;
                
                while (params_it != stack.end()) {
                    call->params.push_back(*params_it);
                    ++params_it;
                }
                
                stack.erase(forward_it, stack.end());
            };
            
            auto push_list_begin = [&grammar]() {
                auto &stack = grammar.stack;
                stack.push_back(grammar.registerNodeAST(new List()));
                stack.push_back(nullptr); // mark the beginning of a parenthesis
            };
            
            auto push_list_end = [&grammar]() {
                auto &stack = grammar.stack;
                auto back_it = stack.rbegin();
                while (back_it != stack.rend() && *back_it != nullptr) {
                    ++back_it;
                }
                if (back_it == stack.rend() || back_it == (stack.rend()-1))
                    throw std::runtime_error("didn't find list beginning");
                
                if (*(back_it+1) == nullptr || (*(back_it+1))->type != LIST)
                    throw std::runtime_error("didn't find list beginning");
                
                auto list = reinterpret_cast<List*>(*(back_it+1));
                
                // &*ri == &*(ri.base() - 1)
                auto forward_it = back_it.base()-1;
                auto params_it = forward_it + 1;
                
                while (params_it != stack.end()) {
                    list->items.push_back(*params_it);
                    ++params_it;
                }
                
                stack.erase(forward_it, stack.end());
            };
            
            
            auto push_string = [&grammar](std::vector<char> st) {
                grammar.stack.push_back(grammar.registerNodeAST(new String(std::string(st.begin(),st.end()))));
            };
            
            auto push_number = [&grammar](uint64_t number) {
                grammar.stack.push_back(grammar.registerNodeAST(new Number(number)));
            };
            
            
            name    %= ( qi::char_("a-zA-Z_") >> *qi::char_("a-zA-Z_0-9") );
            
            //string  %= "\"" >> (*qi::char_("a-zA-Z_0-9 ><")) [push_string]  >> "\"";
            
            
//            In general URIs as defined by RFC 3986 (see Section 2: Characters) may
//            contain any of the following characters:
//            
//            ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=
//            
//            Any other character needs to be encoded with the percent-encoding
//            (%hh). Each part of the URI has further restrictions about what
//            characters need to be represented by an percent-encoded word.

            
            string  %= "\"" >> (*(qi::char_ - '"')) [push_string]  >> "\"";
            
            number  %= qi::ulong_;
            
            // quoted_string %= qi::lexeme['"' >> +(qi::char_ - '"') >> '"'];
            
            list  = qi::lit("[") [push_list_begin] >> - ( expression >> * ("," >> expression) ) >> qi::lit("]") [push_list_end];
            
            expression = number [push_number] | string | call | list;
            
            call = name [push_call_begin] >> "(" >> - ( expression >> * ( "," >> expression ) ) >> qi::lit(")") [push_call_end];
            
            start = ( name [push_program_begin] >> * ( "." >> call ) ) [ push_program_end ];
            
        }
        
        template <typename Iterator>
        void Parser<Iterator>::parse(iterator_type begin, iterator_type end)
        {
            // clear any previous result
            // this->allocated_objects.clear();
            
            int result = 0;
            
            iterator_type walker = begin;
            
            Parser &parser = *this;
            bool ok = qi::parse(walker, end, parser, result);
            if (!ok || walker != end) {
                std::stringstream ss;
                auto offset = walker - begin;
                int margin = 3;
                ss << "Couldn't parse expression at location " << offset << std::endl;
                ss << std::string(margin, ' ') << std::string(begin,end) << std::endl;
                ss << std::string(offset + margin,' ') << "^" << std::endl;
                throw std::runtime_error(ss.str());
            }
        }
        
    }

}