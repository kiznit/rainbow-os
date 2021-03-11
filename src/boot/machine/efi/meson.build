includes += '../third_party/uefi'

sources += files(
    'efiboot.cpp',
    'efidisplay.cpp',
    'efifilesystem.cpp',
    'jumpkernel.cpp',
    'malloc.cpp',
    arch / 'reloc.cpp',
    arch / 'start.S',
)


link_script = meson.current_source_dir() / arch / 'efi.lds'
