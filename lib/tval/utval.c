// ---------------------------------------------------------------------------------------------------------------------
// unit tests
// compile: gcc -std=c99 -W -Wall -Wextra -pedantic -Ofast utval.c -o utval
// usage: ./utval [noperf]

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tval.h"

typedef bool (cmp_t)(val_t, val_t);

static inline bool
less_than (val_t a, val_t b)
{
  num_t aa = numtv(a);
  num_t bb = numtv(b);
  return aa < bb;
}

static inline bool
iless_than (val_t a, val_t b)
{
  i64_t aa = inttv(a);
  i64_t bb = inttv(b);
  return aa < bb;
}

static inline bool
rless_than (val_t a, val_t b)
{
  num_t aa = numtvr(a);
  num_t bb = numtvr(b);
  return aa < bb;
}

#if 1 // 0 or 1 selects the algorithm
/*
bfind(num):   71286406 iter/sec (0.70 sec)
bfind(int):   92144497 iter/sec (0.54 sec)
bfind(ref):   76279629 iter/sec (0.66 sec)
*/
static int
bfind (const val_t *arr, int n, val_t val, cmp_t *cmp)
{
  assert(arr && cmp);
  int low = 0, cnt = n;
  while (cnt > 0) {
    int stp = cnt >> 1;
    int mid = low+stp;
    if (cmp(arr[mid], val))
      low = mid+1, cnt -= stp+1;
    else
      cnt = stp;
  }
  return low; // tbl[low] <= val
}
#else
/*
bfind(num):   74207428 iter/sec (0.67 sec)
bfind(int):   85231147 iter/sec (0.59 sec)
bfind(ref):   74047746 iter/sec (0.68 sec)
*/
static int
bfind (const val_t *arr, int n, val_t val, cmp_t *cmp)
{
  assert(arr && cmp);
  int min=0, max=n;
  while (min < max) {
    int mid = min+((max-min)>>1);
    int tst = cmp(arr[mid],val);
    if (tst) min = mid+1; else max = mid;
  }
  return min;
}
#endif

static void
prttv (val_t v, const str_t *s_)
{
  printf("\n--- '%s'\n", s_ ? s_ : "");
  printf("typ: %d : %s\n"  , typtv(v), namtv(v));
  printf("hex: 0x%016llX\n", bittv(v));

  if (tvisnul(v)) printf("val: nul\n");
  if (tvisnan(v)) printf("val: nan\n");
  if (tvisnil(v)) printf("val: nil\n");
  if (tvislog(v)) printf("log: %s\n"  , logtv(v) ? "true" : "false");
  if (tvisint(v)) printf("int: %lld\n", inttv(v));
  if (tvisnum(v)) printf("num: %g\n"  , numtv(v));
  if (tvisins(v)) printf("ins: %llu\n", instv(v));
  if (tvisfun(v)) printf("fun: %p\n"  , hextv(v));
  if (tvisptr(v)) printf("ptr: %p\n"  , ptrtv(v));
  if (tvisstr(v)) printf("str: %p\n"  , ptrtv(v));
  if (tvisarr(v)) printf("arr: %p\n"  , ptrtv(v));
  if (tvisobj(v)) printf("obj: %p\n"  , ptrtv(v));
  if (tvisref(v)) {
                  printf("ref: %p\n"  , ptrtv(v));
    v = tvget(v); printf("typ: %d\n"  , typtv(v));
    if (tvisint(v))
                  printf("val: %lld\n", inttv(v));
  }
}

