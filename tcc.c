/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcc.h"
#if ONE_SOURCE
# include "libtcc.c"
#endif
#include "tcctools.c"

/* tcc --help 输出的文本 */
static const char help[] =
    "Tiny C Compiler "TCC_VERSION" - Copyright (C) 2001-2006 Fabrice Bellard\n"
    "Usage: tcc [options...] [-o outfile] [-c] infile(s)...\n"
    "       tcc [options...] -run infile (or --) [arguments...]\n"
    "General options:\n"
    "  -c           compile only - generate an object file\n"
    "  -o outfile   set output filename\n"
    "  -run         run compiled source\n"
    "  -fflag       set or reset (with 'no-' prefix) 'flag' (see tcc -hh)\n"
    "  -std=c99     Conform to the ISO 1999 C standard (default).\n"
    "  -std=c11     Conform to the ISO 2011 C standard.\n"
    "  -Wwarning    set or reset (with 'no-' prefix) 'warning' (see tcc -hh)\n"
    "  -w           disable all warnings\n"
    "  -v --version show version\n"
    "  -vv          show search paths or loaded files\n"
    "  -h -hh       show this, show more help\n"
    "  -bench       show compilation statistics\n"
    "  -            use stdin pipe as infile\n"
    "  @listfile    read arguments from listfile\n"
    "Preprocessor options:\n"
    "  -Idir        add include path 'dir'\n"
    "  -Dsym[=val]  define 'sym' with value 'val'\n"
    "  -Usym        undefine 'sym'\n"
    "  -E           preprocess only\n"
    "Linker options:\n"
    "  -Ldir        add library path 'dir'\n"
    "  -llib        link with dynamic or static library 'lib'\n"
    "  -r           generate (relocatable) object file\n"
    "  -shared      generate a shared library/dll\n"
    "  -rdynamic    export all global symbols to dynamic linker\n"
    "  -soname      set name for shared library to be used at runtime\n"
    "  -Wl,-opt[=val]  set linker option (see tcc -hh)\n"
    "Debugger options:\n"
    "  -g           generate stab runtime debug info\n"
    "  -gdwarf[-x]  generate dwarf runtime debug info\n"
#ifdef TCC_TARGET_PE
    "  -g.pdb       create .pdb debug database\n"
#endif
#ifdef CONFIG_TCC_BCHECK
    "  -b           compile with built-in memory and bounds checker (implies -g)\n"
#endif
#ifdef CONFIG_TCC_BACKTRACE
    "  -bt[N]       link with backtrace (stack dump) support [show max N callers]\n"
#endif
    "Misc. options:\n"
    "  -x[c|a|b|n]  specify type of the next infile (C,ASM,BIN,NONE)\n"
    "  -nostdinc    do not use standard system include paths\n"
    "  -nostdlib    do not link with standard crt and libraries\n"
    "  -Bdir        set tcc's private include/library dir\n"
    "  -M[M]D       generate make dependency file [ignore system files]\n"
    "  -M[M]        as above but no other output\n"
    "  -MF file     specify dependency file name\n"
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
    "  -m32/64      defer to i386/x86_64 cross compiler\n"
#endif
    "Tools:\n"
    "  create library  : tcc -ar [crstvx] lib [files]\n"
#ifdef TCC_TARGET_PE
    "  create def file : tcc -impdef lib.dll [-v] [-o lib.def]\n"
#endif
    ;

static const char help2[] =
    "Tiny C Compiler "TCC_VERSION" - More Options\n"
    "Special options:\n"
    "  -P -P1                        with -E: no/alternative #line output\n"
    "  -dD -dM                       with -E: output #define directives\n"
    "  -pthread                      same as -D_REENTRANT and -lpthread\n"
    "  -On                           same as -D__OPTIMIZE__ for n > 0\n"
    "  -Wp,-opt                      same as -opt\n"
    "  -include file                 include 'file' above each input file\n"
    "  -isystem dir                  add 'dir' to system include path\n"
    "  -static                       link to static libraries (not recommended)\n"
    "  -dumpversion                  print version\n"
    "  -print-search-dirs            print search paths\n"
    "  -dt                           with -run/-E: auto-define 'test_...' macros\n"
    "Ignored options:\n"
    "  -arch -C --param -pedantic -pipe -s -traditional\n"
    "-W[no-]... warnings:\n"
    "  all                           turn on some (*) warnings\n"
    "  error[=warning]               stop after warning (any or specified)\n"
    "  write-strings                 strings are const\n"
    "  unsupported                   warn about ignored options, pragmas, etc.\n"
    "  implicit-function-declaration warn for missing prototype (*)\n"
    "  discarded-qualifiers          warn when const is dropped (*)\n"
    "-f[no-]... flags:\n"
    "  unsigned-char                 default char is unsigned\n"
    "  signed-char                   default char is signed\n"
    "  common                        use common section instead of bss\n"
    "  leading-underscore            decorate extern symbols\n"
    "  ms-extensions                 allow anonymous struct in struct\n"
    "  dollars-in-identifiers        allow '$' in C symbols\n"
    "  test-coverage                 create code coverage code\n"
    "-m... target specific options:\n"
    "  ms-bitfields                  use MSVC bitfield layout\n"
