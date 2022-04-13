/*
    Copyright (c) 2021, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#define EFIAPI __attribute__((ms_abi))

namespace efi
{
    // UEFI defines BOOLEAN as a uint8_t, False as 0 and True as 1.
    // Luckily for us it appears bool meets these criteria. If this turns
    // out not to be true for some platform, we will have to revisit the
    // decision to use bool in the UEFI interfaces and structures.
    static_assert(sizeof(bool) == sizeof(uint8_t));

    // Characters in UEFI:
    //
    // char8:  8-bit ASCII using the ISO-Latin-1 character set
    // char16: UCS-2 encoding (the Private Usage Area [0xE000-0xF8FF] is used by UEFI)
    //         Note: this is NOT UTF-16, it really is UCS-2.

    using intn_t = std::make_signed_t<size_t>;
    using uintn_t = size_t;

    static_assert(sizeof(intn_t) == sizeof(size_t));
    static_assert(sizeof(uintn_t) == sizeof(size_t));

    static constexpr uintn_t EncodeError(uintn_t error)
    {
        constexpr auto shift = sizeof(uintn_t) * 8 - 1;
        return (uintn_t(1) << shift) | error;
    }

    static constexpr uintn_t EncodeWarning(uintn_t warning) { return warning; }

    enum Status : uintn_t
    {
        Success = 0,

        LoadError = EncodeError(1),
        InvalidParameter = EncodeError(2),
        Unsupported = EncodeError(3),
        BadBufferSize = EncodeError(4),
        BufferTooSmall = EncodeError(5),
        NotReady = EncodeError(6),
        DeviceError = EncodeError(7),
        WriteProtected = EncodeError(8),
        OutOfResource = EncodeError(9),
        VolumeCorrupted = EncodeError(10),
        VolumeFull = EncodeError(11),
        NoMedia = EncodeError(12),
        MediaChanged = EncodeError(13),
        NotFound = EncodeError(14),
        AccessDenied = EncodeError(15),
        NoResponse = EncodeError(16),
        NoMapping = EncodeError(17),
        Timeout = EncodeError(18),
        NotStarted = EncodeError(19),
        AlreadyStarted = EncodeError(20),
        Aborted = EncodeError(21),
        IcmpError = EncodeError(22),
        TftpError = EncodeError(23),
        ProtocolError = EncodeError(24),
        IncompatibleVersion = EncodeError(25),
        SecurityViolation = EncodeError(26),
        CrcError = EncodeError(27),
        EndOfMedia = EncodeError(28),
        EndOfFile = EncodeError(31),
        InvalidLanguage = EncodeError(32),
        CompromisedData = EncodeError(33),
        HttpError = EncodeError(35),

        WarningUnknownGlyph = EncodeWarning(1),
        WarningDeleteFailure = EncodeWarning(2),
        WarningWriteFailure = EncodeWarning(3),
        WarningBufferTooSmall = EncodeWarning(4),
        WarningStaleData = EncodeWarning(5),
        WarningFileSystem = EncodeWarning(6),
    };

    static constexpr bool Error(Status status) { return static_cast<std::make_signed_t<uintn_t>>(status) < 0; }

    using Handle = void*;
    using Event = void*;
    using PhysicalAddress = uint64_t;
    using VirtualAddress = uint64_t;

    struct Guid
    {
        uint32_t data1;
        uint16_t data2;
        uint16_t data3;
        uint8_t data4[8];
    };

    static_assert(sizeof(Guid) == 16);

    struct Time
    {
        uint16_t year;  // 1900...9999
        uint8_t month;  // 1..12
        uint8_t day;    // 1..31
        uint8_t hour;   // 0..23
        uint8_t minute; // 0..59
        uint8_t second; // 0..59
        uint8_t pad1;
        uint32_t nanosecond; // 0..999999999
        int16_t timeZone;    // -1440..1440 or 2047
        uint8_t daylight;    // -1..1
        uint8_t pad2;
    };

    static_assert(sizeof(efi::Time) == 16);

    struct TimeCapabilities
    {
        uint32_t resolution;
        uint32_t accuracy;
        bool setsToZero;
    };

    static_assert(sizeof(efi::TimeCapabilities) == 12);

    struct TableHeader
    {
        uint64_t signature;
        uint32_t revision;
        uint32_t headerSize;
        uint32_t crc32;
        uint32_t reserved;
    };

    static_assert(sizeof(efi::TableHeader) == 24);

} // namespace efi