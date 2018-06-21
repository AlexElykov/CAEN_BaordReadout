# CAEN_BaordReadout
To run:

make
./main -l 0 -c 0


Provided one has the required CAEN libraries and up to PCIe date drivers,
this code will read the buffer from a single or multiple V1724 boards,
depending on how one configures the code.

Note that the boards are numbered [0:N] for each link.
Hence, when addressing multiple boards each of which is linked by
a separate optical link to different ports of a single PCIe
link number goes from 0...x, while the number of the board for each link is 0.

For more details look at the comments in the code.

Useful register values to run the boards (can be obviously changed):
REGISTER - VALUE
EF24        1
EF1C        1
EF00        10
8120        FF
8000        310
8080        310000
800C        A
8020        258
811C        110
8100        0
81A0        200
8098        1000
8038        12C
8060        3E8
