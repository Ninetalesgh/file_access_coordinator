#pragma once

#include <cstdio>
#include <vector>

#ifndef INLINE
	#define INLINE
#endif

#ifndef min
	#define min(a,b) (a < b ? a : b)
	#define BSE_OVERWRITE_MIN
#endif
#ifndef max
	#define max(a,b) (a > b ? a : b)
	#define BSE_OVERWRITE_MAX
#endif

template <typename T> struct _BSE_ALWAYS_FALSE { constexpr static bool value = false; };
#define BSE_ALWAYS_FALSE(T) _BSE_ALWAYS_FALSE<T>::value

using s8  = signed char;
using s16 = short;
using s32 = int;
using s64 = long long;
using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

#define _DEFINE_ENUM_OPERATORS_INTERNAL(enumtypename, basictype)\
INLINE enumtypename  operator |  ( enumtypename a, enumtypename b ) { return enumtypename( basictype( a ) | basictype( b ) ); }\
INLINE enumtypename& operator |= ( enumtypename& a, enumtypename b ) { a = b | a; return a; }\
INLINE enumtypename  operator &  ( enumtypename a, enumtypename b ) { return enumtypename( basictype( a ) & basictype( b ) ); }\
INLINE enumtypename& operator &= ( enumtypename& a, enumtypename b ) { a = b & a; return a; }\
INLINE enumtypename  operator ~  ( enumtypename a ) { return enumtypename( ~basictype( a ) ); }\
INLINE enumtypename  operator ^  ( enumtypename a, enumtypename b ) { return enumtypename( basictype( a ) ^ basictype( b ) ); }\
INLINE enumtypename& operator ^= ( enumtypename& a, enumtypename b ) { a = b ^ a; return a; }\
INLINE bool flags_contain(enumtypename a, enumtypename b) { return (basictype(a) & basictype(b)) == basictype(b);}

#define BSE_DEFINE_ENUM_OPERATORS_U8(enumtypename)  _DEFINE_ENUM_OPERATORS_INTERNAL(enumtypename, u8)
#define BSE_DEFINE_ENUM_OPERATORS_U16(enumtypename) _DEFINE_ENUM_OPERATORS_INTERNAL(enumtypename, u16)
#define BSE_DEFINE_ENUM_OPERATORS_U32(enumtypename) _DEFINE_ENUM_OPERATORS_INTERNAL(enumtypename, u32)
#define BSE_DEFINE_ENUM_OPERATORS_U64(enumtypename) _DEFINE_ENUM_OPERATORS_INTERNAL(enumtypename, u64)

#define array_count(array) (sizeof(array) / (sizeof((array)[0])))

#define BSE_STACK_BUFFER_TINY 2048
#define BSE_STACK_BUFFER_SMALL 4096
#define BSE_STACK_BUFFER_MEDIUM 8192
#define BSE_STACK_BUFFER_LARGE 16384
#define BSE_STACK_BUFFER_HUGE 32768
#define BSE_STACK_BUFFER_GARGANTUAN 65536
#define BSE_STACK_BUFFER_GARGANTUAN_PLUS 212992

