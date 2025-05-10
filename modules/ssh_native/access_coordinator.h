#pragma once

#include <core/object/ref_counted.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "string_format.h"
#include "debug.h"


class AccessCoordinator : public Object {
	GDCLASS(AccessCoordinator, Object)

protected:
	static void _bind_methods();

public: 

//TODO Settings.. Volume.. etc



  void init();

private:
  int request_exec(char const* request, char* buffer, int bufferSize, bool outputToStandardOut = true);
  int request_exec(char const* request);

  template<typename... Args> int request_exec_format(Args... args)
  {
    char debugBuffer[BSE_STACK_BUFFER_LARGE]; 
    s32 bytesToWrite = string_format( debugBuffer, BSE_STACK_BUFFER_LARGE - 2, args... ) - 1 /* ommit null */;
    if ( bytesToWrite > 0 )
    {
      debugBuffer[bytesToWrite] = '\0';
      return request_exec((char const*)debugBuffer);
    }

  return SSH_ERROR;
  }


  int _init(char const* user, char const* sshUser, char const* sshHost, char const* sshPassword);
  int shutdown();
  
  int upload_file(const char* localPath, char const* remotePath);

  int download_file(char const* localPath, char const* remotePath);
  int reserve_remote_file_for_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp);

  int release_remote_file_from_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp, bool overridePermissions = false);


  ssh_session mSession;
  char mIpAddress[128];
  char const* mUser;
  char const* mSshHostname;
  char const* mSshUsername;
  char const* mPassword;
  char const* mRemoteDir;
  char const* mFilename;
};


