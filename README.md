# PCI Finger-Print

PCI finger-print utility intended for platform identification.

To get current system PCI full finger-print:

    pfp scan

## Finger-Print file format

Finger-print file is a line-oriented text file. Note: all hexadecimal
digits are lower case.

1.  The line is a sequence of non-NL symbols terminated by NL.
2.  The line comment is a line started from an octothorp (#) symbol.
3.  The argument name is a sequence of lower case Latin letters.
4.  The space is a sequece of one or more space and/or horizontal tabulation
    symbols.
5.  The match pattern is a sequence of non-space symbols.
6.  The pattern comment is a non-empty sequence of non-parenthesis symbols
    started from a left parenthesis and enclosed with a right parenthesis.
7.  The match argument is a line started from an argument name, followed by a
    space, followed by an equal sign, followed by other space, followed by a
    match pattern, optionaly followed by pair of space and pattern comment.
8.  Sequences of non-empty lines groupped to blocks.
9.  The comment block is a block of line comments.
10. The match rule is a block of match arguments.
11. The finger-print is a set of match rules.

Slot pattern format:

1. The bus prefix is a one o two didit hexadecimal integer number, followed
   by a semicolon symbol.
2. The device is a one or two digit hexadecimal integer number from set
   [0,1f].
3. The function is an octal digit.
4. The slot pattern is an optional bus prefix, followed by a device,
   followed by a dot symbol, followed by a function. If bus prefix is not
   specified then zero bus assumed.

Class pattern format:

1. Class and subclass are two-digit hexadecimal integer numbers.
2. The programming interface is a hexadecimal integer number from set [0,ff].
3. The class pattern is a sequence from a class, followed by a subclass,
   optionally followed by pair of dot symbol and programming interface. If
   programming interface is not specified then any one will be accepted.

Identifier pattern is a four-digit hexadecimal integer number.

## Match arguments

Followed arguments are recognized:

1. parent: parent device slot.
2. slot: device slot.
3. class: class, subclass and programming interface.
4. vendor: vendor identifier (chip vendor).
5. device: device identifier, vendor-specific.
6. svendor: subsystem vendor (who are made device from chips).
7. sdevice: subsystem device, subsystem vendor specific.
