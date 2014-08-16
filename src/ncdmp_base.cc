#include "ncdmp_base.hh"

#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>

#include <sstream>
#include <iomanip>
#include <string>

#include <locale>
#include <iomanip>
#include <ctime>
#include <chrono>

#include "MercatorProjection.hh"
#include "TimeBinFunction.hh"

#include <boost/date_time/posix_time/posix_time.hpp>

//-----------------------------------------------------------------------------
// time routines
//-----------------------------------------------------------------------------

std::time_t mkTimestamp(int year, int month, int day, int hour, int min, int sec)
{
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    timeinfo.tm_year = year	 - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = min;
    timeinfo.tm_sec  = sec;
    timeinfo.tm_isdst = 1; // <--- consider dst

    //
    return mktime ( &timeinfo );
}

std::time_t parse_datetime_ISO8601_extended(std::string st) {

    // this one needs to exist
    auto date_time_split_pt = std::find(st.begin(),st.end(),'T');

    // check this
    assert (date_time_split_pt != st.end());

    //
    // date
    //

    auto date_st_length = date_time_split_pt - st.begin();

    auto year_pt  = st.begin();
    auto month_pt = (date_st_length >= 7  ? st.begin() + 5 : st.end());
    auto day_pt   = (date_st_length == 10 ? st.begin() + 8 : st.end());

    // 1980 or 1980-12 or 1980-12-01
    assert (date_st_length == 4 || date_st_length == 7 || date_st_length == 10);

    int year = std::stoi(std::string(year_pt, year_pt+4));

    int month = 1;
    if (month_pt != st.end()) {
        assert( *(month_pt-1) == '-' );
        month = std::stoi(std::string(month_pt, month_pt + 2));
    }

    int day = 1;
    if (day_pt != st.end()) {
        assert( *(day_pt-1) == '-' );
        day = std::stoi(std::string(day_pt, day_pt + 2));
    }

    //
    // time zone delimiter
    //

    int time_zone_offset_in_minutes =  0;
    auto time_timezone_split_pt = std::find(date_time_split_pt,st.end(),'Z');
    if (time_timezone_split_pt == st.end()) {

        int sign = 1;
        time_timezone_split_pt = std::find(date_time_split_pt,st.end(),'+');

        if (time_timezone_split_pt == st.end()) {
            sign = -1;
            time_timezone_split_pt = std::find(date_time_split_pt,st.end(),'-');
        }

        if (time_timezone_split_pt != st.end()) {

            auto timezone_st_length = st.end() - time_timezone_split_pt;
            assert (timezone_st_length == 3 || timezone_st_length == 6);

            // now it can be hh or hh:mm
            time_zone_offset_in_minutes = 60 * std::stoi(std::string(time_timezone_split_pt + 1, st.end()));

            if (timezone_st_length == 6) {
                time_zone_offset_in_minutes += std::stoi(std::string(time_timezone_split_pt + 4, st.end()));
            }

            time_zone_offset_in_minutes *= sign;
        }
    }

    //
    // time
    //

    auto time_st_length = time_timezone_split_pt - date_time_split_pt;

    auto hour_pt  = date_time_split_pt + 1;
    auto min_pt   = (time_st_length >= 6  ? hour_pt + 3 : st.end());
    auto sec_pt   = (time_st_length == 9  ? min_pt  + 3 : st.end());

    // 1980 or 1980-12 or 1980-12-01
    assert (time_st_length == 3 || time_st_length == 6 || time_st_length == 9);

    int hour = std::stoi(std::string(hour_pt, hour_pt+2));

    int min = 0;
    if (min_pt != st.end()) {
        assert( *(min_pt-1) == ':' );
        min = std::stoi(std::string(min_pt, min_pt + 2));
    }

    int sec = 0;
    if (sec_pt != st.end()) {
        assert( *(sec_pt-1) == ':' );
        sec = std::stoi(std::string(sec_pt, sec_pt + 2));
    }

#if 0
    std::cout << "Year:  " << year << std::endl;
    std::cout << "Month: " << month << std::endl;
    std::cout << "Day:   " << day << std::endl;

    std::cout << "Hour:  " << hour << std::endl;
    std::cout << "Min:   " << min << std::endl;
    std::cout << "Sec:   " << sec << std::endl;

    std::cout << "TZmin: " << time_zone_offset_in_minutes << std::endl;
#endif

    return mkTimestamp(year, month, day, hour, min, sec);

}

