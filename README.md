# HABDEC - RTTY decoder for High Altitude Balloons

![alt text](./webClientScreenshot.png)

Habdec is a C++11 software to decode RTTY telemetry from High Altitude Balloon and upload it to [UKHAS Habitat](http://habitat.habhub.org/)

Some facts:
- builds and runs on Windows/Linux and x64/RaspberryPI/OdroixXU4 platforms
- Uses [SoapySDR](https://github.com/pothosware/SoapySDR) to consume IQ stream from your SDR
- runs in command line (headless)
- provides websocket server so can be cotrolled from any web browser [even on your phone](https://www.youtube.com/watch?v=dli8FEFy5tM)
- can be easily integrated into your own code

Original motivation for habdec was to have a portable tracking device you could take to a chasecar or into a field.

Fatty laptop with dl-fldigi, full-fledged SDR app and required internet connection is not too comfortable.

Ultimatelly habdec is intended for integration into bigger app, probably based on OpenStreetmap and running on portable RaspberryPI or OdroidXU4. Right now you can run it on headless board and cotrol from your mobile.

## Getting habdec

To get habdec you can download precompiled exec or build it from source.

#### Getting Binary:
- Install SoapySDR binaries from [here](https://github.com/pothosware/SoapySDR/wiki#installation)
- Download habdec executable for your platform from [here](missing link)

#### Building from source:
To build habdec you need a C++11 compiler and CMake version 3.8.2
You also need to build or install some dependencies:
- basic decoder: [FFTW](http://www.fftw.org/)
- websocket server: [SoapySDR](https://github.com/pothosware/SoapySDR), [boost 1.68](https://www.boost.org/)

Instructions how to build habdec and it's dependencies are described in [BuildInstructions.md](./BuildInstructions.md)

## Usage

`habdecWebsocketServer.exe --help`
```
Command Line Interface options:
  --config arg (=./habdecWebsocketServer.opts)
                                        Last run config file. Autosaved on
                                        every successfull decode.

CLI opts:
  --help                                Display help message
  --device arg (=-1)                    SDR Device Numer
  --sampling_rate arg (=0)              Sampling Rate, as supported by device
  --port arg (=5555)                    Command Port, example: --port
                                        127.0.0.1:5555
  --station arg (=habdec)               HABHUB station callsign
  --freq arg                            frequency in MHz
  --gain arg                            gain
  --print arg                           live print received chars
  --rtty arg                            rtty: baud bits stops, example -rtty
                                        300 8 2
  --biast arg                           biasT
  --bias_t arg                          biasT
  --afc arg                             Auto Frequency Correction
```

The only mandatory parameter is --sampling_rate. Provide value that is supported by your SDR device.

### Examples:
Print available devices

```
habdecWebsocketServer.exe --device -1


Reading config from file ./habdecWebsocketServer.opts
Current options:
        device: -1
        sampling_rate: 2.048e+06
        command_host: 0.0.0.0
        command_port: 5,555
        station: habdec
        freq: 4.3435e+08
        gain: 15
        live_print: 1
        baud: 300
        rtty_ascii_bits: 8
        rtty_ascii_stops: 2
        biast: 0
ERROR: Unable to find host: Nieznany host.
Found Rafael Micro R820T tuner
[INFO] [UHD] Win32; Microsoft Visual C++ version 14.0; Boost_106700; UHD_3.13.0.2-1-g78745bda

Available devices:
0:
        device_id 0
        driver airspy
        label AIRSPY [440464c8:39627b4f]
        serial 440464c8:39627b4f
        Sampling Rates:
                2.5e+06
                1e+07

1:
        available Yes
        driver rtlsdr
        label Generic RTL2832U OEM :: 00000001
        manufacturer Realtek
        product RTL2838UHIDIR
        rtl 0
        serial 00000001
        tuner Rafael Micro R820T
Found Rafael Micro R820T tuner
        Sampling Rates:
                250,000
                1.024e+06
                1.536e+06
                1.792e+06
                1.92e+06
                2.048e+06
                2.16e+06
                2.56e+06
                2.88e+06
                3.2e+06


No SDR device specified. Select one by specifying it's NUMBER
Failed Device Setup. EXIT.

```

Run with AirSpy

    habdecWebsocketServer.exe --device 0 --sampling_rate 2.5e6

Specify websocket address and port

    habdecWebsocketServer.exe --device 0 --sampling_rate 2.5e6 -port 12.13.14.15:5555

Some more options
```
habdecWebsocketServer.exe   --device 0 --sampling_rate 2.5e6
                            --port 12.13.14.15:5555 --station Fred
                            --rtty 300 8 2
                            --print 1
                            --freq 434.5 --gain 20 --biast 1 --afc 1
```


### Web Client

To control habdec parameters from your browser, start    `webClient/index.html` and connect to ip:port


## Known Limitations

- RTTY Modes **NOT** supported: 5bit baudot, 1.5 bit stop
- SSTV is not supported
- Decoding will stop if decimation setting is too low or too high. It was tested to work with stream around 40kHz bandwidth.
- Automatic Frequency Correction needs more work. Use consciously.
- Connecting from browser is not very reliable yet, sometimes you need to refresh and wait.
- Currently, upload to HABHUB is realized with python habLogger.py which is called with system(). This will get ported to C++.
- habdec was developed and tested with [AirSpy](https://airspy.com/) and [OdroidXU4](http://hardkernel.com/). Support for windows and RtlSdr is less tested.


## Reporting Problems

Use bugtracker, please.

## Contributions

Gladly accepted :)

## Authors

* **Michał Frątczak** - *parts of code from* - [GQRX](https://github.com/csete/gqrx)

## License

This project is licensed under the GNU General Public License
