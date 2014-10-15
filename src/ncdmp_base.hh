#include "DumpFile.hh"

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
// split
//-----------------------------------------------------------------------------

std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems);


//-----------------------------------------------------------------------------
// Record
//-----------------------------------------------------------------------------

struct EndOfFile {};
struct WrongNumberOfInputTokens {};

struct Record {

    static const int BUFFER_SIZE = 4096;

    Record(dumpfile::DumpFileDescription &input_file_description);

    void         readNext(std::istream &is);
    float        getFloat(dumpfile::Field *field);
    uint64_t     getUInt(dumpfile::Field *field);
    void         getUInts(dumpfile::Field *field, std::vector<uint64_t> &output);
    std::time_t  getTime(dumpfile::Field *field);

public:
    const dumpfile::DumpFileDescription &input_file_description;
    char buffer[BUFFER_SIZE];
    std::vector<std::string> tokens;
};


//-----------------------------------------------------------------------------
// FieldMapType
//-----------------------------------------------------------------------------

enum FieldMapType { FD_DIM_DMQ, FD_DIM_TBIN, FD_DIM_CAT, FD_DIM_TIME_FUNCTION,
                    FD_VAR_UINT, FD_VAR_ONE, FD_FIELD_COPY };

//-----------------------------------------------------------------------------
// TimeFunction
//-----------------------------------------------------------------------------

enum TimeFunction { FUNCTION_HOUR_OF_DAY,
                    FUNCTION_DAY_OF_WEEK,
                    FUNCTION_MONTH_OF_YEAR,
                    FUNCTION_UNKNOWN };

std::ostream &operator<<(std::ostream& os, const TimeFunction &time_function);

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

struct MappingScheme;
struct FieldDescription;
struct FD_DimDMQ;
struct FD_DimTBin;
struct FD_DimCat;
struct FD_DimTimeFunction;
struct FD_VarUInt;
struct FD_VarOne;
struct FD_FieldCopy;

//-----------------------------------------------------------------------------
// FieldDescription
//-----------------------------------------------------------------------------

struct FieldDescription {

    FieldDescription(std::string name, FieldMapType type);

    virtual bool isValid(dumpfile::DumpFileDescription &input_description, std::string &invalid_reason);

    virtual FD_DimDMQ*          asDimDMQ();
    virtual FD_DimTBin*         asDimTBin();
    virtual FD_DimCat*          asDimCat();
    virtual FD_DimTimeFunction* asDimTimeFunction();
    virtual FD_VarUInt*         asVarUInt();
    virtual FD_VarOne*          asVarOne();
    virtual FD_FieldCopy*       asFieldCopy();

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);
    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);
    virtual void dump(Record &record, std::ostream& os);

    bool checkFieldPresenceAndType
        ( dumpfile::DumpFileDescription &input_description,
          std::string field_name,
          std::string target_type_name,
          std::string &problem);

    void setMappingScheme(MappingScheme *mapping_scheme);
    bool isOutputBinary() const;
    bool isOutputText() const;

    std::string    name;
    FieldMapType   type;
    MappingScheme* mapping_scheme;

};

//-----------------------------------------------------------------------------
// FD_DimDMQ
//-----------------------------------------------------------------------------

struct FD_DimDMQ : public FieldDescription {
    FD_DimDMQ(std::string name,
              std::string lat_field_name,
              std::string lon_field_name,
              int quadtree_levels);

    virtual FD_DimDMQ *asDimDMQ();

    virtual bool isValid(dumpfile::DumpFileDescription &input_description,
                         std::string &invalid_reason);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void dump(Record &record, std::ostream& os);

    std::string      lat_field_name;
    dumpfile::Field *lat_field;

    std::string lon_field_name;
    dumpfile::Field *lon_field;

    int quadtree_levels;
};

std::ostream &operator<<(std::ostream& os, const FD_DimDMQ &fd);

//-----------------------------------------------------------------------------
// FD_DimTBin
//-----------------------------------------------------------------------------

struct FD_DimTBin : public FieldDescription {

