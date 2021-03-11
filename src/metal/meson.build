sources += files(
    'src/c++/new.cpp',
    'src/c++/shared_ptr.cpp',
    'src/libc/abort.cpp',
    'src/libc/assert.cpp',
    'src/libc/memcmp.cpp',
    'src/libc/memcpy.cpp',
    'src/libc/memmove.cpp',
    'src/libc/memset.cpp',
    'src/libc/printf.cpp',
    'src/libc/puts.cpp',
    'src/libc/strcmp.cpp',
    'src/libc/strcpy.cpp',
    'src/libc/strlen.cpp',
    'src/runtime/cpprt.cpp',
    'src/runtime/crti.c',
    'src/runtime/crtn.c',
)

if (arch == 'ia32' or arch == 'x86_64')
    sources += files(
        'src/arch/x86/cpu.cpp',
        'src/arch/x86/cpuid.cpp',
        'src/arch/x86/mtrr.cpp',
    )
endif
