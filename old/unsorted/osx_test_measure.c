#include "nanocube_platform.h"
#include "nanocube_alloc.h"
#include "nanocube_index.h"
#include "nanocube_measure.h"

#include "nanocube_parser.h"

#include "nanocube_platform.c"
#include "nanocube_alloc.c"
#include "nanocube_index.c"
#include "nanocube_measure.c"

#include "nanocube_parser.c"

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include "osx_nanocube_platform.c"

void
test_number_expression()
{
    // initialize platform
    PlatformAPI platform;
    osx_init_platform(&platform);
    
    // initialize stdout
    PlatformFileHandle platform_stdout;
    platform_stdout.open = 1;
    platform_stdout.write = 1;
    platform_stdout.read = 0;
    platform_stdout.last_seek_success = 0;
    platform_stdout.last_read = 0;
    platform_stdout.handle = stderr;



    // print
    char buffer[Kilobytes(4)];
    Print print;
    Print_init(&print, buffer, buffer + sizeof(buffer));

   

    // reserve heap memory for ast, types, and symbols 
    PlatformMemory compiler_memory = 
        platform.allocate_memory(Megabytes(32), 0, 0);

    PlatformMemory ast_memory = 
        platform.allocate_memory(Megabytes(32), 0, 0);

   
    Compiler compiler;
    Compiler_init(&compiler, compiler_memory.memblock.begin, compiler_memory.memblock.end);

    // parse expression 
    char text[] = "a=(((2*3)+5+1)/3)-3;2*a;";

    Tokenizer tok;
    Tokenizer_init(&tok, text, cstr_end(text));

    Parser parser;
    Parser_init(&parser, &tok, ast_memory.memblock.begin, ast_memory.memblock.end);

    b8 ok = Parser_run(&parser);

    // now is the time!
    if (ok) {
    
        TypeValue* type_value = Compiler_reduce(&compiler, parser.ast_first);

        Print_clear(&print);
        Print_cstr(&print, text);
        Print_align(&print, 100, 0);
        Print_cstr(&print, " -> ");        
        Print_uint(&print, ok);
        Print_cstr(&print, "\n");        
        Print_cstr(&print, "result: ");        
        Print_uint(&print, (u64) (*((double*) type_value->value)));
        platform.write_to_file(&platform_stdout, print.begin, print.end);
    }

    // free memory
    platform.free_memory(compiler_memory.handle);
    platform.free_memory(ast_memory.handle);

    //
    // @todo start bringing the measure, binding, and target
    // vocabulary into the compiler. The expression statements
    // should all be considered queries to be solved
    //
    // After compilation, each source in each measure should
    // be queried and then the resulting relations should
    // be combined.
    //

}


typedef struct MeasureCompilerTypes
{
    TypeID number_type_id;
    TypeID string_type_id;
    TypeID measure_type_id;
    TypeID target_type_id;
    TypeID binding_type_id;
} MeasureCompilerTypes;


internal MeasureCompilerTypes g_measure_compiler_types;

internal TypeValue*
function_dive_1(Compiler *compiler, TypeValue* params_begin, TypeValue *params_end)
{
    Assert(params_end - params_begin == 1);
    TypeValue *depth_value_type = params_begin;

    // Make sure single parameter is a number
    Assert(depth_value_type->type_id = g_measure_compiler_types.number_type_id); 

    // value should be an integer number
    int depth_value = (int) *((double*) depth_value_type->value);

    Assert(depth_value >= 0);

    Target *target = (Target*) Compiler_alloc(compiler, sizeof(Target));
    target->type = TARGET_FIND_DIVE;
    target->anchor=0;
    target->find_dive.path.begin  = 0;
    target->find_dive.path.length = 0;
    target->find_dive.depth = (PathLength) depth_value;
    
    TypeValue *result = (TypeValue*) Compiler_alloc(compiler, sizeof(TypeValue));
    result->type_id = g_measure_compiler_types.target_type_id;
    result->value   = target;
    
    return result;
}

internal TypeValue*
function_anchor(Compiler *compiler, TypeValue* params_begin, TypeValue *params_end)
{
    Assert(params_end - params_begin == 2);

    TypeValue *dimension_name_tv = params_begin;
    TypeValue *target_tv         = params_begin + 1;

    Assert(dimension_name_tv->type_id == g_measure_compiler_types.string_type_id); 
    Assert(target_tv->type_id         == g_measure_compiler_types.target_type_id); 

//     // value should be an integer number
//     Target *target = (Target*) Compiler_alloc(compiler, sizeof(Target));
//     target->type = TARGET_FIND_DIVE;
//     target->anchor=0;
//     target->find_dive.path.begin  = 0;
//     target->find_dive.path.length = 0;
//     target->find_dive.depth = (PathLength) depth_value;
//     
//     TypeValue *result = (TypeValue*) Compiler_alloc(compiler, sizeof(TypeValue));
//     result->type_id = g_measure_compiler_types.target_type_id;
//     result->value   = target;
    
    return result;
}