//-----------------------------------------------------------------------------
// Split routines
//-----------------------------------------------------------------------------

std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
    return elems;
}

void deque_split(const std::string &s, char delim,
                  std::deque<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
}

//-----------------------------------------------------------------------------
// Record
//-----------------------------------------------------------------------------

Record::Record(dumpfile::DumpFileDescription &input_file_description):
    input_file_description(input_file_description)
{}

void Record::readNext(std::istream &is) {
    if (input_file_description.encoding == dumpfile::DumpFileDescription::binary) {
        is.read(buffer, input_file_description.record_size);
        if (is.eof()) {
            throw EndOfFile();
        }
    }
    else if (input_file_description.encoding == dumpfile::DumpFileDescription::text) {
        is.getline(buffer, BUFFER_SIZE);
        if (is.eof()) {
            throw EndOfFile();
        }

        std::string line(buffer);
        tokens.clear();
        split(line,' ',tokens);

        if (static_cast<int>(tokens.size()) != input_file_description.num_tokens) {
            throw WrongNumberOfInputTokens();
        }
    }
}

float Record::getFloat(dumpfile::Field *field) {
    if (input_file_description.isBinary()) {
        float result;
        int index =  field->offset_inside_record;
        std::copy(&buffer[index], &buffer[index] + sizeof(float), (char*) &result );
        return result;
    }
    else {
        int index =  field->first_token_index;
        return std::stof(tokens.at(index));
    }
}

uint64_t Record::getUInt(dumpfile::Field *field) {
    if (input_file_description.isBinary()) {
        uint64_t result = 0;
        int index =  field->offset_inside_record;
        int num_bytes = field->field_type.num_bytes;
        std::copy(&buffer[index], &buffer[index] + num_bytes, (char*) &result );
        return result;
    }
    else { // text
        int index =  field->first_token_index;
        return std::stoul(tokens.at(index));
    }
}

void Record::getUInts(dumpfile::Field *field, std::vector<uint64_t> &output) {
    int no_tokens = field->getNumTokens();
    if (input_file_description.isBinary()) {
        if (no_tokens == 1) {
            uint64_t v = 0;
            int index =  field->offset_inside_record;
            int num_bytes = field->field_type.num_bytes;
            std::copy(&buffer[index], &buffer[index] + num_bytes, (char*) &v );
            output.push_back(v);
        }
        else {
            uint64_t v = 0;
            int index =  field->offset_inside_record;
            int num_bytes = field->field_type.num_bytes;
            int no_bytes_per_token = num_bytes / no_tokens; // the remaining here must be 0
            for (int i=0;i<no_tokens;i++) {
                int offset = i * no_bytes_per_token;
                std::copy(&buffer[index] + offset,
                          &buffer[index] + offset + no_bytes_per_token, (char*) &v );
                output.push_back(v);
            }
        }
    }
    else { // text
        int index =  field->first_token_index;
        for (int i=0;i<no_tokens;i++) {
            output.push_back(std::stoul(tokens.at(index+i)));
        }
    }
}


std::time_t Record::getTime(dumpfile::Field *field) {
    if (input_file_description.isBinary()) {
        std::time_t result; // this should be 8 bytes
        int index =  field->offset_inside_record;
        std::copy(&buffer[index], &buffer[index] + sizeof(std::time_t), (char*) &result );
        return result;
    }
    else { // text
        int index =  field->first_token_index;
        return parse_datetime_ISO8601_extended(tokens.at(index));
    }
}

//-----------------------------------------------------------------------------
// TimeFunction
//-----------------------------------------------------------------------------

