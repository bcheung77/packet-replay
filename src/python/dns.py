import struct
import io

# DNS packet validation function that avoids comparing the TTLs
def validate(expected, actual):
    ttls = []
    reader = io.BytesIO(expected)

    header_items = struct.unpack("!HHHHHH", reader.read(12))

    qd_count = header_items[2];
    an_count = header_items[3];
    ns_count = header_items[4];
    ar_count = header_items[5];

    total_record_count = an_count + ns_count + ar_count

    for _ in range(qd_count):
        skip_question(reader)

    for _ in range(total_record_count):
        skip_name(reader)
        reader.read(4) # type + class
        ttls.append(reader.tell())
        data_len = struct.unpack("!H", reader.read(2))[0]
        reader.read(data_len)
        # TODO: handle OPT records which use TTL for other purposes

    start_idx = 0

    # this can be optimized by doing the slice comparison whenever a TTL is detected but I 
    # think this implementation is clearer
    for ttl in ttls:
        if expected[start_idx : ttl] != actual[start_idx : ttl]:
            print("e:", expected[start_idx : ttl])
            print("a:", actual[start_idx : ttl])
            print("failure", start_idx, ":", ttl)
            return False

        start_idx = ttl + 4

    return expected[start_idx:] == actual[start_idx:]

def skip_question(reader):
    skip_name(reader)
    reader.read(4) # QTYPE + QCLASS

def skip_name(reader):
    label_len = struct.unpack("B", reader.read(1))[0]

    while label_len > 0:
        if label_len > 63:
            #pointer
            reader.read(1)
            break;
        else:
            reader.read(label_len)
        label_len = struct.unpack("B", reader.read(1))[0]

    