#ifdef TCC_TARGET_ARM
    "  float-abi                     hard/softfp on arm\n"
#endif
#ifdef TCC_TARGET_X86_64
    "  no-sse                        disable floats on x86_64\n"
#endif
    "-Wl,... linker options:\n"
    "  -nostdlib                     do not link with standard crt/libs\n"
    "  -[no-]whole-archive           load lib(s) fully/only as needed\n"
    "  -export-all-symbols           same as -rdynamic\n"
    "  -export-dynamic               same as -rdynamic\n"
    "  -image-base= -Ttext=          set base address of executable\n"
    "  -section-alignment=           set section alignment in executable\n"
#ifdef TCC_TARGET_PE
    "  -file-alignment=              set PE file alignment\n"
    "  -stack=                       set PE stack reserve\n"
    "  -large-address-aware          set related PE option\n"
    "  -subsystem=[console/windows]  set PE subsystem\n"
    "  -oformat=[pe-* binary]        set executable output format\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < nul\n"
#else
    "  -rpath=                       set dynamic library search path\n"
    "  -enable-new-dtags             set DT_RUNPATH instead of DT_RPATH\n"
    "  -soname=                      set DT_SONAME elf tag\n"
#if defined(TCC_TARGET_MACHO)
    "  -install_name=                set DT_SONAME elf tag (soname macOS alias)\n"
#endif
    "  -Bsymbolic                    set DT_SYMBOLIC elf tag\n"
    "  -oformat=[elf32/64-* binary]  set executable output format\n"
    "  -init= -fini= -Map= -as-needed -O   (ignored)\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < /dev/null\n"
#endif
    "See also the manual for more details.\n"
    ;

static const char version[] =
    "tcc version "TCC_VERSION
#ifdef TCC_GITHASH
    " "TCC_GITHASH
#endif
    " ("
#ifdef TCC_TARGET_I386
        "i386"
#elif defined TCC_TARGET_X86_64
        "x86_64"
#elif defined TCC_TARGET_C67
        "C67"
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_EABI
        " eabi"
#  ifdef TCC_ARM_HARDFLOAT
        "hf"
#  endif
# endif
#elif defined TCC_TARGET_ARM64
        "AArch64"
#elif defined TCC_TARGET_RISCV64
        "riscv64"
#endif
#ifdef TCC_TARGET_PE
        " Windows"
#elif defined(TCC_TARGET_MACHO)
        " Darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
        " FreeBSD"
#elif TARGETOS_OpenBSD
        " OpenBSD"
#elif TARGETOS_NetBSD
        " NetBSD"
#else
        " Linux"
#endif
    ")\n"
    ;

