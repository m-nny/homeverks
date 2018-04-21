import time
from io import BytesIO, StringIO


def lzw_compress(uncompressed: str) -> str:
    """Compress a string to a list of output symbols."""
    # Build the dictionary.
    dict_size = 256
    bit_len = 8
    dictionary = {(i,): i for i in range(dict_size)}

    w = ()
    s_result = StringIO()
    for i in range(0, len(uncompressed), 8):
        c = int(uncompressed[i:i + 8], 2)
        wc = w + (c,)
        if wc in dictionary:
            w = wc
        else:
            code = dictionary[w]
            s_result.write('{:0{}b}'.format(code, bit_len))
            # Add wc to the dictionary.
            dictionary[wc] = dict_size
            dict_size += 1
            if dict_size > (1 << bit_len):
                bit_len += 1
            w = (c,)
    # Output the code for w.
    print('dict_size {}'.format(dict_size))
    if w:
        code = dictionary[w]
        s_result.write('{:0{}b}'.format(code, bit_len))
    return s_result.getvalue()
    # b_result, rem = str_to_bytes(s_result.getvalue())
    # return b_result


def lwz_decompress(compressed: str) -> str:
    """Decompress a list of output ks to a string."""

    # Build the dictionary.
    dict_size = 256
    bit_len = 8
    # dictionary = dict((i, chr(i)) for i in range(dict_size))
    dictionary = {i: (i,) for i in range(dict_size)}

    # s_comp = StringIO(bytes_to_str(compressed))
    s_comp = StringIO(compressed)

    result = StringIO()
    tmp = s_comp.read(bit_len)
    w = (int(tmp, 2),)
    for x in w:
        result.write()
    result.write(bytes(w))
    # result.write(w)

    while True:
        if dict_size == (1 << bit_len):
            bit_len += 1
        tmp = s_comp.read(bit_len)
        if not tmp:
            break
        k = int(tmp, 2)
        if k in dictionary:
            entry = dictionary[k]
        elif k == dict_size:
            entry = w + (w[0],)
        else:
            raise ValueError('Bad compressed k: %s' % k)
        # for x in entry:
        result.write(bytes(entry))
        # result.write(entry)

        # Add w+entry[0] to the dictionary.
        dictionary[dict_size] = w + (entry[0],)
        dict_size += 1

        w = entry
    print('dict_size {}'.format(dict_size))
    return result.getvalue()


def compare(uncompressed: bytes, compressed: str):
    orig_bits = len(uncompressed) * 8
    comp_bits = len(compressed)
    print("{} vs {}\nRate:{}".format(orig_bits, comp_bits, orig_bits / comp_bits))


def str_to_bytes(data: str) -> (bytes, int):
    rem = (8 - len(data) % 8) % 8
    if rem > 0:
        data = "0" * rem + data
    # for i in range(0, len(data), 8):
    #     print(data[i:i + 8], int(data[i:i + 8], 2))
    return bytes(int(data[i: i + 8], 2) for i in range(0, len(data), 8)), rem


def bytes_to_str(data: bytes, rem: int = 0) -> str:
    return "".join("{:08b}".format(x) for x in data)[rem:]


def main():
    # How to use:
    path = '1.bmp1'
    try:
        with open(path, 'rb') as file:
            init_data = file.read()
    except EnvironmentError:
        init_data = b'TOBEORNOTTOBEORTOBEORNOT'

    # b_string = "101"
    # b_bytes, rem = str_to_bytes(b_string)
    # b_de_destring = bytes_to_str(b_bytes, rem)
    # print("|", b_string, "|")
    # print(b_bytes, rem)
    # print(b_de_destring)
    # print(b_string == b_de_destring)
    # return

    time1 = time.time()
    binary_data = bytes_to_str(init_data)
    compressed_data = lzw_compress(binary_data)
    time2 = time.time()
    print('Compression function took %0.3f ms' % ((time2 - time1) * 1000.0))
    print(len(init_data), len(compressed_data))
    compare(init_data, compressed_data)
    time2 = time.time()
    decompressed_data = lwz_decompress(compressed_data)
    time3 = time.time()

    print('Decompression function took %0.3f ms' % ((time3 - time2) * 1000.0))

    print(len(init_data), len(decompressed_data))
    print(init_data == decompressed_data)


if __name__ == "__main__":
    main()
