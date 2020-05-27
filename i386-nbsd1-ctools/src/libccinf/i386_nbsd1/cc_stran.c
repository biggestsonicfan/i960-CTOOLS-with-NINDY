/* Analyze file differences for GNU DIFF.
   Copyright (C) 1988, 1989, 1992, 1993 Free Software Foundation, Inc.

This file is part of GNU DIFF.

GNU DIFF is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU DIFF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU DIFF; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

  This source was originally part of GNU DIFF, it has been modified
  to do what I needed.  Kevin B. Smith 1/31/95.

*/

#define const 
#define INT_MAX 0x7fffffff
#define heuristic 1
#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

/* The basic algorithm is described in:
   "An O(ND) Difference Algorithm and its Variations", Eugene Myers,
   Algorithmica Vol. 1 No. 2, 1986, pp. 251-266;
   see especially section 4.2, which describes the variation used below.
   Unless the --minimal option is specified, this code uses the TOO_EXPENSIVE
   heuristic, by Paul Eggert, to limit the cost to O(N**1.5 log N)
   at the price of producing suboptimal output for large inputs with
   many differences.

   The basic algorithm was independently discovered as described in:
   "Algorithms for Approximate String Matching", E. Ukkonen,
   Information and Control Vol. 64, 1985, pp. 100-118.  */

static int *xvec, *yvec;        /* Vectors being compared. */
static int *fdiag;              /* Vector, indexed by diagonal, containing
                                   1 + the X coordinate of the point furthest
                                   along the given diagonal in the forward
                                   search of the edit matrix. */
static int *bdiag;              /* Vector, indexed by diagonal, containing
                                   the X coordinate of the point furthest
                                   along the given diagonal in the backward
                                   search of the edit matrix. */

int too_expensive = 256;

unsigned char *changed1_flag;
unsigned char *changed0_flag;

#define SNAKE_LIMIT 20	/* Snakes bigger than this are considered `big'.  */

struct partition
{
  int xmid, ymid;	/* Midpoints of this partition.  */
  int lo_minimal;	/* Nonzero if low half will be analyzed minimally.  */
  int hi_minimal;	/* Likewise for high half.  */
};

static int diag ();
static void compareseq ();

/* Find the midpoint of the shortest edit script for a specified
   portion of the two files.

   Scan from the beginnings of the files, and simultaneously from the ends,
   doing a breadth-first search through the space of edit-sequence.
   When the two searches meet, we have found the midpoint of the shortest
   edit sequence.

   If MINIMAL is nonzero, find the minimal edit script regardless
   of expense.  Otherwise, if the search is too expensive, use
   heuristics to stop the search and report a suboptimal answer.

   Set PART->(XMID,YMID) to the midpoint (XMID,YMID).  The diagonal number
   XMID - YMID equals the number of inserted lines minus the number
   of deleted lines (counting only lines before the midpoint).
   Return the approximate edit cost; this is the total number of
   lines inserted or deleted (counting only lines before the midpoint),
   unless a heuristic is used to terminate the search prematurely.

   Set PART->LEFT_MINIMAL to nonzero iff the minimal edit script for the
   left half of the partition is known; similarly for PART->RIGHT_MINIMAL.

   This function assumes that the first lines of the specified portions
   of the two files do not match, and likewise that the last lines do not
   match.  The caller must trim matching lines from the beginning and end
   of the portions it is going to specify.

   If we return the "wrong" partitions,
   the worst this can do is cause suboptimal diff output.
   It cannot cause incorrect diff output.  */

static int
diag (xoff, xlim, yoff, ylim, minimal, part)
     int xoff, xlim, yoff, ylim, minimal;
     struct partition *part;
{
  int *const fd = fdiag;	/* Give the compiler a chance. */
  int *const bd = bdiag;	/* Additional help for the compiler. */
  int const *const xv = xvec;	/* Still more help for the compiler. */
  int const *const yv = yvec;	/* And more and more . . . */
  int const dmin = xoff - ylim;	/* Minimum valid diagonal. */
  int const dmax = xlim - yoff;	/* Maximum valid diagonal. */
  int const fmid = xoff - yoff;	/* Center diagonal of top-down search. */
  int const bmid = xlim - ylim;	/* Center diagonal of bottom-up search. */
  int fmin = fmid, fmax = fmid;	/* Limits of top-down search. */
  int bmin = bmid, bmax = bmid;	/* Limits of bottom-up search. */
  int c;			/* Cost. */
  int odd = (fmid - bmid) & 1;	/* True if southeast corner is on an odd
				   diagonal with respect to the northwest. */

