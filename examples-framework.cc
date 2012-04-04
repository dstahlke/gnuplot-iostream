#include <map>
#include <cstdio>
#include "boost/foreach.hpp"

std::map<std::string, void (*)(void)> demos;

void register_demo(const std::string &label, void (*fn)(void)) {
	demos[label] = fn;
}

void register_demos();

int main(int argc, char **argv) {
	register_demos();

	if(argc < 2) {
		printf("Usage: %s <demo_name>\n", argv[0]);
		printf("Choose one of the following demos:\n");
		typedef std::pair<std::string, void (*)(void)> demo_pair;
		BOOST_FOREACH(const demo_pair &pair, demos) {
			printf("    %s\n", pair.first.c_str());
		}
		return 0;
	}

	std::string arg(argv[1]);
	if(!demos.count(arg)) {
		printf("No such demo '%s'\n", arg.c_str());
		return 1;
	}

	demos[arg]();
}