#define BSE_STRINGIZE(s) _BSE_STRINGIZE_HELPER(s)
#define _BSE_STRINGIZE_HELPER(s) #s


  //TODO inconsistent that string_length doesn't include the \0

  //returns the number of bytes in the string, not counting the \0
  s32 string_length( char const* string );

  //returns the number of utf8 characters in the string, not counting the \0
  s32 string_length_utf8( char const* utf8String );

  //returns the number of lines in the string
  s32 string_line_count( char const* string );

  //returns the number of bytes until a \n or a \0, including it
  s32 string_line_length( char const* string );

  //returns the number of utf8 characters until a \n or a \0, including it
  s32 string_line_length_utf8( char const* utf8String );

  //returns 1 if the strings match including null termination
  //returns 0 if they don't
  bool string_match( char const* a, char const* b );

  //returns 1 if string begins with subString
  //returns 0 if they don't
  bool string_begins_with( char const* string, char const* subString );

  //returns destination
  char* string_copy( char* destination, char const* origin, s32 capacity );

  //returns destination
  //copy until character is hit
  char* string_copy_until( char* destination, char const* origin, s32 capacity, char until );

  //replaces all of target with replacement
  void string_replace_char( char* str, char target, char replacement );
  void string_replace_char( char* dest, s32 capacity, char const* source, char find, char const* replacement );

  //returns pointer to where subString begins in string
  //returns nullptr if the subString is not part of string
  char* string_contains( char* string, char const* subString );
  char const* string_contains( char const* string, char const* subString );

  //returns nullptr if the character doesn't exist in the string
  char const* string_find_last( char const* string, char character );

  //returns the next character in the string after parsing the codepoint
  char const* string_parse_utf8( char const* utf8String, s32* out_codepoint );

  //returns the character after reading the expected value
  char const* string_parse_value( char const* floatString, float* out_float );
  char const* string_parse_value( char const* intString, s32* out_int );
  char const* string_parse_value( char const* intString, s64* out_int );

  //returns the number of bytes written including the \0
  template<typename... Args> INLINE s32 string_format( char* destination, s32 capacity, Args... args );



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////inl/////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


  template<typename Arg> INLINE s32 string_format_internal( char* destination, s32 capacity, Arg value );
  template<typename Arg> INLINE s32 string_format_internal( char* destination, s32 capacity, std::vector<Arg> const& values );

  template<typename Arg> s32 string_format( char* destination, s32 capacity, Arg value )
  {
    s32 bytesWritten = string_format_internal( destination, capacity, value );

    bytesWritten = min( capacity - 1, bytesWritten );
    destination[bytesWritten] = '\0'; //end of string_format

    return bytesWritten + 1;
  }

  template<typename Arg, typename... Args>
  INLINE s32 string_format( char* destination, s32 capacity, Arg arg, Args... args )
  {
    s32 bytesWritten = string_format_internal( destination, capacity, arg );
    bytesWritten += string_format( destination + bytesWritten, capacity - bytesWritten, args... );

    return bytesWritten;
  }

  template<> s32 string_format_internal<char const*>( char* destination, s32 capacity, char const* value );
  template<> s32 string_format_internal<char*>( char* destination, s32 capacity, char* value );
  template<> s32 string_format_internal<u64>( char* destination, s32 capacity, u64 value );
  template<> s32 string_format_internal<u32>( char* destination, s32 capacity, u32 value );
  template<> s32 string_format_internal<u16>( char* destination, s32 capacity, u16 value );
  template<> s32 string_format_internal<u8>( char* destination, s32 capacity, u8 value );
  template<> s32 string_format_internal<s64>( char* destination, s32 capacity, s64 value );
  template<> s32 string_format_internal<s32>( char* destination, s32 capacity, s32 value );
  template<> s32 string_format_internal<s16>( char* destination, s32 capacity, s16 value );
  template<> s32 string_format_internal<s8>( char* destination, s32 capacity, s8 value );
