# Target is x86_64 UEFI
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-uefi)
set(CMAKE_C_COMPILER_TARGET x86_64-unknown-uefi)
set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-uefi)

# Select tools
set(CMAKE_ASM_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_LINKER x86_64-w64-mingw32-ld)

# Don't run the linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compiler flags
set(ARCH_FLAGS "-fconcepts -ffreestanding -mno-red-zone -fno-stack-protector -msoft-float -mno-mmx -mno-sse -fno-finite-loops")
set(CMAKE_ASM_FLAGS "${ARCH_FLAGS}")
set(CMAKE_C_FLAGS "${ARCH_FLAGS} -Werror=return-type")
set(CMAKE_CXX_FLAGS "${ARCH_FLAGS} -fno-exceptions -fno-unwind-tables -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-e,_start -Wl,--subsystem,10")

# I shouldn't need the following, but for some reason I do for UEFI.
# I don't need it for the kernel where it just works as expected.
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

# Adjust the default behaviour of the FIND_XXX() commands:
# Search headers, libraries and packages in the target environment.
# Search programs in the host environment.
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Include paths for UEFI headers
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../../include)
