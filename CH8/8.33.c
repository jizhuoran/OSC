#include <stdio.h>

int main(int argc, char const *argv[])
{
	int entries;
	printf("please enter the address: \n");
	scanf("%d", &entries);
	printf("The address %d contains: \n", entries);
	printf("page number = %d \n", entries/4096);
	printf("offset = %d \n", entries % 4096);
	return 0;
}