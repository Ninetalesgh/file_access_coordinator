#pragma once

#include <core/object/ref_counted.h>
#include <scene/gui/dialogs.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "string_format.h"

class AccessCoordinator : public Node {
	GDCLASS(AccessCoordinator, Node)

protected:
	static void _bind_methods();

public: 
  bool init(String filepath, String user, String sshUsername, String sshHostname, String sshPassword, String remoteBaseDir);
  bool reserve();
  bool download();
  bool upload();
  bool release(bool overridePermissions = false);

  String fetch_output();
  String output;

void on_download_dialog_confirm();
void on_upload_dialog_confirm();
void on_force_release_dialog_confirm();

private:
  void show_confirmation_dialog(String title, String message, void (AccessCoordinator::*on_confirm)());

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
  int shutdown_session();
  
  int upload_file(const char* localPath, char const* remotePath);

  int download_file(char const* localPath, char const* remotePath);
  
  int reserve_remote_file_for_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp);

  int release_remote_file_from_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp, bool overridePermissions = false);


  String mFullLocalPath;
  String mUser;
  String mSshHostname;
  String mSshUsername;
  String mSshPassword;
  String mRemoteBaseDir;

  //Derived
  char mIpAddress[128];
  String mFilename;
  String mFullRemotePath;

  //
  ssh_session mSession = nullptr;
  ConfirmationDialog* mConfirmationDialog = nullptr;
  Vector<String> mReservedFileCache;
};