std::ostream &operator<<(std::ostream& os, const TimeFunction &time_function) {

    if (time_function == FUNCTION_HOUR_OF_DAY) {
        os << "dim-hour";
    }
    else if (time_function == FUNCTION_DAY_OF_WEEK) {
        os << "dim-weekday";
    }
    else if (time_function == FUNCTION_MONTH_OF_YEAR) {
        os << "dim-month";
    }
    else if (time_function == FUNCTION_UNKNOWN) {
        os << "unkown-time-function";
    }
    return os;
}

//-----------------------------------------------------------------------------
// FieldDescription
//-----------------------------------------------------------------------------

FieldDescription::FieldDescription(std::string name, FieldMapType type):
    name(name),
    type(type),
    mapping_scheme(nullptr)
{}

bool FieldDescription::isValid(dumpfile::DumpFileDescription &input_description, std::string &invalid_reason) {
    return true;
    // throw std::exception();
}

FD_DimDMQ *FieldDescription::asDimDMQ() { return nullptr; }

FD_DimTBin *FieldDescription::asDimTBin() { return nullptr; }

FD_DimCat *FieldDescription::asDimCat() { return nullptr; }

FD_DimTimeFunction *FieldDescription::asDimTimeFunction() { return nullptr; }

FD_VarUInt *FieldDescription::asVarUInt() { return nullptr; }

FD_VarOne *FieldDescription::asVarOne() { return nullptr; }

FD_FieldCopy *FieldDescription::asFieldCopy() { return nullptr; }

void FieldDescription::cacheFields(const dumpfile::DumpFileDescription &f)
{}

void FieldDescription::append(dumpfile::DumpFileDescription &output_file_descritpion)
{}

void FieldDescription::dump(Record &record, std::ostream& os)
{}

bool FieldDescription::checkFieldPresenceAndType
( dumpfile::DumpFileDescription &input_description,
  std::string field_name,
  std::string target_type_name,
  std::string &problem)
{
    try {
        dumpfile::Field *field = input_description.getFieldByName(field_name);

        if (!(field->field_type == dumpfile::FieldTypesList::getFieldType(target_type_name))) {
            std::stringstream ss;
            ss << "(Invalid Type) "
               << type
               << " requires an input field of type "
               << target_type_name << ", but "
               << field_name
               << " has type "
               << field->field_type.name;
            problem = ss.str();
            return false;
        }
    }
    catch(std::exception &e) {

        std::stringstream ss;
        ss << "(Input Field Not Found) could not find input field " << field_name;
        problem = ss.str();
        return false;

    }
    return true;
}

void FieldDescription::setMappingScheme(MappingScheme *mapping_scheme) {
    this->mapping_scheme = mapping_scheme;
}

bool FieldDescription::isOutputBinary() const {
    if (mapping_scheme == nullptr) {
        throw std::exception();
    }
    return mapping_scheme->output_file_descritpion.isBinary();
}

bool FieldDescription::isOutputText() const {
    if (mapping_scheme == nullptr) {
        throw std::exception();
    }
    return mapping_scheme->output_file_descritpion.isText();
}

//-----------------------------------------------------------------------------
// FD_DimDMQ
//-----------------------------------------------------------------------------

FD_DimDMQ::FD_DimDMQ(std::string name,
                     std::string lat_field_name,
                     std::string lon_field_name,
                     int quadtree_levels):
    FieldDescription(name, FD_DIM_DMQ),
    lat_field_name(lat_field_name),
    lat_field(nullptr),
    lon_field_name(lon_field_name),
    lon_field(nullptr),
    quadtree_levels(quadtree_levels)
{}

FD_DimDMQ *FD_DimDMQ::asDimDMQ() { return this; }