  fd[fmid] = xoff;
  bd[bmid] = xlim;

  for (c = 1;; ++c)
    {
      int d;			/* Active diagonal. */
      int big_snake = 0;

      /* Extend the top-down search by an edit step in each diagonal. */
      fmin > dmin ? fd[--fmin - 1] = -1 : ++fmin;
      fmax < dmax ? fd[++fmax + 1] = -1 : --fmax;
      for (d = fmax; d >= fmin; d -= 2)
	{
	  int x, y, oldx, tlo = fd[d - 1], thi = fd[d + 1];

	  if (tlo >= thi)
	    x = tlo + 1;
	  else
	    x = thi;
	  oldx = x;
	  y = x - d;
	  while (x < xlim && y < ylim && xv[x] == yv[y])
	    ++x, ++y;
	  if (x - oldx > SNAKE_LIMIT)
	    big_snake = 1;
	  fd[d] = x;
	  if (odd && bmin <= d && d <= bmax && bd[d] <= x)
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}

      /* Similarly extend the bottom-up search.  */
      bmin > dmin ? bd[--bmin - 1] = INT_MAX : ++bmin;
      bmax < dmax ? bd[++bmax + 1] = INT_MAX : --bmax;
      for (d = bmax; d >= bmin; d -= 2)
	{
	  int x, y, oldx, tlo = bd[d - 1], thi = bd[d + 1];

	  if (tlo < thi)
	    x = tlo;
	  else
	    x = thi - 1;
	  oldx = x;
	  y = x - d;
	  while (x > xoff && y > yoff && xv[x - 1] == yv[y - 1])
	    --x, --y;
	  if (oldx - x > SNAKE_LIMIT)
	    big_snake = 1;
	  bd[d] = x;
	  if (!odd && fmin <= d && d <= fmax && x <= fd[d])
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c;
	    }
	}

      if (minimal)
	continue;

      /* Heuristic: check occasionally for a diagonal that has made
	 lots of progress compared with the edit distance.
	 If we have any such, find the one that has made the most
	 progress and return it as if it had succeeded.

	 With this heuristic, for files with a constant small density
	 of changes, the algorithm is linear in the file size.  */

      if (c > 200 && big_snake && heuristic)
	{
	  int best;

	  best = 0;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      int dd = d - fmid;
	      int x = fd[d];
	      int y = x - d;
	      int v = (x - xoff) * 2 - dd;
	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if (v > best
		      && xoff + SNAKE_LIMIT <= x && x < xlim
		      && yoff + SNAKE_LIMIT <= y && y < ylim)
		    {
		      /* We have a good enough best diagonal;
			 now insist that it end with a significant snake.  */
		      int k;

		      for (k = 1; xv[x - k] == yv[y - k]; k++)
			if (k == SNAKE_LIMIT)
			  {
			    best = v;
			    part->xmid = x;
			    part->ymid = y;
			    break;
			  }
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	      return 2 * c - 1;
	    }

	  best = 0;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      int dd = d - bmid;
	      int x = bd[d];
	      int y = x - d;
	      int v = (xlim - x) * 2 + dd;
	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if (v > best
		      && xoff < x && x <= xlim - SNAKE_LIMIT
		      && yoff < y && y <= ylim - SNAKE_LIMIT)
		    {
		      /* We have a good enough best diagonal;
			 now insist that it end with a significant snake.  */
		      int k;

		      for (k = 0; xv[x + k] == yv[y + k]; k++)
			if (k == SNAKE_LIMIT - 1)
			  {
			    best = v;
			    part->xmid = x;
			    part->ymid = y;
			    break;
			  }
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}

      /* Heuristic: if we've gone well beyond the call of duty,
	 give up and report halfway between our best results so far.  */
      if (c >= too_expensive)
	{
	  int fxybest, fxbest;
	  int bxybest, bxbest;

	  fxbest = bxbest = 0;  /* Pacify `gcc -Wall'.  */

	  /* Find forward diagonal that maximizes X + Y.  */
	  fxybest = -1;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      int x = min (fd[d], xlim);
	      int y = x - d;
	      if (ylim < y)
		x = ylim + d, y = ylim;
	      if (fxybest < x + y)
		{
		  fxybest = x + y;
		  fxbest = x;
		}
	    }

	  /* Find backward diagonal that minimizes X + Y.  */
	  bxybest = INT_MAX;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      int x = max (xoff, bd[d]);
	      int y = x - d;
	      if (y < yoff)
		x = yoff + d, y = yoff;
	      if (x + y < bxybest)
		{
		  bxybest = x + y;
		  bxbest = x;
		}
	    }