static void
checktv (void)
{
  const num_t inf = 1.0/0.0;
  const num_t nan = 0.0/0.0;
  val_t  v;

  printf("\n** constants **\n");

  v = tvnul();                                   prttv(v, "nul");
  v = tvnan();                                   prttv(v, "nan");
  v = tvnil();                                   prttv(v, "nil");
  v = tvtrue();                                  prttv(v, "true");
  v = tvfalse();                                 prttv(v, "false");

  printf("\n** values **\n");

  v = tvlog(  0);                                prttv(v, "0l");
  v = tvlog(  1);                                prttv(v, "1l");
  v = tvlog( -1);                                prttv(v, "-1l");

  v = tvint(  0);                                prttv(v, "0i");
  v = tvint( 10);                                prttv(v, "10i");
  v = tvint(-10);                                prttv(v, "-10i");

  v = tvint( (1LL<<44)-1);                       prttv(v, "2^44-1i");
  v = tvint(-(1LL<<44)+1);                       prttv(v, "-2^44+1i");
  v = tvint( (1LL<<45)-1);                       prttv(v, "2^45-1i");
  v = tvint(-(1LL<<45)+1);                       prttv(v, "-2^45+1i");
  v = tvint( (1LL<<46)-1);                       prttv(v, "2^46-1i");
  v = tvint(-(1LL<<46)+1);                       prttv(v, "-2^46+1i");

  v = tvins(  0);                                prttv(v, "0ins");
  v = tvins( 10);                                prttv(v, "10ins");
  v = tvins(-10);                                prttv(v, "-10ins");

  printf("\n** numbers **\n");

  v = tvnum(  0.);                               prttv(v, "0d");
  v = tvnum( -0.);                               prttv(v, "-0d");
  v = tvnum( 10.);                               prttv(v, "10d");
  v = tvnum(-10.);                               prttv(v, "-10d");

  v = tvnum( inf);                               prttv(v, "+inf");
  v = tvnum(-inf);                               prttv(v, "-inf");
  v.__u = DEF_NINF;                              prttv(v, "+inf");
  v.__d = -v.__d;                                prttv(v, "-inf");

  v = tvnum(nan);                                prttv(v, "nan");
  v = tvnum(nan*nan);                            prttv(v, "nan^2");
  v.__u = 0x7FF8000000000000ULL;                 prttv(v, "nnan");
  v.__u = 0xFFF8000000000000ULL;                 prttv(v, "tnan");

  printf("\n** pointers **\n");

  char c1, c2;
  fun_t* f = (fun_t*)checktv;
  void *p1 = &c1, *p2 = &c2;

  v = tvfun(0);                                  prttv(v, "&f");
  v = tvfun(f);                                  prttv(v, "&f");
  v = tvptr(0);                                  prttv(v, "null");
  v = tvptr(&v);                                 prttv(v, "&v");
  v = tvptr(p1);                                 prttv(v, "p1");
  v = tvptr(p2);                                 prttv(v, "p2");
  v = tvstr(&c1);                                prttv(v, "&c1");
  v = tvstr(&c2);                                prttv(v, "&c2");
  v = tvarr(p1);                                 prttv(v, "a1");
  v = tvarr(p2);                                 prttv(v, "a2");
  v = tvobj(p1);                                 prttv(v, "o1");
  v = tvobj(p2);                                 prttv(v, "o2");

  printf("\n** references **\n");

  val_t  va[4] = { tvint(100), tvint(101), tvint(102), tvint(103) };
  val_t *vp[4] = { va+0, va+1, va+2, va+3 };

  v = tvref(vp[0]);                              prttv(v, "&v1");
  v = tvref(vp[1]);                              prttv(v, "&v2");
  v = tvref(vp[2]);                              prttv(v, "&v3");
  v = tvref(vp[3]);                              prttv(v, "&v4");

  for (int i=1; i<4; i++) va[i] = tvref(&va[i-1]); // chain va's

  v = tvref(vp[0]);                              prttv(v, "&r1");
  v = tvref(vp[1]);                              prttv(v, "&r2");
  v = tvref(vp[2]);                              prttv(v, "&r3");
  v = tvref(vp[3]);                              prttv(v, "&r4");
}

