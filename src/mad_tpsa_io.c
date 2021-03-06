/*
 o-----------------------------------------------------------------------------o
 |
 | TPSA I/O module implementation
 |
 | Methodical Accelerator Design - Copyright CERN 2016+
 | Support: http://cern.ch/mad  - mad at cern.ch
 | Authors: L. Deniau, laurent.deniau at cern.ch
 |          C. Tomoiaga
 | Contrib: -
 |
 o-----------------------------------------------------------------------------o
 | You can redistribute this file and/or modify it under the terms of the GNU
 | General Public License GPLv3 (or later), as published by the Free Software
 | Foundation. This file is distributed in the hope that it will be useful, but
 | WITHOUT ANY WARRANTY OF ANY KIND. See http://gnu.org/licenses for details.
 o-----------------------------------------------------------------------------o
*/

#include <math.h>
#include <string.h>
#include <assert.h>

#include "mad_mem.h"
#include "mad_desc_impl.h"

#ifdef    MAD_CTPSA_IMPL
#include "mad_ctpsa_impl.h"
#define  SPC "                     "
#else
#include "mad_tpsa_impl.h"
#define  SPC
#endif

// --- LOCAL FUNCTIONS -------------------------------------------------------

static inline void
read_ords(int n, ord_t ords[n], FILE *stream)
{
  assert(ords && stream);
  for (int i = 0; i < n; ++i) {
    int ord_read = fscanf(stream, "%hhu", ords+i);
    ensure(ord_read == 1);
  }
}

static inline void
print_ords(int n, ord_t ords[n], FILE *stream)
{
  assert(ords && stream);
  for (int i = 0; i+1 < n; i += 2)
    fprintf(stream, "  %hhu %hhu", ords[i], ords[i+1]);
  if (n % 2)
    fprintf(stream, "  %hhu"     , ords[n-1]);
}

static inline char**
scan_var_names(FILE *stream, int nmv, char *buf)
{
  assert(stream);
  // try 4th line
  int read_cnt = fscanf(stream, " MAP %s", buf);
  if (!read_cnt)
    return NULL;

  char **var_names = mad_malloc(nmv * sizeof(*var_names));
  assert(var_names);
  for (int i = 0; i < nmv; ++i) {
    read_cnt = fscanf(stream, "%s", buf);
    ensure(read_cnt && !feof(stream) && !ferror(stream));
    var_names[i] = mad_malloc(strlen(buf) * sizeof(var_names));
    strcpy(var_names[i],buf);
  }
  return var_names;
}

// --- PUBLIC FUNCTIONS -------------------------------------------------------

#ifdef MAD_CTPSA_IMPL

extern D* mad_tpsa_scan_hdr(FILE*);

D*
FUN(scan_hdr) (FILE *stream_)
{
  return mad_tpsa_scan_hdr(stream_);
}

#else

D*
FUN(scan_hdr) (FILE *stream_)
{
  if (!stream_) stream_ = stdin;

  const int BUF_SIZE = 100;
  char buf[BUF_SIZE];
  ord_t mo, ko;
  int nmv, nk = 0, read_cnt = 0;
  D *d = NULL;

  // discard leading white space and the name (which is 10 chars and comma)
  read_cnt = fscanf(stream_, " %*11c");

  // 1st line
  read_cnt = fscanf(stream_, " NO =%5hhu, NV =%5d, KO =%5hhu, NK =%5d",
                    &mo, &nmv, &ko, &nk);

  // rest of lines
  ord_t var_ords[nmv];
  if (read_cnt == 4) {                      // GTPSA -- process rest of lines
    ord_t map_ords[nmv], knb_ords[nk ? nk : 1];
    read_cnt = fscanf(stream_, " MAP ORDS: ");
    ensure(!feof(stream_) && !ferror(stream_));
    read_ords(nmv,map_ords,stream_);

    read_cnt = fscanf(stream_, " ||| VAR ORDS: ");
    ensure(!feof(stream_) && !ferror(stream_));
    read_ords(nmv,var_ords,stream_);
    read_ords(nk, knb_ords,stream_);

    char **var_names = scan_var_names(stream_,nmv,buf);
    d = mad_desc_newk(nmv,var_ords,map_ords,(str_t*)var_names,nk,knb_ords,ko);
    if (var_names) {
      for (int i = 0; i < nmv; ++i)
        mad_free(var_names[i]);
      mad_free(var_names);
    }
    ensure(fgets(buf, BUF_SIZE, stream_));  // finish  3rd line
  }
  else if (read_cnt == 2) {                 // TPSA  -- ignore  rest of lines; default values
    mad_mono_fill(nmv,var_ords,mo);
    ensure(fgets(buf, BUF_SIZE, stream_));  // finish  1st line
    ensure(fgets(buf, BUF_SIZE, stream_));  // discard 2nd line
    ensure(fgets(buf, BUF_SIZE, stream_));  // discard 3rd line
    d = mad_desc_new(nmv,var_ords,NULL,NULL);
  }
  else if (read_cnt < 2)
    printf("ERROR WHILE READING TPSA HEADER: Could not read (NO,NV)\n");
  else if (read_cnt == 3)
    printf("ERROR WHILE READING TPSA HEADER: Could not read NK\n");

  ensure(fgets(buf, BUF_SIZE, stream_)); // discard coeff header line

  return d;
}