    FD_DimTBin(std::string name,
               std::string time_field_name,
               std::string tbin_spec,
               int num_bytes,
               bool binary_tree_variation = false);

    virtual FD_DimTBin *asDimTBin();

    virtual bool isValid(dumpfile::DumpFileDescription &input_description,
                         std::string &invalid_reason);

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream& os);

    std::string      time_field_name;
    dumpfile::Field *time_field;

    TimeBinFunction  tbin_function;
    int num_bytes;
    
    bool binary_tree_variation { false };
};

std::ostream &operator<<(std::ostream& os, const FD_DimTBin &fd);

//-----------------------------------------------------------------------------
// FD_DimCat
//-----------------------------------------------------------------------------

struct FD_DimCat : public FieldDescription {

    FD_DimCat(std::string name,
              std::string uint_field_name);

    virtual FD_DimCat *asDimCat();

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream& os);

    std::string uint_field_name;
    dumpfile::Field *uint_field;
};

std::ostream &operator<<(std::ostream& os, const FD_DimCat &fd);

//-----------------------------------------------------------------------------
// FD_DimTimeFunction
//-----------------------------------------------------------------------------

struct FD_DimTimeFunction : public FieldDescription {

    FD_DimTimeFunction(std::string name,
                   std::string time_field_name,
                   TimeFunction time_function);

    virtual FD_DimTimeFunction *asDimTimeFunction();

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream& os);


    std::string      time_field_name;
    dumpfile::Field *time_field;

    TimeFunction     time_function;
};

std::ostream &operator<<(std::ostream& os, const FD_DimTimeFunction &fd);


//-----------------------------------------------------------------------------
// FD_VarUInt
//-----------------------------------------------------------------------------

struct FD_VarUInt : public FieldDescription {

    FD_VarUInt(std::string name,
                   std::string uint_field_name,
                   int num_bytes);

    virtual FD_VarUInt *asVarUInt();

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream& os);

    std::string      uint_field_name;
    dumpfile::Field *uint_field;
    int              num_bytes;
};

std::ostream &operator<<(std::ostream& os, const FD_VarUInt &fd);

//-----------------------------------------------------------------------------
// FD_VarOne
//-----------------------------------------------------------------------------

struct FD_VarOne : public FieldDescription {

    FD_VarOne(std::string name,
              int num_bytes);

    virtual FD_VarOne *asVarOne();

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream& os);

    int num_bytes;
};

std::ostream &operator<<(std::ostream& os, const FD_VarOne &fd);

//-----------------------------------------------------------------------------
// FD_FieldCopy
//-----------------------------------------------------------------------------

struct FD_FieldCopy : public FieldDescription {

    FD_FieldCopy(std::string name,
                 std::string field_name);

    virtual FD_FieldCopy *asFieldCopy();

    virtual void cacheFields(const dumpfile::DumpFileDescription &f);

    virtual void append(dumpfile::DumpFileDescription &output_file_descritpion);

    virtual void dump(Record &record, std::ostream &os);

    std::string field_name;
    dumpfile::Field *field;

};

std::ostream &operator<<(std::ostream& os, const FD_FieldCopy &fd);


//-----------------------------------------------------------------------------
// MappingScheme
//-----------------------------------------------------------------------------

struct MappingScheme {

    void addFieldMap(FieldDescription* field_map);

    // Given the input file description, check if the mapping scheme is valid.
    bool isValid(dumpfile::DumpFileDescription &input_description);

    void cacheFields(const dumpfile::DumpFileDescription &dump_file_description);

    void prepare(const dumpfile::DumpFileDescription &input_file_description,
                 const dumpfile::DumpFileDescription::Encoding output_encoding);

    void dumpNextRecord(std::istream &is, std::ostream &os);

    dumpfile::DumpFileDescription  *input_file_description;
    dumpfile::DumpFileDescription   output_file_descritpion;
    std::vector<FieldDescription*>  field_descriptions;

};

std::ostream &operator<<(std::ostream& os, const MappingScheme &mapping_scheme);
