
/*

  This is example.c.  It shows an example of use of the ghist960
  commmon variable: __ghist_no_fs_func 

  This is for a target that does not support open() or write() (to a file).

  Perhaps it supports printf()...

  Here, instead of writing to a file, we'll sort the buckets then print to
  stderr.
*/

struct ip_count {
    unsigned long ip,hits;
};
    
static int cmp(left,right)
    struct ip_count *left,*right;
{
    return right->hits - left->hits;
}

static void print_out_ip(struct ip_count *ip) {
    if (ip->hits)
	    printf("ghist: %5d 0x%08x\n",ip->hits,ip->ip);
}

void __ghist_no_fs_func(int *bucket_ptr,int bucket_count,char *first_bucket,int bucket_size)
{
    int i,total_hits = 0;
    struct ip_count *ips = (struct ip_count *) malloc(bucket_count*sizeof(struct ip_count));

    printf("\n\nghist: Let's print out all non-zero buckets (unsorted):\n\n");
    for (i=0;i < bucket_count;i++) {
	ips[i].ip = ((unsigned long) first_bucket) + (i*bucket_size);
	total_hits += ips[i].hits = bucket_ptr[i];
	print_out_ip(ips+i);
    }

    printf("ghist: total hits were: %d\n",total_hits);

    /* Next, let's sort the buckets: */

    qsort(ips,bucket_count,sizeof(struct ip_count),cmp);

    /* Now, let's print the buckets out: */

    printf("\n\nghist: Let's print out the sorted buckets:\n\n");

    for (i=0;i < bucket_count && (1 || ips[i].hits);i++)
	    print_out_ip(ips+i);
}