bool FD_DimDMQ::isValid(dumpfile::DumpFileDescription &input_description,
                                std::string &invalid_reason) {
    std::string lat_invalid_reason;
    bool lat_is_valid = this->checkFieldPresenceAndType(input_description,
                                                        lat_field_name,
                                                        "float",
                                                        invalid_reason);

    std::string lon_invalid_reason;
    bool lon_is_valid = this->checkFieldPresenceAndType(input_description,
                                                        lon_field_name,
                                                        "float",
                                                        invalid_reason);


    std::stringstream ss;
    if (!lat_is_valid)
        ss << "(latitude) " << lat_invalid_reason << ", ";
    if (!lon_is_valid)
        ss << "(longitude) " << lon_invalid_reason;

    if (!lat_is_valid || !lon_is_valid) {
        invalid_reason = ss.str();
        return false;
    }
    else return true;
}

void FD_DimDMQ::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    output_file_descritpion.metadata[name+"__origin"] = "degrees_mercator_quadtree" + std::to_string(quadtree_levels);

    std::string field_type_name = "nc_dim_quadtree_" + std::to_string(quadtree_levels);
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    output_file_descritpion.addField(name, field_type);
}

void FD_DimDMQ::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        lat_field = f.getFieldByName(lat_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << lat_field << std::endl;
        throw std::exception();
    }

    try {
        lon_field = f.getFieldByName(lon_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << lon_field << std::endl;
        throw std::exception();
    }
}

void FD_DimDMQ::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    float latitude  = record.getFloat(lat_field);
    float longitude = record.getFloat(lon_field);

    int32_t tile_x;
    int32_t tile_y;
    mercator::MercatorProjection::tileOfLongitudeLatitude(longitude, latitude, quadtree_levels, tile_x, tile_y);

    if (this->isOutputText()) {
        os << tile_x << " " << tile_y;
    }
    else {
        os.write(reinterpret_cast<char*>(&tile_x), sizeof(tile_x));
        os.write(reinterpret_cast<char*>(&tile_y), sizeof(tile_y));
    }
}

std::ostream &operator<<(std::ostream& os, const FD_DimDMQ &fd) {
    os << "dim-dmq" << "=" << fd.name << "," << fd.lat_field_name << "," << fd.lon_field_name << "," << fd.quadtree_levels;
    return os;
}



//-----------------------------------------------------------------------------
// FD_DimTBin
//-----------------------------------------------------------------------------

FD_DimTBin::FD_DimTBin(std::string name,
                       std::string time_field_name,
                       std::string tbin_spec,
                       int num_bytes,
                       bool binary_tree_variation):
    FieldDescription(name, FD_DIM_TBIN),
    time_field_name(time_field_name),
    time_field(nullptr),
    tbin_function(tbin_spec),
    num_bytes(num_bytes),
    binary_tree_variation(binary_tree_variation)
{}

FD_DimTBin *FD_DimTBin::asDimTBin() { return this; }

bool FD_DimTBin::isValid(dumpfile::DumpFileDescription &input_description,
                                 std::string &invalid_reason) {
    return this->checkFieldPresenceAndType(input_description,
                                           time_field_name,
                                           "uint64",
                                           invalid_reason);
}

void FD_DimTBin::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        time_field = f.getFieldByName(time_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << time_field_name << std::endl;
        throw std::exception();
    }
}

void FD_DimTBin::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    output_file_descritpion.metadata["tbin"] = tbin_function.getSpecificationString();
    
    std::string field_type_name = "nc_dim_time_" + std::to_string(num_bytes);
    if (binary_tree_variation)
        field_type_name = "nc_dim_bintree_" + std::to_string(8 * num_bytes);
    
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    output_file_descritpion.addField(name, field_type);
}

void FD_DimTBin::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    std::time_t t = record.getTime(time_field);

    // TODO: check local time info... clear up thing in the spec and here
    int64_t bin = tbin_function.getBin(t);

    if (this->isOutputText()) {
        os << bin;
    }
    else {
        // CHECK: this is safe (since num_bytes should be at most 64-bits)
        os.write(reinterpret_cast<char*>(&bin), num_bytes);
    }
}