internal void
prepare_measure_compiler(Compiler *compiler)
{
    Type *measure_type = TypeTable_insert_type_cstr(&compiler->type_table,"Measure");

    // target_type is associates with TargetType on nanocube_measure.h
    Type *target_type  = TypeTable_insert_type_cstr(&compiler->type_table,"Target");

    Type *binding_type = TypeTable_insert_type_cstr(&compiler->type_table,"Binding"); 

    g_measure_compiler_types.number_type_id  = compiler->number_type_id;
    g_measure_compiler_types.string_type_id  = compiler->string_type_id;
    g_measure_compiler_types.measure_type_id = measure_type->id;
    g_measure_compiler_types.target_type_id  = target_type->id;
    g_measure_compiler_types.binding_type_id = binding_type->id;
    
    // and operators
    TypeID parameter_types[2];
    parameter_types[0] = g_measure_compiler_types.number_type_id;
    parameter_types[1] = g_measure_compiler_types.number_type_id;
    SymbolTable_insert_function_cstr( 
            &compiler->symbol_table, 
            "dive", 
            target_type->id, 
            parameter_types, 
            parameter_types + 1, 
            function_dive_1);
}

void
test_measure()
{
    // initialize platform
    PlatformAPI platform;
    osx_init_platform(&platform);

    
    // initialize stdout
    PlatformFileHandle platform_stdout;
    platform_stdout.open = 1;
    platform_stdout.write = 1;
    platform_stdout.read = 0;
    platform_stdout.last_seek_success = 0;
    platform_stdout.last_read = 0;
    platform_stdout.handle = stderr;


    // print
    char buffer[Kilobytes(4)];
    Print print;
    Print_init(&print, buffer, buffer + sizeof(buffer));


    // reserve heap memory for ast, types, and symbols 
    PlatformMemory compiler_memory = 
        platform.allocate_memory(Megabytes(32), 0, 0);

    PlatformMemory ast_memory = 
        platform.allocate_memory(Megabytes(32), 0, 0);

   
    Compiler compiler;
    Compiler_init(&compiler, compiler_memory.memblock.begin, compiler_memory.memblock.end);

    prepare_measure_compiler(&compiler);

    // parse expression 
    char text[] = "a=dive(3);a;";

    Tokenizer tok;
    Tokenizer_init(&tok, text, cstr_end(text));

    Parser parser;
    Parser_init(&parser, &tok, ast_memory.memblock.begin, ast_memory.memblock.end);

    b8 ok = Parser_run(&parser);

    // now is the time!
    if (ok) {
    
        TypeValue* type_value = Compiler_reduce(&compiler, parser.ast_first);

        Print_clear(&print);
        Print_cstr(&print, text);
        Print_align(&print, 100, 0);
        Print_cstr(&print, " -> ");        
        Print_uint(&print, ok);
        Print_cstr(&print, "\n");        
        Print_cstr(&print, "result: ");        
        Type* type = (compiler.type_table.types.begin + type_value->type_id);
        Print_str(&print, type->name.begin, type->name.end);
        platform.write_to_file(&platform_stdout, print.begin, print.end);
    }

    // free memory
    platform.free_memory(compiler_memory.handle);
    platform.free_memory(ast_memory.handle);

    //
    // @todo start bringing the measure, binding, and target
    // vocabulary into the compiler. The expression statements
    // should all be considered queries to be solved
    //
    // After compilation, each source in each measure should
    // be queried and then the resulting relations should
    // be combined.
    //

}

int 
main() 
{
    // test_number_expression();
    test_measure();
    return 0;
}
int 
main() 
{
    // initialize platform
    PlatformAPI platform;
    osx_init_platform(&platform);

   // initialize stdout
    PlatformFileHandle platform_stdout;
    platform_stdout.open = 1;
    platform_stdout.write = 1;
    platform_stdout.read = 0;
    platform_stdout.last_seek_success = 0;
    platform_stdout.last_read = 0;
    platform_stdout.handle = stderr;


    
    // print
    char buffer[Kilobytes(4)];
    Print print;
    Print_init(&print, buffer, buffer + sizeof(buffer));

   

    // reserve heap memory for ast, types, and symbols 
    PlatformMemory memory = 
        platform.allocate_memory(Megabytes(64), 0, 0);

    LinearAllocator memsrc;
    LinearAllocator_init(&memsrc, memory.memblock.begin, memory.memblock.begin, memory.memblock.end);

    Compiler compiler;
    Compiler_init(&compiler, &memsrc);

    // parse expression 
    char text[] = "a=(((2*3)+5+1)/3)-3;2*a;";

    Tokenizer tok;
    Tokenizer_init(&tok, text, cstr_end(text));

    Parser parser;
    Parser_init(&parser, &tok, ast_memory.memblock.begin, ast_memory.memblock.end);

    b8 ok = Parser_run(&parser);

    // now is the time!
    if (ok) {
        TypeValue* type_value = Compiler_reduce(&compiler, parser.ast_first);

        Print_clear(&print);
        Print_cstr(&print, text);
        Print_align(&print, 100, 0);
        Print_cstr(&print, " -> ");        
        Print_uint(&print, ok);
        Print_cstr(&print, "\n");        
        Print_cstr(&print, "result: ");        
        Print_uint(&print, (u64) (*((double*) type_value->value)));
        platform.write_to_file(&platform_stdout, print.begin, print.end);
    }

    // free memory
    platform.free_memory(compiler_memory.handle);
    platform.free_memory(ast_memory.handle);

    //
    // @todo start bringing the measure, binding, and target
    // vocabulary into the compiler. The expression statements
    // should all be considered queries to be solved
    //
    // After compilation, each source in each measure should
    // be queried and then the resulting relations should
    // be combined.
    //



}