	  /* Use the better of the two diagonals.  */
	  if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff))
	    {
	      part->xmid = fxbest;
	      part->ymid = fxybest - fxbest;
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	    }
	  else
	    {
	      part->xmid = bxbest;
	      part->ymid = bxybest - bxbest;
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	    }
	  return 2 * c - 1;
	}
    }
}

/* Compare in detail contiguous subsequences of the two files
   which are known, as a whole, to match each other.

   The results are recorded in the vectors files[N].changed_flag, by
   storing a 1 in the element for each line that is an insertion or deletion.

   The subsequence of file 0 is [XOFF, XLIM) and likewise for file 1.

   Note that XLIM, YLIM are exclusive bounds.
   All line numbers are origin-0 and discarded lines are not counted.
 
   If MINIMAL is nonzero, find a minimal difference no matter how
   expensive it is.  */

static void
compareseq (xoff, xlim, yoff, ylim, minimal)
     int xoff, xlim, yoff, ylim, minimal;
{
  int * const xv = xvec; /* Help the compiler.  */
  int * const yv = yvec;

  /* Slide down the bottom initial diagonal. */
  while (xoff < xlim && yoff < ylim && xv[xoff] == yv[yoff])
    ++xoff, ++yoff;
  /* Slide up the top initial diagonal. */
  while (xlim > xoff && ylim > yoff && xv[xlim - 1] == yv[ylim - 1])
    --xlim, --ylim;

  /* Handle simple cases. */
  if (xoff == xlim)
    while (yoff < ylim)
      changed1_flag[yoff++] = 1;
  else if (yoff == ylim)
    while (xoff < xlim)
      changed0_flag[xoff++] = 1;
  else
    {
      int c;
      struct partition part;

      /* Find a point of correspondence in the middle of the files.  */

      c = diag (xoff, xlim, yoff, ylim, minimal, &part);

      if (c == 1)
	{
	  /* This should be impossible, because it implies that
	     one of the two subsequences is empty,
	     and that case was handled above without calling `diag'.
	     Let's verify that this is true.  */
	  abort ();
	}
      else
	{
	  /* Use the partitions to split this problem into subproblems.  */
	  compareseq (xoff, part.xmid, yoff, part.ymid, part.lo_minimal);
	  compareseq (part.xmid, xlim, part.ymid, ylim, part.hi_minimal);
	}
    }
}

void
pf_vdiff (v1, v2, c1, c2, l1, l2)
int *v1;
int *v2;
unsigned char *c1;
unsigned char *c2;
int l1;
int l2;
{
  int i;
  int *ip;

  for (i = 0; i < l1; i++)
    c1[i] = 0;

  for (i = 0; i < l2; i++)
    c2[i] = 0;

  xvec = v1;
  yvec = v2;
  changed0_flag = c1;
  changed1_flag = c2;

  ip = (int *)malloc((l1 + l2 + 3)*(2 * sizeof(int)));
  fdiag = ip;
  bdiag = ip + (l1 + l2 + 3);
  fdiag += l2 + 1;
  bdiag += l2 + 1;
  
  compareseq(0, l1, 0, l2, 0);

  free(ip);
}
