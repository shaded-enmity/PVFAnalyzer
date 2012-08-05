/**********************************************************************
          DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

**********************************************************************/

#include <getopt.h>

#include "cfg.h"
#include "version.h"

struct option my_opts[] = {
	{"file",    required_argument, 0, 'f'},
	{"help",    no_argument,       0, 'h'},
	{"outfile", required_argument, 0, 'o'},
	{"verbose", no_argument,       0, 'v'},
	{0,0,0,0} // this line be last
};

static bool verbose                = false;
static std::string input_filename  = "input.cfg";
static std::string output_filename = "output.dot";


static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Read CFG from file. [input.cfg]" << std::endl;
	std::cout << "\t-o <file>          Write graphviz output to file. [output.dot]" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
}



static void
banner()
{
	version_t version = global_program_version;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Analyzer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "f:ho:v")) != -1) {
	}
	return false;
}


int main(int argc, char **argv)
{
	if (not parseInputFromOptions(argc, argv))
		return 1;

	return 0;
}