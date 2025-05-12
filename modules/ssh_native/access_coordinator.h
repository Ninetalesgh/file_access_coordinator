#pragma once

#ifdef NO_GODOT
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
struct String {
  String(){}
  String(char const* s) { str = s;}
  String(String const& other) { str = other.str;}
  String(std::string const& other) { str = other;}

  String const& utf8() const { return *this; }
  char const* get_data() const { return str.c_str(); }
  bool is_empty() { return str.empty(); }
  String get_file() 
  {
    auto pos = str.find_last_of("/\\");
    if (pos != std::string::npos)
    {
      return str.substr(pos + 1);
    }
    else
    {
      return String(str);
    }
  }
  std::string str;
  String const& operator =(String const& other) { str = other.get_data(); return *this;}
  String const& operator =(char const* other) { str = other; return *this;}

  friend bool operator ==(String a, String const b) {return a.str == b.str;}
};

template<typename T> struct Vector {
  int find(T const& obj)
  { 
    auto it = std::find(vec.begin(), vec.end(), obj);
    if (it == vec.end()) return -1;
    else return (int)std::distance(vec.begin(), it);
  }
  void push_back(T obj) {vec.push_back(obj);}
  void remove_at(int i) {vec.erase(vec.begin() + i);}
  operator std::vector<T>() {return vec;}
  std::vector<T> vec;
};
#else
#include <core/object/ref_counted.h>
#include <scene/gui/dialogs.h>
#endif

#include <libssh/libssh.h>
#include <libssh/sftp.h>




#ifdef NO_GODOT
class AccessCoordinator {
#else
class AccessCoordinator : public Node {
	GDCLASS(AccessCoordinator, Node)
#endif
protected:
	static void _bind_methods();

public: 
  bool set_filepath(String filepath);
  bool init(String filepath, String user, String sshHostname, String sshUsername, String sshPassword, String remoteBaseDir);
  bool reserve();
  bool download();
  bool upload();
  bool release(bool overridePermissions = false);

  String fetch_output();
  String output;

  int shutdown_session();
  
  void on_download_dialog_confirm();
  void on_upload_dialog_confirm();
  void on_force_release_dialog_confirm();

  bool is_current_base_config_valid();
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

  int log_section_exit_return_error();
  int _init(char const* user, char const* sshUser, char const* sshHost, char const* sshPassword);
  
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
#ifdef NO_GODOT
  bool mAgreeAllPrompts = false;
#else
  ConfirmationDialog* mConfirmationDialog = nullptr;
#endif
  Vector<String> mReservedFileCache;
};

