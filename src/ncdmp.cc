#include "ncdmp_base.hh"

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------

void test() {
    TimeBinFunction f("2008-12-30_1h");
    std::cout << f.getSpecificationString() << std::endl;
    TimeBinFunction f2(f.getSpecificationString());
    std::cout << f2.getSpecificationString() << std::endl;
}

int main(int argc, char **argv) {

#if 0
    bool     version = false;
#endif

    uint64_t max     = 0;

    dumpfile::DumpFileDescription::Encoding encoding;

    MappingScheme mapping_scheme;

    std::string       type_st("");

    bool    copy_all = false;

    for (int i=1;i<argc;i++) {

        // split into key value
        std::vector<std::string> tokens;
        split(std::string(argv[i]),'=',tokens);

        TimeFunction time_function = FUNCTION_UNKNOWN;
        if (tokens[0].compare("dim-hour") == 0) {
            time_function = FUNCTION_HOUR_OF_DAY;
        }
        else if (tokens[0].compare("dim-weekday") == 0) {
            time_function = FUNCTION_DAY_OF_WEEK;
        }
        else if (tokens[0].compare("dim-month") == 0) {
            time_function = FUNCTION_MONTH_OF_YEAR;
        }

        if (tokens.size() == 0) {
            continue;
        }
#if 0
        else if (tokens[0].compare("--version") == 0) {
            version = true;
        }
#endif
        else if (tokens[0].compare("--cat") == 0) {
            copy_all = true;
        }
        else if (tokens[0].compare("--max") == 0) {
            max = std::stoull(tokens[1]);
        }

        else if (tokens[0].compare("--encoding") == 0) {
            if (tokens[1].compare("t") == 0 || tokens[1].compare("text") == 0) {
                encoding = dumpfile::DumpFileDescription::text;
            }
            else if (tokens[1].compare("b")==0 || tokens[1].compare("binary")==0) {
                encoding = dumpfile::DumpFileDescription::binary;
            }
        }

        else if (tokens[0].compare("dim-dmq") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 4) {
                std::cerr << "(ERROR) field mapping dim-dmq needs 4 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            std::string lat_field_name = params.at(1);
            std::string lon_field_name = params.at(2);
            int quadtree_levels = std::stoi(params.at(3));

            mapping_scheme.addFieldMap(new FD_DimDMQ(name, lat_field_name, lon_field_name, quadtree_levels));

        }

        else if (tokens[0].compare("dim-tbin") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 4) {
                std::cerr << "(ERROR) field mapping dim-tbin needs 4 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name            = params.at(0);
            std::string time_field_name = params.at(1);
            std::string tbin_spec       = params.at(2);
            int num_bytes               = std::stoi(params.at(3));

            mapping_scheme.addFieldMap(new FD_DimTBin(name, time_field_name, tbin_spec, num_bytes));

        }

        else if (tokens[0].compare("dim-tbin2") == 0) {
            
            std::vector<std::string> params;
            split(tokens.at(1),',',params);
            
            if (params.size() != 4) {
                std::cerr << "(ERROR) field mapping dim-tbin needs 4 input parameters, but "
                << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }
            
            std::string name            = params.at(0);
            std::string time_field_name = params.at(1);
            std::string tbin_spec       = params.at(2);
            int num_bytes               = std::stoi(params.at(3));
            
            bool binary_tree_variation = true;
            mapping_scheme.addFieldMap(new FD_DimTBin(name, time_field_name, tbin_spec, num_bytes, binary_tree_variation));
            
        }

        else if (tokens[0].compare("dim-cat") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 2) {
                std::cerr << "(ERROR) field mapping dim-cat needs 2 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            std::string field_uint_name = params.at(1);

            mapping_scheme.addFieldMap(new FD_DimCat(name, field_uint_name));

        }

        else if (time_function != FUNCTION_UNKNOWN) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 2) {
                std::cerr << "(ERROR) field mapping " << time_function << " needs 2 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            std::string time_field_name = params.at(1);

            mapping_scheme.addFieldMap(new FD_DimTimeFunction(name, time_field_name, time_function));

        }

        else if (tokens[0].compare("var-uint") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 3) {
                std::cerr << "(ERROR) field mapping var-uint needs 3 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            std::string field_uint_name = params.at(1);
            int num_bytes = std::stoi(params.at(2));
            // std::cout << num_bytes << std::endl;

            mapping_scheme.addFieldMap(new FD_VarUInt(name, field_uint_name, num_bytes));

        }

        else if (tokens[0].compare("var-one") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 2) {
                std::cerr << "(ERROR) field mapping var-one needs 1 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            int num_bytes = std::stoi(params.at(1));

            mapping_scheme.addFieldMap(new FD_VarOne(name, num_bytes));

        }

        else if (tokens[0].compare("copy") == 0) {

            std::vector<std::string> params;
            split(tokens.at(1),',',params);

            if (params.size() != 2) {
                std::cerr << "(ERROR) field mapping copy needs 2 input parameters, but "
                          << params.size() << " provided on: " << argv[i] << std::endl;
                throw std::exception();
            }

            std::string name = params.at(0);
            std::string field_name = params.at(1);

            mapping_scheme.addFieldMap(new FD_FieldCopy(name, field_name));

        }

        // TODO add some filter option on certain fields
    }

    // log scheme
    // std::cout << mapping_scheme << std::endl;

    
    
#if 0
    std::ifstream istream("/Users/llins/att/data/lolla/lolla.dmp");
#else
    auto &istream = std::cin;
#endif
    
    
    // read input description
    dumpfile::DumpFileDescription input_description;
    istream >> input_description;

    // std::cout << input_description << std::endl;
    
    if (copy_all) {
        mapping_scheme = MappingScheme();
        for (auto field: input_description.fields) {
            mapping_scheme.addFieldMap(new FD_FieldCopy(field->name, field->name));
        }
    }

    //
    if (input_description.encoding == dumpfile::DumpFileDescription::unknown) {
        std::cerr << "[WARNING] Input file does not define encoding type. Assuming binary." << std::endl;
        input_description.encoding = dumpfile::DumpFileDescription::binary;
    }

    // write input description
    // std::cout << input_description;

    // check if mapping scheme is compatible with input_description
    // TODO: perform more checkings
    mapping_scheme.isValid(input_description);

    //
    mapping_scheme.cacheFields(input_description);

    mapping_scheme.prepare(input_description, encoding);

    // std::cout << "Output Description" << std::endl;

    // stream out the schema
    std::cout << mapping_scheme.output_file_descritpion;

    // signal of data start
    std::cout << std::endl;

    // stream out records
    max = max > 0 ? max : (1L << 60);
    try {
        for (uint64_t i=0;i<max;i++) {
            mapping_scheme.dumpNextRecord(std::cin, std::cout);
            if (mapping_scheme.output_file_descritpion.isText()) {
                std::cout << std::endl;
            }
        }
    }
    catch (EndOfFile &e) {
        // std::cout << "Done." << std::endl;
    }
    catch(...) {
        std::cerr << "Error!" << std::endl;
    }
}
