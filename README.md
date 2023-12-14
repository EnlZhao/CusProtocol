# Custom Protocol Communication

- Realize custom protocol communication based on Socket interface

## File Structure

```bash
> tree
.
├── LICENSE
├── README.md
├── client
│   └── client.cc
├── exeFile
│   ├── client.exe
│   └── server.exe
├── makefile
├── myPacket
│   ├── mypacket.cc
│   └── mypacket.hh
└── server
    └── server.cc

5 directories, 9 files
```

## Usage

- Simply execute the Makefile to compile the program.
- Run the server first, then run the client. ("server.exe" & "client.exe" both in "exeFile" folder)

```bash
$ make all
```