std::ostream &operator<<(std::ostream& os, const FD_DimTBin &fd) {
    os << "dim-dmq" << "=" << fd.name << "," << fd.time_field_name << "," << fd.tbin_function.getSpecificationString() << "," << fd.num_bytes;
    return os;
}

//-----------------------------------------------------------------------------
// FD_DimCat
//-----------------------------------------------------------------------------

FD_DimCat::FD_DimCat(std::string name,
                     std::string uint_field_name):
    FieldDescription(name, FD_DIM_CAT),
    uint_field_name(uint_field_name),
    uint_field(nullptr)
{}

FD_DimCat *FD_DimCat::asDimCat() { return this; }

void FD_DimCat::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        uint_field = f.getFieldByName(uint_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << uint_field_name << std::endl;
        throw std::exception();
    }
}

void FD_DimCat::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    std::string field_type_name = "nc_dim_cat_" + std::to_string(uint_field->field_type.num_bytes);
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    dumpfile::Field *output_field = output_file_descritpion.addField(name, field_type);
    output_field->copyValNames(*uint_field);
}

void FD_DimCat::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    uint64_t value = record.getUInt(uint_field);

    if (this->isOutputText()) {
        os << value;
    }
    else {
        // CHECK: this is safe (since num_bytes should be at most 64-bits)
        os.write(reinterpret_cast<char*>(&value), uint_field->field_type.num_bytes);
    }
}

std::ostream &operator<<(std::ostream& os, const FD_DimCat &fd) {
    os << "dim-cat" << "=" << fd.name << "," << fd.uint_field_name;
    return os;
}

//-----------------------------------------------------------------------------
// FD_DimTimeFunction
//-----------------------------------------------------------------------------

FD_DimTimeFunction::FD_DimTimeFunction(std::string name,
                                       std::string time_field_name,
                                       TimeFunction time_function):
    FieldDescription(name, FD_DIM_TIME_FUNCTION),
    time_field_name(time_field_name),
    time_field(nullptr),
    time_function(time_function)
{}

FD_DimTimeFunction *FD_DimTimeFunction::asDimTimeFunction() { return this; }

void FD_DimTimeFunction::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        time_field = f.getFieldByName(time_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << time_field_name << std::endl;
        throw std::exception();
    }
}

void FD_DimTimeFunction::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    std::string field_type_name = "nc_dim_cat_1";
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    dumpfile::Field *output_field = output_file_descritpion.addField(name, field_type);

    if (time_function == FUNCTION_HOUR_OF_DAY) {
        for (int i=0;i<24;i++) {
            output_field->addValueName(i,std::to_string(i)+"h");
        }
    }
    else if (time_function == FUNCTION_DAY_OF_WEEK) {
        output_field->addValueName(1,"Mon");
        output_field->addValueName(2,"Tue");
        output_field->addValueName(3,"Wed");
        output_field->addValueName(4,"Thr");
        output_field->addValueName(5,"Fri");
        output_field->addValueName(6,"Sat");
        output_field->addValueName(7,"Sun");
    }
    else if (time_function == FUNCTION_MONTH_OF_YEAR) {
        output_field->addValueName(1,"Jan");
        output_field->addValueName(2,"Feb");
        output_field->addValueName(3,"Mar");
        output_field->addValueName(4,"Apr");
        output_field->addValueName(5,"May");
        output_field->addValueName(6,"Jun");
        output_field->addValueName(7,"Jul");
        output_field->addValueName(8,"Aug");
        output_field->addValueName(9,"Sep");
        output_field->addValueName(10,"Oct");
        output_field->addValueName(11,"Nov");
        output_field->addValueName(12,"Dec");
    }
}

void FD_DimTimeFunction::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    std::time_t t = record.getTime(time_field);

    std::tm *time_record = std::localtime(&t);

    uint64_t value;

    if (time_function == FUNCTION_HOUR_OF_DAY) {
        value = time_record->tm_hour;
    }
    else if (time_function == FUNCTION_DAY_OF_WEEK) {
        value = time_record->tm_wday == 0 ? 7 : time_record->tm_wday;
    }
    else if (time_function == FUNCTION_MONTH_OF_YEAR) {
        value = time_record->tm_mon + 1;
    }

    if (this->isOutputText()) {
        os << value;
    }
    else {
        // CHECK: this is safe (since num_bytes should be at most 64-bits)
        os.write(reinterpret_cast<char*>(&value), 1);
    }
}

