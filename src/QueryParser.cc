#include "QueryParser.hh"

#include "QuadTree.hh"

namespace query {

namespace parser {

//------------------------------------------------------------------------------
// ExpressionException
//------------------------------------------------------------------------------

QueryParserException::QueryParserException(const std::string &message):
    std::runtime_error(message)
{}

//------------------------------------------------------------------------------
// TYPE_NAMES
//------------------------------------------------------------------------------

const std::vector<std::string> TYPE_NAMES = {
    "ADDRESS",
    "DIMENSION",
    "ADDRESS_FUNCTION",
    "SINGLE_TARGET",
    "RANGE_TARGET",
    "DIVE_TARGET",
    "POLYGON_TARGET"
};

//------------------------------------------------------------------------------
// Expression
//------------------------------------------------------------------------------

Expression::Expression(ExpressionType type):
    type(type)
{}

Expression::~Expression()
{}

Address*   Expression::asAddress() {
    throw QueryParserException("Cannot convert Expression to Number (type was: " + TYPE_NAMES[type] +")");
}

AddressFunction* Expression::asAddressFunction() {
    throw QueryParserException("Cannot convert Expression to Function (type was: " + TYPE_NAMES[type] +")");
}

Dimension* Expression::asDimension() {
    throw QueryParserException("Cannot convert Expression to Dimension (type was: " + TYPE_NAMES[type] +")");
}

SingleTarget *Expression::asSingleTarget()
{
    throw QueryParserException("Cannot convert Expression to SingleTarget (type was: " + TYPE_NAMES[type] +")");
}

RangeTarget* Expression::asRangeTarget() {
    throw QueryParserException("Cannot convert Expression to RangeTarget (type was: " + TYPE_NAMES[type] +")");
}

SequenceTarget* Expression::asSequenceTarget() {
    throw QueryParserException("Cannot convert Expression to SequenceTarget (type was: " + TYPE_NAMES[type] +")");
}

DiveTarget *Expression::asDiveTarget()
{
    throw QueryParserException("Cannot convert Expression to DiveTarget (type was: " + TYPE_NAMES[type] +")");
}

AddressExpression *Expression::asAddressExpression()
{
    throw QueryParserException("Cannot convert Expression to AddressExpression (type was: " + TYPE_NAMES[type] +")");
}

TargetExpression *Expression::asTargetExpression()
{
    throw QueryParserException("Cannot convert Expression to TargetExpression (type was: " + TYPE_NAMES[type] +")");
}

//------------------------------------------------------------------------------
// TargetExpression
//------------------------------------------------------------------------------

TargetExpression::TargetExpression(ExpressionType type):
    Expression(type)
{}

TargetExpression *TargetExpression::asTargetExpression()
{
    return this;
}

//------------------------------------------------------------------------------
// AddressExpression
//------------------------------------------------------------------------------

AddressExpression::AddressExpression(ExpressionType type):
    Expression(type)
{}

AddressExpression *AddressExpression::asAddressExpression()
{
    return this;
}

//------------------------------------------------------------------------------
// Address
//------------------------------------------------------------------------------

Address::Address(uint64_t number):
        AddressExpression(ADDRESS),
        number(number)
{}

RawAddress Address::getRawAddress() const
{
    return (RawAddress) number;
}

Address* Address::asAddress() {
    return this;
}

//------------------------------------------------------------------------------
// Function
//------------------------------------------------------------------------------

AddressFunction::AddressFunction(std::string name):
    AddressExpression(ADDRESS_FUNCTION),
    name(name)
{}

RawAddress AddressFunction::getRawAddress() const
{
    // only qaddr for now
    // assert(parameters.size() == 3 && name.compare("qaddr") == 0);
    
    if (parameters.size() == 3 && name.compare("qaddr") == 0) {
        
        int x     = (int) parameters[0];
        int y     = (int) parameters[1];
        int level = (int) parameters[2];
        
        typedef quadtree::Address<29,int> QAddr;
        
        QAddr addr(x, y, level, true);
        
#if 0
        std::cout << "QAddr(x:" << x << ", y:" << y << ", level:" << level << ")" << std::endl;
        std::cout << "   x:     " << addr.getLevelXCoord() << std::endl;
        std::cout << "   y:     " << addr.getLevelYCoord() << std::endl;
        std::cout << "   level: " << addr.level            << std::endl;
        std::cout << "   raw:   " << addr.raw()            << std::endl;
        QAddr addr2(addr.raw());
        std::cout << "QAddr(" << addr.raw() << ")" << std::endl;
        std::cout << "   x:     " << addr2.getLevelXCoord() << std::endl;
        std::cout << "   y:     " << addr2.getLevelYCoord() << std::endl;
        std::cout << "   level: " << addr2.level            << std::endl;
        std::cout << "   raw:   " << addr2.raw()            << std::endl;
#endif
        
        // std::cout << "getRawAddress: " << addr.raw() << "  from  " << addr << " check " << QAddr(addr.raw()).raw() << std::endl;
        
        return (RawAddress) addr.raw();
    }
    else if (parameters.size() == 0 && name.compare("root") == 0) {
        
        // root function
        return (RawAddress) 0xFFFFFFFFFFFFFFFFULL; // indicates root
        
    }
    else throw QueryParserException("Don't know function " + std::string(name));
}

AddressFunction* AddressFunction::asAddressFunction() {
    return this;
}

//------------------------------------------------------------------------------
// SingleTarget
//------------------------------------------------------------------------------

SingleTarget::SingleTarget(AddressExpression *address):
    TargetExpression(SINGLE_TARGET),
    address(address)
{}

SingleTarget *SingleTarget::asSingleTarget()
{
    return this;
}

void SingleTarget::updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const
{
    q.setFindAndDiveTarget(dimension_index, address->getRawAddress(), (int) 0);
}

//------------------------------------------------------------------------------
// RangeTarget
//------------------------------------------------------------------------------

RangeTarget::RangeTarget(AddressExpression *min, AddressExpression *max):
    TargetExpression(RANGE_TARGET),
    min(min), max(max)
{}

RangeTarget *RangeTarget::asRangeTarget() {
    return this;
}

void RangeTarget::updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const
{
    q.setRangeTarget(dimension_index,min->getRawAddress(), max->getRawAddress());
}

//------------------------------------------------------------------------------
// SequenceTarget
//------------------------------------------------------------------------------

SequenceTarget::SequenceTarget():
    TargetExpression(SEQUENCE_TARGET)
{}

void SequenceTarget::addAddressExpression(AddressExpression* addr_exp) {
    this->addresses.push_back(addr_exp);
}

SequenceTarget *SequenceTarget::asSequenceTarget() {
    return this;
}

void SequenceTarget::updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const {
    std::vector<RawAddress> raw_addresses;
    for (AddressExpression *addr_exp: addresses) {
        raw_addresses.push_back(addr_exp->getRawAddress());
    }
    q.setSequenceTarget(dimension_index, raw_addresses);
}

//------------------------------------------------------------------------------
// DiveTarget
//------------------------------------------------------------------------------

DiveTarget::DiveTarget(AddressExpression *base_address, uint64_t dive_depth):
    TargetExpression(DIVE_TARGET),
    base_address(base_address),
    dive_depth(dive_depth)
{}

DiveTarget *DiveTarget::asDiveTarget()
{
    return this;
}

void DiveTarget::updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const
{
    q.setFindAndDiveTarget(dimension_index,base_address->getRawAddress(),(int) this->dive_depth);
}

//------------------------------------------------------------------------------
// BaseWidthCountTarget
//------------------------------------------------------------------------------

BaseWidthCountTarget::BaseWidthCountTarget(AddressExpression *base_address, uint64_t width, uint64_t count):
    TargetExpression(BASE_WIDTH_COUNT_TARGET),
    base_address(base_address),
    width(width),
    count(count)
{}

BaseWidthCountTarget* BaseWidthCountTarget::asBaseWidthCountTarget()
{
    return this;
}

void BaseWidthCountTarget::updateQueryDescription(int dimension_index, ::query::QueryDescription &q) const
{
    q.setBaseWidthCountTarget(dimension_index, base_address->getRawAddress(),(int) this->width, (int) this->count);
}

//------------------------------------------------------------------------------
// Dimension
//------------------------------------------------------------------------------

Dimension::Dimension(std::string name, bool anchored):
    Expression(DIMENSION),
    name(name), anchored(anchored), target(nullptr)
{}

void Dimension::setTarget(TargetExpression *target)
{
    this->target = target;
}

Dimension *Dimension::asDimension()
{
    return this;
}

//------------------------------------------------------------------------------
// QueryParser
//------------------------------------------------------------------------------

QueryParser::QueryParser():
    QueryParser::base_type(query_expression),
    anchor_flag(false)
{

    // auto push_name              = boost::bind(&QueryParser::pushName,          this, _1);
    auto push_address             = boost::bind(&QueryParser::pushAddress,              this, _1);
    auto push_number              = boost::bind(&QueryParser::pushNumber,               this, _1);
    auto start_function           = boost::bind(&QueryParser::startFunction,            this, _1);
    auto start_dimension          = boost::bind(&QueryParser::startDimension,           this, _1);
    auto push_dive_target         = boost::bind(&QueryParser::pushDiveTarget,           this, _1);
    auto add_function_parameter   = boost::bind(&QueryParser::addFunctionParameter,     this, _1);
    auto push_single_target       = boost::bind(&QueryParser::pushSingleTarget,         this);
    auto push_range_target        = boost::bind(&QueryParser::pushRangeTarget,          this);
    auto push_sequence_target     = boost::bind(&QueryParser::pushSequenceTarget,       this);
    auto push_sequence_start      = boost::bind(&QueryParser::pushSequenceStart,        this);
    auto set_anchor               = boost::bind(&QueryParser::setAnchor,                this);
    auto set_target               = boost::bind(&QueryParser::setTarget,                this);
    auto collect_dimensions       = boost::bind(&QueryParser::collectDimensions,        this);
    auto push_bwc_target          = boost::bind(&QueryParser::pushBaseWidthCountTarget, this);

    name    %= ( qi::char_("a-zA-Z_") >> *qi::char_("a-zA-Z_0-9") );

    number  %= qi::ulong_;

    function_expression  =
            ( name [start_function]
              >> '(' >> - ( number [add_function_parameter]
                            >> * (',' >> number [add_function_parameter] ) )
              >> ')' );

    anchor_expression = "@" ;

    sequence_start_expression = "<" ;

    raw_numeric_address = number [push_address];

    address_expression = ( raw_numeric_address ||
                           function_expression );

    range_expression = ( '[' >> address_expression >> ',' >> address_expression >> ']' )[push_range_target];

    sequence_expression = ( sequence_start_expression [push_sequence_start] >>
                            - ( address_expression >> * (',' >> address_expression )  ) >>
                            '>' ) [push_sequence_target];

    dive_or_single_expression  = ( address_expression[push_single_target] >>
                                   - (
                                       ('+' >> number[push_dive_target])
                                       ||
                                       (':' >> number[push_number] >> ':' >> number[push_number])[push_bwc_target]
                                       )
                                   );

    target_expression = (  dive_or_single_expression               // <--- ambiguous
                           || range_expression
                           || sequence_expression
                           );

    dimension_expression = - ( anchor_expression [set_anchor] ) >>  name [ start_dimension ]
                                                                    >> "=" >> target_expression [ set_target ];

    query_expression = *( dimension_expression
                          >> * ( "/" >> dimension_expression ) )[collect_dimensions];

}

void QueryParser::pushAddress(uint64_t number) {
    // std::cout << "push address: " << number << std::endl;
    stack.push(allocate(new Address(number)));
}

void QueryParser::startFunction(std::string name) {
    // std::cout << "push function: " << name << std::endl;
    stack.push(allocate(new AddressFunction(name)));
}

void QueryParser::addFunctionParameter(uint64_t number) {

    // push parameter
    AddressFunction *function = stack.top()->asAddressFunction();
    function->parameters.push_back(number);
}

void QueryParser::startDimension(std::string name) {
    // std::cout << "start Dimension: " << name << std::endl;
    stack.push(allocate(new Dimension(name, anchor_flag)));
    anchor_flag = false; // consume anchor_flag
}

void QueryParser::pushRangeTarget() {
    // std::cout << "pop max and min then push RangeTarget" << std::endl;

    AddressExpression *max = stack.top()->asAddressExpression();
    stack.pop();

    AddressExpression *min = stack.top()->asAddressExpression();
    stack.pop();

    stack.push(allocate(new RangeTarget(min,max)));
}

void QueryParser::pushSequenceStart() {
    // std::cout << "pop max and min then push RangeTarget" << std::endl;
    stack.push(nullptr);
}

void QueryParser::pushSequenceTarget() {
    // std::cout << "pop max and min then push RangeTarget" << std::endl;

    SequenceTarget* seq_target = allocate(new SequenceTarget());
    
    while (stack.top() != nullptr) {
        seq_target->addAddressExpression(stack.top()->asAddressExpression());
        stack.pop();
    }
    stack.pop(); // pop the nullptr marker

    stack.push(seq_target);
}

void QueryParser::pushDiveTarget(uint64_t dive_depth) {
    // std::cout << "pop depth an base address then push DiveTarget" << std::endl;

    //
    SingleTarget *single_target = stack.top()->asSingleTarget();
    stack.pop();

    stack.push(allocate(new DiveTarget(single_target->address, dive_depth)));

    // delete single_target;
}

void QueryParser::pushNumber(uint64_t number) {
    this->number_stack.push(number);
}

void QueryParser::pushBaseWidthCountTarget() {
    // std::cout << "pop depth an base address then push DiveTarget" << std::endl;

    uint64_t count = number_stack.top(); number_stack.pop();
    uint64_t width = number_stack.top(); number_stack.pop();

    //
    // AddressExpression *base = stack.top()->asAddressExpression(); stack.pop();

    //
    SingleTarget *single_target = stack.top()->asSingleTarget();
    stack.pop();

    stack.push(allocate(new BaseWidthCountTarget(single_target->address, width, count)));

    // delete single_target;
}

void QueryParser::pushSingleTarget()
{
    AddressExpression *addr = stack.top()->asAddressExpression();
    stack.pop();

    stack.push(allocate(new SingleTarget(addr)));
}

void QueryParser::setTarget() {
    // top of stack should be the expression parameter
    TargetExpression *target = stack.top()->asTargetExpression();
    stack.pop();

    // push parameter
    Dimension *dimension = stack.top()->asDimension();
    dimension->setTarget(target);
}

void QueryParser::setAnchor() {
    // std::cout << "setAnchor: " << std::endl;
    anchor_flag = true;
}

void QueryParser::collectDimensions()
{
    while (!stack.empty()) {
        Dimension *dimension = stack.top()->asDimension();
        dimensions.push_back(dimension);
        stack.pop();
    }
    // reverse vector
    // std::reverse(dimensions.begin(), dimensions.end());
}


void QueryParser::parse(std::string query_st)
{
    // clear any previous result
    this->allocated_objects.clear();
    
    
    int result = 0;
    typename QueryParser::iterator_type begin = query_st.begin();
    typename QueryParser::iterator_type end   = query_st.end();
    QueryParser &parser = *this;
    bool ok = qi::parse(begin, end, parser, result);
    if (!ok || begin != end) {
        std::stringstream ss;
        auto offset = begin - query_st.begin();
        int margin = 3;
        ss << "Couldn't parse expression at location " << offset << std::endl;
        ss << std::string(margin, ' ') << query_st << std::endl;
        ss << std::string(offset + margin,' ') << "^" << std::endl;
        throw QueryParserException(ss.str());
    }
}

//------------------------------------------------------------------------------
// Print::Item
//------------------------------------------------------------------------------

Print::Item::Item()
{}

Print::Item::Item(Expression *e, int level, std::string prefix):
    expression(e),
    level(level),
    prefix(prefix)
{}

//------------------------------------------------------------------------------
// Print
//------------------------------------------------------------------------------

Print::Print(QueryParser &query_parser):
    query_parser(query_parser)
{}

void Print::print(std::ostream &os) const {

    for (Dimension *dim: query_parser.dimensions) {
        stack.push(Item(dim,0,""));
    }

    while (!stack.empty()) {
        Item item = stack.top();
        Expression *e      = item.expression;
        std::string prefix = item.prefix;
        int         level  = item.level;
        stack.pop();

        os << std::string(3 * level, ' ') << prefix;

        if (e->type == ADDRESS) {
            Address *address = e->asAddress();
            os << "Address[address=" << address->number << "]";
            os << std::endl;
        }
        else if (e->type == ADDRESS_FUNCTION) {
            AddressFunction *function = e->asAddressFunction();
            os << "Function[name=" << function->name << " params=";
            bool first = true;
            for (uint64_t p: function->parameters) {
                if (!first) {
                    os << ",";
                }
                os << p;
                first = false;
            }
            os << "]" << std::endl;
        }
        else if (e->type == DIMENSION) {
            Dimension *dimension = e->asDimension();
            os << "Dimension[name=" << dimension->name << " "
               << "anchored=" << dimension->anchored << "]";
            os << std::endl;
            stack.push(Item(dimension->target,level+1, "target: "));
        }
        else if (e->type == SINGLE_TARGET) {
            SingleTarget *single_target = e->asSingleTarget();
            os << "SingleTarget[]";
            os << std::endl;
            stack.push(Item(single_target->address,level+1,"base: "));
        }
        else if (e->type == RANGE_TARGET) {
            RangeTarget *range_target = e->asRangeTarget();
            os << "RangeTarget[]";
            os << std::endl;
            stack.push(Item(range_target->max,level+1,"max: "));
            stack.push(Item(range_target->min,level+1,"min: "));
        }
        else if (e->type == SEQUENCE_TARGET) {
            SequenceTarget *sequence_target = e->asSequenceTarget();
            os << "SequenceTarget[]";
            os << std::endl;
            int i=0;
            for (auto addr: sequence_target->addresses) {
                stack.push(Item(addr,level+1,"["+std::to_string(i++)+"]"));
            }
        }
        else if (e->type == DIVE_TARGET) {
            DiveTarget *dive_target = e->asDiveTarget();
            os << "DiveTarget[dive_depth="+std::to_string(dive_target->dive_depth)+"]";
            os << std::endl;
            stack.push(Item(dive_target->base_address,level+1,"base: "));
        }

    }
}

//------------------------------------------------------------------------------
// << operator
//------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const Print &print) {
    print.print(os);
    return os;
}


} // namespace parser

} // namespace query



