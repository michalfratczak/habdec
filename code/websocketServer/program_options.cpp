/*

	Copyright 2018 Michal Fratczak

	This file is part of habdec.

    habdec is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    habdec is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with habdec.  If not, see <https://www.gnu.org/licenses/>.
*/


#include <string>
#include <iostream>
#include <fstream>
#include <regex>

#include <boost/program_options.hpp>

#include "GLOBALS.h"
#include "common/console_colors.h"

void prog_opts(int ac, char* av[])
{
	namespace po = boost::program_options;
	using namespace std;

	try
	{
		po::options_description generic("CLI opts");
		generic.add_options()
			("help", "Display help message")
			("device",	po::value<int>()->default_value(-1), "SDR Device Numer")
			("sampling_rate",	po::value<double>()->default_value(0), "Sampling Rate, as supported by device")

			("port",	po::value<string>()->default_value("5555"),	"Command Port, example: --port 127.0.0.1:5555")
			("station",	po::value<string>()->default_value(""),	"HABHUB station callsign")

			("freq",	po::value<float>(), "frequency in MHz")
			("gain",	po::value<int>(), "gain")
			("print",   po::value<bool>(), "live print received chars, values: 0, 1")
			("rtty",	po::value< std::vector<float> >()->multitoken(), "rtty: baud bits stops, example: -rtty 300 8 2")
			("biast",	po::value<bool>(), "biasT, values: 0, 1")
			("bias_t",	po::value<bool>(), "biasT, values: 0, 1")
			("afc",		po::value<bool>(), "Auto Frequency Correction, values: 0, 1")

			("sentence_cmd",	po::value<string>(), "call external command with sentence as parameter")
		;

		po::options_description cli_options("Command Line Interface options");
		cli_options.add(generic);

		string config_file;
		cli_options.add_options()
            ("config", po::value<string>(&config_file), "Last run config file. Autosaved on every successfull decode.");
            // ("config", po::value<string>(&config_file)->default_value("./habdecWebsocketServer.opts"), "Last run config file. Autosaved on every successfull decode.");

		po::options_description file_options;
        file_options.add(generic);

		po::variables_map vm;
		store( po::command_line_parser(ac, av).options(cli_options).allow_unregistered().run(), vm );
		notify(vm);

		if(vm.count("help"))
        {
            cout<<cli_options<<endl;
			exit(0);
		}

		if(config_file != "")
		{
			ifstream ifs(config_file.c_str());
			if (!ifs)
			{
				cout << "Can not open config file: " << config_file << endl;
			}
			else
			{
				cout<<C_RED<<"Reading config from file "<<config_file<<C_OFF<<endl;
				store(parse_config_file(ifs, file_options, 1), vm);
				notify(vm);
			}
		}

		if (vm.count("device"))
		{
			GLOBALS::get().device_ = vm["device"].as<int>();
		}
		if (vm.count("sampling_rate"))
		{
			GLOBALS::get().sampling_rate_ = vm["sampling_rate"].as<double>();
		}
		if (vm.count("port")) // [host:][port]
		{
			smatch match;
			regex_match( vm["port"].as<string>(), match, std::regex(R"_(([\w\.]*)(\:?)(\d*))_") );

			if(match.size() == 4)
			{
				if(match[2] == "" && match[3] == "") // special case when only port is given: --port 5555
				{
					GLOBALS::get().command_port_ = stoi(match[1]);
				}
				else
				{
					if(match[1] != "")	GLOBALS::get().command_host_ = match[1];
					if(match[3] != "")	GLOBALS::get().command_port_ = stoi(match[3]);
				}
			}
		}
		if (vm.count("station"))
		{
			GLOBALS::get().station_callsign_ = vm["station"].as<string>();
		}
		if (vm.count("sentence_cmd"))
		{
			GLOBALS::get().sentence_cmd_ = vm["sentence_cmd"].as<string>();
		}
		if (vm.count("freq"))
		{
			GLOBALS::get().frequency_ = vm["freq"].as<float>() * 1e6;
		}
		if (vm.count("gain"))
		{
			GLOBALS::get().gain_ = vm["gain"].as<int>();
		}
		if (vm.count("print"))
		{
			GLOBALS::get().live_print_ = vm["print"].as<bool>();
		}
		if (vm.count("biast"))
		{
			GLOBALS::get().biast_ = vm["biast"].as<bool>();
		}
		if (vm.count("bias_t"))
		{
			GLOBALS::get().biast_ = vm["bias_t"].as<bool>();
		}
		if (vm.count("afc"))
		{
			GLOBALS::get().afc_ = vm["afc"].as<bool>();
		}
		if (vm.count("rtty"))
		{
			vector<float> rtty_tokens = vm["rtty"].as< vector<float> >();
			if( rtty_tokens.size() != 3 )
			{
				cout<<C_RED<<"--rtty option needs 3 args: baud ascii-bits stop-bits"<<C_OFF<<endl;
				exit(1);
			}

			if(rtty_tokens[2] != 1 && rtty_tokens[2] != 2)
			{
				cout<<C_RED<<"Only 1 or 2 stop bits are supported."<<C_OFF<<endl;
				exit(1);
			}
			if(rtty_tokens[1] != 7 && rtty_tokens[1] != 8)
			{
				cout<<C_RED<<"ASCII Bits must be 7 or 8"<<C_OFF<<endl;
				exit(1);
			}

			GLOBALS::get().baud_ = rtty_tokens[0];
			GLOBALS::get().rtty_ascii_bits_ = rtty_tokens[1];
			GLOBALS::get().rtty_ascii_stops_ = rtty_tokens[2];
		}
	}
	catch(exception& e)
	{
		cout << e.what() << "\n";
	}

	// print globals:
	cout<<"Current options: "<<endl;
	cout<<"\tdevice: "<<GLOBALS::get().device_<<endl;
	cout<<"\tsampling_rate: "<<GLOBALS::get().sampling_rate_<<endl;
	cout<<"\tcommand_host: "<<GLOBALS::get().command_host_<<endl;
	cout<<"\tcommand_port: "<<GLOBALS::get().command_port_<<endl;
	cout<<"\tsentence_cmd: "<<GLOBALS::get().sentence_cmd_<<endl;
	cout<<"\tstation: "<<GLOBALS::get().station_callsign_<<endl;
	cout<<"\tfreq: "<<GLOBALS::get().frequency_<<endl;
	cout<<"\tgain: "<<GLOBALS::get().gain_<<endl;
	cout<<"\tlive_print: "<<GLOBALS::get().live_print_<<endl;
	cout<<"\tbaud: "<<GLOBALS::get().baud_<<endl;
	cout<<"\trtty_ascii_bits: "<<GLOBALS::get().rtty_ascii_bits_<<endl;
	cout<<"\trtty_ascii_stops: "<<GLOBALS::get().rtty_ascii_stops_<<endl;
	cout<<"\tbiast: "<<GLOBALS::get().biast_<<endl;

	GLOBALS::DumpToFile("./habdecWebsocketServer.opts");
}