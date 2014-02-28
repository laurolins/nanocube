#pragma once

#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <ostream>
#include <exception>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/bind.hpp>

#include "Query.hh"

namespace qi      = boost::spirit::qi;

namespace query {

namespace parser {

//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------

struct Expression;
struct Address;
struct AddressFunction;
struct Dimension;
struct SingleTarget;
struct RangeTarget;
struct SequenceTarget;
struct DiveTarget;

struct AddressExpression;
struct TargetExpression;

//------------------------------------------------------------------------------
// ExpressionException
//------------------------------------------------------------------------------

struct QueryParserException: public std::runtime_error {
public:
    QueryParserException(const std::string &message);
};

//------------------------------------------------------------------------------
// ExpressionType (type of expressions)
//------------------------------------------------------------------------------

enum ExpressionType { ADDRESS, DIMENSION, ADDRESS_FUNCTION,
                      SINGLE_TARGET, RANGE_TARGET, DIVE_TARGET,
                      SEQUENCE_TARGET, BASE_WIDTH_COUNT_TARGET };

//------------------------------------------------------------------------------
// Expression
//------------------------------------------------------------------------------

struct Expression {

public: // Subtypes


public: // Constructor e Destructor

    Expression(ExpressionType type);

    virtual ~Expression();

    virtual Address*           asAddress();
    virtual AddressFunction*   asAddressFunction();
    virtual Dimension*         asDimension();

    virtual SingleTarget*      asSingleTarget();
    virtual RangeTarget*       asRangeTarget();
    virtual SequenceTarget*    asSequenceTarget();
    virtual DiveTarget*        asDiveTarget();

    virtual AddressExpression* asAddressExpression();
    virtual TargetExpression*  asTargetExpression();

public: // Data Memebers

    ExpressionType type;

};

//------------------------------------------------------------------------------
// TargetExpression
//------------------------------------------------------------------------------

struct TargetExpression: public Expression {

    TargetExpression(ExpressionType type);

    TargetExpression* asTargetExpression();

    virtual void updateQueryDescription(int dimension_index, ::query::QueryDescription &qd) const = 0;

};

//------------------------------------------------------------------------------
// Number
//------------------------------------------------------------------------------

struct AddressExpression: public Expression {

    AddressExpression(ExpressionType type);

    AddressExpression* asAddressExpression();

    virtual RawAddress getRawAddress() const = 0;

};

//------------------------------------------------------------------------------
// Address
//------------------------------------------------------------------------------

struct Address: public AddressExpression {

public: // Constructor

    Address(uint64_t number);

public: // Methods

    virtual RawAddress getRawAddress() const;

    Address *asAddress();

public:

    uint64_t number;

};

//------------------------------------------------------------------------------
// Function
//------------------------------------------------------------------------------

struct AddressFunction: public AddressExpression {

public: // Constructor

    AddressFunction(std::string name);

public: // Methods

    AddressFunction *asAddressFunction();

    RawAddress getRawAddress() const;

public: // Data Members

    std::string name;

    std::vector<uint64_t> parameters;

};


//------------------------------------------------------------------------------
// SingleTarget
//------------------------------------------------------------------------------

struct SingleTarget: public TargetExpression {

public: // Constructor

    SingleTarget(AddressExpression *address);

public: // Methods

    SingleTarget *asSingleTarget();

    void updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const;

public: // data members

    AddressExpression *address;

};


//------------------------------------------------------------------------------
// RangeTarget
//------------------------------------------------------------------------------

struct RangeTarget: public TargetExpression {

public: // Constructor

    RangeTarget(AddressExpression *min, AddressExpression *max);

public: // Methods

    RangeTarget *asRangeTarget();

    void updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const;

public: // data members

    AddressExpression *min;

    AddressExpression *max;

};

//------------------------------------------------------------------------------
// SequenceTarget
//------------------------------------------------------------------------------

struct SequenceTarget: public TargetExpression {

public: // Constructor

    SequenceTarget();

public: // Methods

    SequenceTarget *asSequenceTarget();

    void addAddressExpression(AddressExpression *addr_exp);

    void updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const;

public: // data members

