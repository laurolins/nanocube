#include "nanocube_platform.h"
#include "nanocube_parser.h"

#include "nanocube_platform.c"
#include "nanocube_parser.c"

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "win32_nanocube_platform.c"

int 
main() 
{
    PlatformAPI platform;
    
    win32_init_platform(&platform);

    PlatformFileHandle platform_stdout;
    platform_stdout.open = 1;
    platform_stdout.write = 1;
    platform_stdout.read = 0;
    platform_stdout.last_seek_success = 0;
    platform_stdout.last_read = 0;
    platform_stdout.handle = stderr;

#if 0
    {

        char text[] = "=+ ==-+ 1 2 +1e23 1e-3 1.1e-05 1. . .5 ab2 _23 __23 23_aab ( { [ ] } ) \"lauro didier lins\"\"sofia palha\" 1/2.4 // comment\nthis is really .23+12-45+a // sums...";

        Tokenizer tok;
        Tokenizer_init(&tok, text, cstr_end(text));

        char buffer[Kilobytes(4)];
        Print print;
        Print_init(&print, buffer, buffer + sizeof(buffer));

        while (Tokenizer_next(&tok)) 
        {
            Print_clear(&print);
            Print_str(&print, tok.token.begin, tok.token.end);
            Print_align(&print, 30, 0);
            Print_cstr(&print, " -> ");        
            Print_cstr(&print, NanocubeTokenType_cstr(tok.token.type));
            Print_cstr(&print, "\n");        
            platform.write_to_file(&platform_stdout, print.begin, print.end);
        }

        Print_clear(&print);
        Print_cstr(&print, TokenizerNextResult_cstr(tok.next_result_detail));
        Print_cstr(&print, " line: ");
        Print_uint(&print, (u64) tok.line);
        Print_cstr(&print, " col: ");
        Print_uint(&print, (u64) tok.column);
        Print_cstr(&print, " state: ");
        Print_uint(&print, (u64) (tok.state - tok.states));
        Print_cstr(&print, "\n");        
        platform.write_to_file(&platform_stdout, print.begin, print.end);

    }


    {
        char *texts[] = {
            "",
            "a;",
            "1;",
            "\"st\";",
            "a=1;",
            "a=b;",
            "a=\"st\";",
            "f(x)",
            "f(x);",
            "a = 10; b = 30; 4;",
            "theft=crimes.r(\"kind\",\"theft\");",
            "theftcoef=thefts/crimes + 100;",
            "theftcoef.a(\"location\",dive(\"@\",8));",
        };

        int n = sizeof(texts)/sizeof(char*);


        PlatformMemory ast_memory = platform.allocate_memory(Megabytes(32),0,0);
        char *ast_memory_begin    = (char*) ast_memory.memblock.base;
        char *ast_memory_capacity = ast_memory_begin + ast_memory.memblock.size;

        for (int i=0;i<n;++i) {
            char *text = texts[i];

            Tokenizer tok;
            Tokenizer_init(&tok, text, cstr_end(text));

            char buffer[Kilobytes(4)];
            Print print;
            Print_init(&print, buffer, buffer + sizeof(buffer));

            Parser parser;
            Parser_init(&parser, &tok, ast_memory_begin, ast_memory_capacity);

            b8 ok = Parser_run(&parser);

            Print_clear(&print);
            Print_cstr(&print, text);
            Print_align(&print, 100, 0);
            Print_cstr(&print, " -> ");        
            Print_uint(&print, ok);
            Print_cstr(&print, "\n");        
            platform.write_to_file(&platform_stdout, print.begin, print.end);
        }

        platform.free_memory(ast_memory.handle);
    
    }
#endif

    {
        char *texts[] = {
            "r(crimes,10,x,20);10;crimes;10+10;a.b+8;\"lauro\";(1+1);value=exp(2,crimes);"
        };

        int n = sizeof(texts)/sizeof(char*);

        PlatformMemory ast_memory = platform.allocate_memory(Megabytes(32),0,0);
        char *ast_memory_begin    = ast_memory.memblock.begin;
        char *ast_memory_capacity = ast_memory.memblock.end;

        for (int i=0;i<n;++i) {
            char *text = texts[i];

            Tokenizer tok;
            Tokenizer_init(&tok, text, cstr_end(text));

            char buffer[Kilobytes(4)];
            Print print;
            Print_init(&print, buffer, buffer + sizeof(buffer));

            Parser parser;
            Parser_init(&parser, &tok, ast_memory_begin, ast_memory_capacity);

            b8 ok = Parser_run(&parser);

            Print_clear(&print);
            Print_cstr(&print, text);
            Print_align(&print, 100, 0);
            Print_cstr(&print, " -> ");        
            Print_uint(&print, ok);
            Print_cstr(&print, "\n");        
            platform.write_to_file(&platform_stdout, print.begin, print.end);


            for (AST_Node* it = parser.ast_first; it != 0; it = it->next)
            {
                Print_clear(&print);
                Print_cstr(&print, AST_Node_Type_cstr(it->type));
                Print_cstr(&print, "\n");        
                platform.write_to_file(&platform_stdout, print.begin, print.end);
            }


        }

        platform.free_memory(ast_memory.handle);
    
    }

}
