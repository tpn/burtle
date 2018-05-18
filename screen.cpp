#include <stdio.h>
#include <stddef.h>
#include <windows.h>

//
// try to find an adequate long-message mixing function for SpookyHash
//

typedef  unsigned long long  u8;

#ifdef _MSC_VER
# define INLINE __forceinline
#else
# define INLINE inline
#endif

class U8Helper
{
public:
  // return x rotated left by k
  static INLINE u8 Rot64(u8 x, int k)
  {
    return (x << k) | (x >> (64-(k)));
  }

  // count how many bits are set in an 8-byte value
  static u8 Count(u8 x)
  {
    x = ((x & 0x5555555555555555LL) + ((x & 0xaaaaaaaaaaaaaaaaLL) >> 1));
    x = ((x & 0x3333333333333333LL) + ((x & 0xccccccccccccccccLL) >> 2));
    x = ((x & 0x0f0f0f0f0f0f0f0fLL) + ((x & 0xf0f0f0f0f0f0f0f0LL) >> 4));
    x = ((x & 0x00ff00ff00ff00ffLL) + ((x & 0xff00ff00ff00ff00LL) >> 8));
    x = ((x & 0x0000ffff0000ffffLL) + ((x & 0xffff0000ffff0000LL) >> 16));
    x = ((x & 0x00000000ffffffffLL) + ((x & 0xffffffff00000000LL) >> 32));
    return x;
  }
};


// random number generator
class Random : U8Helper
{ 
public:
    inline u8 Value()
    {
        u8 e = m_a - Rot64(m_b, 23);
        m_a = m_b ^ Rot64(m_c, 16);
        m_b = m_c + Rot64(m_d, 11);
        m_c = m_d + e;
        m_d = e + m_a;
        return m_d;
    }

    inline void Init( u8 seed)
    {
        m_a = 0xdeadbeef; 
        m_b = m_c = m_d = seed;
        for (int i=0; i<20; ++i) 
            (void)Value();
    }

private:
    u8 m_a; 
    u8 m_b; 
    u8 m_c; 
    u8 m_d;
};


// generate, test, and report mixing functions
class Sieve : U8Helper
{
public:
  Sieve(int seed, FILE *fp)
  {
    _r.Init(seed);
    _fp = fp;
  }

  ~Sieve()
  {
  }

  // generate a new function at random
  void Generate()
  {
    for (int iOp=0; iOp<_ops; ++iOp)
    {
      _op[iOp] = _r.Value() & 3;
      _v1[iOp] = _r.Value() % _vars;
      _v2[iOp] = _r.Value() % _vars;
    }
    _op[0] = 2;
    _v1[0] = 2;
    _v2[0] = 10;
    _op[2] = 2;
    _v1[1] = 11;
    _v2[1] = 0;
    _op[2] = 3;
    _v1[2] = 0;
    _v2[2] = 0;
    _v1[3] = 11;
    _v2[3] = 1;
    for (int iVar=0; iVar<_vars; ++iVar)
    {
      _s[iVar] = _s[iVar + _vars] = (_r.Value() & 63);
    }
  }

  int Test()
  {
    int minVal = _vars*64;
    for (int iVar=0; iVar<_vars; ++iVar)
    {
      for (int iMess=0; iMess<1 /* _vars */; ++iMess) // hack
      {
	int aVal = OneTest(0, iVar, iMess);
	if (aVal == 0)
        {
	  return 0;
	}
	if (aVal < minVal) minVal = aVal;
	aVal = OneTest(1, iVar, iMess);
	if (aVal == 0)
	{
	  return 0;
	}
	if (aVal < minVal) minVal = aVal;
      }
    }
    printf("// minVal = %d\n", minVal);
    return 1;
  }

  void Pre()
  {
    fprintf(_fp, "#include <stddef.h>\n");
    fprintf(_fp, "#include <stdio.h>\n");
    fprintf(_fp, "#include <windows.h>\n");
    fprintf(_fp, "\n");
    fprintf(_fp, "typedef  unsigned long long  u8;\n");
    fprintf(_fp, "typedef  unsigned long       u4;\n");
    fprintf(_fp, "typedef  char                u1;\n");
    fprintf(_fp, "\n");
    fprintf(_fp, "#define VAR %d\n", _vars);
    fprintf(_fp, "#define ITERS (100000000)\n");
    fprintf(_fp, "#define CUT 4000\n");
    fprintf(_fp, "#define Rot64(x,k) (((x)<<(k)) | ((x)>>(64-(k))))\n");
    fprintf(_fp, "\n");
  }

