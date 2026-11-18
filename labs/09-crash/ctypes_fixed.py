import ctypes


def clear_buffer(buffer):
    ctypes.memset(ctypes.addressof(buffer), 0, ctypes.sizeof(buffer))


if __name__ == "__main__":
    buffer = ctypes.create_string_buffer(b"agent", 8)
    clear_buffer(buffer)
    print("buffer:", list(buffer.raw))
