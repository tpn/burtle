// Humans have 23 chromosomes, which come in pairs.
// During conception, each chromosome crosses over at 2 random points,
// so you get the middle from one parent and the ends from the other.
// With the exception of the sex (XY) chromosome in males:
// the whole X comes from the mom, whole Y from the dad.
// The 23 chromosomes shuffle, so you get the ends of one from dad, ends of another from mom.
//
// Simulate how many ancestors n generations back contributed to a final person, working backwards.
// Although there are 2^^n ancestors, the number actually contributing genetic material is O(n).

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include <stack>
#include <vector>
#include <map>

typedef  unsigned char       u1;  // 1 byte unsigned
typedef  unsigned short int  u2;  // 2 byte unsigned
typedef  unsigned long int   u4;  // 4 byte unsigned
typedef  unsigned long long  u8;  // 8 byte unsigned

static u8 totalMeiosis = 0;
static u8 totalCrossovers = 0;

#ifndef NULL
# define NULL 0
#endif

#ifdef _MSC_VER
# define INLINE __forceinline
#else
# define INLINE inline
#endif

using namespace std;

#define ASSERT(...) LogAssert(__LINE__, __VA_ARGS__)

static void LogAssert(int lineNum, bool condition, const char *fmt, ...)
{	
    if (!condition)
    {	
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf(": %d\n", lineNum);
        exit(1);
    }
}

class U8Helper
{
public:
    // return x rotated left by k
    static INLINE u8 Rot64(u8 x, int k)
    {
        return (x << k) | (x >> (64-(k)));
    }
};


// random number generator
class Random : U8Helper
{ 
public:
    inline u8 Value()
    {
        u8 e = m_a - Rot64(m_b, 7);
        m_a = m_b ^ Rot64(m_c, 13);
        m_b = m_c + Rot64(m_d, 37);
        m_c = m_d + e;
        m_d = e + m_a;
        return m_d;
    }
    