  // print the function in C++ code
  void ReportCode(int version)
  {
    fprintf(_fp, "void function%d(u8 *data, u8 *state)\n", version);
    fprintf(_fp, "{\n");
    
    for (int iVar=0; iVar<_vars; ++iVar)
    {
      fprintf(_fp, "    u8 s%d = state[%d];\n", iVar, iVar, iVar);
    }

    for (int iIter=0; iIter<_iters; ++iIter)
    {
      for (int iVar=0; iVar<_vars; ++iVar)
      {
	fprintf(_fp, "    s%d += data[%d];\n", iVar, iVar);
	for (int iOp=0; iOp<_ops; ++iOp)
	{
	  PrintOp(_fp, _op[iOp], 
		  (_v1[iOp] + iVar) % _vars, 
		  (_v2[iOp] + iVar) % _vars, 
		  _s[iVar]);
	}
      }
    }
    
    for (int iVar=0; iVar<_vars; ++iVar)
    {
      fprintf(_fp, "    state[%d] = s%d;\n", iVar, iVar);
    }

    fprintf(_fp, "}\n");
    fprintf(_fp, "\n");
    fprintf(_fp, "void wrapper%d(u8 *data, u8 *state)\n", version);
    fprintf(_fp, "{\n");
    fprintf(_fp, "  u8 a = GetTickCount();\n");
    fprintf(_fp, "  for (int i=0; i<ITERS; ++i) {\n");
    fprintf(_fp, "    function%d(data, state);\n", version);
    fprintf(_fp, "  }\n");
    fprintf(_fp, "  u8 z = GetTickCount();\n");
    fprintf(_fp, "  if (z-a < CUT) {\n");
    fprintf(_fp, "    printf(\"");
    ReportStructure(version);
    fprintf(_fp, "  %%lld\\n\", z-a);\n");
    fprintf(_fp, "  }\n");
    fprintf(_fp, "}\n");
    fprintf(_fp, "\n");
  }

  void ReportStructure(int version)
  {
    for (int iOp=0; iOp<_ops; ++iOp)
    {
      fprintf(_fp, "%1d %2d %2d ", _op[iOp], _v1[iOp], _v2[iOp]);
    }
    fprintf(_fp, " ");
    for (int iVar=0; iVar<_vars; ++iVar)
    {
      fprintf(_fp, "%2d ", _s[iVar]);
    }
  }

  void Post(int numFunctions)
  {
    int i;
    fprintf(_fp, "\n");
    fprintf(_fp, "int main(int argc, char **argv)\n");
    fprintf(_fp, "{\n");
    fprintf(_fp, "  u8 a, state[VAR], data[VAR];\n");
    fprintf(_fp, "  int i;\n");
    fprintf(_fp, "  for (int i=0; i<VAR; ++i) state[i] = data[i] = i+argc;\n");
    for (i=0; i<numFunctions; ++i)
    {
      fprintf(_fp, "  wrapper%d(data, state);\n", i);
    }
    fprintf(_fp, "}\n");
    fprintf(_fp, "\n");
  }


private:

  // print operation
  static void INLINE PrintOp(FILE *fp, int k, int x, int y, int s)
  {
    if (k == 0) fprintf(fp, "    s%d += s%d;\n", x, y);
    if (k == 1) fprintf(fp, "    s%d -= s%d;\n", x, y);
    if (k == 2) fprintf(fp, "    s%d ^= s%d;\n", x, y);
    if (k == 3) fprintf(fp, "    s%d = Rot64(s%d,%d);\n", x, x, s);
  }

  // evaluate operation
  static void INLINE Op(int k, u8 &x, u8 y, int s)
  {
    if (k == 0) x += y;
    else if (k == 1) x -= y;
    else if (k == 2) x ^= y;
    else if (k == 3) x = Rot64(x, s);
  }

  // evaluate operation in reverse
  static void INLINE ROp(int k, u8 &x, u8 y, int s)
  {
    if (k == 0) x -= y;
    else if (k == 1) x += y;
    else if (k == 2) x ^= y;
    else if (k == 3) x = Rot64(x, (64-s));
  }

  // evaluate the function
  void Fun(int *shifts, u8 *state, u8 *data)
  {
    for (int iIter=0; iIter < _iters; ++iIter)
    {
      for (int iVar=0; iVar <_vars; ++iVar)
      {
	state[iVar] += data[iVar];
	for (int iOp=0; iOp < _ops; ++iOp)
	{
	  Op(_op[iOp], 
	     state[(_v1[iOp] + iVar) % _vars], 
	     state[(_v2[iOp] + iVar) % _vars],
	     shifts[iVar]);
	}
      }
    }
  }

  // evaluate the function in reverse
  void RFun(int *shifts, u8 *state, u8 *data)
  {
    for (int iIter=_iters; iIter--;)
    {
      for (int iVar=_vars; iVar--;)
      {
	// the data is not being added symmetrically, but the goal is to test all deltas,
	// not test them in the reverse order that they were tested forwards.
	state[(iVar + 1) % _vars] -= data[_vars - iVar - 1];
	for (int iOp=_ops; iOp--;)
	{
	  ROp(_op[iOp], 
	      state[(_v1[iOp] + iVar) % _vars], 
	      state[(_v2[iOp] + iVar) % _vars], 
	      shifts[iVar]);
	}
      }
    }
  }

