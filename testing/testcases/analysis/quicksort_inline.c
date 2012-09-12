/*
 * Quicksort as given at http://alienryderflex.com/quicksort/
 */
int quickSort(int *arr, int elements) {

  #define  MAX_LEVELS  1000

  int  piv, beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R ;

  beg[0]=0; end[0]=elements;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L]; if (i==MAX_LEVELS-1) return 0;
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L; }
    else {
      i--; }}
  return 1;
}


int array[NUM];

int main()
{
//#define NUM 100
	int i;
	for (i = NUM; i > 0; --i) {
		array[NUM-i] = i;
	}
	quickSort(array, NUM);

	return 0;
}
