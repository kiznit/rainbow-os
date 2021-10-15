UEFI Bootloader
===============


UEFI Firmware Bugs
------------------

### AllocatePool() and AllocatePages() using an application defined memory type.

According to the UEFI specification, memory types with value 0x80000000
and above are reserved to the application. Using a memory type in that
range will make the next call to GetMemoryMap() hang some computers. This
is happening on my main development machine (Asus Hero Maximus VI,
firmware build 1603 2014/09/19).

The workaround is to not use application defined memory types and use
EfiLoaderData instead. It does mean that we can't identify our own
allocations when walking the memory map, which is rather unfortunate.


### SetVirtualAddressMap() calling into EFI Boot Services.

You cannot call SetVirtualAddressMap() before you call ExitBootServices().
If you try, you will get the EFI_UNSUPPORTED error.

SetVirtualAddressMap() is not supposed to call into boot services, but
of course it does. So you have to make sure you keep the boot services
memory around while calling SetVirtualAddressMap().


### Continuous runtime services memory must be allocated continuously

If the memory map returned by the firmware contains multiple descriptors
containing continuous memory for runtime services, they must be mapped
continuously into virtual memory. In theory this isn't required, but of
course some firmwares do it wrong.


### Writing UEFI variables will brick some machines.

This has been reported for some Samsung laptops. Details can be found
here: http://mjg59.dreamwidth.org/22855.html.

Linux seems to work around this specific issue by making sure less than
half of the available variable memory is used at any given time.
