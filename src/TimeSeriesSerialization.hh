#pragma once

#include <TimeSeries.hh>

#include <cassert>

#include <sstream>

#include <serialization/BasicTypes.hpp>
#include <serialization/CoreSchema.hpp>
#include <serialization/Deserializer.hpp>
#include <serialization/FieldType.hpp>
#include <serialization/Schema.hpp>
#include <serialization/Schema.hpp>
#include <serialization/SchemaLog.hpp>
#include <serialization/SchemaSerialization.hpp>
#include <serialization/Serializer.hpp>
#include <serialization/Type.hpp>
#include <serialization/Util.hpp>

namespace serialization
{

using namespace timeseries;

using namespace serialization::util;

template <typename TSTime, typename TSCount>
Schema &operator<<(Schema &schema, TimeSeries<TSTime, TSCount> &)
{
    typedef TimeSeries<TSTime, TSCount>  TS;
    typedef Entry<TSTime, TSCount>       TSEntry;

    // create new custom type
    serialization::CustomType *type =
            serialization::retrieveOrCreateCustomType<TS>(schema, getTypeName<TS>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // links
        CustomType *field_type =
            serialization::retrieveOrCreateCustomType<TSEntry>(schema, getTypeName<TSEntry>());

        type->addField(FieldType(field_type, FieldType::VALUE, FieldType::REPEAT_16), "links");

    }
    return schema;
}

template <typename TSTime, typename TSCount>
Schema &operator<<(Schema &schema, Entry<TSTime, TSCount> &)
{
    typedef TimeSeries<TSTime, TSCount>   TS;
    typedef Entry<TSTime, TSCount>        TSEntry;

    assert(sizeof(TSTime) == 4 && sizeof(TSCount) == 4);

    // create new custom type
    serialization::CustomType *type =
            serialization::retrieveOrCreateCustomType<TSEntry>(schema, getTypeName<TSEntry>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // tagged pointer
        type->addField(FieldType(CoreSchema::ID_uint32), "time");
        type->addField(FieldType(CoreSchema::ID_uint32), "count");
    }

    return schema;
}

template <typename TSTime, typename TSCount>
Serializer &operator<<(Serializer &s, Entry<TSTime, TSCount> &e)
{
    uint32 time  = e.time;
    uint32 count = e.count;

    s << time;
    s << count;

    return s;
}

template <typename TSTime, typename TSCount>
Serializer &operator<<(Serializer &s, TimeSeries<TSTime, TSCount> &ts)
{
    uint16 size = static_cast<uint16>(ts.entries.size());
    s << size;
    for (uint16 i=0;i<size;i++)
        s << ts.entries[i];

    return s;
}

template <typename TSTime, typename TSCount>
Deserializer &operator>>(Deserializer &d, Entry<TSTime, TSCount> &e)
{
    uint32 time, count;

    d >> time;
    d >> count;

    e.time  = time;
    e.count = count;

    return d;
}

template <typename TSTime, typename TSCount>
Deserializer &operator>>(Deserializer &d, TimeSeries<TSTime, TSCount> &ts)
{
    // TODO: make repeated 16-bit counter
    uint16 size;
    d >> size;

    ts.entries.resize(size);
    for (uint16 i=0; i<size; i++)
        d >> ts.entries[i];

    return d;
}

template <typename TSTime, typename TSCount>
void registerTimeSeriesObjectFactories(Schema &schema, Deserializer &deserializer)
{
    serialization::util::addObjectFactory<TimeSeries<TSTime, TSCount>>(schema, deserializer);
}

template <typename TSTime, typename TSCount>
void registerTimeSeriesToSchema(Schema &schema)
{
    TimeSeries<TSTime, TSCount> ts;
    Entry<TSTime, TSCount>      e;
    schema << ts << e;
}

}