static void
perftv (void)
{
  printf("\n** performance (conversions) **\n\n");

  enum { N = 1000000000, L=10 };
  clock_t t0, t1;
  num_t dt;

  // check for compiler optimization (~0.8 sec)
  t0 = clock();
  for (i64_t i=0; i<N; i++)
    assert(inttv(tvint(i)) == i);
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("int->tv->int: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  // check for compiler optimization (~0.8 sec)
  t0 = clock();
  for (u64_t i=0; i<N; i++)
    assert(instv(tvins(i)) == i);
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("ins->tv->ins: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  // check for compiler optimization (~0 sec)
  t0 = clock();
  for (i64_t i=0; i<N; i++)
    assert(numtv(tvnum(i)) == i);
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("num->tv->num: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  // check for compiler optimization (~0.8 sec)
  t0 = clock();
  for (str_t *s=0; s < (str_t*)N; s++)
    assert(strtv(tvstr(s)) == s);
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("str->tv->str: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  // check for compiler optimization (~1.2 sec)
  val_t  va[4] = { tvint(100), tvint(101), tvint(102), tvint(103) };
  val_t *vp[4] = { va+0, va+1, va+2, va+3 };

  t0 = clock();
  for (i64_t i=0; i<N/4; i++)
    assert(inttvr(tvref(vp[i & 3])) == 100 + (i&3));
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("ref->tv->int: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  // check for compiler optimization (~1.2 sec)
  for (int i=1; i<4; i++) va[i] = tvref(&va[i-1]); // chain va's
  t0 = clock();
  for (i64_t i=0; i<N/4; i++)
    assert(inttvr(tvref(vp[i & 3])) == 100);
  t1 = clock();
  dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
  if (N/dt > 1e100) dt = 0;
  printf("ref->..->int: %*.f iter/sec (%.2f sec)\n", L, N/dt, dt);

  printf("\n** performance (bfind) **\n\n");

  {
    enum { an=8, AL=20 };
    int idx[an] = {0,1,1,1,4,4,4,7};

    val_t arr[an] = { tvnum( 5),tvnum(10),tvnum(10),tvnum(10),
                      tvnum(20),tvnum(20),tvnum(20),tvnum(30) };
    t0 = clock();
    for (i64_t i=0; i<N/AL; i++) // 50000000
      assert(bfind(arr, an, arr[i&(an-1)], less_than) == idx[i&(an-1)]);
    t1 = clock();
    dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
    if (N/AL/dt > 1e100) dt = 0;
    printf("bfind(num): %*.f iter/sec (%.2f sec)\n", L, N/AL/dt, dt);

    val_t iarr[an] = { tvint( 5),tvint(10),tvint(10),tvint(10),
                       tvint(20),tvint(20),tvint(20),tvint(30) };
    t0 = clock();
    for (i64_t i=0; i<N/AL; i++) // 50000000
      assert(bfind(iarr, an, iarr[i&(an-1)], iless_than) == idx[i&(an-1)]);
    t1 = clock();
    dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
    if (N/AL/dt > 1e100) dt = 0;
    printf("bfind(int): %*.f iter/sec (%.2f sec)\n", L, N/AL/dt, dt);

    val_t rarr[an] = { tvref(arr+0),tvref(arr+1),tvref(arr+2),tvref(arr+3),
                       tvref(arr+4),tvref(arr+5),tvref(arr+6),tvref(arr+7) };
    t0 = clock();
    for (i64_t i=0; i<N/AL; i++) // 50000000
      assert(bfind(rarr, an, arr[i&(an-1)], rless_than) == idx[i&(an-1)]);
    t1 = clock();
    dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
    if (N/AL/dt > 1e100) dt = 0;
    printf("bfind(ref): %*.f iter/sec (%.2f sec)\n", L, N/AL/dt, dt);
  }
  {
    enum { an=9, AL=20 };
    int idx[an] = { 0,1,1,1,4,5,5,5,8 };

    val_t iarr[an] = { tvint( 5),tvint(10),tvint(10),tvint(10),tvint(20),
                       tvint(30),tvint(30),tvint(30),tvint(40) };
    t0 = clock();
    for (i64_t i=0; i<N/AL; i++) // 50000000
      assert(bfind(iarr, an, iarr[i%9], iless_than) == idx[i%9]);
    t1 = clock();
    dt = (num_t)(t1-t0)/CLOCKS_PER_SEC;
    if (N/AL/dt > 1e100) dt = 0;
    printf("bfind(int): %*.f iter/sec (%.2f sec)\n", L, N/AL/dt, dt);

    assert(bfind(iarr, an, tvint( 0), iless_than) == 0);
    assert(bfind(iarr, an, tvint(41), iless_than) == 9);
  }
}

/*
  Intel i5 2.3 GHz 2 cores (early 2011)

  gcc (MacPorts gcc7 7.2.0)
  ** performance (conversions) **
  int->tv->int: 1339559606 iter/sec (0.75 sec)
  ins->tv->ins:        inf iter/sec (0.00 sec)
  num->tv->num:        inf iter/sec (0.00 sec)
  str->tv->str:        inf iter/sec (0.00 sec)
  ref->tv->int: 1532292294 iter/sec (0.65 sec)
  ref->..->int:  748938567 iter/sec (1.34 sec)

  gcc (MacPorts gcc48 4.8.5_2)
  ** performance (conversions) **
  int->tv->int:  856162185 iter/sec (1.17 sec)
  ins->tv->ins:        inf iter/sec (0.00 sec)
  num->tv->num:        inf iter/sec (0.00 sec)
  str->tv->str: 1173075570 iter/sec (0.85 sec)
  ref->tv->int: 1253016638 iter/sec (0.80 sec)
  ref->..->int:  875523344 iter/sec (1.14 sec)
*/

int main(int argc, char *argv[])
{
  if (argc == 1) {
    checktv(); perftv();
  } else
  for (int n=1; n < argc; n++) {
    if (!strcmp(argv[n], "check")) checktv();
    if (!strcmp(argv[n], "perf" )) perftv();
  }
  return 0;
}
