import sys
import struct
import subprocess
from ctypes import *
import time 

ntdll = windll.ntdll
psapi = windll.Psapi
kernel32 = windll.kernel32


def debug_print(message):
    print(message)
    kernel32.OutputDebugStringA(message + "\n")


def get_device_handle(device):
    open_existing = 0x3
    generic_read = 0x80000000
    generic_write = 0x40000000


    handle = kernel32.CreateFileA(device,
                                  generic_read | generic_write,
                                  None,
                                  None,
                                  open_existing,
                                  None,
                                  None)

    if not handle:
        debug_print("\t[-] Unable to get device handle")
        sys.exit(-1)
    return handle


def close_handle(handle):

    return kernel32.CloseHandle(handle)


if __name__ == "__main__":
    # constant declaration
    buffer_size = 0x8
    bytes_returned = c_ulong()
    IOCTL = 0x0022200B
    device_name = "\\\\.\\HideProc"

    device_handle = get_device_handle(device_name)

    
    success = kernel32.DeviceIoControl(device_handle,
                                       IOCTL,
                                       0,
                                       buffer_size,
                                       None,
                                       0,
                                       byref(bytes_returned),
                                       None)


    time.sleep(5)
    if not close_handle(device_handle):
        debug_print("\t[-] Unable to close device handle")