//  template<> s32 string_format_internal<Vector2i>( char* destination, s32 capacity, Vector2i value );
//  template<> s32 string_format_internal<Vector2>( char* destination, s32 capacity, Vector2 value );

  s32 string_format_float( char* destination, s32 capacity, float value, s32 const postPeriodDigits );
  template<> s32 string_format_internal<float>( char* destination, s32 capacity, float value ) { return string_format_float( destination, capacity, value, 2 ); }

  template<typename Arg> INLINE s32 string_format_internal( char* destination, s32 capacity, std::vector<Arg> const& values )
  {
    char* writer = destination;
    s32 capacityLeft = capacity;

    s32 bytesWritten = string_format_internal( writer, capacityLeft, "{ " );
    writer += bytesWritten;
    capacityLeft -= bytesWritten;

    for ( s32 i = 0; i < values.size(); ++i )
    {
      if ( capacityLeft <= 1 )
      {
        break;
      }

      bytesWritten = string_format( writer, capacityLeft, values[i], ", " ) - 1;
      writer += bytesWritten;
      capacityLeft -= bytesWritten;
    }
    bytesWritten = string_format_internal( writer - 2, capacityLeft + 2, " }" );

    return capacity - capacityLeft;
  }

  template<typename Arg> INLINE s32 string_format_internal( char* destination, s32 capacity, Arg value )
  {
    static_assert(BSE_ALWAYS_FALSE(Arg), "Type doesn't exist for string formatting yet (probably through a log call), feel free to make your own specialization.");
    return 0;
  }


  template<> s32 string_format_internal<char const*>( char* destination, s32 capacity, char const* value )
  {
    s32 result = 0;
    if ( value )
    {
      while ( *value != '\0' && capacity > result )
      {
        *destination++ = *value++;
        ++result;
      }
    }
    return result;
  }

  template<> s32 string_format_internal<char*>( char* destination, s32 capacity, char* value )
  {
    return string_format_internal( destination, capacity, (char const*) value );
  }

  template<> s32 string_format_internal<u64>( char* destination, s32 capacity, u64 value )
  {
    s32 result = 0;
    if ( capacity )
    {
      if ( value )
      {
        u64 divisor = 10000000000000000000ULL;
        while ( divisor && capacity > result )
        {
          u8 place = u8( value / divisor );
          if ( result || place )
          {
            value %= divisor;
            *destination++ = '0' + place;
            ++result;
          }
          divisor /= 10;
        }
      }
      else
      {
        *destination = '0';
        ++result;
      }
    }
    return result;
  }

  template<> s32 string_format_internal<u32>( char* destination, s32 capacity, u32 value )
  {
    s32 result = 0;
    if ( capacity )
    {
      if ( value )
      {
        u32 divisor = 1000000000;
        while ( divisor && capacity > result )
        {
          u8 place = u8( value / divisor );
          if ( result || place )
          {
            value %= divisor;
            *destination++ = '0' + place;
            ++result;
          }
          divisor /= 10;
        }
      }
      else
      {
        *destination = '0';
        ++result;
      }
    }
    return result;
  }

  template<> s32 string_format_internal<u16>( char* destination, s32 capacity, u16 value )
  {
    s32 result = 0;
    if ( capacity )
    {
      if ( value )
      {
        u32 divisor = 10000;
        while ( divisor && capacity > result )
        {
          u8 place = u8( value / divisor );
          if ( result || place )
          {
            value %= divisor;
            *destination++ = '0' + place;
            ++result;
          }
          divisor /= 10;
        }
      }
      else
      {
        *destination = '0';
        ++result;
      }
    }
    return result;
  }


  template<> s32 string_format_internal<u8>( char* destination, s32 capacity, u8 value )
  {
    s32 result = 0;
    if ( capacity )
    {
      if ( value )
      {
        u32 divisor = 100;
        while ( divisor && capacity > result )
        {
          u8 place = u8( value / divisor );
          if ( result || place )
          {
            value %= divisor;
            *destination++ = '0' + place;
            ++result;
          }
          divisor /= 10;
        }
      }
      else
      {
        *destination = '0';
        ++result;
      }
    }
    return result;
  }

  template<> s32 string_format_internal<s64>( char* destination, s32 capacity, s64 value )
  {
    s32 result = 0;
    if ( value >> ((sizeof( s64 ) * 8) - 1) && capacity >= 0 )
    {
      *destination++ = '-';
      value = -value;
      ++result;
    }
    return result + string_format_internal( destination, capacity - result, u64( value ) );
  }

  template<> s32 string_format_internal<s32>( char* destination, s32 capacity, s32 value )
  {
    s32 result = 0;
    if ( value >> ((sizeof( s32 ) * 8) - 1) && capacity >= 0 )
    {
      *destination++ = '-';
      value = -value;
      ++result;
    }
    return result + string_format_internal( destination, capacity - result, u32( value ) );
  }

  template<> s32 string_format_internal<s16>( char* destination, s32 capacity, s16 value )
  {
    s32 result = 0;
    if ( value >> ((sizeof( s16 ) * 8) - 1) && capacity >= 0 )
    {
      *destination++ = '-';
      value = -value;
      ++result;
    }
    return result + string_format_internal( destination, capacity - result, u16( value ) );
  }

  template<> s32 string_format_internal<s8>( char* destination, s32 capacity, s8 value )
  {
    s32 result = 0;
    if ( value >> ((sizeof( s8 ) * 8) - 1) && capacity >= 0 )
    {
      *destination++ = '-';
      value = -value;
      ++result;
    }
    return result + string_format_internal( destination, capacity - result, u8( value ) );
  }

  s32 string_format_float( char* destination, s32 capacity, float value, s32 const postPeriodDigits )
  {
    return snprintf( destination, capacity, "%.2f", value );
  }

//  template<> s32 string_format_internal<Vector2i>( char* destination, s32 capacity, Vector2i value )
//  {
//    return string_format( destination, capacity, "{", value.x, ", ", value.y, "}" ) - 1;
//  }

