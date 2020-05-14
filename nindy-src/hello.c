main()
{
int i, ovrhead, tot_time;

	ovrhead = init_bentime(20);
	tot_time = bentime();
	if (malloc (10) == 0)
		write (1, "nope on malloc\n", 14);
	for (i = 0; i < 10; i++)
		printf("NINDY monitor \"hello\" #%i\n", i);
	tot_time = bentime() - tot_time - ovrhead;
	printf ("Elapsed time = %ld uS\n", tot_time);
}

