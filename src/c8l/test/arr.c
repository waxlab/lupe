#include "../arr.h"
#include <assert.h>
#include <string.h>
#include <errno.h>


static int test_push() {
	int i = 0;
	int *num = c8l_arrnew(*num,2);
	char **str = c8l_arrnew(*str,2);

	if (num != NULL) {
		num[0]=10;
		num[1]=20;

		for (i=0; i<20; i++) c8l_arrpush(num, i);
		assert(c8l_arrlen(num) == 20);
		assert(num[19] == 19);


		while(c8l_arrlen(num) > 0) c8l_arrpop(num,-1);
		assert(c8l_arrlen(num) == 0);

		c8l_arrcapsz(num,100);
		assert(c8l_arrcap(num) >= 100);

		c8l_arrfree(num);
		assert(num == NULL);
		c8l_arrfree(num); /* no risk for double free */

	} else {
		printf("Error for num: %s\n",strerror(errno));
	}

	if (str != NULL) {
		c8l_arrpush(str, "olá mundo");
		c8l_arrpush(str, "como vai?");
		c8l_arrpush(str, "terceira linha!");
		c8l_arrpush(str, "será que funfa?");
		c8l_arrpush(str, "será que funfas?");

		printf("%s | %s | %s | %s | Total: %lu\n",str[0],str[1],str[2],str[3],c8l_arrlen(str));

	} else {
		printf("Error for str: %s\n", strerror(errno));
	}

	return 0;
}


int main() {
	test_push();
	return 0;
}

/* vim: set fdm=indent fdn=1 ts=2 sts=2 sw=2: */
