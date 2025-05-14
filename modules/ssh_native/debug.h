#pragma once

#ifndef NO_GODOT
#include <core/string/print_string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "string_format.h"
#include "access_coordinator.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////// Logging ///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
///// These are working even with BSE_BUILD_RELEASE.                      ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FAC_WINGUI && defined(NO_GODOT)
#define log_info( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::ALL}, __VA_ARGS__)
#define log_warning( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::ALL}, __VA_ARGS__)
#define log_error( ... ) bse::debug::log({bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL}, __VA_ARGS__)
#else
#define log_info( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_warning( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_error( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////// Development Logging ///////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
///// These only work with BSE_BUILD_DEBUG and BSE_BUILD_DEVELOPMENT      ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(BSE_BUILD_DEBUG_DEVELOPMENT)
# define debug_log_info( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::LOCAL_AND_FILE), "[DEV] ", __VA_ARGS__)
# define debug_log_warning( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::LOCAL_AND_FILE), "[DEV] ", __VA_ARGS__)
# define debug_log_error( ... ) bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::LOCAL_AND_FILE), "[DEV] ", __VA_ARGS__)
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
# define BREAK {this, bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::LOCAL_CONSOLE), "break in ", __FILE__," #", __LINE__ );}
# define assert(expression) if (!(expression)) { bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL), "assert in ", __FILE__," #", __LINE__ );}
#else
# define BREAK {this, bse::debug::log(bse::debug::LogParameters(this, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::REMOTE_AND_FILE), "break in ", __FILE__," #", __LINE__ );}
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

#ifndef FAC_WINGUI && defined(NO_GODOT)
    struct LogParameters
    {
      LogSeverity severity;
      LogOutputType type;
    };
#else
    struct LogParameters
    {
      AccessCoordinator* forwardInstance;
      LogSeverity severity;
      LogOutputType type;
      LogParameters(AccessCoordinator* _instance, LogSeverity _severity, LogOutputType _type)
        : forwardInstance(_instance)
        , severity(_severity)
        , type(_type){}
    };
#endif
    //forward trivial messages directly to the output
    void _log(LogParameters const& parameters, char const* message ) 
    {  
      if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_INFO)
      {
        #ifndef FAC_WINGUI && defined(NO_GODOT)
        printf(message);
        #else
        if (parameters.forwardInstance)
        {
          parameters.forwardInstance->output += message;
          if (parameters.forwardInstance->mNewLogSignal)
          {
            parameters.forwardInstance->mNewLogSignal();
          }
          std::cout << message << std::flush;
        }
        #endif
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_VERBOSE)
      {
        #ifndef FAC_WINGUI && defined(NO_GODOT)
        printf(message);
        #else
        if (parameters.forwardInstance)
        {
          parameters.forwardInstance->output += message;
          if (parameters.forwardInstance->mNewLogSignal)
          {
            parameters.forwardInstance->mNewLogSignal();
          }
          std::cout << message << std::flush;
        }
        #endif
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_WARNING)
      {
        #ifndef FAC_WINGUI && defined(NO_GODOT)
        printf(message);
        #else
        if (parameters.forwardInstance)
        {
          parameters.forwardInstance->output += message;
          if (parameters.forwardInstance->mNewLogSignal)
          {
            parameters.forwardInstance->mNewLogSignal();
          }
          std::cout << message << std::flush;
        }
        #endif
      }
      else if (parameters.severity == LogSeverity::BSE_LOG_SEVERITY_ERROR)
      {
        #ifndef FAC_WINGUI && defined(NO_GODOT)
        printf(message);
        #else
        if (parameters.forwardInstance)
        {
          parameters.forwardInstance->output += message;
          if (parameters.forwardInstance->mNewLogSignal)
          {
            parameters.forwardInstance->mNewLogSignal();
          }
          std::cout << message << std::flush;
        }
        #endif
      }
    }

    template<typename... Args> void log( LogParameters const& parameters, Args... args )
    {
      char debugBuffer[BSE_STACK_BUFFER_LARGE];
      s32 bytesToWrite = string_format( debugBuffer, BSE_STACK_BUFFER_LARGE - 1, args... ) - 1 /* ommit null */;
      if ( bytesToWrite > 0 )
      {
        if ( debugBuffer[bytesToWrite - 1] != '\n' && !(debugBuffer[0] == '\r' && debugBuffer[1] != '\n'))
        {
          debugBuffer[bytesToWrite++] = '\n';
        }

        debugBuffer[bytesToWrite] = '\0';
        _log(parameters, (char const*)debugBuffer);
      }
    }
  };
};

