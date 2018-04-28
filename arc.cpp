#include <stdio.h>

int main(int argc, char** argv) {
	auto lambda = [](auto x, auto y) {return x + y;};
	auto i = lambda(1, 3);
	printf("hey %d\n", i);
	return 0;
}

