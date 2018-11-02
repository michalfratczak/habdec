#ifdef __linux__
	const char* C_BLACK = 		"\033[1;30m";
	const char* C_RED = 		"\033[1;31m";
	const char* C_GREEN = 		"\033[1;32m";
	const char* C_BROWN = 		"\033[1;33m";
	const char* C_BLUE = 		"\033[1;34m";
	const char* C_MAGENTA = 	"\033[1;35m";
	const char* C_CYAN = 		"\033[1;36m";
	const char* C_LIGHTGREY = 	"\033[1;37m";
	const char* C_OFF =   		"\033[0m";
	const char* C_CLEAR =   	"\033[2K";
// #elif _WIN32
// #elif __APPLE__
// #elif __unix__ // all unices not caught above
// #elif defined(_POSIX_VERSION)
#else
	const char* C_BLACK = 		"";
	const char* C_RED = 		"";
	const char* C_GREEN = 		"";
	const char* C_BROWN = 		"";
	const char* C_BLUE = 		"";
	const char* C_MAGENTA = 	"";
	const char* C_CYAN = 		"";
	const char* C_LIGHTGREY = 	"";
	const char* C_OFF =   		"";
	const char* C_CLEAR =   	"";

#endif