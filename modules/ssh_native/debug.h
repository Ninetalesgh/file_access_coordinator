#pragma once


#include <core/string/print_string.h>

#include "string_format.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////
////////// Logging ///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
///// These are working even with BSE_BUILD_RELEASE.                      ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#define log_info( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::ALL}, __VA_ARGS__)
#define log_warning( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::ALL}, __VA_ARGS__)

#if defined(BSE_BUILD_DEBUG)
#define log_error( ... ) { BREAK; bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL}, __VA_ARGS__); }
#else 
#define log_error( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL}, __VA_ARGS__)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////// Development Logging ///////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
///// These only work with BSE_BUILD_DEBUG and BSE_BUILD_DEVELOPMENT      ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(BSE_BUILD_DEBUG_DEVELOPMENT)
# define debug_log_info( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::LOCAL_AND_FILE}, "[DEV] ", __VA_ARGS__)
# define debug_log_warning( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::LOCAL_AND_FILE}, "[DEV] ", __VA_ARGS__)
# define debug_log_error( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::LOCAL_AND_FILE}, "[DEV] ", __VA_ARGS__)
#else
# define debug_log_info( ... ) {}
# define debug_log_warning( ... ) {}
# define debug_log_error( ... ) {}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////// Asserts, Debug Breaks and Checks //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(assert)
# undef assert
#endif

#if defined(BSE_BUILD_DEBUG)
# define BREAK GENERATE_TRAP()
# define assert(expression) { if ( !(expression) ) BREAK; }
#elif defined(BSE_BUILD_DEVELOPMENT)
# define BREAK {bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::LOCAL_CONSOLE}, "break in ", __FILE__," #", __LINE__ );}
# define assert(expression) if (!(expression)) { bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL}, "assert in ", __FILE__," #", __LINE__ );}
#else
# define BREAK {bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::REMOTE_AND_FILE}, "break in ", __FILE__," #", __LINE__ );}
# define assert(expression) CRASH_COND(!expression)
#endif

namespace bse
{
  namespace debug
  {
    enum class LogSeverity : u8
    {
      BSE_LOG_SEVERITY_NONE    = 0x0,
      BSE_LOG_SEVERITY_ERROR   = 0x1,
      BSE_LOG_SEVERITY_WARNING = 0x2,
      BSE_LOG_SEVERITY_INFO    = 0x3,
      BSE_LOG_SEVERITY_VERBOSE = 0x4,
    };
    enum class LogOutputType : u8
    {
      NONE              = 0x0,
      LOCAL_CONSOLE     = 0x1,
      REMOTE_HOST       = 0x2,
      LOCAL_AND_REMOTE  = LOCAL_CONSOLE | REMOTE_HOST,
      WRITE_TO_LOG_FILE = 0x4,
      LOCAL_AND_FILE    = LOCAL_CONSOLE | WRITE_TO_LOG_FILE,
      REMOTE_AND_FILE   = REMOTE_HOST | WRITE_TO_LOG_FILE,
      ALL               = 0xff
    };
    BSE_DEFINE_ENUM_OPERATORS_U8( LogOutputType );

    struct LogParameters
    {
      LogSeverity severity;
      LogOutputType type;
    };

    //forward trivial messages directly to the output
    void log( LogParameters const& parameters, char const* message ) 
    {  
      if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_INFO)
      {
        __print_line(message);
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_VERBOSE)
      {
        __print_line(message);
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_WARNING)
      {
        WARN_PRINT_ED(message);
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_ERROR)
      {
        ERR_PRINT_ED(message);
      }
    }

    void log( LogParameters const& parameters, char* message ) { log( parameters, (char const*)message ); }

    template<typename... Args> void log( LogParameters const& parameters, Args... args )
    {
      char debugBuffer[BSE_STACK_BUFFER_LARGE];
      s32 bytesToWrite = string_format( debugBuffer, BSE_STACK_BUFFER_LARGE, args... ) - 1 /* ommit null */;
      if ( bytesToWrite > 0 )
      {
        debugBuffer[bytesToWrite] = '\0';
        log(parameters, (char const*)debugBuffer);
      }
    }
  };
};