    inline void Init( u8 seed)
    {
        m_a = 0xdeadbeefdeadbeefLL; 
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


extern "C" void ShowMap(const map<u8, u8>& m)
{
    map<u8, u8>::const_iterator iter = m.begin();
    while (iter != m.end())
    {
        printf("%llx, %llx\n", iter->first, iter->second);
        iter++;
    }
}


// a chromosome pair 
class CPair
{
public:
    
    void Clear(int id)
    {
        _id = id;
        _male.clear();
        _female.clear();
    }
    
    void SetToRoot(int id)
    {
        Clear(id);
        _male[0] = ~(u8)0;
        _female[0] = ~(u8)0;
    }

    static void CheckEmpty(map<u8, u8>* m, u8 start, u8 nextToLast)
    {
        // this requires the destination to be empty for this range
        map<u8, u8>::iterator iter = m->upper_bound(nextToLast);
        map<u8, u8>::reverse_iterator riter(iter);
        if (riter != m->rend())
        {
            ASSERT(riter->first < start, "first=%llx, start=%llx", riter->first, start);
            ASSERT(riter->second < start, "second=%llx, start=%llx", riter->second, start);
        }
    }

    static void Move(map<u8, u8>* source, map<u8, u8>* dest, u8 start, u8 nextToLast)
    {
        CheckEmpty(dest, start, nextToLast);
        
        // find the last source entry before start
        map<u8, u8>::iterator iter = source->upper_bound(start);
        map<u8, u8>::reverse_iterator riter(iter);
        u8 i = 0;
        u8 j = 0;
        if (riter != source->rend())
        {
            i = riter->first;
        }
        iter = source->lower_bound(i);
        
        // Nothing to move?  Quit early.
        if (iter == source->end())
            goto done;
        
        i = iter->first;
        if (i > nextToLast)
            goto done;

        if (i < start)
        {
            j = iter->second;
            ASSERT(j >= i, "j too small\n");
            
            // first entry started before our range, that is a special case
            if (j > nextToLast)
            {
                // an entry totally overlaps our range
                iter->second = start - 1;
                (*source)[nextToLast + 1] = j;
                (*dest)[start] = nextToLast;
                goto done;
            }
            else if (iter->second >= start)
            {
                // an entry partially overlaps our range
                (*dest)[start] = iter->second;
                iter->second = start - 1;
            }
            
            iter = source->upper_bound(j);
        }
        
        // later entries start within or after our range
        for (;;)
        {
            if (iter == source->end() || iter->first > nextToLast)
                goto done;
            
            i = iter->first;
            j = iter->second;
            ASSERT(i >= start, "i is too small %llx %llx", i, start);
            
            if (j > nextToLast)
            {
                source->erase(iter);
                (*source)[nextToLast+1] = j;
                (*dest)[i] = nextToLast;
                goto done;
            }
            else
            {
                source->erase(iter);
                (*dest)[i] = j;
                iter = source->upper_bound(j);
            }
        }

    done:
        CheckEmpty(source, start, nextToLast);
    }
    
    void SwapRange(u8 first, u8 nextToLast)
    {
        _tempMale.clear();
        _tempFemale.clear();
        Move(&_male, &_tempMale, first, nextToLast);
        Move(&_female, &_tempFemale, first, nextToLast);
        Move(&_tempMale, &_female, first, nextToLast);
        Move(&_tempFemale, &_male, first, nextToLast);
        ASSERT(_tempMale.size() == 0, "got %lld", _tempMale.size());
        ASSERT(_tempFemale.size() == 0, "got %lld", _tempFemale.size());
    }
    
    // Do reverse meiosis, and do this many crossovers within this chromosome pair.
    void Meiosis(CPair* father, Random* rand, double crossovers, bool xy, bool isMale)
    {
        if (!xy || !isMale)
        {
            // figure out where the crossings are
            static const int c_max_cross = 10;
            u8 crossings[c_max_cross];
            int nCross = 0;

            // do as many crossings as they asked for
            for (int i=0; i<(int)crossovers; ++i)
                crossings[nCross++] = rand->Value();
            crossovers -= (int)crossovers;
            if ((rand->Value() / (double)(65536.0 * 65536.0 * 65536.0 * 65536.0)) < crossovers)
                crossings[nCross++] = rand->Value();
            
            totalMeiosis++;
            totalCrossovers += nCross;

            // the start of the chromosome randomly goes to the male or the female
            if (rand->Value() & 1)
            {
                crossings[nCross++] = 0;
            }
            
            // sort the crossings
            for (int i=1; i<nCross; ++i)
                for (int j=i; j--;)
                {
                    if (crossings[j+1] >= crossings[j])
                        break;
                    u8 temp = crossings[j];
                    crossings[j] = crossings[j+1];
                    crossings[j+1] = temp;
                }
            
            // inherit your genetic material on each side from either your mother or your father
            int iCross = 0;
            for (; iCross < nCross - 1; iCross += 2)
            {
                ASSERT(crossings[iCross+1] >= crossings[iCross], "boo");
                if (crossings[iCross+1] > crossings[iCross])
                {
                    SwapRange(crossings[iCross], crossings[iCross+1] - 1);
                }
            }
            if (iCross == nCross - 1)
            {
                SwapRange(nCross > 0 ? crossings[nCross - 1] : 0, ~(u8)0);
            }
        }
        
        // Now actually give the male chromosome to the father, female to the mother, and clear the other side
        father->_id = _id;
        if (!isMale && xy)
        {
            // daughters get an X chromosome from the father, not Y
            father->_female = _male;
            father->_male.clear();
            _male.clear();
        }
        else
        {
            father->_male = _male;
            father->_female.clear();
            _male.clear();
        }
    }
    
    bool IsUsed()
    {
        return (_male.size() > 0 || _female.size() > 0);
    }
    
    int GetId()
    {
        return _id;
    }

    // compute what fraction of this side is contributing DNA
    double GetSideFraction(map<u8, u8>* m)
    {
        double fraction = 0.0;
        for (map<u8, u8>::iterator iter = m->begin(); iter != m->end(); iter++)
        {
            fraction += ((iter->second - iter->first) + 1.0);
        }
        fraction /= (65536.0 * 65536.0 * 65536.0 * 65536.0);
        return fraction;
    }

    double GetFraction()
    {
        return (GetSideFraction(&_male) + GetSideFraction(&_female)) / 2.0;
    }

    static void UnitTestMap()
    {
        map<u8, u8> m;
        ShowMap(m);
        m[3] = 5;
        m[7] = 9;
        m[11] = 13;
        m[15] = 17;
        m[19] = 21;

        map<u8, u8>::iterator iter = m.lower_bound(11);
        map<u8, u8>::reverse_iterator riter(iter);
        ASSERT(iter->first == 11, "first=%llx", iter->first);
        ASSERT(riter->first == 7, "first=%llx", riter->first);
    }

    static void UnitTestMove()
    {
        CPair c;

        // move nothing
        Move(&c._male, &c._tempMale, 1, 33);
        ASSERT(c._male.empty(), "should be empty");
        ASSERT(c._tempMale.empty(), "should be empty");
        
        // move contained ranges
        c._male[1] = 3;
        c._male[4] = 5;
        Move(&c._male, &c._tempMale, 0, 6);
        ASSERT(c._male.size() == 0, "should be empty");
        ASSERT(c._tempMale.size() == 2, "should be 2");
        ASSERT(c._tempMale[1] == 3, "got %llx", c._tempMale[1]);
        ASSERT(c._tempMale[4] == 5, "wrong");
        c._tempMale.clear();

        // move center
        c._male[1] = 8;
        Move(&c._male, &c._tempMale, 4, 7);
        ASSERT(c._male[1] == 3, "got %llx", c._male[1]);
        ASSERT(c._male[8] == 8, "got %llx", c._male[8]);
        ASSERT(c._male.size() == 2, "got %llx", c._male.size());
        ASSERT(c._tempMale[4] == 7, "should be 7");
        ASSERT(c._tempMale.size() == 1, "should be 1");
        c._male.clear();
        c._tempMale.clear();

        // move overlapping start and finish
        c._female[1] = 3;
        c._female[5] = 7;
        c._female[9] = 11;
        c._female[13] = 15;
        Move(&c._female, &c._tempFemale, 6, 10);
        ASSERT(c._female.size() == 4, "got %llx", c._female.size());
        ASSERT(c._female[1] == 3, "got %llx", c._female[1]);
        ASSERT(c._female[5] == 5, "got %llx", c._female[5]);
        ASSERT(c._female[11] == 11, "got %llx", c._female[11]);
        ASSERT(c._female[13] == 15, "got %llx", c._female[13]);
        ASSERT(c._tempFemale[6] == 7, "got %llx", c._tempFemale[6]);
        ASSERT(c._tempFemale[9] == 10, "got %llx", c._tempFemale[9]);
        c._female.clear();
        c._tempFemale.clear();
    }

    static void UnitTestSwap()
    {
        CPair c;

        c.Clear(0);
        ASSERT(c._female.size() == 0, "got %llx", c._female.size());
        ASSERT(c._male.size() == 0, "got %llx", c._female.size());

        c._female[20] = 40;
        c.SwapRange(10, 30);
        ASSERT(c._female.size() == 1, "got %llx", c._female.size());
        ASSERT(c._female[31] == 40, "got %llx", c._female[31]);
        ASSERT(c._male.size() == 1, "got %llx", c._male.size());
        ASSERT(c._male[20] == 30, "got %llx", c._male[20]);

        c.Clear(0);
        c._male[20] = 40;
        c.SwapRange(30, 50);
        ASSERT(c._male.size() == 1, "got %llx", c._male.size());
        ASSERT(c._male[20] == 29, "got %llx", c._male[20]);
        ASSERT(c._female.size() == 1, "got %llx", c._female.size());
        ASSERT(c._female[30] == 40, "got %llx", c._female[30]);
    }
    
private:
    int _id;  // chromosome number
    map<u8, u8> _male;
    map<u8, u8> _female;
    map<u8, u8> _tempMale;
    map<u8, u8> _tempFemale;
};


class GenomeFactory;

class Genome
{
public:
    static const int c_cpairs = 23;             // number of human chromosome pairs
    static const int c_xy = 22;                 // ID of the X/Y chromosome, which does crossover only in females
    static const double c_female_crossovers = 41.1;  // number of crossovers total per human female meiosis
    static const double c_male_crossovers = 26.4;    // number of crossovers total per human male meiosis
    
    Genome() 
    {
        for (int i=0; i<c_cpairs; ++i)
        {
            _unused.push_back(new CPair());
        }
    }
    
    ~Genome()
    {
        while (_used.size())
        {
            CPair* c = _used.back();
            _used.pop_back();
            delete c;
        }
        while (_unused.size())
        {
            CPair* c = _unused.back();
            _unused.pop_back();
            delete c;
        }
    }
    
    // mark that this entire genome is used
    void SetToRoot(bool isMale)
    {
        CPair* c;
        _isMale = isMale;
        while (_unused.size())
        {
            c = _unused.back();;
            _unused.pop_back();
            _used.push_back(c);
        }
        ASSERT(_used.size() == c_cpairs, "wrong number of chromosome pairs, %d vs %d\n", _used.size(), c_cpairs);
        for (int i=0; i<_used.size(); ++i)
        {
            _used[i]->SetToRoot(i);
        }
    }
    
    // split this genome randomly into a mother and father, reusing this genome for the mother
    void Meiosis(Genome* father, Random* rand)
    {
        // Humans are formed from an egg and a sperm; they get half their chromosomes straight from the mother
        // and the other half straight from the mother.  Before producing eggs and sperm, meiosis mixes those
        // chromosomes through crossover then splits them into haploid cells.
        //
        // Since we are doing in this in reverse, meiosis happens in the parent immediately after the parent
        // is formed.  Meiosis still required pairing homologous chromosomes in the parent.

        // clear the parent
        while (father->_used.size())
        {
            CPair* pair = father->_used.back();
            father->_used.pop_back();
            father->_unused.push_back(pair);
        }

        // how many crossovers per chromosome pair?
        double crossoversPerPair = 
            _isMale ? c_male_crossovers/(c_cpairs-1) : c_female_crossovers/c_cpairs;

        // do reverse meiosis on each used chromosome pair
        for (int i=0; i<_used.size(); ++i)
        {
            ASSERT(father->_unused.size() > 0, "_unused=%Id", father->_unused.size());
            CPair* pair = father->_unused.back();
            father->_unused.pop_back();
            
            _used[i]->Meiosis(pair, rand, crossoversPerPair, _used[i]->GetId() == c_xy, _isMale);
            
            if (pair->IsUsed())
            {
                father->_used.push_back(pair);
            }
            else
            {
                father->_unused.push_back(pair);
            }
        }
        
        // Now we're our own mother, and the father is the father.  Some pairs might now be unused.
        _isMale = false;
        father->_isMale = true;
        for (int i=_used.size(); i--;)
        {
            if (!_used[i]->IsUsed())
            {
                CPair* pair = _used[i];
                _used[i] = _used[_used.size() - 1];
                _used.pop_back();
                _unused.push_back(pair);
            }
        }
        ASSERT(_used.size() + _unused.size() == c_cpairs, "_used=%Id _unused=%Id", _used.size(), _unused.size());
        ASSERT(father->_used.size() + father->_unused.size() == c_cpairs, "_used=%Id, _unused=%Id",
               father->_used.size(), father->_unused.size());
    }

    // If this person has 1/200 of the DNA of the original person, return 8, because 1/200 is nearest 2^^-8.
    double GetFraction()
    {
        double fraction = 0.0;
        for (int i=_used.size(); i--;)
        {
            fraction += _used[i]->GetFraction();
        }
        return fraction / c_cpairs;
    }
    
    bool IsUsed()
    {
        return (_used.size() != 0);
    }
    
    void Clear()
    {
        while (_used.size())
        {
            CPair* c = _used.back();
            _used.pop_back();
            _unused.push_back(c);
        }
    }
    
private:
    vector<CPair*> _used;
    vector<CPair*> _unused;
    bool _isMale;
};

class GenomeFactory
{
public:
    GenomeFactory() 
    {
    }
    
    ~GenomeFactory()
    {
        while (!_unused.empty())
        {
            Genome* person = _unused.top();
            _unused.pop();
            delete person;
        }
    }
    
    Genome* Create()
    {
        if (!_unused.empty())
        {
            Genome* person = _unused.top();
            _unused.pop();
            return person;
        }
        else
        {
            return new Genome();
        }
    }
    
    void Destroy(Genome* person)
    {
        _unused.push(person);
    }
    
private:
    stack<Genome*> _unused;
};

class Pedigree
{
public:
    Pedigree()
    {
        _factory = new GenomeFactory();
    }
    
    ~Pedigree()
    {
        delete _factory;
    }
    
    void Generate(Random* rand, int generations, map<u8,u8>* ancestors, map<u1, u8>* inheritance)
    {
        vector<Genome*> parents;
        vector<Genome*> grandparents;
        
        // generate an initial person who uses all his genes
        Genome* person = _factory->Create();
        person->SetToRoot(false);
        parents.push_back(person);
        
        // measure inheritance from all generations
        for (int i=0; i<generations; ++i)
        {
            // this person had this many genetic ancestors this far back
            ancestors[i][parents.size()]++;
            
            // split the parents into grandparents
            while (parents.size())
            {
                person = parents.back();
                parents.pop_back();

                // this genetic ancestor contributed this much DNA
                double fraction = person->GetFraction();
                if (fraction > 0.5/(65536.0 * 65536.0 * 65536.0 * 65536.0))
                {
                    double inverseFraction = 1.0/fraction;
                    u1 k = 0;
                    double power = 0.707;
                    while (power <= inverseFraction)
                    {
                        power *= 2.0;
                        k += 1;
                    }
                    inheritance[i][k]++;
                }

                Genome* father = _factory->Create();
                person->Meiosis(father, rand);
                if (person->IsUsed())
                    grandparents.push_back(person);
                else
                    _factory->Destroy(person);
                if (father->IsUsed())
                    grandparents.push_back(father);
                else
                    _factory->Destroy(father);
            }
            
            // promote the granparents of this generation to the parents of the next generation
            while (grandparents.size())
            {
                person = grandparents.back();
                grandparents.pop_back();
                parents.push_back(person);
            }

	    // verify there is a total of 1.0 DNA across all ancestors
	    double wholeDNA = 0.0;
	    for (int i=0; i<parents.size(); ++i)
	    {
	        wholeDNA += parents[i]->GetFraction();
	    }
	    ASSERT(wholeDNA > 1.0 - 0.00001 && wholeDNA < 1.0 + 0.00001, "actualDNA: %g", wholeDNA);
        }
        
        // clean everyone up
        while (parents.size())
        {
            person = parents.back();
            parents.pop_back();
            _factory->Destroy(person);
        }
    }
    
private:
    GenomeFactory* _factory;
};

int main(int argc, char** argv)
{
    CPair::UnitTestMap();
    CPair::UnitTestMove();
    CPair::UnitTestSwap();
    
    Random rctx;
    const int c_maxGenerations = 1000;
    int generations = 20;
    int trials = 1000;
    int seed = 0;
    if (argc > 4)
    {
        printf("Usage: ancestor [#generations [#trials [#seed]]]\n");
        printf("example: ancestor %d %d %d\n", generations, trials, seed);
        return 1;
    }
    
    if (argc > 1)
    {
        sscanf(argv[1], "%d", &generations);
        printf("generations: %d\n", generations);
        if (generations > c_maxGenerations)
        {
            printf("too many generations, %d > %d\n", generations, c_maxGenerations);
            return 2;
        }
        if (argc > 2)
        {
            sscanf(argv[2], "%d", &trials);
            printf("trials: %d\n", trials);
            if (argc > 3)
            {
                sscanf(argv[3], "%d", &seed);
                printf("seed: %d\n", seed);
            }
        }
    }
    
    rctx.Init(seed);

    // ancestors[i][j] is how many people i generations ago had j ancestors
    map<u8,u8> ancestors[c_maxGenerations]; 

    // inheritance[i][j] is how many ancestors i generations ago contributed nearest 2^^-j of the DNA
    map<u1,u8> inheritance[c_maxGenerations];

    // A pedigree represents a final person, and everyone who contributed to them
    Pedigree person;
    
    for (int i=0; i<trials; ++i)
    {
        person.Generate(&rctx, generations, ancestors, inheritance);
        printf("gathered trial %d\n", i);
    }
    
    printf("Number of ancestors n generations back: generation, ancestors, contributing\n");
    for (int i=0; i<generations; ++i)
    {
        u8 sum = 0;
        u8 minAncestors = ancestors[i].begin()->first;
        u8 maxAncestors = 0;
        for (map<u8,u8>::iterator iter = ancestors[i].begin(); iter != ancestors[i].end(); iter++)
        {
            sum += iter->first * iter->second;
            maxAncestors = iter->first;
        }
        char stuff[100];
        if (i < 40)
            sprintf(stuff, "%lld", ((u8)1)<<i);
        else
            sprintf(stuff, "2<sup>%d</sup>", i);
        printf("<tr><td>%d</td><td>%s</td><td>%lld</td><td>%6g</td><td>%lld</td></tr>\n", 
               i, stuff, minAncestors, ((double)sum)/trials, maxAncestors);
    }
    printf("\n\n");
    printf("n: i:j means n generations back, j percent of ancestors contributed nearest 2^^-i of the DNA\n");
    for (int i=0; i<generations; ++i)
    {
        printf("gen %d: ", i);
        for (map<u1,u8>::iterator iter = inheritance[i].begin(); iter != inheritance[i].end(); iter++)
        {
            int expected = (iter->second + trials/2) / trials;
            if (expected > 0)
            {
                printf("%d:%lld, ", iter->first - 1, expected);
            }
        }
        printf("\n");
    }
    printf("\n");

    printf("crossovers per chromosome meiosis: %f\n", (float)totalCrossovers/(float)totalMeiosis);

    return 0;
}
