/*
Copyright (c) 2013 Daniel Stahlke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <map>
#include <cstdio>
#include <boost/foreach.hpp>

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