std::ostream &operator<<(std::ostream& os, const FD_DimTimeFunction &fd) {
    os << fd.time_function << "=" << fd.name << "," << fd.time_field_name;
    return os;
}

//-----------------------------------------------------------------------------
// FD_VarUInt
//-----------------------------------------------------------------------------

FD_VarUInt::FD_VarUInt(std::string name,
                       std::string uint_field_name,
                       int num_bytes):
    FieldDescription(name, FD_VAR_UINT),
    uint_field_name(uint_field_name),
    uint_field(nullptr),
    num_bytes(num_bytes)
{}

FD_VarUInt *FD_VarUInt::asVarUInt() { return this; }

void FD_VarUInt::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        uint_field = f.getFieldByName(uint_field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << uint_field_name << std::endl;
        throw std::exception();
    }
}

void FD_VarUInt::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    std::string field_type_name = "nc_var_uint_" + std::to_string(num_bytes);
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    output_file_descritpion.addField(name, field_type);
}

void FD_VarUInt::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    uint64_t value = record.getUInt(uint_field);

    if (this->isOutputText()) {
        os << value;
    }
    else {
        // CHECK: this is safe (since num_bytes should be at most 64-bits)
        os.write(reinterpret_cast<char*>(&value), num_bytes);
    }
}


std::ostream &operator<<(std::ostream& os, const FD_VarUInt &fd) {
    os << "var-uint" << "=" << fd.name << "," << fd.uint_field_name << "," << fd.num_bytes;
    return os;
}

//-----------------------------------------------------------------------------
// FD_VarOne
//-----------------------------------------------------------------------------

FD_VarOne::FD_VarOne(std::string name,
                     int num_bytes):
    FieldDescription(name, FD_VAR_ONE),
    num_bytes(num_bytes)
{}

FD_VarOne *FD_VarOne::asVarOne() { return this; }

void FD_VarOne::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    std::string field_type_name = "nc_var_uint_" + std::to_string(num_bytes);
    const dumpfile::FieldType &field_type = dumpfile::FieldTypesList::getFieldType(field_type_name);
    output_file_descritpion.addField(name, field_type);
}

void FD_VarOne::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc
    uint64_t value = 1;

    if (this->isOutputText()) {
        os << value;
    }
    else {
        // CHECK: this is safe (since num_bytes should be at most 64-bits)
        os.write(reinterpret_cast<char*>(&value), num_bytes);
    }
}

std::ostream &operator<<(std::ostream& os, const FD_VarOne &fd) {
    os << "var-one" << "=" << fd.name << "," << fd.num_bytes;
    return os;
}

//-----------------------------------------------------------------------------
// FD_FieldCopy
//-----------------------------------------------------------------------------

FD_FieldCopy::FD_FieldCopy(std::string name,
                           std::string field_name):
    FieldDescription(name, FD_FIELD_COPY),
    field_name(field_name),
    field(nullptr)
{}

FD_FieldCopy *FD_FieldCopy::asFieldCopy() { return this; }

void FD_FieldCopy::cacheFields(const dumpfile::DumpFileDescription &f) {
    try {
        field = f.getFieldByName(field_name);
    }
    catch (...) {
        std::cerr << "Cache field doesn't exist: " << field_name << std::endl;
        throw std::exception();
    }
}

void FD_FieldCopy::append(dumpfile::DumpFileDescription &output_file_descritpion)
{
    dumpfile::Field *output_field = output_file_descritpion.addField(name, field->field_type);
    output_field->copyValNames(*field);
}

