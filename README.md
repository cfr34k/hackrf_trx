# HackRF TRX

This program allows to use the HackRF in half duplex mode with dynamic RX/TX
switching. Currently this is implemented using standard input/output.

While there is no data incoming on STDIN, the HackRF is in receive mode and
writes samples to STDOUT. When data is seen on STDIN, the receive process is
stopped and the incoming data is transmitted. When the incoming data stream
stops or only values of constant value are read on STDIN, the HackRF is
switched back to receive mode.

## Setup

Copy `src/config.h.template` to `src/config.h` and adjust it to your needs.
Then use `./make.sh` to build the program. The binary will be called
`build/hackrf_trx`.

## License

MIT license, see COPYING.