#endif // !MAD_CTPSA_IMPL

void
FUN(scan_coef) (T *t, FILE *stream_)
{
  assert(t);
  if (!stream_) stream_ = stdin;

  NUM c;
  int nv = t->d->nv, read_cnt = -1;
  ord_t o, ords[nv];
  FUN(clear)(t);

#ifndef MAD_CTPSA_IMPL
  while ((read_cnt = fscanf(stream_, "%*d %lG %hhu", &c, &o)) == 2) {
#else
  while ((read_cnt = fscanf(stream_, "%*d %lG%lGi %hhu", (num_t*)&c, (num_t*)&c+1, &o)) == 2) {
#endif 

    #ifdef DEBUG
      printf("c=%.2f o=%d\n", c, o);
    #endif
    read_ords(nv,ords,stream_);
    ensure(mad_mono_ord(nv,ords) == o);  // consistency check
    if (o > t->mo)  // TODO: give warning ?
      break;        // printed by increasing orders
    FUN(setm)(t,nv,ords, 0.0,c);
  }
}

T*
FUN(scan) (FILE *stream_)
{
  // TODO
  (void)stream_;
  ensure(!"NOT YET IMPLEMENTED");
}

void
FUN(print) (const T *t, str_t name_, FILE *stream_)
{
  assert(t);
  // TODO: print map vars and name

  if (!stream_) stream_ = stdout;
  D *d = t->d;

  // print header
  if (!name_) name_ = "-UNNAMED--";
  fprintf(stream_, "\n %10s, NO =%5hhu, NV =%5d, KO =%5hhu, NK =%5d\n MAP ORDS:",
          name_,d->mo,d->nmv,d->ko, d->nv - d->nmv);
  print_ords(d->nmv,d->map_ords,stream_);
  fprintf(stream_, " ||| VAR ORDS: ");
  print_ords(d->nv,d->var_ords,stream_);
  if (d->var_names_) {
    fprintf(stream_, "\n MAP NAME: ");
    for (int i = 0; i < d->nmv; ++i)
      fprintf(stream_, "%s ", d->var_names_[i]);
  }
  else
    fprintf(stream_, "\n *******************************************************");

  if (!t->nz) {
    fprintf(stream_, "\n   ALL COMPONENTS ZERO \n");
    return;
  }

  fprintf(stream_, "\n    I  COEFFICIENT         " SPC " ORDER   EXPONENTS");
  int idx = 1;
  for (int c = 0; c < d->nc; ++c)
    if (mad_bit_get(t->nz,d->ords[c]) && fabs(t->coef[c]) > 1e-10) {

#ifndef MAD_CTPSA_IMPL
      fprintf(stream_, "\n%6d  %21.14lE%5hhu   "          , idx, VAL(t->coef[c]), d->ords[c]);
#else
      fprintf(stream_, "\n%6d  %21.14lE%+21.14lEi%5hhu   ", idx, VAL(t->coef[c]), d->ords[c]);      
#endif

      print_ords(d->nv, d->To[c], stream_);
      idx++;
    }
  fprintf(stream_, "\n\n");
}
