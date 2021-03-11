project('boot', ['c', 'cpp'], version : '1.0')

arch = get_option('arch')
machine = get_option('machine')

readelf = find_program('readelf', required: false)
objcopy = find_program('objcopy')
objdump = find_program('objdump', required: false)


add_project_arguments(
    '-ffreestanding',
    '-fbuiltin',
    '-mgeneral-regs-only',
    language: ['c', 'cpp']
)

add_project_arguments(
    '-DKERNEL_ARCH=' + arch,
    '-DKERNEL_' + arch.to_upper(),
    language: ['c', 'cpp']
)

add_project_link_arguments(
    '-nostdlib',
    '-z', 'noexecstack',
    '-Wl,--warn-common',
    '-Wl,--no-undefined',
    '-Wl,--fatal-warnings',
    language: ['c', 'cpp']
)


includes = [
    '..',
    '../include',
    '../metal/include',
    '../metal/include/c++',
    '../third_party',
    '../../user/include',
]

sources = files(
    'boot.cpp',
    'display.cpp',
    'elfloader.cpp',
    'fatal.cpp',
    'memory.cpp',
    'runtime/crt.cpp',
    'runtime/newlib.cpp',
)

subdir('arch')
subdir('machine/' + machine)
subdir('modules/graphics')
subdir('modules/metal')


cpp = meson.get_compiler('cpp')
libgcc = cpp.find_library('libgcc')
crtbegin = run_command(cpp, '-print-file-name=crtbegin.o').stdout().strip()
crtend = run_command(cpp, '-print-file-name=crtend.o').stdout().strip()


libs = [
    libgcc,
]


bootloader = executable('bootloader',
    sources: sources,
    objects: [crtbegin, crtend],
    include_directories: includes,
    dependencies: libs,
    link_args: [
        '-T' + meson.source_root() / link_script,
    ],
    link_depends: link_script,
)


custom_target(
    'bootloader.efi',
    input: bootloader,
    output: 'boot.efi',
    command: [ objcopy,
        '-j', '.text',
        '-j', '.rodata',
        '-j', '.data',
        '-j', '.dynamic',
        '-j', '.dynsym',
        '-j', '.rel.*',
        '-j', '.rela.*',
        '-j', '.reloc',
        '--target', 'efi-app-' + arch,
        '@INPUT@',
        '@OUTPUT@'
    ],
    build_by_default: true,
)


if (readelf.found())
    custom_target(
        'readelf',
        input: bootloader,
        output: 'bootloader.elf',
        command: [ readelf, '-a', '@INPUT@' ],
        capture: true,
        build_by_default: true,
    )
endif


if (objdump.found())
    custom_target(
        'bootloader.disassembly',
        input: bootloader,
        output: 'bootloader.disassembly',
        command: [ objdump, '-D', '@INPUT@' ],
        capture: true,
        build_by_default: true,
    )

    custom_target(
        'crtbegin.o.disassembly',
        input: crtbegin,
        output: 'crtbegin.o.disassembly',
        command: [ objdump, '-D', '@INPUT@' ],
        capture: true,
        build_by_default: true,
    )

    custom_target(
        'crtend.o.disassembly',
        input: crtend,
        output: 'crtend.o.disassembly',
        command: [ objdump, '-D', '@INPUT@' ],
        capture: true,
        build_by_default: true,
    )
endif
