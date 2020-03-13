'''
A format checker that checks if the specified number of CAN bytes for a PID is
valid, given the output type for the PID.

Specifically, the rule is as follows:
- For formula-type PIDs (ones with val != "nop"), CAN bytes between 1 and 4 are
  OK.
- For float-type passthrough PIDs, CAN bytes must be 4. This is because we do
  intepret the raw bytes as an IEEE 754, which occupies 4 bytes.
- For int-type passthrough PIDs, the output type must be at least as large as
  the number of CAN bytes. This is so that we do only widening casts when we
  output a value, as narrowing casts would lose data.
'''

import jsonschema


FORMAT_NAME = 'nop-bytes'


def check(desc):
    '''Returns True if this PID node is a passthrough (nop) node and has a byte
    count that matches the type, according to the rules specified above.'''

    try:
        expr = desc['expr']
    except KeyError:
        raise jsonschema.exceptions.FormatError('no "expr" key in %s' % desc)

    try:
        val = expr['val']
    except KeyError:
        raise jsonschema.exceptions.FormatError('no "val" key in expr in %s'
            % desc)

    if val != 'nop':
        # No CAN byte restrictions on non-nop values.
        return True

    try:
        can_bytes = desc['bytes']
    except KeyError:
        raise jsonschema.exceptions.FormatError('no "bytes" key in %s' % desc)
    try:
        can_bytes = int(can_bytes)
    except ValueError:
        raise jsonschema.exceptions.FormatError(
            'can_bytes value is not an int, in %s' % desc)

    try:
        pid_type = expr['type']
    except KeyError:
        raise jsonschema.exceptions.FormatError('no "expr" key in %s' % desc)

    if pid_type == 'float' and can_bytes != 4:
        raise jsonschema.exceptions.FormatError(
            'float output type must have 4 CAN bytes in %s' % desc)
    elif pid_type in ('int8', 'uint8') and can_bytes != 1:
        raise jsonschema.exceptions.FormatError(
            'int8/uint8 types must have 1 CAN byte in %s' % desc)
    elif pid_type in ('int16', 'uint16') and not (1 <= can_bytes <= 2):
        raise jsonschema.exceptions.FormatError(
            'int8/uint8 types must have 1-2 CAN bytes in %s' % desc)
    elif pid_type in ('int32', 'uint32') and not (1 <= can_bytes <= 4):
        raise jsonschema.exceptions.FormatError(
            'int32/uint32 types must have 1-2 CAN bytes in %s' % desc)

    return True
