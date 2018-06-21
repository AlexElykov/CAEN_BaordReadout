# CAEN_BoardReadout
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