void FD_FieldCopy::dump(Record &record, std::ostream& os)
{
    // mercator conversion etc

    std::vector<uint64_t> values;
    record.getUInts(field, values);

    if (this->isOutputText()) {
        // unsuported

        bool first = true;
        for (uint64_t v: values) {
            if (!first) {
                os << " ";
            }
            first = false;
            os << v;
        }
    }
    else {
        int no_bytes_per_token = field->getNumBytes() / field->getNumTokens();
        for (uint64_t v: values) {
            os.write(reinterpret_cast<char*>(&v), no_bytes_per_token);
        }
    }
}


std::ostream &operator<<(std::ostream& os, const FD_FieldCopy &fd) {
    os << "copy" << "=" << fd.name << "," << fd.field_name;
    return os;
}

//-----------------------------------------------------------------------------
// MappingScheme
//-----------------------------------------------------------------------------

void MappingScheme::addFieldMap(FieldDescription* field_map) {
    field_map->setMappingScheme(this);
    field_descriptions.push_back(field_map);
}

// Given the input file description, check if the mapping scheme is valid.
bool MappingScheme::isValid(dumpfile::DumpFileDescription &input_description) {

    typedef std::pair<FieldDescription*, std::string> InvalidField;
    std::vector<InvalidField> invalid_fields;
    for (FieldDescription *fd: field_descriptions) {
        std::string invalid_reason;
        if (!fd->isValid(input_description, invalid_reason)) {
            invalid_fields.push_back(InvalidField(fd, invalid_reason));
        }
    }

    if (invalid_fields.size() == 0) {
        return true;
    }

    // check number of time fields (needs to be exactly one)
    for (InvalidField &f: invalid_fields) {
        //            dumpfile::FieldDescription &field_description = f.first;
        //            dumpfile::FieldDescription &field_description = f.first;
        std::cout << f.second << std::endl;
    }
    return false;

}

void MappingScheme::cacheFields(const dumpfile::DumpFileDescription &dump_file_description) {
    for (FieldDescription *field_description: field_descriptions) {
        field_description->cacheFields(dump_file_description);
    }
}

void MappingScheme::prepare(const dumpfile::DumpFileDescription &input_file_description,
                            const dumpfile::DumpFileDescription::Encoding output_encoding) {

    //
    // this->isValid(input_file);

    //
    this->input_file_description = const_cast<dumpfile::DumpFileDescription*>(&input_file_description);

    this->cacheFields(input_file_description);

    output_file_descritpion.name = input_file_description.name;
    output_file_descritpion.encoding = output_encoding;

    for (FieldDescription *fd: field_descriptions) {
        fd->append(output_file_descritpion);
    }
}


void MappingScheme::dumpNextRecord(std::istream &is, std::ostream &os) {

    static Record input_record(*this->input_file_description);

    input_record.readNext(is);

    bool first = true;
    for (FieldDescription *fd: field_descriptions) {
        if (!first && output_file_descritpion.isText()) {
            os << " ";
        }
        fd->dump(input_record, os);
        first = false;
    }
}


std::ostream &operator<<(std::ostream& os, const MappingScheme &mapping_scheme) {
    os << "MappingScheme:" << std::endl;
    for (FieldDescription *field: mapping_scheme.field_descriptions) {
        switch (field->type) {
        case FD_DIM_DMQ:
            os << "    " << *field->asDimDMQ() << std::endl;
            break;
        case FD_DIM_TBIN:
            os << "    " << *field->asDimTBin() << std::endl;
            break;
        case FD_DIM_CAT:
            os << "    " << *field->asDimCat() << std::endl;
            break;
        case FD_DIM_TIME_FUNCTION:
            os << "    " << *field->asDimTimeFunction() << std::endl;
            break;
        case FD_VAR_UINT:
            os << "    " << *field->asVarUInt() << std::endl;
            break;
        case FD_VAR_ONE:
            os << "    " << *field->asVarOne() << std::endl;
            break;
        case FD_FIELD_COPY:
            os << "    " << *field->asFieldCopy() << std::endl;
            break;
        default:
            break;
        }
    }
    return os;

}