/* 为什么都是用static 修饰的方法呢? */
static void print_dirs(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void print_search_dirs(TCCState *s)
{
    printf("install: %s\n", s->tcc_lib_path);
    /* print_dirs("programs", NULL, 0); */
    print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);
    print_dirs("libraries", s->library_paths, s->nb_library_paths);
    printf("libtcc1:\n  %s/%s\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);
#if !defined TCC_TARGET_PE && !defined TCC_TARGET_MACHO
    print_dirs("crt", s->crt_paths, s->nb_crt_paths);
    printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
}

static void set_environment(TCCState *s)
{
    char * path;

    path = getenv("C_INCLUDE_PATH");
    printf("C_INCLUDE_PATH: %s\n", path);
    if(path != NULL) {
        tcc_add_sysinclude_path(s, path);
    }
    path = getenv("CPATH");
    printf("CPATH: %s\n", path);
    if(path != NULL) {
        tcc_add_include_path(s, path);
    }
    path = getenv("LIBRARY_PATH");
    printf("LIBRARY_PATH: %s\n", path);
    if(path != NULL) {
        tcc_add_library_path(s, path);
    }
}

static char *default_outputfile(TCCState *s, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    snprintf(buf, sizeof(buf), "%s", name);
    ext = tcc_fileextension(buf);
#ifdef TCC_TARGET_PE
    if (s->output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (s->output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
#endif
    if ((s->just_deps || s->output_type == TCC_OUTPUT_OBJ) && !s->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");
    return tcc_strdup(buf);
}

/* 返回时钟 */
static unsigned getclock_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
#endif
}

int main(int argc0, char **argv0)
{
    /* TCCState 是啥? */
    TCCState *s, *s1;
    int ret, opt, n = 0, t = 0, done;
    unsigned start_time = 0, end_time = 0;
    const char *first_file;
    int argc; char **argv;
    FILE *ppfp = stdout;
    printf("ppfp 是啥,为什么取这个名字: ppfp 是指向 stdcout 的文件指针----------------++\n");



redo: /* 使用了 label, why */
    argc = argc0, argv = argv0;
    s = s1 = tcc_new();
    printf("#### tcc state 的状态使用 tcc_new() >>>>>>>>> \n");
#ifdef CONFIG_TCC_SWITCHES /* predefined options */
    tcc_set_options(s, CONFIG_TCC_SWITCHES);
    printf("#### tcc_set_options, 这个 options 是什么呢?\n");
#endif
    opt = tcc_parse_args(s, &argc, &argv, 1);
    printf("#### 解析 tcc 的参数 tcc_parse_args \n");
    if (opt < 0) { 
        printf("命令行参数解析错误\n"); 
        return 1;
    }

    if (n == 0) {
        printf("命令行解析的参数 为 0 个则打印帮助语句\n");
        if (opt == OPT_HELP) {
            fputs(help, stdout);
            if (!s->verbose)
                return 0;
            ++opt;
        }
        if (opt == OPT_HELP2) {
            fputs(help2, stdout);
            return 0;
        }
        if (opt == OPT_M32 || opt == OPT_M64)
            return tcc_tool_cross(s, argv, opt);
        if (s->verbose)
            printf(" 嘿嘿，当前版本是: %s", version);
        if (opt == OPT_AR)
            return tcc_tool_ar(s, argc, argv);
#ifdef TCC_TARGET_PE
        if (opt == OPT_IMPDEF)
            return tcc_tool_impdef(s, argc, argv);
#endif
        if (opt == OPT_V)
            return 0;
        if (opt == OPT_PRINT_DIRS) {
            /* initialize search dirs */
            set_environment(s);
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
            print_search_dirs(s);
            return 0;
        }
        printf("##### 一共有多少个输入文件呢? %d 个\n", s->nb_files);
        if (s->nb_files == 0) {
            tcc_error_noabort("no input files");
        } else if (s->output_type == TCC_OUTPUT_PREPROCESS) { 
            printf("tcc.c 只是进行预处理..................\n");
            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("could not write '%s'", s->outfile);
            }
        } else if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) { 
            printf("生成的是目标文件................\n");
            if (s->nb_libraries)
                tcc_error_noabort("cannot specify libraries with -c");
            else if (s->nb_files > 1 && s->outfile) {
                // #define tcc_error_noabort   TCC_SET_STATE(_tcc_error_noabort)
                tcc_error_noabort("cannot specify output file with -c many files");
            }
        }

        printf("##### 所以 opt 返回值是啥呢？ ====> %d\n", opt);
        if (s->nb_errors)
            return 1;
        if (s->do_bench) { 
            printf("有 bench 标识, 准备记录起始时间了");
            start_time = getclock_ms();
        }
    }

    set_environment(s);
    if (s->output_type == 0)
        s->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(s, s->output_type);
    s->ppfp = ppfp;

    // 如果输出是在内存里面或者预处理的话，做了 blabla......
    if ((s->output_type == TCC_OUTPUT_MEMORY || s->output_type == TCC_OUTPUT_PREPROCESS) && (s->dflag & 16)) { 
        /* -dt option */
        if (t)
            s->dflag |= 32;
        s->run_test = ++t;
        if (n)
            --n;
    }

    /* compile or add each files or library */
    first_file = NULL;
    do {
        struct filespec *f = s->files[n];
        s->filetype = f->type;
        printf("#### 编译单个文件 -> %s\n", f->name);
        if (f->type & AFF_TYPE_LIB) {
            printf("AFF_TYPE_LIB类型......... 使用枚举来判断\n");
            ret = tcc_add_library_err(s, f->name);
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", f->name);
            if (!first_file)        // 为什么要有 first_file 呢?
                first_file = f->name;
            ret = tcc_add_file(s, f->name);
            printf("tcc_add_file 是干啥的..........., %s\n", f->name);
        }
        done = ret || ++n >= s->nb_files;
        printf("判断是否 done了..................\n");
    } while (!done && (s->output_type != TCC_OUTPUT_OBJ || s->option_r));

    if (s->do_bench) {
        printf("判断是否是 bench, 如果是的话计算时间");
        end_time = getclock_ms();
    }
        

    if (s->run_test) {
        t = 0;
    } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
        ;
    } else if (0 == ret) {
        if (s->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
            ret = tcc_run(s, argc, argv);
#endif
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            if (!s->just_deps && tcc_output_file(s, s->outfile))
                ;
            else if (s->gen_deps)
                gen_makedeps(s, s->outfile, s->deps_outfile);
        }
    }

    done = 1;
    if (t)
        done = 0; /* run more tests with -dt -run */
    else if (s->nb_errors)
        ret = 1;
    else if (n < s->nb_files)
        done = 0; /* compile more files with -c */
    else if (s->do_bench)
        tcc_print_stats(s, end_time - start_time);
    tcc_delete(s);
    if (!done) {
        printf("#### 没有完成,跳转到 redo 继续.........\n");
        goto redo;}
    if (ppfp && ppfp != stdout)
        fclose(ppfp);
    printf("编译结束了....... 返回的结果是 %d, 是否成功: %s\n", ret, ret ? "失败" : "成功");
    return ret;
}