//  template<> s32 string_format_internal<Vector2>( char* destination, s32 capacity, Vector2 value )
//  {
//    return string_format( destination, capacity, "{", value.x, ", ", value.y, "}" ) - 1;
//  }

  INLINE s32 string_length( char const* string )
  {
    char const* reader = string;
    while ( *reader++ != '\0' ) {}
    return u32( u64( reader - string ) ) - 1;
  }

  INLINE s32 string_length_utf8( char const* utf8String )
  {
    u32 result = 0;
    while ( *utf8String )
    {
      s32 codepoint;
      utf8String = string_parse_utf8( utf8String, &codepoint );
      ++result;
    }
    return result;
  }

  INLINE bool string_match( char const* a, char const* b )
  {
    if ( a == nullptr || b == nullptr )
    {
      return false;
    }

    bool result = false;
    while ( *a == *b )
    {
      if ( *a == '\0' )
      {
        result = true;
        break;
      }

      ++a;
      ++b;
    }

    return result;
  }

  INLINE bool string_begins_with( char const* string, char const* subString )
  {
    if ( string == nullptr || subString == nullptr )
    {
      return true;
    }

    bool result = false;
    while ( *string == *subString )
    {
      ++string;
      ++subString;
      if ( *subString == '\0' )
      {
        result = true;
        break;
      }
    }

    return result;
  }

  INLINE char* string_copy( char* destination, char const* origin, s32 capacity )
  {
    for ( s32 i = 0; i < capacity; ++i )
    {
      destination[i] = origin[i];

      if ( origin[i] == '\0' )
      {
        break;
      }
    }
    return destination;
  }

  INLINE char* string_copy_until( char* destination, char const* origin, s32 capacity, char until )
  {
    for ( s32 i = 0; i < capacity; ++i )
    {
      if ( origin[i] != until )
      {
        destination[i] = origin[i];
      }
      else
      {
        break;
      }
    }
    return destination;
  }

  void string_replace_char( char* str, char target, char replacement )
  {
    while ( *str != '\0' )
    {
      if ( *str == target )
      {
        *str = replacement;
      }
      ++str;
    }
  }

  void string_replace_char( char* dest, s32 capacity, char const* source, char find, char const* replacement )
  {
    s32 replacementLength = string_length(replacement);

    while ( capacity > 0 )
    {
      if ( *source == find )
      {
        string_copy(dest, replacement, capacity);
        dest += replacementLength;
        capacity -= replacementLength;
      }
      else
      {  
        *dest++ = *source;
        --capacity;
      }
      
      if (*source == '\0')
      {
        break;
      }

      ++source;
    }
  }

  INLINE s32 string_line_count( char const* string )
  {
    //TODO \r treatment
    char const* reader = string;
    s32 result = 1;
    while ( *reader != '\0' )
    {
      if ( *reader == '\n' )
      {
        ++result;
      }

      ++reader;
    }

    return result;
  }

  INLINE s32 string_line_length( char const* string )
  {
    char const* reader = string;
    while ( *reader != '\0' && *reader != '\n' ) { ++reader; }
    return u32( reader - string );
  }

  INLINE s32 string_line_length_utf8( char const* utf8String )
  {
    u32 result = 1;
    while ( *utf8String && *utf8String != '\n' )
    {
      s32 codepoint;
      utf8String = string_parse_utf8( utf8String, &codepoint );
      ++result;
    }
    return result;
  }

  char* string_contains( char* string, char const* subString )
  {
    if ( string == nullptr || subString == nullptr ) return 0;

    char* result = nullptr;
    char* reader = string;

    while ( *reader != '\0' )
    {
      if ( *reader == *subString )
      {
        char* a = reader;
        char const* b = subString;
        while ( *a == *b )
        {
          ++a;
          ++b;
        }

        if ( *b == '\0' )
        {
          result = reader;
          break;
        }

        reader = a;
      }
      else
      {
        ++reader;
      }
    }

    return result;
  }

  char const* string_contains( char const* string, char const* subString )
  {
    if ( string == nullptr || subString == nullptr ) return 0;

    char const* result = nullptr;
    char const* reader = string;

    while ( *reader != '\0' )
    {
      if ( *reader == *subString )
      {
        char const* a = reader;
        char const* b = subString;
        while ( *a == *b && *a != '\0' )
        {
          ++a;
          ++b;
        }

        if ( *b == '\0' )
        {
          result = reader;
          break;
        }

        reader = a;
      }
      else
      {
        ++reader;
      }
    }

    return result;
  }

  char const* string_find_last( char const* string, char character )
  {
    if ( string )
    {
      char const* end = string + string_length( string );
      if ( *string != '\0' )
      {
        while ( --end != string )
        {
          if ( *end == character )
          {
            return end;
          }
        }
      }
    }

    return nullptr;
  }

  s32 string_get_unicode_codepoint( char const* utf8String, s32* out_extraBytes /*= nullptr*/ )
  {
    s32 result = 0;
    u8 const* reader = (u8 const*) utf8String;
    u8 const extraByteCheckMask = 0b10000000;
    u8 const extraByteValueMask = 0b00111111;

    u8 unicodeMask = 0b11000000;

    s32 extraBytes = 0;
    result = *(u8*) (reader);
    while ( *utf8String & unicodeMask )
    {
      unicodeMask >>= 1;
      ++reader;
      if ( (*reader & ~extraByteValueMask) == extraByteCheckMask )
      {
        result <<= 6;
        result += (*reader) & extraByteValueMask;

        ++extraBytes;
      }
      else
      {
        break;
      }
    }

    if ( extraBytes )
    {
      s32 maskLength = 1 << (5 * extraBytes + 6);
      result &= (maskLength - 1);
    }
    else
    {
      //TODO maybe check whether it's a valid 1 byte character? aka: < 128
    }

    if ( out_extraBytes ) *out_extraBytes = extraBytes;
    return result;
  }

  INLINE char const* string_parse_utf8( char const* utf8String, s32* out_codepoint )
  {
    char const* nextChar = nullptr;
    if ( *utf8String != '\0' )
    {
      s32 extraBytes = 0;
      *out_codepoint = string_get_unicode_codepoint( utf8String, &extraBytes );
      nextChar = utf8String + 1 + extraBytes;
    }
    else
    {
      *out_codepoint = 0;
    }
    return nextChar;
  }

  char const* string_parse_value( char const* floatString, float* out_float )
  {
    float result = 0.0f;
    float sign = 1.0f;
    char const* reader = floatString;
    float divisor = 0.0f;

    while ( *reader == ' ' ) { ++reader; }
    while ( *reader != ' ' && *reader != '\0' )
    {
      if ( *reader == '-' )
      {
        sign = -1.0f;
      }
      else if ( *reader == '.' && divisor == 0.0f )
      {
        divisor = 10.0f;
      }
      else if ( *reader == 'f' )
      {
        ++reader;
        break;
      }
      else if ( *reader >= '0' && *reader <= '9' )
      {
        if ( divisor == 0.0f )
        {
          result *= 10.0f;
          result += float( *reader - '0' );
        }
        else
        {
          result += float( *reader - '0' ) / divisor;
          divisor *= 10.0f;
        }
      }

      ++reader;
    }

    *out_float = result * sign;
    return reader;
  }

  char const* string_parse_value( char const* intString, s32* out_int )
  {
    s32 result = 0;
    s32 sign = 1;
    char const* reader = intString;
    while ( *reader == ' ' ) { ++reader; }
    while ( *reader != ' ' && *reader != '\0' )
    {
      if ( *reader == '-' )
      {
        sign = -1;
      }
      else if ( *reader >= '0' && *reader <= '9' )
      {
        result *= 10;
        result += s32( *reader - '0' );

      }

      ++reader;
    }

    *out_int = result * sign;
    return reader;
  }

  char const* string_parse_value( char const* intString, s64* out_int )
  {
    s64 result = 0;
    s64 sign = 1;
    char const* reader = intString;
    while ( *reader == ' ' ) { ++reader; }
    while ( *reader != ' ' && *reader != '\0' )
    {
      if ( *reader == '-' )
      {
        sign = -1;
      }
      else if ( *reader >= '0' && *reader <= '9' )
      {
        result *= 10;
        result += s64( *reader - '0' );
      }

      ++reader;
    }

    *out_int = result * sign;
    return reader;
  }


#ifdef BSE_OVERWRITE_MIN
	#undef min
	#undef BSE_OVERWRITE_MIN
#endif

#ifdef BSE_OVERWRITE_MAX
	#undef max
	#undef BSE_OVERWRITE_MAX
#endif