    std::vector<AddressExpression*> addresses;

};

//------------------------------------------------------------------------------
// DiveTarget
//------------------------------------------------------------------------------

struct DiveTarget: public TargetExpression {

public: // Constructor

    DiveTarget(AddressExpression *base_address, uint64_t dive_depth);

public: // Methods

    DiveTarget *asDiveTarget();

    void updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const;

public: // data members

    AddressExpression *base_address;

    uint64_t    dive_depth;

};


//------------------------------------------------------------------------------
// BaseWidthCountTarget
//------------------------------------------------------------------------------

struct BaseWidthCountTarget: public TargetExpression {

public: // Constructor

    BaseWidthCountTarget(AddressExpression *base_address, uint64_t width, uint64_t count);

public: // Methods

    BaseWidthCountTarget *asBaseWidthCountTarget();

    void updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const;

public: // data members

    AddressExpression* base_address;
    uint64_t           width;
    uint64_t           count;

};

//------------------------------------------------------------------------------
// Dimension
//------------------------------------------------------------------------------

struct Dimension: public Expression {

    Dimension(std::string name, bool anchored);

public: // Methods

    void setTarget(TargetExpression *target);

    Dimension *asDimension();

public: // Data Member

    std::string name;
    bool        anchored;

    TargetExpression *target;

};


//------------------------------------------------------------------------------
// QueryParser
//------------------------------------------------------------------------------

struct QueryParser: qi::grammar<std::string::const_iterator> {

public: // subtypes

    typedef typename std::string::const_iterator iterator_type;

public: // constructor

    QueryParser();

public: //

    void parse(std::string query_st);

private: // methods

    void pushAddress(uint64_t number);
    void pushName(std::string name);
    void setAnchor();

    void startFunction(std::string name);
    void addFunctionParameter(uint64_t number);

    void startDimension(std::string name);
    void setTarget();

    void pushDimensionSpec();

    void pushRangeTarget();
    void pushDiveTarget(uint64_t dive_depth);
    void pushSingleTarget();

    void pushSequenceTarget();
    void pushSequenceStart();

    void pushNumber(uint64_t number);

    void pushBaseWidthCountTarget();

    void collectDimensions();
   
    template <typename T>
    T* allocate(T *obj) {
        allocated_objects.push_back(std::unique_ptr<Expression>(obj));
        return obj;
    }
    
public: // data members
    
    std::vector<std::unique_ptr<Expression>> allocated_objects;

    std::stack<Expression*>    stack;
    std::vector<Dimension*>    dimensions;
    std::stack<uint64_t>       number_stack;
    bool                       anchor_flag;

    // qi::rule< Iterator,  double() >      number;
    // qi::rule< Iterator , std::string() > name;
    // qi::rule< Iterator>  expression;
    // qi::rule< Iterator>  factor;
    // qi::rule< Iterator>  term;
    // qi::rule< Iterator>  logic_expression;

    // function type
    qi::rule<iterator_type, std::string()> name;
    qi::rule<iterator_type, uint64_t() >   number;

    qi::rule<iterator_type>  address_expression;
    qi::rule<iterator_type>  raw_numeric_address;
    qi::rule<iterator_type>  function_expression;

    qi::rule<iterator_type>  target_expression;

    qi::rule<iterator_type>  list_expression;
    qi::rule<iterator_type>  dive_or_single_expression;
    qi::rule<iterator_type>  range_expression;
    qi::rule<iterator_type>  sequence_expression;

    qi::rule<iterator_type>  sequence_start_expression;
    qi::rule<iterator_type>  anchor_expression;
    qi::rule<iterator_type>  dimension_expression;

    qi::rule<iterator_type>  query_expression;
};

//------------------------------------------------------------------------------
// Print
//------------------------------------------------------------------------------

struct Print {

    struct Item {
        Item();
        Item(Expression *e, int level, std::string prefix);

        Expression *expression;
        int         level;
        std::string prefix;
    };

    Print(QueryParser &query_parser);

    void print(std::ostream &os) const;

    mutable std::stack<Item> stack;
    QueryParser &query_parser;
};

//------------------------------------------------------------------------------
// << operator
//------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const Print &print);

} // parser namespace

} // query namespace