/**
 * add by yangxu, 用于打印 sym 的详情
 */
void print_sym(Sym *sym) {
    if (!sym) {
        printf("Symbol is NULL\n");
        return;
    }
    
    printf("Symbol Information:\n");
    printf("Token (v): %d\n", sym->v);
    printf("Register/Const/Local (r): %d\n", sym->r);
    
    // Symbol attributes
    printf("Attributes:\n");
    printf("  Aligned: %d\n", sym->a.aligned);
    printf("  Packed: %d\n", sym->a.packed);
    printf("  Weak: %d\n", sym->a.weak);
    printf("  Visibility: %d\n", sym->a.visibility);
    printf("  DLL Export: %d\n", sym->a.dllexport);
    printf("  DLL Import: %d\n", sym->a.dllimport);
    printf("  Address Taken: %d\n", sym->a.addrtaken);
    printf("  No Debug: %d\n", sym->a.nodebug);
    
    // Associated number or function attributes
    printf("Associated Number (c): %d\n", sym->c);
    
    // Function attributes, if available
    printf("Function Attributes:\n");
    printf("  Call Convention: %d\n", sym->f.func_call);
    printf("  Function Type: %d\n", sym->f.func_type);
    printf("  No Return: %d\n", sym->f.func_noreturn);
    printf("  Constructor: %d\n", sym->f.func_ctor);
    printf("  Destructor: %d\n", sym->f.func_dtor);
    
    // Type information
    if (sym->type.ref) {
        printf("Type Information:\n");
        printf("  Type ID (t): %d\n", sym->type.t);
        printf("  Type Ref Symbol Token: %d\n", sym->type.ref->v);
    } else {
        printf("Type Reference is NULL\n");
    }
    
    // Previous symbol
    if (sym->prev) {
        printf("Previous Symbol Token: %d\n", sym->prev->v);
    } else {
        printf("No previous symbol in the stack.\n");
    }
    
    // Previous token symbol
    if (sym->prev_tok) {
        printf("Previous Token Symbol: %d\n", sym->prev_tok->v);
    } else {
        printf("No previous token symbol.\n");
    }
}



// 假设你已经定义了 Sym 结构体，并且有打印符号信息的相关函数

// 打印从第一个节点开始到最后一个节点的符号信息
void print_sym_chain(Sym *sym) {
    // Step 1: 回溯到第一个节点
    while (sym && sym->prev) {
        sym = sym->prev;  // 通过 prev 指针回溯到第一个节点
    }
    
    // Step 2: 从第一个节点开始打印
    printf("Printing symbol chain from the first node:\n");
    
    while (sym) {
        // 打印当前符号的信息
        printf("Symbol token: %d\n", sym->v); // 打印符号的标记
        // 你可以添加更多字段的打印逻辑，例如:
        // printf("Symbol type: %d\n", sym->type.t);

        // 移动到下一个符号
        sym = sym->next; // 这里假设 next 链接到下一个相关符号
    }
}