  void Eval(int forwards, int start, u8 *state, u8 *data)
  {
    if (forwards)
    {
      Fun(&_s[start], state, data);
    }
    else
    {
      RFun(&_s[start], state, data);
    }
  }

  int OneTest(int forwards, int start, int goose)
  {
    static const int _measures = 10;  // number of different ways of looking
    static const int _trials = 3;     // number of pairs of hashes
    static const int _limit =3*64;    // minimum number of bits affected
    u8 a[_measures][_vars];
    u8 zero[_vars];
    int minVal = _vars*64;

    for (int iZero = 0; iZero < _vars; ++iZero)
    {
      zero[iZero] = 0;
    }

    // iBit covers just key[0], because that is the variable we start at
    for (int iBit=0; iBit<64; ++iBit)
    {  
      for (int iBit2=iBit; iBit2<_vars*64; ++iBit2)
      {  
	u8 total[_measures][_vars];  // accumulated affect per bit
	for (int iMeasure=0; iMeasure<_measures; ++iMeasure)
	{
	  for (int iVar=0; iVar<_vars; ++iVar)
	  {
	    total[iMeasure][iVar] = 0;
	  }
	}

	for (int iTrial=0; iTrial<_trials; ++iTrial)
	{
	  // test one pair of inputs
	  u8 data[_vars];
	  for (int iVar=0; iVar<_vars; ++iVar)
	  {
	    u8 value = _r.Value();
	    // if (1 || iVar != goose) value = 0;  // hack
	    a[0][iVar] = value;  // input/output of first of pair
	    a[1][iVar] = value;  // input/output of second of pair
	    data[iVar] = 0;
	  }
	  
	  // evaluate first of pair
	  Eval(forwards, start, a[0], zero);
	  
	  // evaluate second of pair, differing in one bit
	  data[iBit/64] ^= (((u8)1) << (iBit & 63));
	  if (iBit2 != iBit)
	  {
	    data[iBit2/64] ^= (((u8)1) << (iBit2 & 63));
	  }
	  Eval(forwards, start, a[1], data);
	  
	  for (int iVar=0; iVar<_vars; ++iVar)
	  {
	    a[2][iVar] = a[0][iVar] ^ a[1][iVar];  // xor of first and second
	    a[3][iVar] = a[0][iVar] - a[1][iVar];
	    a[3][iVar] ^= a[3][iVar]>>1;   // "-" of first and second, graycoded
	    a[4][iVar] = a[0][iVar] + a[1][iVar];
	    a[4][iVar] ^= a[4][iVar]>>1;   // "+" of first and second, graycoded
	    a[5][iVar] = ~a[0][iVar];      // a[5..9] are complements of a[0..4]
	    a[6][iVar] = ~a[1][iVar];         
	    a[7][iVar] = ~a[2][iVar];
	    a[8][iVar] = ~a[3][iVar];
	    a[9][iVar] = ~a[4][iVar];
	  }
	  for (int iMeasure=0; iMeasure<_measures; ++iMeasure)
	  {
	    for (int iVar=0; iVar<_vars; ++iVar)
	    {
	      total[iMeasure][iVar] |= a[iMeasure][iVar];
	    }
	  }
	}
	for (int iMeasure=0; iMeasure<_measures; ++iMeasure)
	{
	  int counter = 0;
	  for (int iVar=0; iVar<_vars; ++iVar)
	  {
	    counter += Count(total[iMeasure][iVar]);
	  }
	  if (counter < _limit)
	  {
	    if (0)
	    {
	      printf("fail %d %d %d\n", iMeasure, iBit, counter);
	    }
	    return 0;
	  }
	  if (counter < minVal)
	  {
	    minVal = counter;
	  }
	}
      }
    }
    return minVal;
  }

  static const int _vars = 12;
  static const int _ops = 4;
  static const int _iters = 1;

  FILE *_fp;       // output file pointer
  Random _r;       // random number generator

  int _op[_ops];   // what type of operation (values in 0..3)
  int _v1[_ops];   // which variable first (values in 0..VAR-1)
  int _v2[_ops];   // which variable next (values in 0..VAR-1)
  int _s[2*_vars]; // shift constant (values 0..63)
};

  
void driver(u8 seed, FILE *fp, int numFunctions)
{
  Sieve sieve(seed, fp);

  sieve.Pre();

  int version = 0;
  while (version < numFunctions)
  {
    sieve.Generate();
    if (sieve.Test())
    {
      sieve.ReportStructure(version++);
      printf("\n");
    }
  }

  sieve.Post(numFunctions);
}

int main(int argc, char **argv)
{
  driver(21, stdout, 200);
}
