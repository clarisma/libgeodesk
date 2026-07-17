/* C++ code produced by gperf version 3.1 */
/* Command-line: 'c:\\dev\\geodesk-py\\tools\\gperf' -L C++ -t --class-name=SpatialUnit_AttrHash --lookup-function-name=lookup SpatialUnit_attr.txt  */
/* Computed positions: -k'1-2,8' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "SpatialUnit_attr.txt"

// Keywords
#include <assert.h>

#line 6 "SpatialUnit_attr.txt"
struct Unit { const char *name; int index; };

#define TOTAL_KEYWORDS 24
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 17
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 38
/* maximum key range = 38, duplicates = 0 */

class SpatialUnit_AttrHash
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static struct Unit *lookup (const char *str, size_t len);
};

inline unsigned int
SpatialUnit_AttrHash::hash (const char *str, size_t len)
{
  static unsigned char asso_values[] =
    {
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      23, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 15, 39,  0,
      20,  0,  0, 39,  8,  0, 39,  5, 39,  0,
      39, 39, 39,  0, 39,  0, 30, 39, 39, 39,
      39, 15, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39
    };
  unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[static_cast<unsigned char>(str[7])];
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
        hval += asso_values[static_cast<unsigned char>(str[1])];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[static_cast<unsigned char>(str[0])];
        break;
    }
  return hval;
}

struct Unit *
SpatialUnit_AttrHash::lookup (const char *str, size_t len)
{
  static struct Unit wordlist[] =
    {
      {""},
#line 9 "SpatialUnit_attr.txt"
      {"m",          SpatialUnit::METERS,},
#line 17 "SpatialUnit_attr.txt"
      {"mi",         SpatialUnit::MILES,},
#line 27 "SpatialUnit_attr.txt"
      {"mi2",                 SpatialUnit::MILES | SpatialUnit::AREA_UNIT,},
#line 12 "SpatialUnit_attr.txt"
      {"feet",       SpatialUnit::FEET,},
#line 16 "SpatialUnit_attr.txt"
      {"miles",      SpatialUnit::MILES,},
#line 8 "SpatialUnit_attr.txt"
      {"meters",     SpatialUnit::METERS,},
#line 11 "SpatialUnit_attr.txt"
      {"km",         SpatialUnit::KILOMETERS,},
#line 21 "SpatialUnit_attr.txt"
      {"km2",                 SpatialUnit::KILOMETERS | SpatialUnit::AREA_UNIT,},
      {""},
#line 29 "SpatialUnit_attr.txt"
      {"hc",                  SpatialUnit::HECTARES | SpatialUnit::AREA_UNIT,},
#line 22 "SpatialUnit_attr.txt"
      {"square_feet",         SpatialUnit::FEET | SpatialUnit::AREA_UNIT,},
#line 26 "SpatialUnit_attr.txt"
      {"square_miles",        SpatialUnit::MILES | SpatialUnit::AREA_UNIT,},
#line 18 "SpatialUnit_attr.txt"
      {"square_meters",       SpatialUnit::METERS | SpatialUnit::AREA_UNIT,},
      {""},
#line 10 "SpatialUnit_attr.txt"
      {"kilometers", SpatialUnit::KILOMETERS,},
#line 28 "SpatialUnit_attr.txt"
      {"hectares",            SpatialUnit::HECTARES | SpatialUnit::AREA_UNIT,},
#line 31 "SpatialUnit_attr.txt"
      {"ac",                  SpatialUnit::ACRES | SpatialUnit::AREA_UNIT,},
      {""}, {""},
#line 30 "SpatialUnit_attr.txt"
      {"acres",               SpatialUnit::ACRES | SpatialUnit::AREA_UNIT,},
      {""},
#line 20 "SpatialUnit_attr.txt"
      {"square_kilometers",   SpatialUnit::KILOMETERS | SpatialUnit::AREA_UNIT,},
      {""}, {""},
#line 19 "SpatialUnit_attr.txt"
      {"m2",                  SpatialUnit::METERS | SpatialUnit::AREA_UNIT,},
      {""},
#line 24 "SpatialUnit_attr.txt"
      {"square_yards",        SpatialUnit::YARDS | SpatialUnit::AREA_UNIT,},
      {""}, {""}, {""}, {""},
#line 13 "SpatialUnit_attr.txt"
      {"ft",         SpatialUnit::FEET,},
#line 23 "SpatialUnit_attr.txt"
      {"ft2",                 SpatialUnit::FEET | SpatialUnit::AREA_UNIT,},
      {""},
#line 14 "SpatialUnit_attr.txt"
      {"yards",      SpatialUnit::YARDS,},
      {""},
#line 15 "SpatialUnit_attr.txt"
      {"yd",         SpatialUnit::YARDS,},
#line 25 "SpatialUnit_attr.txt"
      {"yd2",                 SpatialUnit::YARDS | SpatialUnit::AREA_UNIT,}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
